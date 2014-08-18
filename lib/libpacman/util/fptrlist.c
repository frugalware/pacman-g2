/*
 *  fptrlist.c
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

#include "util/fptrlist.h"

#include "fstdlib.h"

#ifndef F_NOCOMPAT
#define previous prev
#endif

/* Add items to a list in sorted order. Use the given comparison function to
 * determine order.
 */
FPtrList *_pacman_list_add_sorted(FPtrList *list, void *data, _pacman_fn_cmp fn)
{
	FPtrListItem *add, *end = f_ptrlist_end(list), *prev = NULL, *iter = f_ptrlist_first(list);

	add = _pacman_list_new();
	add->data = data;

	/* Find insertion point. */
	while(iter != end) {
		if(fn(add->data, iter->data) <= 0) break;
		prev = iter;
		iter = iter->next;
	}

	/*  Insert node before insertion point. */
	add->prev = prev;
	add->next = iter;

	if(iter != NULL) {
		iter->prev = add;   /*  Not at end.  */
	}

	if(prev != NULL) {
		prev->next = add;       /*  In middle.  */
	} else {
		list = add;           /*  Start or empty, new list head.  */
	}

	return(list);
}

/* Remove an item in a list. Use the given comparison function to find the
 * item.
 * If the item is found, 'data' is pointing to the removed element.
 * Otherwise, it is set to NULL.
 * Return the new list (without the removed element).
 */
FPtrList *_pacman_list_remove(FPtrList *haystack, void *needle, _pacman_fn_cmp fn, void **data)
{
	FPtrListItem *end = f_ptrlist_end(haystack), *i = f_ptrlist_first(haystack);

	if(data != end) {
		*data = NULL;
	}

	while(i) {
		if(i->data == NULL) {
			continue;
		}
		if(fn(needle, i->data) == 0) {
			break;
		}
		i = i->next;
	}

	if(i) {
		/* we found a matching item */
		if(i->next) {
			i->next->prev = i->prev;
		}
		if(i->prev) {
			i->prev->next = i->next;
		}
		if(i == haystack) {
			/* The item found is the first in the chain */
			haystack = haystack->next;
		}

		if(data) {
			*data = i->data;
		}
		i->data = NULL;
		free(i);
	}

	return(haystack);
}

/* Reverse the order of a list
 *
 * The caller is responsible for freeing the old list
 */
FPtrList *_pacman_list_reverse(FPtrList *list)
{
	/* simple but functional -- we just build a new list, starting
	 * with the old list's tail
	 */
	FPtrList *newlist = f_ptrlist_new();
	FPtrListItem *it;

	for(it = f_ptrlist_last(list); it; it = f_ptrlistitem_previous(it)) {
		newlist = _pacman_list_add(newlist, it->data);
	}

	return(newlist);
}

FPtrList *f_list_new()
{
#ifndef F_NOCOMPAT
	return NULL;
#else
	FPtrList *self;

	ASSERT((self = f_zalloc(sizeof(*self))) != NULL, return NULL);

	f_ptrlist_init(self);
	return self;
#endif
}

int f_list_contains(const FPtrList *list, FPtrListItemComparatorFunc comparator, const void *comparator_data)
{
	return f_list_find_const(list, comparator, comparator_data) != NULL;
}

FPtrListItem *f_list_end(FPtrList *self)
{
	return (FPtrListItem *)f_list_end_const(self);
}

const FPtrListItem *f_list_end_const(const FPtrList *self)
{
	return NULL;
}

FPtrListItem *f_list_find(FPtrList *self, FPtrListItemComparatorFunc comparator, const void *comparator_data)
{
	return (FPtrListItem *)f_list_find_const(self, comparator, comparator_data);
}

const FPtrListItem *f_list_find_const(const FPtrList *list, FPtrListItemComparatorFunc comparator, const void *comparator_data)
{
	const FPtrListItem *end = f_list_end_const(list);
	const FPtrListItem *it;

	for(it = f_ptrlist_first_const(list); it != end; it = it->next) {
		if(comparator(it, comparator_data) == 0) {
			break;
		}
	}
	return it != end ? it : NULL;
}

void f_list_foreach(const FPtrList *list, FPtrListItemVisitorFunc visitor, void *visitor_data)
{
	const FPtrListItem *it;

	for(it = f_ptrlist_first_const(list); it != f_list_end_const(list); it = it->next) {
		visitor((FPtrListItem *)it, visitor_data);
	}
}

