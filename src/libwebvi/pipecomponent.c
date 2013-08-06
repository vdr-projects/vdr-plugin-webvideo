#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include "pipecomponent.h"
#include "menubuilder.h"
#include "linkextractor.h"
#include "mainmenu.h"
#include "webvicontext.h"
#include "version.h"

#define WEBVID_USER_AGENT "libwebvi/" LIBWEBVI_VERSION " libcurl/" LIBCURL_VERSION

#define INITIALIZE_PIPE(pipetype, process, finish, delete) \
  pipetype *self = malloc(sizeof(pipetype)); \
  memset(self, 0, sizeof(pipetype)); \
  pipe_component_initialize(&self->pipe_data, (process), (finish), (delete))

#define INITIALIZE_PIPE_WITH_FDSET(pipetype, process, finish, delete, fdset, handle_socket) \
  pipetype *self = malloc(sizeof(pipetype)); \
  memset(self, 0, sizeof(pipetype)); \
  pipe_component_initialize_fdset(&self->pipe_data, (process), (finish), (delete), (fdset), (handle_socket))

struct PipeDownloader {
  PipeComponent pipe_data;
  CURL *curl;
  CURLM *curlmulti;
};

struct PipeLinkExtractor {
  PipeComponent pipe_data;
  LinkExtractor *link_extractor;
};

struct PipeCallbackWrapper {
  PipeComponent pipe_data;
  void *write_data;
  void *finish_data;
  ssize_t (*write_callback)(const char *, size_t, void *);
  void (*finish_callback)(RequestState, void *);
};

struct PipeMainMenuDownloader {
  PipeComponent pipe_data;
  int kickstart_fd;
  const WebviContext *context; /* borrowed reference */
};

struct PipeExternalDownloader {
  PipeComponent pipe_data;
  gchar *url;
  int fd;
};

struct PipeLocalFile {
  PipeComponent pipe_data;
  gchar *filename;
  int fd;
};

struct PipeLibquvi {
  PipeComponent pipe_data;
  gchar *url;
  GPid pid;
  gint quvi_output;
  xmlParserCtxtPtr parser;
};

static void pipe_component_delete(PipeComponent *self);

static gboolean pipe_link_extractor_append(PipeComponent *instance, char *buf, size_t len);
static void pipe_link_extractor_finished(PipeComponent *instance, RequestState state);
static void pipe_link_extractor_delete(PipeComponent *instance);

static gboolean pipe_callback_wrapper_process(PipeComponent *instance,
                                              char *buf, size_t len);
static void pipe_callback_wrapper_finished(PipeComponent *instance,
                                           RequestState state);
static void pipe_callback_wrapper_delete(PipeComponent *instance);

static void pipe_downloader_finished(PipeComponent *instance, RequestState state);
static void pipe_downloader_delete(PipeComponent *instance);
static size_t curl_write_wrapper(char *ptr, size_t size, size_t nmemb, void *userdata);

static void pipe_mainmenu_downloader_fdset(PipeComponent *instance, 
                                           fd_set *readfd,
                                           fd_set *writefd,
                                           fd_set *excfd,
                                           int *maxfd);
static gboolean pipe_mainmenu_downloader_handle_socket(PipeComponent *instance,
                                                       int fd, int bitmask);
static void pipe_mainmenu_downloader_delete(PipeComponent *instance);

static void pipe_local_file_fdset(PipeComponent *instance, fd_set *readfd,
                                  fd_set *writefd, fd_set *excfd, int *maxfd);
static gboolean pipe_local_file_handle_socket(PipeComponent *instance,
                                              int fd, int bitmask);
static void pipe_local_file_delete(PipeComponent *instance);

static void pipe_external_downloader_fdset(PipeComponent *instance,
                                           fd_set *readfd, fd_set *writefd,
                                           fd_set *excfd, int *maxfd);
static gboolean pipe_external_downloader_handle_socket(
  PipeComponent *instance, int fd, int bitmask);
static void pipe_external_downloader_delete(PipeComponent *instance);

static void pipe_libquvi_fdset(PipeComponent *instance, fd_set *readfd,
                               fd_set *writefd, fd_set *excfd, int *maxfd);
static gboolean pipe_libquvi_handle_socket(PipeComponent *instance,
                                           int fd, int bitmask);
