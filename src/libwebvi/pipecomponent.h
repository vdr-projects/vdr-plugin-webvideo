/*
 * pipecomponent.h
 *
 * Copyright (c) 2013 Antti Ajanki <antti.ajanki@iki.fi>
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

#ifndef __PIPECOMPONENT_H
#define __PIPECOMPONENT_H

#include <stdlib.h>
#include <sys/select.h>
#include <glib.h>
#include <curl/curl.h>
#include "libwebvi.h"

typedef struct PipeComponent {
  struct PipeComponent *next;
  RequestState state;
  gboolean (*process)(struct PipeComponent *self, char *, size_t);
  void (*finished)(struct PipeComponent *self, RequestState state);
  void (*delete)(struct PipeComponent *self);
  void (*fdset)(struct PipeComponent *self, fd_set *readfd,
                fd_set *writefd, fd_set *excfd, int *max_fd);
  gboolean (*handle_socket)(struct PipeComponent *self, int fd, int ev_bitmask);
} PipeComponent;

struct WebviContext;
struct LinkTemplates;

typedef struct PipeDownloader PipeDownloader;
typedef struct PipeLinkExtractor PipeLinkExtractor;
typedef struct PipeCallbackWrapper PipeCallbackWrapper;
typedef struct PipeMainMenuDownloader PipeMainMenuDownloader;
typedef struct PipeLocalFile PipeLocalFile;
typedef struct PipeExternalDownloader PipeExternalDownloader;
typedef struct PipeLibquvi PipeLibquvi;

void pipe_component_initialize(PipeComponent *self,
    gboolean (*process_cb)(PipeComponent *, char *, size_t),
    void (*done_cb)(PipeComponent *, RequestState state),
    void (*delete_cb)(PipeComponent *));
void pipe_component_initialize_fdset(PipeComponent *self,
    gboolean (*process_cb)(PipeComponent *, char *, size_t),
    void (*done_cb)(PipeComponent *, RequestState state),
    void (*delete_cb)(PipeComponent *),
    void (*fdset_cb)(PipeComponent *, fd_set *, fd_set *, fd_set *, int *),
    gboolean (*handle_socket_cb)(PipeComponent *, int, int));
void pipe_component_append(PipeComponent *self, char *buf, size_t length);
void pipe_component_finished(PipeComponent *self, RequestState state);
void pipe_component_set_next(PipeComponent *self, PipeComponent *next);
RequestState pipe_component_get_state(const PipeComponent *self);
void pipe_fdset(PipeComponent *head, fd_set *readfd,
                fd_set *writefd, fd_set *excfd, int *max_fd);
gboolean pipe_handle_socket(PipeComponent *head, int sockfd, int ev_bitmask);
void pipe_delete_full(PipeComponent *head);

PipeDownloader *pipe_downloader_create(const char *url, CURLM *curlmulti);
void pipe_downloader_start(PipeDownloader *self);

PipeMainMenuDownloader *pipe_mainmenu_downloader_create(struct WebviContext *context);
void pipe_mainmenu_downloader_start(PipeMainMenuDownloader *self);

PipeLinkExtractor *pipe_link_extractor_create(
  const struct LinkTemplates *link_templates, const gchar *baseurl);

PipeLocalFile *pipe_local_file_create(const gchar *filename);
void pipe_local_file_start(PipeLocalFile *self);

PipeCallbackWrapper *pipe_callback_wrapper_create(
    ssize_t (*write_callback)(const char *, size_t, void *),
    void *writedata,
    void (*finish_callback)(RequestState, void *),
    void *finishdata);

PipeExternalDownloader *pipe_external_downloader_create(const gchar *url,
                                                        const gchar *command);
void pipe_external_downloader_start(PipeExternalDownloader *self);

PipeLibquvi *pipe_libquvi_create(const gchar *url);
void pipe_libquvi_start(PipeLibquvi *self);

#endif // __PIPECOMPONENT_H
