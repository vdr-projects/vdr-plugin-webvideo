/*
 * link.h
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

#ifndef __LINK_H
#define __LINK_H

typedef enum {
  LINK_ACTION_PARSE,
  LINK_ACTION_STREAM,
  LINK_ACTION_EXTERNAL_COMMAND
} LinkActionType;

typedef struct Link Link;
typedef struct LinkAction LinkAction;

struct Link *link_create(const char *href, const char *title, LinkActionType type);
const char *link_get_href(const struct Link *self);
const char *link_get_title(const struct Link *self);
LinkActionType link_get_type(const struct Link *self);
void link_delete(struct Link *self);
void g_free_link(gpointer link);

struct LinkAction *link_action_create(LinkActionType type, const char *command);
LinkActionType link_action_get_type(const struct LinkAction *self);
const char *link_action_get_command(const struct LinkAction *self);
void link_action_delete(struct LinkAction *self);
const char *link_action_type_to_string(LinkActionType atype);

#endif // __LINK_H
