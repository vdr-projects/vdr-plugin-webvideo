/*
 * libwebvi.c
 *
 * Copyright (c) 2010-2013 Antti Ajanki <antti.ajanki@iki.fi>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdarg.h>
#include <string.h>
#include <libxml/xmlversion.h>
#include "libwebvi.h"
#include "webvicontext.h"
#include "request.h"
#include "version.h"

static const char *VERSION = "libwebvi/" LIBWEBVI_VERSION;

struct WebviErrorMessage {
  WebviResult code;
  const char *message;
};

int webvi_global_init() {
  LIBXML_TEST_VERSION
  return 0;
}

void webvi_cleanup() {
  webvi_context_cleanup_all();
}

WebviCtx webvi_initialize_context(void) {
  return webvi_context_initialize();
}

void webvi_cleanup_context(WebviCtx ctxhandle) {
  webvi_context_cleanup(ctxhandle);
}

const char* webvi_version(void) {
  return VERSION;
}

const char* webvi_strerror(WebviResult err) {
  static struct WebviErrorMessage error_messages[] = 
    {{WEBVIERR_OK, "Succeeded"},
     {WEBVIERR_INVALID_HANDLE, "Invalid handle"},
     {WEBVIERR_INVALID_PARAMETER, "Invalid parameter"},
     {WEBVIERR_UNKNOWN_ERROR, "Internal error"}};

  for (int i=0; i<(sizeof(error_messages)/sizeof(error_messages[0])); i++) {
    if (err == error_messages[i].code) {
      return error_messages[i].message;
    }
  }

  return "Internal error";
}

WebviResult webvi_set_config(WebviCtx ctxhandle, WebviConfig conf, ...) {
  va_list argptr;
  const char *p;
  WebviResult res = WEBVIERR_OK;

  WebviContext *ctx = get_context_by_handle(ctxhandle);
  if (!ctx)
    return WEBVIERR_INVALID_HANDLE;

  va_start(argptr, conf);

  switch (conf) {
  case WEBVI_CONFIG_TEMPLATE_PATH:
    p = va_arg(argptr, char *);
    webvi_context_set_template_path(ctx, p);
    break;
  case WEBVI_CONFIG_DEBUG:
    p = va_arg(argptr, char *);
    webvi_context_set_debug(ctx, strcmp(p, "0") != 0);
    break;
  case WEBVI_CONFIG_TIMEOUT_CALLBACK:
  {
    webvi_timeout_callback callback = va_arg(argptr, webvi_timeout_callback);
    webvi_context_set_timeout_callback(ctx, callback);
    break;
  }
  case WEBVI_CONFIG_TIMEOUT_DATA:
  {
    void *data = va_arg(argptr, void *);
    webvi_context_set_timeout_data(ctx, data);
    break;
  }
  default:
    res = WEBVIERR_INVALID_PARAMETER;
  };

  va_end(argptr);

  return res;
}

WebviHandle webvi_new_request(WebviCtx ctxhandle, const char *href) {
  WebviContext *ctx = get_context_by_handle(ctxhandle);
  if (!ctx)
          return WEBVI_INVALID_HANDLE;

  WebviRequest *req = request_create(href, ctx);
  return webvi_context_add_request(ctx, req);
}

WebviResult webvi_start_request(WebviCtx ctxhandle, WebviHandle h) {
  WebviContext *ctx = get_context_by_handle(ctxhandle);
  if (!ctx)
    return WEBVIERR_INVALID_HANDLE;

  WebviRequest *req = webvi_context_get_request(ctx, h);
  if (!req)
    return WEBVIERR_INVALID_HANDLE;

  request_start(req);

  return WEBVIERR_OK;
}

WebviResult webvi_stop_request(WebviCtx ctxhandle, WebviHandle h) {
  WebviContext *ctx = get_context_by_handle(ctxhandle);
  if (!ctx)
    return WEBVIERR_INVALID_HANDLE;

  WebviRequest *req = webvi_context_get_request(ctx, h);
  if (!req)
    return WEBVIERR_INVALID_HANDLE;

  request_stop(req);

  return WEBVIERR_OK;
}

WebviResult webvi_delete_request(WebviCtx ctxhandle, WebviHandle h) {
  WebviContext *ctx = get_context_by_handle(ctxhandle);
  if (!ctx)
    return WEBVIERR_INVALID_HANDLE;

  WebviResult res = webvi_stop_request(ctxhandle, h);
  if (res != WEBVIERR_OK)
    return res;

  webvi_context_remove_request(ctx, h);

  return WEBVIERR_OK;
}

WebviResult webvi_set_opt(WebviCtx ctxhandle, WebviHandle h, WebviOption opt, ...) {
  WebviContext *ctx = get_context_by_handle(ctxhandle);
  if (!ctx)
    return WEBVIERR_INVALID_HANDLE;

  WebviRequest *req = webvi_context_get_request(ctx, h);
  if (!req)
    return WEBVIERR_INVALID_HANDLE;

  va_list argptr;
  WebviResult res = WEBVIERR_OK;

  va_start(argptr, opt);

  switch (opt) {
  case WEBVIOPT_WRITEFUNC:
    request_set_write_callback(req, va_arg(argptr, webvi_callback));
    break;
  case WEBVIOPT_READFUNC:
    request_set_read_callback(req, va_arg(argptr, webvi_callback));
    break;
  case WEBVIOPT_WRITEDATA:
    request_set_write_data(req, va_arg(argptr, void *));
    break;
  case WEBVIOPT_READDATA:
    request_set_read_data(req, va_arg(argptr, void *));
    break;
  default:
    res = WEBVIERR_INVALID_PARAMETER;
  };

  va_end(argptr);

  return res;
}

WebviResult webvi_get_info(WebviCtx ctxhandle, WebviHandle h, WebviInfo info, ...) {
  WebviContext *ctx = get_context_by_handle(ctxhandle);
  if (!ctx)
    return WEBVIERR_INVALID_HANDLE;

  WebviRequest *req = webvi_context_get_request(ctx, h);
  if (!req)
    return WEBVIERR_INVALID_HANDLE;

  va_list argptr;
  WebviResult res = WEBVIERR_OK;

  va_start(argptr, info);

  switch (info) {
  case WEBVIINFO_URL:
  {
    char **output = va_arg(argptr, char **);
    if (output) {
      *output = NULL;

      const char *url = request_get_url(req);
      if (url) {
        *output = malloc(strlen(url)+1);
        if (*output) {
          strcpy(*output, url);
        }
      }
    }
    break;
  }

  case WEBVIINFO_CONTENT_LENGTH:
  {
    // FIXME
    long *content_length = va_arg(argptr, long *);
    if (content_length)
      *content_length = -1;
    break;
  }

  case WEBVIINFO_CONTENT_TYPE:
  {
    // FIXME
    char **output = va_arg(argptr, char **);
    if (output) {
      *output = malloc(1);
      **output = '\0';
    }
    break;
  }

  case WEBVIINFO_STREAM_TITLE:
  {
    // FIXME
    char **output = va_arg(argptr, char **);
    if (output) {
      *output = malloc(1);
      **output = '\0';
    }
    break;
  }

  default:
    res = WEBVIERR_INVALID_PARAMETER;
  };

  va_end(argptr);

  return res;
}

WebviResult webvi_fdset(WebviCtx ctxhandle, fd_set *readfd, fd_set *writefd,
                        fd_set *excfd, int *max_fd) {
  WebviContext *ctx = get_context_by_handle(ctxhandle);
  if (!ctx)
    return WEBVIERR_INVALID_HANDLE;

  return webvi_context_fdset(ctx, readfd, writefd, excfd, max_fd);
}

WebviResult webvi_perform(WebviCtx ctxhandle, int sockfd, int ev_bitmask,
                          long *running_handles) {
  WebviContext *ctx = get_context_by_handle(ctxhandle);
  if (!ctx)
    return WEBVIERR_INVALID_HANDLE;

  webvi_context_handle_socket_action(ctx, sockfd, ev_bitmask, running_handles);

  return WEBVIERR_OK;
}

WebviMsg *webvi_get_message(WebviCtx ctxhandle, int *remaining_messages) {
  WebviContext *ctx = get_context_by_handle(ctxhandle);
  if (!ctx)
    return NULL;

  return webvi_context_next_message(ctx, remaining_messages);
}

int webvi_process_some(WebviCtx ctx, int timeout_seconds) {
  fd_set readfds;
  fd_set writefds;
  fd_set excfds;
  int maxfd;
  int s;
  WebviResult res;
  struct timeval timeout;
  long running_handles;

  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  FD_ZERO(&excfds);
  res = webvi_fdset(ctx, &readfds, &writefds, &excfds, &maxfd);
  if (res != WEBVIERR_OK) {
    return -1;
  }

  timeout.tv_sec = timeout_seconds;
  timeout.tv_usec = 0;
  s = select(maxfd+1, &readfds, &writefds, NULL, &timeout);

  if (s == -1) {
    // error
    return -1;
  } else if (s == 0) {
    // timeout
    webvi_perform(ctx, WEBVI_SELECT_TIMEOUT, WEBVI_SELECT_CHECK, &running_handles);
  } else {
    // handle one fd
    for (int fd=0; fd<=maxfd; fd++) {
      if (FD_ISSET(fd, &readfds)) {
        webvi_perform(ctx, fd, WEBVI_SELECT_READ, &running_handles);
      } else if (FD_ISSET(fd, &writefds)) {
        webvi_perform(ctx, fd, WEBVI_SELECT_WRITE, &running_handles);
      } else if (FD_ISSET(fd, &excfds)) {
        webvi_perform(ctx, fd, WEBVI_SELECT_EXCEPTION, &running_handles);
      }
    }
  }

  return running_handles;
}
