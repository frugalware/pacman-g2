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
#include "fptrlist_p.h"

#include "fstdlib.h"
#include "util.h"

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

/* vim: set ts=2 sw=2 noet: */
