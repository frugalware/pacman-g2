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
	if(haystack == NULL) {
		return 0;
	}

	return haystack->contains(needle) ? 1 : 0;
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
	
	if(list == NULL) {
		return NULL;
	}


	for(auto i = list->begin(), end = list->end(); i != end; ++i) {
		const char *str = *i;

		if(!_pacman_list_is_strin(str, newlist)) {
			newlist = f_stringlist_add(newlist, str);
		}
	}
	return newlist;
}

const char *f_stringlistitem_to_str(const FStringListIterator *self)
{
#ifndef F_NOCOMPAT
	return (const char *)f_ptrlistitem_data(self);
#else
	return self->to_str;
#endif
}

int f_stringlist_delete(FStringList *self)
{
	delete self;
	return 0;
}

FStringList *f_stringlist_add(FStringList *self, const char *s)
{
	if(self == NULL) {
		self = new FStringList();
	}
	return &self->add(s);
}

FStringList *f_stringlist_add_stringlist(FStringList *self, const FStringList *src)
{
	if(src == NULL || src->empty()) {
		return self;
	}
	if(self == NULL) {
		self = new FStringList();
	}
	return &self->add(*src);
}

FStringList *f_stringlist_addf(FStringList *self, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	self = f_stringlist_vaddf(self, fmt, ap);
	va_end(ap);
	return self;
}

FStringList *f_stringlist_vaddf(FStringList *self, const char *fmt, va_list ap)
{
	if(self == NULL) {
		self = new FStringList();
	}
	return &self->vaddf(fmt, ap);
}

int f_stringlist_clear(FStringList *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	self->clear();
	return 0;
}

FStringList::FStringList()
{ }

FStringList::FStringList(const FStringList &o)
	: FStringList()
{
	operator = (o);
}

FStringList::FStringList(FStringList &&o)
	: FList(std::move(o))
{ }

FStringList &FStringList::operator = (const FStringList &o)
{
	for(auto lp = o.begin(), end = o.end(); lp != end; ++lp) {
		f_stringlist_add(this, *lp);
	}

	return *this;
}

FStringList &FStringList::operator = (FStringList &&o)
{
	swap(o);
	return *this;
}

FStringList &FStringList::add_nocopy(char *s)
{
	FList::add(s);
	return *this;
}

FStringList &FStringList::add(const char *s)
{
	return add_nocopy(f_strdup(s));
}

FStringList &FStringList::add(const FStringList &o)
{
	for(auto lp = o.begin(), end = o.end(); lp != end; ++lp) {
		add((const char *)*lp);
	}
	return *this;
}

FStringList &FStringList::addf(const char *fmt, ...)
{
	va_list ap;

  va_start(ap, fmt);
	vaddf(fmt, ap);
	va_end(ap);
	return *this;
}

FStringList &FStringList::vaddf(const char *fmt, va_list ap)
{
	char *dest;

	vasprintf(&dest, fmt, ap);
	return add_nocopy(dest);
}

bool FStringList::contains(const char *s) const
{
	return find(s) != end();
}

FStringList::iterator FStringList::find(const char *s)
{
	return find_if(
			[&] (const char *o) -> bool
			{ return f_streq(o, s); });
}

FStringList::const_iterator FStringList::find(const char *s) const
{
	return find_if(
			[&] (const char *o) -> bool
			{ return f_streq(o, s); });
}

FStringList::size_type FStringList::remove(const char *s)
{
	return remove_if(
			[&] (const char *v) -> bool
			{ return strcmp(v, s) == 0; });
}

/* vim: set ts=2 sw=2 noet: */
