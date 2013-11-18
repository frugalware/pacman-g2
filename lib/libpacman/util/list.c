/*
 *  list.c
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
#include <stdio.h>
#include <assert.h>

/* pacman-g2 */
#include "list.h"

#include "fstdlib.h"
#include "util.h"

void f_listitem_delete(FListItem *item, FVisitor *visitor)
{
	ASSERT(item != NULL, return);

	f_visit(item, visitor);
	free(item);
}

int f_list_contains(const FList *list, FListItemComparatorFunc comparator, const void *comparator_data)
{
	return f_list_find(list, comparator, comparator_data) != NULL;
}

int f_list_count(const FList *list)
{
	int i;

	for(i = 0; list; list = list->next, i++);
	return i;
}

int f_list_empty(const FList *list)
{
	return list == NULL;
}

void f_list_foreach(const FList *list, FListItemVisitorFunc visitor, void *visitor_data)
{
	for(; list != NULL; list = list->next) {
		visitor((FListItem *)list, visitor_data);
	}
}

static
int _pacman_ptrlistitem_ptrcmp(const FListItem *item, const void *ptr) {
	return f_ptrcmp(item->data, ptr);
}

int _pacman_list_is_in(void *needle, const pmlist_t *haystack)
{
	return f_list_contains(haystack, _pacman_ptrlistitem_ptrcmp, needle);
}

FListItem *f_list_find(const FList *list, FListItemComparatorFunc comparator, const void *comparator_data)
{
	for(; list != NULL; list = list->next) {
		if(comparator(list, comparator_data) == 0) {
			break;
		}
	}
	return (FListItem *)list;
}

pmlist_t *_pacman_list_add(pmlist_t *list, void *data)
{
	pmlist_t *ptr, *lp;

	ptr = list;
	if(ptr == NULL) {
		ptr = _pacman_list_new();
		if(ptr == NULL) {
			return(NULL);
		}
	}

	lp = _pacman_list_last(ptr);
	if(lp == ptr && lp->data == NULL) {
		/* nada */
	} else {
		lp->next = _pacman_list_new();
		if(lp->next == NULL) {
			return(NULL);
		}
		lp->next->prev = lp;
		lp->last = NULL;
		lp = lp->next;
	}

	lp->data = data;
	ptr->last = lp;

	return(ptr);
}

/* Add items to a list in sorted order. Use the given comparison function to
 * determine order.
 */
pmlist_t *_pacman_list_add_sorted(pmlist_t *list, void *data, _pacman_fn_cmp fn)
{
	pmlist_t *add;
	pmlist_t *prev = NULL;
	pmlist_t *iter = list;

	add = _pacman_list_new();
	add->data = data;

	/* Find insertion point. */
	while(iter) {
		if(fn(add->data, iter->data) <= 0) break;
		prev = iter;
		iter = iter->next;
	}

	/*  Insert node before insertion point. */
	add->prev = prev;
	add->next = iter;

	if(iter != NULL) {
		iter->prev = add;   /*  Not at end.  */
	} else {
		if (list != NULL) {
			list->last = add;   /* Added new to end, so update the link to last. */
		}
	}

	if(prev != NULL) {
		prev->next = add;       /*  In middle.  */
	} else {
		if(list == NULL) {
			add->last = add;
		} else {
			add->last = list->last;
			list->last = NULL;
		}
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
pmlist_t *_pacman_list_remove(pmlist_t *haystack, void *needle, _pacman_fn_cmp fn, void **data)
{
	pmlist_t *i = haystack;

	if(data) {
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
			if(haystack->next) {
				haystack->next->last = haystack->last;
			}
			haystack = haystack->next;
		} else if(i == haystack->last) {
			/* The item found is the last in the chain */
			haystack->last = i->prev;
		}

		if(data) {
			*data = i->data;
		}
		i->data = NULL;
		free(i);
	}

	return(haystack);
}

pmlist_t *_pacman_list_last(pmlist_t *list)
{
	if(list == NULL) {
		return(NULL);
	}

	assert(list->last != NULL);

	return(list->last);
}

/* Reverse the order of a list
 *
 * The caller is responsible for freeing the old list
 */
pmlist_t *_pacman_list_reverse(pmlist_t *list)
{
	/* simple but functional -- we just build a new list, starting
	 * with the old list's tail
	 */
	pmlist_t *newlist = NULL;
	pmlist_t *lp;

	for(lp = list->last; lp; lp = lp->prev) {
		newlist = _pacman_list_add(newlist, lp->data);
	}

	return(newlist);
}

FPtrListItem *f_ptrlistitem_new(void *ptr)
{
	FPtrListItem *item = f_zalloc(sizeof(*item));

	if(item != NULL) {
		item->data = ptr;
	}
	return item;
}

FPtrList *f_ptrlist_new(void)
{
	FPtrListItem *item = f_ptrlistitem_new(NULL);
	item->last = item;
	return (FPtrList *)item;
}

void f_ptrlistitem_delete(FListItem *item, FVisitor *visitor)
{
	ASSERT(item != NULL, return);

	f_listitem_delete(item, visitor);
}

void f_ptrlist_free(FPtrList *list, FVisitor *visitor)
{
	f_ptrlist_clear(list, visitor);
}

void f_ptrlist_clear(FPtrList *list, FVisitor *visitor)
{
	FPtrList *next;

	while(list != NULL) {
		next = list->next;
		if(visitor != NULL) {
			f_visit(list->data, visitor);
		}
		free(list);
		list = next;
	}
}

/* vim: set ts=2 sw=2 noet: */