static gboolean pipe_libquvi_parse(PipeComponent *instance, char *buf, size_t len);
static void pipe_libquvi_finished(PipeComponent *instance, RequestState state);
static xmlChar *quvi_xml_get_stream_url(xmlDoc *doc);
static xmlChar *quvi_xml_get_stream_title(xmlDoc *doc);
static void pipe_libquvi_delete(PipeComponent *instance);

static CURL *start_curl(const char *url, CURLM *curlmulti,
                        PipeComponent *instance);
static void append_to_fdset(int fd, fd_set *fdset, int *maxfd);
static gboolean read_from_fd_to_pipe(PipeComponent *instance,
                                     int *instance_fd_ptr, int fd, int bitmask);


void pipe_component_initialize(PipeComponent *self,
    gboolean (*process_cb)(PipeComponent *, char *, size_t),
    void (*done_cb)(PipeComponent *, RequestState),
    void (*delete_cb)(PipeComponent *))
{
  g_assert(self);
  g_assert(delete_cb);

  memset(self, 0, sizeof(PipeComponent));
  self->process = process_cb;
  self->finished = done_cb;
  self->delete = delete_cb;
  self->state = WEBVISTATE_NOT_FINISHED;
}

void pipe_component_initialize_fdset(PipeComponent *self,
    gboolean (*process_cb)(PipeComponent *, char *, size_t),
    void (*done_cb)(PipeComponent *, RequestState),
    void (*delete_cb)(PipeComponent *),
    void (*fdset_cb)(PipeComponent *, fd_set *, fd_set *, fd_set *, int *),
    gboolean (*handle_socket_cb)(PipeComponent *, int, int))
{
  pipe_component_initialize(self, process_cb, done_cb, delete_cb);
  self->fdset = fdset_cb;
  self->handle_socket = handle_socket_cb;
}

void pipe_component_set_next(PipeComponent *self, PipeComponent *next) {
  g_assert(!self->next);
  self->next = next;
}

void pipe_component_append(PipeComponent *self, char *buf, size_t length) {
  if (self->state == WEBVISTATE_NOT_FINISHED) {
    gboolean propagate = TRUE;
    if (self->process)
      propagate = self->process(self, buf, length);
    if (propagate && self->next)
      pipe_component_append(self->next, buf, length);
  }
}

void pipe_component_finished(PipeComponent *self, RequestState state) {
  if (self->state == WEBVISTATE_NOT_FINISHED) {
    self->state = state;
    if (self->finished)
      self->finished(self, state);
    if (self->next)
      pipe_component_finished(self->next, state);
  }
}

RequestState pipe_component_get_state(const PipeComponent *self) {
  return self->state;
}

void pipe_fdset(PipeComponent *head, fd_set *readfd,
                fd_set *writefd, fd_set *excfd, int *max_fd)
{
  PipeComponent *component = head;
  PipeComponent *next;
  while (component) {
    next = component->next;
    if (component->fdset) {
      component->fdset(component, readfd, writefd, excfd, max_fd);
    }
    component = next;
  }
}

gboolean pipe_handle_socket(PipeComponent *head, int sockfd, int ev_bitmask) {
  PipeComponent *component = head;
  PipeComponent *next;
  while (component) {
    next = component->next;
    if (component->handle_socket) {
      if (component->handle_socket(component, sockfd, ev_bitmask)) {
        return TRUE;
      }
    }
    component = next;
  }
  return FALSE;
}

void pipe_component_delete(PipeComponent *self) {
  if (self && self->delete)
    self->delete(self);
}

void pipe_delete_full(PipeComponent *head) {
  PipeComponent *component = head;
  PipeComponent *next;
  while (component) {
    next = component->next;
    pipe_component_delete(component);
    component = next;
  }
}


/***** PipeLinkExtractor *****/


PipeLinkExtractor *pipe_link_extractor_create(
  const LinkTemplates *link_templates, const gchar *baseurl)
{
  INITIALIZE_PIPE(PipeLinkExtractor, pipe_link_extractor_append,
                  pipe_link_extractor_finished, pipe_link_extractor_delete);
  self->link_extractor = link_extractor_create(link_templates, baseurl);
  return self;
}

