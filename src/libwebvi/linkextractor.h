/*
 * linkextractor.h
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

#ifndef __LINKEXTRACTOR_H
#define __LINKEXTRACTOR_H

#include <glib.h>
#include "linktemplates.h"

typedef struct LinkExtractor LinkExtractor;

LinkExtractor *link_extractor_create(const LinkTemplates *link_templates,
                                     const gchar *baseurl);
void link_extractor_delete(LinkExtractor *link_extractor);
void link_extractor_append(LinkExtractor *link_extractor, const char *buf, size_t len);
GPtrArray *link_extractor_get_links(LinkExtractor *link_extractor);

#endif // __LINKEXTRACTOR_H
