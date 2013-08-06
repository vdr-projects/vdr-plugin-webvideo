#include <glib.h>
#include <string.h>
#include "pipecomponent.h"
#include "webvicontext.h"
#include "request.h"
#include "link.h"

struct WebviRequest {
  PipeComponent *pipe_head;
  gchar *url;
  webvi_callback write_cb;
  webvi_callback read_cb;
  void *writedata;
  void *readdata;
  struct WebviContext *ctx; /* borrowed reference */
};

struct RequestStateMessage {
  RequestState state;
  const char *message;
};

static PipeComponent *pipe_factory(const WebviRequest *self);
static void notify_pipe_finished(RequestState state, void *data);
static const char *pipe_state_to_message(RequestState state);
static gchar *wvt_to_local_file(const WebviContext *ctx, const char *wvt);
static PipeComponent *build_and_start_mainmenu_pipe(const WebviRequest *self);
static PipeComponent *build_and_start_local_pipe(const WebviRequest *self);
static PipeComponent *build_and_start_external_pipe(const WebviRequest *self,
                                                    const char *command);
static PipeComponent *build_and_start_libquvi_pipe(const WebviRequest *self);
static PipeComponent *build_and_start_menu_pipe(const WebviRequest *self);

WebviRequest *request_create(const char *url, struct WebviContext *ctx) {
  WebviRequest *req = malloc(sizeof(WebviRequest));
  memset(req, 0, sizeof(WebviRequest));
  req->url = g_strdup(url);
  req->ctx = ctx;
  return req;
}

void request_delete(WebviRequest *self) {
  if (self) {
    request_stop(self);
    pipe_delete_full(self->pipe_head);
    g_free(self->url);
  }
}

gboolean request_start(WebviRequest *self) {
  if (!self->pipe_head) {
    self->pipe_head = pipe_factory(self);
    if (!self->pipe_head) {
      notify_pipe_finished(WEBVISTATE_NOT_FOUND, self);
    }
  }
  return TRUE;
}

void request_stop(WebviRequest *self) {
  // FIXME
}

PipeComponent *pipe_factory(const WebviRequest *self) {
  PipeComponent *head;
  if (strcmp(self->url, "wvt://mainmenu") == 0) {
    head = build_and_start_mainmenu_pipe(self);

  } else if (strncmp(self->url, "wvt://", 6) == 0) {
    head = build_and_start_local_pipe(self);

  } else {
    const LinkAction *action = link_templates_get_action(
      get_link_templates(self->ctx), self->url);
    LinkActionType action_type = action ? link_action_get_type(action) : LINK_ACTION_PARSE;
    if (action_type == LINK_ACTION_STREAM_LIBQUVI) {
      head = build_and_start_libquvi_pipe(self);
    } else if (action_type == LINK_ACTION_EXTERNAL_COMMAND) {
      const char *command = link_action_get_command(action);
      head = build_and_start_external_pipe(self, command);
    } else {
      head = build_and_start_menu_pipe(self);
    }
  }

  return head;
}

PipeComponent *build_and_start_mainmenu_pipe(const WebviRequest *self) {
  PipeMainMenuDownloader *p1 = pipe_mainmenu_downloader_create(self->ctx);
  PipeComponent *p2 = (PipeComponent *)pipe_callback_wrapper_create(
    self->read_cb, self->readdata, notify_pipe_finished, (void *)self);
  pipe_component_set_next((PipeComponent *)p1, p2);
  pipe_mainmenu_downloader_start(p1);

  return (PipeComponent *)p1;
}

PipeComponent *build_and_start_local_pipe(const WebviRequest *self) {
  gchar *filename = wvt_to_local_file(self->ctx, self->url);
  PipeLocalFile *p1 = pipe_local_file_create(filename);
  g_free(filename);
  PipeComponent *p2 = (PipeComponent *)pipe_callback_wrapper_create(
    self->read_cb, self->readdata, notify_pipe_finished, (void *)self);
  pipe_component_set_next((PipeComponent *)p1, p2);
  pipe_local_file_start(p1);

  return (PipeComponent *)p1;
}

PipeComponent *build_and_start_external_pipe(const WebviRequest *self, const char *command) {
  PipeExternalDownloader *p1 = pipe_external_downloader_create(self->url, command);
  PipeComponent *p2 = (PipeComponent *)pipe_callback_wrapper_create(
    self->read_cb, self->readdata, notify_pipe_finished, (void *)self);
  pipe_component_set_next((PipeComponent *)p1, p2);
  pipe_external_downloader_start(p1);

  return (PipeComponent *)p1;
}