void pipe_link_extractor_delete(PipeComponent *instance) {
  PipeLinkExtractor *self = (PipeLinkExtractor *)instance;
  link_extractor_delete(self->link_extractor);
  free(self);
}

gboolean pipe_link_extractor_append(PipeComponent *instance, char *buf, size_t len) {
  PipeLinkExtractor *self = (PipeLinkExtractor *)instance;
  link_extractor_append(self->link_extractor, buf, len);
  return FALSE;
}

void pipe_link_extractor_finished(PipeComponent *instance,
                                  RequestState state)
{
  PipeLinkExtractor *self = (PipeLinkExtractor *)instance;
  if (state == WEBVISTATE_FINISHED_OK) {
    GPtrArray *links = link_extractor_get_links(self->link_extractor);
    MenuBuilder *menu_builder = menu_builder_create();
    menu_builder_append_link_list(menu_builder, links);
    char *menu = menu_builder_to_string(menu_builder);
    if (self->pipe_data.next) {
      pipe_component_append(self->pipe_data.next, menu, strlen(menu));
      pipe_component_finished(self->pipe_data.next, state);
    }
    menu_builder_delete(menu_builder);
    free(menu);
    g_ptr_array_free(links, TRUE);
  }
}


/***** PipeDownloader *****/


PipeDownloader *pipe_downloader_create(const char *url, CURLM *curlmulti) {
  INITIALIZE_PIPE(PipeDownloader, NULL, pipe_downloader_finished,
                  pipe_downloader_delete);
  self->curl = start_curl(url, curlmulti, (PipeComponent *)self);
  if (!self->curl) {
    pipe_downloader_delete((PipeComponent *)self);
    return NULL;
  }
  self->curlmulti = curlmulti;
  return self;
}

void pipe_downloader_start(PipeDownloader *self) {
  CURLMcode mcode = curl_multi_add_handle(self->curlmulti, self->curl);
  if (mcode != CURLM_OK) {
    pipe_component_finished(&self->pipe_data, WEBVISTATE_INTERNAL_ERROR);
  }
}

size_t curl_write_wrapper(char *ptr, size_t size, size_t nmemb, void *userdata)
{
  PipeComponent *pipedata = (PipeComponent *)userdata;
  if (pipe_component_get_state(pipedata) == WEBVISTATE_NOT_FINISHED) {
    pipe_component_append(pipedata, ptr, size*nmemb);
    return size*nmemb;
  } else {
    return 0;
  }
}

void pipe_downloader_finished(PipeComponent *instance, RequestState state) {
  PipeDownloader *self = (PipeDownloader *)instance;
  curl_multi_remove_handle(self->curlmulti, self->curl);
}

void pipe_downloader_delete(PipeComponent *instance) {
  PipeDownloader *self = (PipeDownloader *)instance;
  if (self->pipe_data.state == WEBVISTATE_NOT_FINISHED) {
    curl_multi_remove_handle(self->curlmulti, self->curl);
  }
  curl_easy_cleanup(self->curl);
  free(instance);
}


/***** PipeMainMenuDownloader *****/


PipeMainMenuDownloader *pipe_mainmenu_downloader_create(WebviContext *context) {
  INITIALIZE_PIPE_WITH_FDSET(PipeMainMenuDownloader, NULL, NULL,
                             pipe_mainmenu_downloader_delete,
                             pipe_mainmenu_downloader_fdset,
                             pipe_mainmenu_downloader_handle_socket);
  self->context = context;
  self->kickstart_fd = -1;
  return self;
}

void pipe_mainmenu_downloader_start(PipeMainMenuDownloader *self) {
  char *mainmenu = build_mainmenu(webvi_context_get_template_path(self->context));
  if (!mainmenu) {
    pipe_component_finished(&self->pipe_data, WEBVISTATE_INTERNAL_ERROR);
    return;
  }

  pipe_component_append(&self->pipe_data, mainmenu, strlen(mainmenu));
  pipe_component_finished(&self->pipe_data, WEBVISTATE_FINISHED_OK);

  /* To kickstart downloading, create a fake read event for the client
   * so that select returns immediately instead of waiting until the
   * timeout. */
  int pipefd[2];
  if (pipe(pipefd) != -1) {
    write(pipefd[1], "*", 1);
    close(pipefd[1]);
    self->kickstart_fd = pipefd[0];
  }

  g_free(mainmenu);
}

