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

void f_stringlist_detach (FList *list) {
	f_list_detach (list, (FCopyFunc)f_strdup, NULL);
}

/* Condense a list of strings into one long (space-delimited) string
 *  */
char *f_stringlist_join (FList *stringlist, const char *sep)
{
	char *str;
	size_t size = 1, sep_size = strlen (sep);
	FList *lp;

	f_foreach (lp, stringlist) {
		size += strlen(lp->data) + sep_size;
	}
	str = f_malloc(size);
	if (str == NULL) {
		return NULL;
	}
	str[0] = '\0';
	f_foreach (lp, stringlist) {
		strcat(str, lp->data);
		strcat(str, sep);
	}
	/* shave off the last sep */
	str[strlen(str) - sep_size] = '\0';

	return(str);
}

FList *f_stringlist_find (FList *list, const char *str) {
	return f_list_detect (list, (FDetectFunc)strcmp, (void *)str);
}

FList *f_stringlist_remove_all (FList *list, const char *str) {
	FList *excludes = f_list_new ();

	_f_list_exclude (&list, &excludes, (FDetectFunc)strcmp, str);
	f_list_delete (excludes, (FVisitorFunc)f_free, NULL);
	return list;
}

FList *f_stringlist_uniques (FList *list) {
	  return f_list_uniques (list, (FCompareFunc)strcmp, NULL);
}

/* vim: set ts=2 sw=2 noet: */
