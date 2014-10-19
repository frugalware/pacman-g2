/*
 *  fptrlist.cpp
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
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

#include "fptrlist.h"
#include "fstdlib.h"

void *f_ptrlistitem_data(const FPtrListIterator *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return **self;
}

FPtrListIterator *f_ptrlistitem_next(FPtrListIterator *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return self->next();
}

size_t f_ptrlistiterator_count(const FPtrListIterator *self, const FPtrListIterator *end)
{
	int size = 0;

	for(const FPtrListIterator *it = self; it != end; it = it->next(), size++);
	return size;
}

FPtrList *f_ptrlist_new(void)
{
	return new FPtrList();
}

int f_ptrlist_delete(FPtrList *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	self->clear();
	delete self;
	return 0;
}

FPtrList *f_ptrlist_add(FPtrList *list, void *data)
{
	if (list == NULL) {
		list = new FPtrList();
	}
	list->add(data);
	return list;
}

size_t f_ptrlist_count(const FPtrList *self)
{
	if(self != NULL) {
		return self->size();
	}
	return 0;
}

FPtrListIterator *f_ptrlist_end(FPtrList *self)
{
	if(self != NULL) {
		return self->end();
	}
	return NULL;
}

FPtrListIterator *f_ptrlist_first(FPtrList *self)
{
	if(self != NULL) {
		return self->begin();
	}
	return NULL;
}

/* vim: set ts=2 sw=2 noet: */
