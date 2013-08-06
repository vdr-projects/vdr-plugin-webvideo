/*
 * linktemplates.h
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

#ifndef __LINKTEMPLATES_H
#define __LINKTEMPLATES_H

#include "link.h"

typedef struct LinkTemplates LinkTemplates;

LinkTemplates *link_templates_create();
void link_templates_delete(LinkTemplates *conf);
void link_templates_load(LinkTemplates *conf, const char *filename);
int link_templates_size(const LinkTemplates *conf);
const struct LinkAction *link_templates_get_action(const LinkTemplates *conf,
                                                   const char *url);

#endif // __LINKTEMPLATES_H
