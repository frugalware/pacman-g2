/*
 *  stringlist.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* pacman-g2 */
#include "stringlist.h"

#include "util.h"

/* Test for existence of a string in a FStringList
 */
int _pacman_list_is_strin(const char *needle, FStringList *haystack)
{
	FStringList *lp;

	for(lp = haystack; lp; lp = f_ptrlistitem_next(lp)) {
		const char *str = f_stringlistitem_to_str(lp);

		if(str && !strcmp(str, needle)) {
			return(1);
		}
	}
	return(0);
}

/* Filter out any duplicate strings in a list.
 *
 * Not the most efficient way, but simple to implement -- we assemble
 * a new list, using is_in() to check for dupes at each iteration.
 *
 */
FStringList *_pacman_list_remove_dupes(FStringList *list)
{
	FStringList *i, *newlist = NULL;

	for(i = list; i; i = f_ptrlistitem_next(i)) {
		const char *str = f_stringlistitem_to_str(i);

		if(!_pacman_list_is_strin(str, newlist)) {
			newlist = _pacman_stringlist_append(newlist, str);
		}
	}
	return newlist;
}

FStringList *_pacman_list_strdup(FStringList *list)
{
	FStringList *newlist = NULL;
	FStringList *lp;

	for(lp = list; lp; lp = f_ptrlistitem_next(lp)) {
		newlist = f_stringlist_append(newlist, f_stringlistitem_to_str(lp));
	}

	return(newlist);
}

static
struct FVisitor f_stringlistitem_destroyvisitor = {
	.fn = (FVisitorFunc)free,
	.data = NULL,
};

FStringListItem *f_stringlistitem_new(const char *str)
{
#ifndef F_NOCOMPAT
	return f_ptrlistitem_new(f_strdup(str));
#else
#endif
}

int f_stringlistitem_delete(FStringListItem *self)
{
#ifndef F_NOCOMPAT
	return f_ptrlistitem_delete(self, &f_stringlistitem_destroyvisitor);
#else
#endif
}

const char *f_stringlistitem_to_str(const FStringListItem *self)
{
#ifndef F_NOCOMPAT
	return f_ptrlistitem_data(self);
#else
	return self->to_str;
#endif
}

FStringList *f_stringlist_new(void)
{
	return f_list_new();
}

int f_stringlist_delete(FStringList *self)
{
	return f_ptrlist_delete(self, &f_stringlistitem_destroyvisitor);
}

FStringList *f_stringlist_append(FStringList *list, const char *s)
{
	return f_ptrlist_append(list, f_strdup(s));
}

FStringList *f_stringlist_append_stringlist(FStringList *dest, const FStringList *src)
{
	const FStringList *lp;

	for(lp = src; lp; lp = f_ptrlistitem_next(lp)) {
		dest = f_stringlist_append(dest, f_stringlistitem_to_str(lp));
	}
	return dest;
}

int f_stringlist_clear(FStringList *self)
{
	return f_ptrlist_clear(self, &f_stringlistitem_destroyvisitor);
}

/* vim: set ts=2 sw=2 noet: */
