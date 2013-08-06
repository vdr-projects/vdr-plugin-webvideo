/*
 * urlutils.h
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

#ifndef __URLUTILS_H
#define __URLUTILS_H

#include <glib.h>

gchar *relative_url_to_absolute(const gchar *baseurl, const gchar *href);
gchar *url_scheme(const gchar *baseurl);
gchar *url_root(const gchar *baseurl);
gchar *url_path_including_file(const gchar *baseurl);
gchar *url_path_dirname(const gchar *baseurl);
gchar *url_path_and_query(const gchar *baseurl);

#endif // __URLUTILS_H
