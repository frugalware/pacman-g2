/*
 *  stringlist.c
 *
 *  Copyright (c) 2013 by Michel Hermier <hermier@frugalware.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

#include "config.h"

#include <string.h>

#include "fstringlist.h"

#include "fstring.h"

FList *f_stringlist_append (FList *list, const char *str) {
	return f_list_append (list, f_strdup (str));
}

FList *f_stringlist_deep_copy (FList *list) {
	return f_list_deep_copy (list, (FCopyFunc)f_strdup, NULL);
}

FList *f_stringlist_find (FList *list, const char *str) {
	return f_list_detect (list, (FDetectFunc)strcmp, (void *)str);
}

FList *f_stringlist_uniques (FList *list) {
	  return f_list_uniques (list, (FCompareFunc)strcmp, NULL);
}

/* vim: set ts=2 sw=2 noet: */