void pipe_mainmenu_downloader_fdset(PipeComponent *instance, fd_set *readfd,
                                    fd_set *writefd, fd_set *excfd, int *maxfd)
{
  PipeMainMenuDownloader *self = (PipeMainMenuDownloader *)instance;
  append_to_fdset(self->kickstart_fd, readfd, maxfd);
}

gboolean pipe_mainmenu_downloader_handle_socket(PipeComponent *instance,
                                                int fd, int bitmask)
{
  PipeMainMenuDownloader *self = (PipeMainMenuDownloader *)instance;
  char buf;
  if (self->kickstart_fd != -1) {
    read(self->kickstart_fd, &buf, 1);
    close(self->kickstart_fd);
    self->kickstart_fd = -1;
  }
  return TRUE;
}

void pipe_mainmenu_downloader_delete(PipeComponent *instance) {
  PipeMainMenuDownloader *self = (PipeMainMenuDownloader *)instance;
  if (self->kickstart_fd != -1) {
    close(self->kickstart_fd);
  }
  free(self);
}


/***** PipeCallbackWrapper *****/


PipeCallbackWrapper *pipe_callback_wrapper_create(
    ssize_t (*write_callback)(const char *, size_t, void *),
    void *writedata,
    void (*finish_callback)(RequestState, void *),
    void *finishdata)
{
  INITIALIZE_PIPE(PipeCallbackWrapper,
                  pipe_callback_wrapper_process,
                  pipe_callback_wrapper_finished,
                  pipe_callback_wrapper_delete);
  self->write_data = writedata;
  self->finish_data = finishdata;
  self->write_callback = write_callback;
  self->finish_callback = finish_callback;
  return self;
}

gboolean pipe_callback_wrapper_process(PipeComponent *instance,
                                       char *buf, size_t len)
{
  PipeCallbackWrapper *self = (PipeCallbackWrapper *)instance;
  if (self->write_callback)
    self->write_callback(buf, len, self->write_data);
  return TRUE;
}

void pipe_callback_wrapper_finished(PipeComponent *instance,
                                    RequestState state)
{
  PipeCallbackWrapper *self = (PipeCallbackWrapper *)instance;
  if (self->finish_callback)
    self->finish_callback(state, self->finish_data);
}

void pipe_callback_wrapper_delete(PipeComponent *instance) {
  PipeCallbackWrapper *self = (PipeCallbackWrapper *)instance;
  free(self);
}

PipeLocalFile *pipe_local_file_create(const gchar *filename) {
  INITIALIZE_PIPE_WITH_FDSET(PipeLocalFile, NULL, NULL,
                             pipe_local_file_delete,
                             pipe_local_file_fdset,
                             pipe_local_file_handle_socket);
  self->filename = g_strdup(filename);
  self->fd = -1;
  return self;
}

void pipe_local_file_start(PipeLocalFile *self) {
  if (!self->filename) {
    pipe_component_finished((PipeComponent *)self, WEBVISTATE_NOT_FOUND);
  }

  self->fd = open(self->filename, O_RDONLY);
  if (self->fd == -1) {
    pipe_component_finished((PipeComponent *)self, WEBVISTATE_NOT_FOUND);
  }
}

void pipe_local_file_fdset(PipeComponent *instance, fd_set *readfd,
                           fd_set *writefd, fd_set *excfd, int *maxfd)
{
  PipeLocalFile *self = (PipeLocalFile *)instance;
  append_to_fdset(self->fd, readfd, maxfd);
}

gboolean pipe_local_file_handle_socket(PipeComponent *instance, int fd, int bitmask) {
  PipeLocalFile *self = (PipeLocalFile *)instance;
  return read_from_fd_to_pipe(instance, &self->fd, fd, bitmask);
}

void pipe_local_file_delete(PipeComponent *instance) {
  PipeLocalFile *self = (PipeLocalFile *)instance;
  g_free(self->filename);
  if (self->fd != -1)
    close(self->fd);
  free(self);
}


/***** PipeExternalDownloader *****/


