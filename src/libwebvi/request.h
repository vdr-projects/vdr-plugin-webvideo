/*
 * request.h
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

#ifndef __REQUEST_H
#define __REQUEST_H

#include "pipecomponent.h"

struct WebviContext;

typedef struct WebviRequest WebviRequest;

WebviRequest *request_create(const char *url, struct WebviContext *ctx);
void request_set_write_callback(WebviRequest *instance, webvi_callback func);
void request_set_read_callback(WebviRequest *instance, webvi_callback func);
void request_set_write_data(WebviRequest *instance, void *data);
void request_set_read_data(WebviRequest *instance, void *data);
const char *request_get_url(const WebviRequest *instance);
gboolean request_start(WebviRequest *instance);
void request_stop(WebviRequest *instance);
void request_fdset(WebviRequest *instance, fd_set *readfd,
                   fd_set *writefd, fd_set *excfd, int *max_fd);
gboolean request_handle_socket(WebviRequest *instance, int sockfd, int ev_bitmask);
void request_delete(WebviRequest *instance);

#endif // __REQUEST_H
