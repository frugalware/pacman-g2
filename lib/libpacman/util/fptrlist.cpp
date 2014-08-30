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

#include "util/fptrlist.h"

#include "fstdlib.h"

/* Add items to a list in sorted order. Use the given comparison function to
 * determine order.
 */
FPtrList *f_ptrlist_add_sorted(FPtrList *list, void *data, _pacman_fn_cmp fn)
{
#ifndef F_NOCOMPAT
	FPtrListIterator *add, *end = f_ptrlist_end(list), *previous = NULL, *iter = f_ptrlist_first(list);

	add = new FCListItem();
	add->m_data = data;

	/* Find insertion point. */
	while(iter != end) {
		if(fn(f_ptrlistitem_data(add), f_ptrlistitem_data(iter)) <= 0) break;
		previous = iter;
		iter = iter->m_next;
	}

	/*  Insert node before insertion point. */
	add->m_previous = previous;
	add->m_next = iter;

	if(iter != NULL) {
		iter->m_previous = add;   /*  Not at end.  */
	}

	if(previous != NULL) {
		previous->m_next = add;       /*  In middle.  */
	} else {
		list = add;           /*  Start or empty, new list head.  */
	}

	return(list);
#else
	if (list == NULL) {
		list = new FPtrList();
		list->add(data);
	} else {
		// FIXME: Search insertion point.
	}
	return list;
#endif
}

/* Remove an item in a list. Use the given comparison function to find the
 * item.
 * If the item is found, 'data' is pointing to the removed element.
 * Otherwise, it is set to NULL.
 * Return the new list (without the removed element).
 */
FPtrList *_pacman_list_remove(FPtrList *haystack, void *needle, _pacman_fn_cmp fn, void **data)
{
#ifndef F_NOCOMPAT
	FPtrListIterator *end = f_ptrlist_end(haystack), *i = f_ptrlist_first(haystack);

	if(*data != end) {
		*data = NULL;
	}

	while(i) {
		if(i->m_data == NULL) {
			continue;
		}
		if(fn(needle, i->m_data) == 0) {
			break;
		}
		i = i->m_next;
	}

	if(i) {
		/* we found a matching item */
		if(i->m_next) {
			i->m_next->m_previous = i->m_previous;
		}
		if(i->m_previous) {
			i->m_previous->m_next = i->m_next;
		}
		if(i == haystack) {
			/* The item found is the first in the chain */
			haystack = haystack->m_next;
		}

		if(data) {
			*data = i->m_data;
		}
		i->m_data = NULL;
		free(i);
	}

	return(haystack);
#else
	// FIXME: Implement me
	return haystack;
#endif
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

	for(auto it = list->rbegin(), end = list->rend(); it != end; it = it->previous()) {
		newlist = newlist->add(f_ptrlistitem_data(it));
	}

	return(newlist);
}

void *f_ptrlistitem_data(const FPtrListIterator *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return self->m_data;
}

int f_ptrlistitem_insert_after(FPtrListIterator *self, FPtrListIterator *previous)
{
	FPtrListIterator *next;

	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(previous != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	next = previous->m_next;
	previous->m_next = self;
	self->m_next = next;
	self->m_previous = previous;
#ifndef F_NOCOMPAT
	if (next != NULL)
#endif
	next->m_previous = self;
	return 0;
}

FPtrListIterator *f_ptrlistitem_next(FPtrListIterator *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return self->m_next;
}

size_t f_ptrlistiterator_count(const FPtrListIterator *self, const FPtrListIterator *end)
{
	int size = 0;

	for(const FPtrListIterator *it = self; it != end; it = it->next(), size++);
	return size;
}

FPtrList *f_ptrlist_new(void)
{
#ifndef F_NOCOMPAT
	FPtrList *self;

	ASSERT((self = f_zalloc(sizeof(*self))) != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return self;
#else
	return new FPtrList();
#endif
}

int f_ptrlist_delete(FPtrList *self, FVisitor *visitor)
{
#ifndef F_NOCOMPAT
	return f_ptrlist_clear(self, visitor);
#else
	self->clear(visitor);
	delete self;
	return 0;
#endif
}

#ifndef F_NOCOMPAT
FPtrList *FPtrList::add(const void *data)
{
	return f_ptrlist_add(this, data);
}
#endif

// FIXME: Change return value to bool
FPtrList *f_ptrlist_add(FPtrList *list, void *data)
{
#ifndef F_NOCOMPAT
	FPtrList *ptr, *lp;

	ptr = list;
	if(ptr == NULL) {
		ptr = f_ptrlist_new();
		if(ptr == NULL) {
			return(NULL);
		}
	}

	lp = f_ptrlist_last(ptr);
	if(lp == ptr && lp->m_data == NULL) {
		/* nada */
	} else {
		lp->m_next = f_ptrlist_new();
		if(lp->m_next == NULL) {
			return(NULL);
		}
		lp->m_next->m_previous = lp;
		lp = lp->m_next;
	}

	lp->m_data = data;

	return(ptr);
#else
	list->add(data);

	return list;
#endif
}

int f_ptrlist_clear(FPtrList *list, FVisitor *visitor)
{
	FPtrListIterator *end = f_ptrlist_end(list), *it = f_ptrlist_first(list), *next;

	while(it != end) {
		next = it->next();
		if(visitor != NULL) {
			f_visit(f_ptrlistitem_data(it), visitor);
		}
		free(it);
		it = next;
	}
	return 0;
}

size_t f_ptrlist_count(const FPtrList *self)
{
#ifndef F_NOCOMPAT
	return f_ptrlistiterator_count(self->begin(), self->end());
#else
	return self->size();
#endif
}

FPtrListIterator *f_ptrlist_end(FPtrList *self)
{
	return (FPtrListIterator *)f_ptrlist_end_const(self);
}

const FPtrListIterator *f_ptrlist_end_const(const FPtrList *self)
{
	return NULL;
}

FPtrListIterator *f_ptrlist_first(FPtrList *self)
{
	return (FPtrListIterator *)f_ptrlist_first_const(self);
}

const FPtrListIterator *f_ptrlist_first_const(const FPtrList *self)
{
	return (FPtrListIterator *)self;
}

FPtrListIterator *f_ptrlist_last(FPtrList *self)
{
	return (FPtrListIterator *)f_ptrlist_last_const(self);
}

const FPtrListIterator *f_ptrlist_last_const(const FPtrList *self)
{
	const FPtrListIterator *it;

	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	it = f_ptrlist_first_const(self);
	while(it->m_next != f_ptrlist_end_const(self)) {
		it = it->m_next;
	}
	return it;
}

FPtrListIterator *f_ptrlist_rend(FPtrList *self)
{
	return (FPtrListIterator *)f_ptrlist_rend_const(self);
}

const FPtrListIterator *f_ptrlist_rend_const(const FPtrList *self)
{
	return NULL;
}

/* vim: set ts=2 sw=2 noet: */
