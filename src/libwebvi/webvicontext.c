#include <stdlib.h>
#include <string.h>
#include <glib/gprintf.h>
#include "webvicontext.h"
#include "libwebvi.h"
#include "request.h"

#define DEFAULT_TEMPLATE_PATH "/etc/webvi/websites"
#define MAX_MESSAGE_LENGTH 128

struct WebviContext {
  GTree *requests;
  LinkTemplates *link_templates;
  WebviHandle next_request;
  CURLM *curl_multi_handle;
  gchar *template_path;
  GArray *finish_messages;
  /* The value returned by the latest webvi_context_next_message() call */
  WebviMsg current_message;
  bool debug;
};

typedef struct RequestAndHandle {
  const WebviRequest *request;
  WebviHandle handle;
} RequestAndHandle;

typedef struct FoundFds {
  fd_set *readfds;
  fd_set *writefds;
  fd_set *excfds;
  int *max_fd;
} FoundFds;

typedef struct SocketToHandle {
  int sockfd;
  int ev_bitmask;
  gboolean handled;
} SocketToHandle;

static WebviCtx handle_for_context(WebviContext *ctx);
static gint cmp_int(gconstpointer a, gconstpointer b, gpointer user_data);
static gboolean gather_fds(gpointer key, gpointer value, gpointer data);
static gboolean handle_request_socket(gpointer key, gpointer value, gpointer data);
static WebviHandle get_handle_for_request(WebviContext *ctx, const WebviRequest *req);
static gboolean search_by_request(gpointer key, gpointer value, gpointer data);
static void check_for_finished_curl(CURLM *multi_handle);
static RequestState curl_code_to_pipe_state(CURLcode curlcode);
static WebviResult curlmcode_to_webvierr(CURLMcode mcode);
static void webvi_log_handler(const gchar *log_domain, GLogLevelFlags log_level,
                              const gchar *message, gpointer user_data);
static void register_context(WebviCtx key, WebviContext *value);
static GTree *get_tls_contexts();
static void webvi_context_delete(WebviContext *ctx);
static void free_tls_context(gpointer data);
static void free_context(gpointer data);
static void free_request(gpointer data);

static GPrivate tls_contexts = G_PRIVATE_INIT(free_tls_context);

WebviCtx webvi_context_initialize() {
  WebviContext *ctx = malloc(sizeof(WebviContext));
  if (!ctx) {
    return 0;
  }

  memset(ctx, 0, sizeof(WebviContext));

  ctx->requests = g_tree_new_full(cmp_int, NULL, NULL, free_request);
  if (!ctx->requests) {
    webvi_context_delete(ctx);
    return 0;
  }

  ctx->finish_messages = g_array_new(FALSE, TRUE, sizeof(WebviMsg));
  if (!ctx->finish_messages) {
    webvi_context_delete(ctx);
    return 0;
  }

  ctx->next_request = 1;

  WebviCtx ctxhandle = handle_for_context(ctx);
  register_context(ctxhandle, ctx);

  return ctxhandle;
}

void register_context(WebviCtx ctxhandle, WebviContext *ctx) {
  GTree *contexts = get_tls_contexts();
  g_tree_insert(contexts, GINT_TO_POINTER(ctxhandle), ctx);
}

void webvi_context_cleanup(WebviCtx ctxhandle) {
  GTree *contexts = get_tls_contexts();
  g_tree_remove(contexts, GINT_TO_POINTER(ctxhandle));
}

void webvi_context_set_debug(WebviContext *self, bool d) {
  GLogFunc logfunc;

  self->debug = d;
  if (self->debug) {
    logfunc = webvi_log_handler;
  } else {
    logfunc = g_log_default_handler;
  }

  g_log_set_handler(LIBWEBVI_LOG_DOMAIN,
                    G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG,
                    logfunc, NULL);
}

void webvi_log_handler(const gchar *log_domain, GLogLevelFlags log_level,
                       const gchar *message, gpointer user_data)
{
  g_fprintf(stderr, "%s: %s\n", log_domain, message);
}

void webvi_context_set_template_path(WebviContext *self, const char *path) {
  if (self->link_templates) {
    link_templates_delete(self->link_templates);
    self->link_templates = NULL;
  }
  if (self->template_path) {
    g_free(self->template_path);
  }
  self->template_path = path ? g_strdup(path) : NULL;
}

const char *webvi_context_get_template_path(const WebviContext *self) {
  return self->template_path ? self->template_path : DEFAULT_TEMPLATE_PATH;
}

