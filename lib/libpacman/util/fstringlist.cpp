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

FStringList *f_stringlist_addf(FStringList *self, const char *fmt, ...)
{
	va_list ap;

	if(self == NULL) {
		self = new FStringList();
	}

	va_start(ap, fmt);
	self->vaddf(fmt, ap);
	va_end(ap);
	return self;
}

int f_stringlist_clear(FStringList *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	self->clear();
	return 0;
}

/* vim: set ts=2 sw=2 noet: */
