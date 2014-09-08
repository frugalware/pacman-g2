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

/* Add items to a list in sorted order. Use the given comparison function to
 * determine order.
 */
FPtrList *f_ptrlist_add_sorted(FPtrList *list, void *data, _pacman_fn_cmp fn)
{
	if(list == NULL) {
		list = new FPtrList();
	}

	FPtrListIterator *add = new FPtrListItem(data);

	/* Find insertion point. */
	FCListItem *previous = list;
	for(FCListItem *end = f_ptrlist_end(list); previous->next() != end; previous = previous->next()) {
		if(fn(data, f_ptrlistitem_data((FPtrListItem *)previous->next())) <= 0) {
			break;
		}
	}

	/*  Insert node before insertion point. */
	add->insert_after(previous);
	return list;
}

/* Remove an item in a list. Use the given comparison function to find the
 * item.
 * If the item is found, return true and 'data' is pointing to the removed element.
 * Otherwise, return false and 'data' it is set to NULL.
 * Return the new list (without the removed element).
 */
bool _pacman_list_remove(FPtrList *haystack, void *needle, _pacman_fn_cmp fn, void **data)
{
	ASSERT(haystack != NULL, RET_ERR(PM_ERR_WRONG_ARGS, false));

	if(data != NULL) {
		*data = NULL;
	}

	for(FPtrListIterator *i = f_ptrlist_first(haystack), *end = f_ptrlist_end(haystack); i != end; i = i->next()) {
		if(fn(needle, i->m_data) == 0) {
			/* we found a matching item */
			if(i->m_next) {
				i->m_next->m_previous = i->m_previous;
			}
			if(i->m_previous) {
				i->m_previous->m_next = i->m_next;
			}
			if(data) {
				*data = i->m_data;
			}
			delete i;
			return true;
		}
	}
	return false;
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
		newlist->add(f_ptrlistitem_data(it));
	}

	return(newlist);
}

void *f_ptrlistitem_data(const FPtrListIterator *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return self->m_data;
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

FPtrList &FPtrList::add(void *data)
{
	(new FPtrListItem(data))->insert_after(last());
	return *this;
}

FPtrList *f_ptrlist_add(FPtrList *list, void *data)
{
	if (list == NULL) {
		list = new FPtrList();
	}
	list->add(data);
	return list;
}

int f_ptrlist_clear(FPtrList *list, FVisitor *visitor)
{
	ASSERT(list != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	FPtrListIterator *end = f_ptrlist_end(list), *it = f_ptrlist_first(list), *next;

	while(it != end) {
		next = it->next();
		if(visitor != NULL) {
			f_visit(f_ptrlistitem_data(it), visitor);
		}
		free(it);
		it = next;
	}
	list->clear(/* visitor */);
	return 0;
}

size_t f_ptrlist_count(const FPtrList *self)
{
	if(self != NULL) {
#ifndef F_NOCOMPAT
		return f_ptrlistiterator_count(self->begin(), self->end());
#else
		return self->size();
#endif
	}
	return 0;
}

FPtrListIterator *f_ptrlist_end(FPtrList *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
	return self->end();
}

const FPtrListIterator *f_ptrlist_end_const(const FPtrList *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
	return self->end();
}

FPtrListIterator *f_ptrlist_first(FPtrList *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
	return self->begin();
}

const FPtrListIterator *f_ptrlist_first_const(const FPtrList *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
	return self->begin();
}

FPtrListIterator *f_ptrlist_last(FPtrList *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
	return self->last();
}

const FPtrListIterator *f_ptrlist_last_const(const FPtrList *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
	return self->last();
}

FPtrListIterator *f_ptrlist_rend(FPtrList *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
	return self->rend();
}

const FPtrListIterator *f_ptrlist_rend_const(const FPtrList *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
	return self->rend();
}

/* vim: set ts=2 sw=2 noet: */