PipeExternalDownloader *pipe_external_downloader_create(const gchar *url,
                                                        const gchar *command)
{
  INITIALIZE_PIPE_WITH_FDSET(PipeExternalDownloader, NULL, NULL,
                             pipe_external_downloader_delete,
                             pipe_external_downloader_fdset,
                             pipe_external_downloader_handle_socket);
  self->url = g_strdup(url);
  self->fd = -1;

  g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
        "external downloader:\nurl: %s\n%s", url, command);

  return self;
}

void pipe_external_downloader_start(PipeExternalDownloader *self) {
  // FIXME
  pipe_component_finished((PipeComponent *)self, WEBVISTATE_INTERNAL_ERROR);
}

void pipe_external_downloader_fdset(PipeComponent *instance, fd_set *readfd,
                                   fd_set *writefd, fd_set *excfd, int *maxfd)
{
  PipeExternalDownloader *self = (PipeExternalDownloader *)instance;
  append_to_fdset(self->fd, readfd, maxfd);
}

gboolean pipe_external_downloader_handle_socket(PipeComponent *instance,
                                                int fd, int bitmask)
{
  PipeExternalDownloader *self = (PipeExternalDownloader *)instance;
  return read_from_fd_to_pipe(instance, &self->fd, fd, bitmask);
}

void pipe_external_downloader_delete(PipeComponent *instance) {
  PipeExternalDownloader *self = (PipeExternalDownloader *)instance;
  if (self->fd != -1)
    close(self->fd);
  g_free(self->url);
  g_free(self);
}


/***** PipeLibquvi *****/


PipeLibquvi *pipe_libquvi_create(const gchar *url) {
  INITIALIZE_PIPE_WITH_FDSET(PipeLibquvi,
                             pipe_libquvi_parse,
                             pipe_libquvi_finished,
                             pipe_libquvi_delete,
                             pipe_libquvi_fdset,
                             pipe_libquvi_handle_socket);
  self->url = g_strdup(url);
  self->pid = -1;
  self->quvi_output = -1;
  return self;
}

void pipe_libquvi_start(PipeLibquvi *self) {
  GError *error = NULL;
  gchar *argv[] = {"quvi", "--xml", self->url};

  g_spawn_async_with_pipes(NULL, argv, NULL,
                           G_SPAWN_SEARCH_PATH | G_SPAWN_STDERR_TO_DEV_NULL,
                           NULL, NULL, &self->pid, NULL, &self->quvi_output,
                           NULL, &error);
  if (error) {
    g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
          "Calling quvi failed: %s", error->message);
    pipe_component_finished((PipeComponent *)self, WEBVISTATE_SUBPROCESS_FAILED);
  }
}

void pipe_libquvi_fdset(PipeComponent *instance, fd_set *readfd,
                        fd_set *writefd, fd_set *excfd, int *maxfd)
{
  PipeLibquvi *self = (PipeLibquvi *)instance;
  append_to_fdset(self->quvi_output, readfd, maxfd);
}

gboolean pipe_libquvi_handle_socket(PipeComponent *instance,
                                    int fd, int bitmask)
{
  PipeLibquvi *self = (PipeLibquvi *)instance;
  return read_from_fd_to_pipe(instance, &self->quvi_output, fd, bitmask);
}

gboolean pipe_libquvi_parse(PipeComponent *instance, char *buf, size_t len) {
  PipeLibquvi *self = (PipeLibquvi *)instance;
  if (!self->parser) {
    self->parser = xmlCreatePushParserCtxt(NULL, NULL, buf, len, "quvioutput.xml");
    g_assert(self->parser);
  } else {
    xmlParseChunk(self->parser, buf, len, 0);
  }

  return FALSE;
}

