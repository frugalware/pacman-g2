/*
 *  flist.c
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

#include "util/flist_p.h"

#include "fstdlib.h"

#include <assert.h>

#ifndef F_NOCOMPAT
#define previous prev
#endif

int f_listitem_delete(FListItem *self, FVisitor *visitor)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	f_visit(self, visitor);
	free(self);
	return 0;
}

int f_listitem_insert_after(FListItem *self, FListItem *previous)
{
	FListItem *next;

	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(previous != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	next = previous->next;
	previous->next = self;
	self->next = next;
	self->previous = previous;
#ifndef F_NOCOMPAT
	if (next != NULL)
#endif
	next->previous = self;
	return 0;
}

FListItem *f_listitem_next(FListItem *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return self->next;
}

FListItem *f_listitem_previous(FListItem *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return self->previous;
}

FList *f_list_new()
{
#ifndef F_NOCOMPAT
	return NULL;
#else
	FList *self;

	ASSERT((self = f_zalloc(sizeof(*self))) != NULL, return NULL);

	f_list_init(self);
	return self;
#endif
}

int f_list_delete(FList *self, FVisitor *visitor)
{
	return f_list_fini(self, visitor);
}

int f_list_init(FList *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

#ifdef F_NOCOMPAT
	self->as_FListItem.next = self->as_FListItem.previous = &self->as_FListItem;
#endif
	return 0;
}

int f_list_fini(FList *self, FVisitor *visitor)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	return f_list_clear(self, visitor);
}

int f_list_all_match(const FList *list, const FMatcher *matcher)
{
	const FListItem *it;

	if(f_list_empty(list)) {
		return 0;
	}

	for(it = f_list_first_const(list); it != f_list_end_const(list); it = it->next) {
		if(f_match(it, matcher) == 0) {
			return 0;
		}
	}
	return 1;
}

int f_list_any_match(const FList *list, const FMatcher *matcher)
{
	const FListItem *it;

	for(it = f_list_first_const(list); it != f_list_end_const(list); it = it->next) {
		if(f_match(it, matcher) != 0) {
			return 1;
		}
	}
	return 0;
}

int f_list_append(FList *self, FListItem *item)
{
	return f_listitem_insert_after(item, f_list_last(self));
}

int f_list_append_unique(FList *self, FListItem *item, FListItemComparatorFunc comparator)
{
	if (!f_list_contains(self, comparator, item)) {
		return f_list_append(self, item);
	}
	return -1;
}

int f_list_clear(FList *self, FVisitor *visitor)
{
	return -1;
}

int f_list_contains(const FList *list, FListItemComparatorFunc comparator, const void *comparator_data)
{
	return f_list_find_const(list, comparator, comparator_data) != NULL;
}

int f_list_count(const FList *list)
{
	const FListItem *it;
	int i;

	for(i = 0, it = f_list_first_const(list); it != f_list_end_const(list); it = it->next, i++);
	return i;
}

int f_list_empty(const FList *list)
{
	return list == NULL;
}

FListItem *f_list_end(FList *self)
{
	return (FListItem *)f_list_end_const(self);
}

const FListItem *f_list_end_const(const FList *self)
{
#ifndef F_NOCOMPAT
	return NULL;
#else
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return &self->as_FListItem;
#endif
}

FListItem *f_list_find(FList *self, FListItemComparatorFunc comparator, const void *comparator_data)
{
	return (FListItem *)f_list_find_const(self, comparator, comparator_data);
}

const FListItem *f_list_find_const(const FList *list, FListItemComparatorFunc comparator, const void *comparator_data)
{
	const FListItem *end = f_list_end_const(list);
	const FListItem *it;

	for(it = f_list_first_const(list); it != end; it = it->next) {
		if(comparator(it, comparator_data) == 0) {
			break;
		}
	}
	return it != end ? it : NULL;
}

FListItem *f_list_first(FList *self)
{
	return (FListItem *)f_list_first_const(self);
}

const FListItem *f_list_first_const(const FList *self)
{
	return (FListItem *)self;
}

void f_list_foreach(const FList *list, FListItemVisitorFunc visitor, void *visitor_data)
{
	const FListItem *it;

	for(it = f_list_first_const(list); it != f_list_end_const(list); it = it->next) {
		visitor((FListItem *)it, visitor_data);
	}
}

FListItem *f_list_last(FList *self)
{
	return (FListItem *)f_list_last_const(self);
}

const FListItem *f_list_last_const(const FList *self)
{
	const FListItem *it;

	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	it = f_list_first_const(self);
	while(it->next != f_list_end_const(self)) {
		it = it->next;
	}
	return it;
}

FListItem *f_list_rend(FList *self)
{
	return (FListItem *)f_list_rend_const(self);
}

const FListItem *f_list_rend_const(const FList *self)
{
#ifndef F_NOCOMPAT
	return NULL;
#else
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return &self->as_FListItem;
#endif
}

