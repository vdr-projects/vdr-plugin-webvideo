/*
 * menubuilder.h
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

#ifndef __MENUBUILDER_H
#define __MENUBUILDER_H

#include <glib.h>
#include "link.h"

typedef struct MenuBuilder MenuBuilder;

MenuBuilder *menu_builder_create();
void menu_builder_set_title(MenuBuilder *self, const char *title);
char *menu_builder_to_string(MenuBuilder *self);
void menu_builder_append_link_plain(MenuBuilder *self, const char *href,
                                    const char *title, const char *class);
void menu_builder_append_link(MenuBuilder *self, const Link *link);
void menu_builder_append_link_list(MenuBuilder *self, GPtrArray *links);
void menu_builder_delete(MenuBuilder *self);

#endif // __MENUBUILDER_H