FPtrListItem *f_list_rend(FPtrList *self)
{
	return (FPtrListItem *)f_list_rend_const(self);
}

const FPtrListItem *f_list_rend_const(const FPtrList *self)
{
	return NULL;
}

FPtrListItem *f_ptrlistitem_new(void *ptr)
{
	FPtrListItem *item = f_zalloc(sizeof(*item));

	if(item != NULL) {
		item->data = ptr;
	}
	return item;
}

int f_ptrlistitem_delete(FPtrListItem *self, FVisitor *visitor)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	f_visit(self, visitor);
	free(self);
	return 0;
}

void *f_ptrlistitem_data(const FPtrListItem *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return self->data;
}

int f_ptrlistitem_insert_after(FPtrListItem *self, FPtrListItem *previous)
{
	FPtrListItem *next;

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

FPtrListItem *f_ptrlistitem_next(FPtrListItem *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return self->next;
}

FPtrListItem *f_ptrlistitem_previous(FPtrListItem *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return self->previous;
}

int f_ptrlistitem_ptrcmp(const FPtrListItem *item, const void *ptr) {
	return f_ptrcmp(item->data, ptr);
}

FPtrList *f_ptrlist_new(void)
{
	FPtrList *self;

	ASSERT((self = f_zalloc(sizeof(*self))) != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	f_ptrlist_init(self);
	return self;
}

int f_ptrlist_delete(FPtrList *self, FVisitor *visitor)
{
	return f_ptrlist_fini(self, visitor);
}

int f_ptrlist_init(FPtrList *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	return 0;
}

int f_ptrlist_fini(FPtrList *self, FVisitor *visitor)
{
	return f_ptrlist_clear(self, visitor);
}

#ifndef F_NOCOMPAT
FPtrList *f_ptrlist_append(FPtrList *list, void *data)
{
	pmlist_t *ptr, *lp;

	ptr = list;
	if(ptr == NULL) {
		ptr = f_ptrlist_new();
		if(ptr == NULL) {
			return(NULL);
		}
	}

	lp = f_ptrlist_last(ptr);
	if(lp == ptr && lp->data == NULL) {
		/* nada */
	} else {
		lp->next = f_ptrlist_new();
		if(lp->next == NULL) {
			return(NULL);
		}
		lp->next->prev = lp;
		lp = lp->next;
	}

	lp->data = data;

	return(ptr);
}
#else
int f_ptrlist_append(FPtrList *self, void *data)
{
}
#endif

int f_ptrlist_clear(FPtrList *list, FVisitor *visitor)
{
	FPtrListItem *end = f_ptrlist_end(list), *it = f_ptrlist_first(list), *next;

	while(it != end) {
		next = f_ptrlistitem_next(it);
		if(visitor != NULL) {
			f_visit(f_ptrlistitem_data(it), visitor);
		}
		free(it);
		it = next;
	}
	return 0;
}

int f_ptrlist_contains_ptr(const FPtrList *list, const void *ptr)
{
	return f_ptrlist_contains(list, f_ptrlistitem_ptrcmp, ptr);
}

int f_ptrlist_count(const FPtrList *self)
{
	const FPtrListItem *it;
	int i;

	for(i = 0, it = f_ptrlist_first_const(self); it != f_list_end_const(self); it = it->next, i++);
	return i;
}

bool f_ptrlist_empty(const FPtrList *self)
{
	return self == NULL;
}

FPtrListItem *f_ptrlist_first(FPtrList *self)
{
	return (FPtrListItem *)f_ptrlist_first_const(self);
}

const FPtrListItem *f_ptrlist_first_const(const FPtrList *self)
{
	return (FPtrListItem *)self;
}

FPtrListItem *f_ptrlist_last(FPtrList *self)
{
	return (FPtrListItem *)f_ptrlist_last_const(self);
}

const FPtrListItem *f_ptrlist_last_const(const FPtrList *self)
{
	const FPtrListItem *it;

	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	it = f_ptrlist_first_const(self);
	while(it->next != f_ptrlist_end_const(self)) {
		it = it->next;
	}
	return it;
}

/* vim: set ts=2 sw=2 noet: */
