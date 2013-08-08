/*
 * webvicontext.h
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

#ifndef __WEBVICONTEXT_H
#define __WEBVICONTEXT_H

#include <stdbool.h>
#include <curl/curl.h>
#include <glib.h>
#include "libwebvi.h"
#include "linktemplates.h"
#include "pipecomponent.h"

typedef struct WebviContext WebviContext;
typedef struct WebviRequest WebviRequest;

WebviContext *get_context_by_handle(WebviCtx handle);

WebviCtx webvi_context_initialize(void);
void webvi_context_cleanup(WebviCtx ctxhandle);
void webvi_context_cleanup_all();

void webvi_context_set_debug(WebviContext *self,
                             bool d);
void webvi_context_set_template_path(WebviContext *self,
                                     const char *path);
const char *webvi_context_get_template_path(const WebviContext *self);
void webvi_context_set_menu_script_path(WebviContext *self,
                                       const char *path);
const char *webvi_context_get_menu_script_path(const WebviContext *self);
const LinkTemplates *get_link_templates(WebviContext *self);
CURLM *webvi_context_get_curl_multi_handle(WebviContext *self);
void webvi_context_set_timeout_callback(WebviContext *ctx,
                                        webvi_timeout_callback callback);
void webvi_context_set_timeout_data(WebviContext *ctx, void *data);

WebviHandle webvi_context_add_request(WebviContext *self, WebviRequest *req);
void webvi_context_remove_request(WebviContext *self, WebviHandle h);
WebviRequest *webvi_context_get_request(WebviContext *self, WebviHandle h);

WebviResult webvi_context_fdset(WebviContext *ctx, fd_set *readfd,
                                fd_set *writefd, fd_set *excfd, int *max_fd);
void webvi_context_handle_socket_action(
  WebviContext *ctx, int sockfd, int ev_bitmask, long *running_handles);

void webvi_context_add_finished_message(WebviContext *messages,
                                        const WebviRequest *req,
                                        RequestState status_code,
                                        const char *message_text);
WebviMsg *webvi_context_next_message(WebviContext *ctx,
                                     int *remaining_messages);

#endif // __WEBVICONTEXT_H
