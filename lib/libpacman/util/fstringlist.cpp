/*
 *  fstringlist.cpp
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
#include "fstringlist.h"

#include "util.h"

/* Test for existence of a string in a FStringList
 */
int _pacman_list_is_strin(const char *needle, FStringList *haystack)
{
	for(auto lp = haystack->begin(), end = haystack->end(); lp != end; lp = lp->next()) {
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
	FStringList *newlist = NULL;

	for(auto i = list->begin(), end = list->end(); i != end; i = i->next()) {
		const char *str = f_stringlistitem_to_str(i);

		if(!_pacman_list_is_strin(str, newlist)) {
			newlist = f_stringlist_add(newlist, str);
		}
	}
	return newlist;
}

FStringList *_pacman_list_strdup(FStringList *list)
{
	FStringList *newlist = NULL;

	for(auto lp = list->begin(), end = list->end(); lp != end; lp = lp->next()) {
		newlist = f_stringlist_add(newlist, f_stringlistitem_to_str(lp));
	}

	return(newlist);
}

static
struct FVisitor f_stringlistitem_destroyvisitor = {
	.fn = (FVisitorFunc)free,
	.data = NULL,
};

const char *f_stringlistitem_to_str(const FStringListIterator *self)
{
#ifndef F_NOCOMPAT
	return f_ptrlistitem_data(self);
#else
	return self->to_str;
#endif
}

int f_stringlist_delete(FStringList *self)
{
	return f_ptrlist_delete(self, &f_stringlistitem_destroyvisitor);
}

FStringList *f_stringlist_add(FStringList *list, const char *s)
{
	if(list == NULL) {
		list = new FStringList();
	}
	list->add(f_strdup(s));
	return list;
}

FStringList *f_stringlist_add_stringlist(FStringList *dest, const FStringList *src)
{
	for(auto lp = src->begin(), end = src->end(); lp != end; lp = lp->next()) {
		dest = f_stringlist_add(dest, f_stringlistitem_to_str(lp));
	}
	return dest;
}

int f_stringlist_clear(FStringList *self)
{
	return f_ptrlist_clear(self, &f_stringlistitem_destroyvisitor);
}

FStringList::FStringList()
{ }

FStringList::FStringList(const FStringList &o)
	: FStringList()
{
	operator = (o);
}

FStringList::FStringList(FStringList &&o)
	: FPtrList(std::move(o))
{ }

FStringList &FStringList::operator = (const FStringList &o)
{
	for(auto lp = o.begin(), end = o.end(); lp != end; lp = lp->next()) {
		f_stringlist_add(this, f_stringlistitem_to_str(lp));
	}

	return *this;
}

FStringList &FStringList::operator = (FStringList &&o)
{
	swap(o);
	return *this;
}

/* vim: set ts=2 sw=2 noet: */