const LinkTemplates *get_link_templates(WebviContext *self) {
  if (!self->link_templates) {
    self->link_templates = link_templates_create();
    if (self->link_templates) {
      const gchar *path = webvi_context_get_template_path(self);
      gchar *template_file = g_strconcat(path, "/links", NULL);
      link_templates_load(self->link_templates, template_file);
      g_free(template_file);
    }
  }

  return self->link_templates;
}

WebviHandle webvi_context_add_request(WebviContext *self, WebviRequest *req) {
  int h = self->next_request++;
  g_tree_insert(self->requests, GINT_TO_POINTER(h), req);
  return (WebviHandle)h;
}

WebviRequest *webvi_context_get_request(WebviContext *self, WebviHandle h) {
  return (WebviRequest *)g_tree_lookup(self->requests, GINT_TO_POINTER(h));
}

void webvi_context_remove_request(WebviContext *self, WebviHandle h) {
  g_tree_remove(self->requests, GINT_TO_POINTER(h));
}

CURLM *webvi_context_get_curl_multi_handle(WebviContext *self) {
  if (!self->curl_multi_handle)
    self->curl_multi_handle = curl_multi_init();
  return self->curl_multi_handle;
}

WebviCtx handle_for_context(WebviContext *ctx) {
  return (WebviCtx)ctx;  // FIXME
}

WebviContext *get_context_by_handle(WebviCtx handle) {
  GTree *contexts = get_tls_contexts();
  return g_tree_lookup(contexts, GINT_TO_POINTER(handle));
}

WebviResult webvi_context_fdset(WebviContext *ctx, fd_set *readfds,
                                fd_set *writefds, fd_set *excfds, int *max_fd)
{
  WebviResult res = WEBVIERR_OK;
  *max_fd = -1;

  // curl sockets
  CURLM *mhandle = webvi_context_get_curl_multi_handle(ctx);
  if (mhandle) {
    CURLMcode mcode = curl_multi_fdset(mhandle, readfds, writefds, excfds, max_fd);
    res = curlmcode_to_webvierr(mcode);
  }

  // non-curl fds
  FoundFds fds;
  fds.readfds = readfds;
  fds.writefds = writefds;
  fds.excfds = excfds;
  fds.max_fd = max_fd;
  g_tree_foreach(ctx->requests, gather_fds, &fds);
  
  return res;
}

gboolean gather_fds(gpointer key, gpointer value, gpointer data) {
  FoundFds *fds = (FoundFds *)data;
  WebviRequest *req = (WebviRequest *)value;
  request_fdset(req, fds->readfds, fds->writefds, fds->excfds, fds->max_fd);
  return FALSE;
}

void webvi_context_handle_socket_action(
  WebviContext *ctx, int sockfd, int ev_bitmask, long *running_handles)
{
  SocketToHandle x;
  x.sockfd = sockfd;
  x.ev_bitmask = ev_bitmask;
  x.handled = FALSE;
  g_tree_foreach(ctx->requests, handle_request_socket, &x);

  int curl_handles = 0;
  if (!x.handled) {
    // sockfd belongs to curl
    CURLM *multi_handle = webvi_context_get_curl_multi_handle(ctx);
    if (multi_handle) {
      curl_socket_t curl_socket;
      int curl_mask = 0;

      if (sockfd == WEBVI_SELECT_TIMEOUT) {
        curl_socket = CURL_SOCKET_TIMEOUT;
        curl_mask = 0;
      } else {
        curl_socket = sockfd;
        if ((ev_bitmask & WEBVI_SELECT_READ) != 0)
          curl_mask |= CURL_CSELECT_IN;
        if ((ev_bitmask & WEBVI_SELECT_WRITE) != 0)
          curl_mask |= CURL_CSELECT_OUT;
        if ((ev_bitmask & WEBVI_SELECT_EXCEPTION) != 0)
          curl_mask |= CURL_CSELECT_ERR;
      }

      curl_multi_socket_action(multi_handle, curl_socket, curl_mask, &curl_handles);
      check_for_finished_curl(multi_handle);
    }
  }

  // FIXME: running_handles
  *running_handles = curl_handles;
}

gboolean handle_request_socket(gpointer key, gpointer value, gpointer data) {
  WebviRequest *req = (WebviRequest *)value;
  SocketToHandle *to_handle = (SocketToHandle *)data;
  return request_handle_socket(req, to_handle->sockfd, to_handle->ev_bitmask);
}

void check_for_finished_curl(CURLM *multi_handle) {
  int num_messages;
  CURLMsg *info;
  while ((info = curl_multi_info_read(multi_handle, &num_messages))) {
    if (info->msg == CURLMSG_DONE) {
      char *instance;
      if (curl_easy_getinfo(info->easy_handle, CURLINFO_PRIVATE, &instance) == CURLE_OK) {
        PipeComponent *pipe = (PipeComponent *)instance;
        pipe_component_finished(pipe, curl_code_to_pipe_state(info->data.result));
      }
    }
  }
}