void pipe_libquvi_finished(PipeComponent *instance, RequestState state) {
  if (state != WEBVISTATE_FINISHED_OK) {
    pipe_component_finished(instance, state);
    return;
  }

  PipeLibquvi *self = (PipeLibquvi *)instance;
  if (!self->parser) {
    g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "No output from quvi!");
    pipe_component_finished(instance, WEBVISTATE_SUBPROCESS_FAILED);
    return;
  }

  xmlParseChunk(self->parser, NULL, 0, 1);
  xmlDoc *doc = self->parser->myDoc;

  xmlChar *dump;
  int dumpLen;
  xmlDocDumpMemory(doc, &dump, &dumpLen);
  g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "quvi output:\n%s", dump);
  xmlFree(dump);
  dump = NULL;

  xmlChar *encoded_url = quvi_xml_get_stream_url(doc);
  if (!encoded_url) {
    g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "No url in quvi output!");
    pipe_component_finished(instance, WEBVISTATE_SUBPROCESS_FAILED);
    return;
  }

  /* URLs in quvi XML output are encoded by curl_easy_escape */
  char *url = curl_unescape((char *)encoded_url, strlen((char *)encoded_url));
  xmlChar *title = quvi_xml_get_stream_title(doc);

  MenuBuilder *menu_builder = menu_builder_create();
  menu_builder_set_title(menu_builder, (char *)title);
  menu_builder_append_link_plain(menu_builder, url, (char *)title, NULL);
  char *menu = menu_builder_to_string(menu_builder);
  if (self->pipe_data.next) {
    pipe_component_append(self->pipe_data.next, menu, strlen(menu));
    pipe_component_finished(self->pipe_data.next, state);
  }

  free(menu);
  menu_builder_delete(menu_builder);
  xmlFree(title);
  curl_free(url);
  xmlFree(encoded_url);
}

xmlChar *quvi_xml_get_stream_url(xmlDoc *doc) {
  xmlNode *root = xmlDocGetRootElement(doc);
  xmlNode *node = root->children;
  while (node) {
    if (xmlStrEqual(node->name, BAD_CAST "link")) {
      xmlNode *link_child = node->children;
      while (link_child) {
        if (xmlStrEqual(link_child->name, BAD_CAST "url")) {
          return xmlNodeGetContent(link_child);
        }
        link_child = link_child->next;
      }
    }
    node = node->next;
  }

  return NULL;
}

xmlChar *quvi_xml_get_stream_title(xmlDoc *doc) {
  xmlNode *root = xmlDocGetRootElement(doc);
  xmlNode *node = root->children;
  while (node) {
    if (xmlStrEqual(node->name, BAD_CAST "page_title")) {
      return xmlNodeGetContent(node);
    }
    node = node->next;
  }

  return NULL;
}

void pipe_libquvi_delete(PipeComponent *instance) {
  PipeLibquvi *self = (PipeLibquvi *)instance;
  if (self->quvi_output != -1) {
    close(self->quvi_output);
  }
  if (self->pid != -1) {
    g_spawn_close_pid(self->pid);
  }
  if (self->parser) {
    xmlFreeParserCtxt(self->parser);
  }
  g_free(self->url);
  free(self);
}


/***** Utility functions *****/


CURL *start_curl(const char *url, CURLM *curlmulti, PipeComponent *instance) {
  CURL *curl = curl_easy_init();
  if (!curl) {
    g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "curl initialization failed");
    return NULL;
  }

  g_log(LIBWEBVI_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Downloading %s", url);

  curl_easy_setopt(curl, CURLOPT_USERAGENT, WEBVID_USER_AGENT);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_wrapper);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, instance);
  if (url)
    curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_PRIVATE, instance);

  // FIXME: headers, cookies

  return curl;
}

void append_to_fdset(int fd, fd_set *fdset, int *maxfd) {
  if (fd != -1) {
    FD_SET(fd, fdset);
    if (fd > *maxfd)
      *maxfd = fd;
  }
}

gboolean read_from_fd_to_pipe(PipeComponent *instance, int *instance_fd_ptr,
                              int fd, int bitmask)
{
  const int buflen = 4096;
  char buf[buflen];
  ssize_t numbytes;

  gboolean owned_socket = (fd == -1) || (fd == *instance_fd_ptr);
  gboolean read_operation = ((bitmask & WEBVI_SELECT_READ) != 0) ||
    (bitmask == WEBVI_SELECT_CHECK);

  if (owned_socket && read_operation) {
    numbytes = read(fd, buf, buflen);
    if (numbytes < 0) {
      /* error */
      pipe_component_finished(instance, WEBVISTATE_IO_ERROR);
    } else if (numbytes == 0) {
      /* end of file */
      pipe_component_finished(instance, WEBVISTATE_FINISHED_OK);
      close(fd);
      *instance_fd_ptr = -1;
    } else {
      pipe_component_append(instance, buf, numbytes);
    }
    
    return TRUE;
  } else {
    return FALSE;
  }
}