PipeComponent *build_and_start_libquvi_pipe(const WebviRequest *self) {
  PipeLibquvi *p1 = pipe_libquvi_create(self->url);
  PipeComponent *p2 = (PipeComponent *)pipe_callback_wrapper_create(
    self->read_cb, self->readdata, notify_pipe_finished, (void *)self);
  pipe_component_set_next((PipeComponent *)p1, p2);
  pipe_libquvi_start(p1);

  return (PipeComponent *)p1;
}

PipeComponent *build_and_start_menu_pipe(const WebviRequest *self) {
  CURLM *curlmulti = webvi_context_get_curl_multi_handle(self->ctx);
  PipeDownloader *p1 = pipe_downloader_create(self->url, curlmulti);
  PipeComponent *p2 = (PipeComponent *)pipe_link_extractor_create(
    get_link_templates(self->ctx), self->url);
  PipeComponent *p3 = (PipeComponent *)pipe_callback_wrapper_create(
    self->read_cb, self->readdata, notify_pipe_finished, (void *)self);
  pipe_component_set_next((PipeComponent *)p1, p2);
  pipe_component_set_next(p2, p3);
  pipe_downloader_start(p1);

  return (PipeComponent *)p1;
}

gchar *wvt_to_local_file(const WebviContext *ctx, const char *wvt) {
  if (strncmp(wvt, "wvt://", 6) != 0)
    return NULL;

  const gchar *template_path = webvi_context_get_template_path(ctx);
  if (!template_path)
    return NULL; // FIXME

  // FIXME: .. in paths

  gchar *filename = g_strdup(wvt+6);
  if (filename[0] == '/') {
    // absolute path
    // The path must be located under the template directory
    if (strncmp(filename, template_path, strlen(template_path)) != 0) {
      g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
        "Invalid path in wvt:// url");
      g_free(filename);
      return NULL;
    }
  } else {
    // relative path, concatenate to template_path
    gchar *absolute_path = g_strconcat(template_path, "/", filename, NULL);
    g_free(filename);
    filename = absolute_path;
  }

  return filename;
}

void request_fdset(WebviRequest *self, fd_set *readfds,
                   fd_set *writefds, fd_set *excfds, int *max_fd)
{
  if (self->pipe_head) {
    pipe_fdset(self->pipe_head, readfds, writefds, excfds, max_fd);
  }
}

gboolean request_handle_socket(WebviRequest *self, int sockfd, int ev_bitmask)
{
  if (self->pipe_head) {
    return pipe_handle_socket(self->pipe_head, sockfd, ev_bitmask);
  } else {
    return FALSE;
  }
}

void request_set_write_callback(WebviRequest *self, webvi_callback func) {
  self->write_cb = func;
}

void request_set_read_callback(WebviRequest *self, webvi_callback func) {
  self->read_cb = func;
}

void request_set_write_data(WebviRequest *self, void *data) {
  self->writedata = data;
}

void request_set_read_data(WebviRequest *self, void *data) {
  self->readdata = data;
}

const char *request_get_url(const WebviRequest *self) {
  return self->url;
}

void notify_pipe_finished(RequestState state, void *data) {
  WebviRequest *req = (WebviRequest *)data;
  const char *msg = pipe_state_to_message(state);
  webvi_context_add_finished_message(req->ctx, req, state, msg);
}

const char *pipe_state_to_message(RequestState state) {
  static struct RequestStateMessage messages[] = 
    {{WEBVISTATE_NOT_FINISHED, "Not finished"},
     {WEBVISTATE_FINISHED_OK, "Success"},
     {WEBVISTATE_MEMORY_ALLOCATION_ERROR, "Out of memory"},
     {WEBVISTATE_NOT_FOUND, "Not found"},
     {WEBVISTATE_NETWORK_READ_ERROR, "Failed to receive data from the network"},
     {WEBVISTATE_IO_ERROR, "IO error"},
     {WEBVISTATE_TIMEDOUT, "Timedout"},
     {WEBVISTATE_SUBPROCESS_FAILED, "Failed to execute a subprocess"},
     {WEBVISTATE_INTERNAL_ERROR, "Internal error"}};

  for (int i=0; i<(sizeof(messages)/sizeof(messages[0])); i++) {
    if (state == messages[i].state) {
      return messages[i].message;
    }
  }

  return "Internal error";
}