RequestState curl_code_to_pipe_state(CURLcode curlcode) {
  switch (curlcode) {
  case CURLE_OK:
    return WEBVISTATE_FINISHED_OK;

  case CURLE_COULDNT_CONNECT:
  case CURLE_TOO_MANY_REDIRECTS:
  case CURLE_GOT_NOTHING:
  case CURLE_RECV_ERROR:
    return WEBVISTATE_NETWORK_READ_ERROR;

  case CURLE_REMOTE_FILE_NOT_FOUND:
    return WEBVISTATE_NOT_FOUND;

  case CURLE_OPERATION_TIMEDOUT:
    return WEBVISTATE_TIMEDOUT;

  default:
    return WEBVISTATE_IO_ERROR;
  }
}

WebviResult curlmcode_to_webvierr(CURLMcode mcode) {
  switch (mcode) {
  case CURLM_OK:
    return WEBVIERR_OK;

  case CURLM_BAD_HANDLE:
  case CURLM_BAD_EASY_HANDLE:
    return WEBVIERR_INVALID_PARAMETER;

  default:
    return WEBVIERR_UNKNOWN_ERROR;
  };
}

void webvi_context_add_finished_message(WebviContext *ctx,
                                        const WebviRequest *req,
                                        RequestState status_code,
                                        const char *message_text)
{
  WebviHandle h = get_handle_for_request(ctx, req);
  if (h != 0) {
    GArray *messages = ctx->finish_messages;
    WebviMsg msg;
    msg.msg = WEBVIMSG_DONE;
    msg.handle = h;
    msg.status_code = status_code;
    msg.data = message_text;
    g_array_append_val(messages, msg);
  }
}

WebviHandle get_handle_for_request(WebviContext *ctx, const WebviRequest *req) {
  RequestAndHandle query_and_result;
  query_and_result.request = req;
  query_and_result.handle = 0;
  g_tree_foreach(ctx->requests, search_by_request, &query_and_result);
  return query_and_result.handle;
}

gboolean search_by_request(gpointer key, gpointer value, gpointer data) {
  WebviHandle h = GPOINTER_TO_INT(key);
  WebviRequest *req = value;
  RequestAndHandle *query_and_result = data;

  if (query_and_result->request == req) {
    query_and_result->handle = h;
    return TRUE; /* stop traversal */
  } else {
    return FALSE;
  }
}

WebviMsg *webvi_context_next_message(WebviContext *ctx, int *remaining_messages) {
  guint len = ctx->finish_messages->len;
  if (len > 0) {
    if (remaining_messages)
      *remaining_messages = (int)len-1;

    ctx->current_message = g_array_index(ctx->finish_messages, WebviMsg, 0);
    g_array_remove_index(ctx->finish_messages, 0);
    return &ctx->current_message;
  } else {
    if (remaining_messages)
      *remaining_messages = 0;
    return NULL;
  }
}

void webvi_context_delete(WebviContext *ctx) {
  if (ctx) {
    if (ctx->finish_messages)
      g_array_free(ctx->finish_messages, TRUE);
    if (ctx->curl_multi_handle)
      curl_multi_cleanup(ctx->curl_multi_handle);
    if (ctx->requests)
      g_tree_unref(ctx->requests);
    /* if (ctx->downloaders) */
    /*   g_ptr_array_unref(ctx->downloaders); */
    if (ctx->link_templates)
      link_templates_delete(ctx->link_templates);
    g_free(ctx->template_path);

    free(ctx);
  }
}

GTree *get_tls_contexts() {
  GTree *contexts = g_private_get(&tls_contexts);
  if (!contexts) {
    contexts = g_tree_new_full(cmp_int, NULL, NULL, free_context);
    g_private_set(&tls_contexts, contexts);
  }

  return contexts;
}

void webvi_context_cleanup_all() {
  /* This will cause free_tls_context to be called if tls_context was set */
  g_private_set(&tls_contexts, NULL);
}

gint cmp_int(gconstpointer a, gconstpointer b, gpointer user_data) {
  int aint = GPOINTER_TO_INT(a);
  int bint = GPOINTER_TO_INT(b);
  return aint - bint;
}

void free_tls_context(gpointer data) {
  if (data) {
    g_tree_destroy((GTree *)data);
  }
}

void free_request(gpointer data) {
  request_delete((WebviRequest *)data);
}

void free_context(gpointer data) {
  webvi_context_delete((WebviContext *)data);
}
