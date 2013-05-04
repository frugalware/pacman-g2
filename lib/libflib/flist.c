/*
 *  list.c
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

#include <assert.h>

#include "flist.h"

#include "fstdlib.h"

static
void *f_ptrcpy(const void *p) {
	return (void *)p;
}

static
void f_list_accumulator (void *data, FList **list) {
	*list = f_list_append (*list, data);
}

FList *f_list_new () {
	return NULL;
}

void f_list_delete (FList *list, FVisitorFunc fn, void *user_data) {
	while ((list = f_list_free (list, fn, user_data)) != NULL)
		/* Do nothing */;
}

FList *f_list_alloc (void *data) {
	FList *list = f_zalloc (sizeof(FList));

	if (list != NULL) {
		list->data = data;
	}
	return list;
}

/**
 * Free a list item.
 *
 * Returns the next item in the list or NULL if item was the last one in the list.
 */
FList *f_list_free (FList *item, FVisitorFunc fn, void *user_data) {
	FList *next = NULL;

	if (item != NULL) {
		if (fn != NULL) {
			fn (item->data, user_data);
		}
		next = item->next;
		if (item->prev != NULL) {
			item->prev->next = next;
		}
		if (next != NULL) {
			next->prev = item->prev;
		}
		free (item);
	}
	return next;
}

void *f_list_get (FList *item) {
	return item != NULL ? item->data : NULL;
}

void f_list_set (FList *item, void *data) {
	if (item) {
		item->data = data;
	}
}

size_t f_list_count(FList *list)
{
	size_t count;

	for (count = 0; list != NULL; list = list->next, count++)
		/* Do nothing */;
	return count;
}

FList *f_list_first (FList *list) {
	if (list != NULL) {
		for (; list->prev != NULL; list = list->prev)
			/* Do nothing */;
	}
	return list;
}

void f_list_insert_after (FList *item, FList *list) {
	FList *last = f_list_last (list);

	assert (item != NULL);
	if (list == NULL) {
		return;
	}
	list->prev = item;
	last->next = item->next;
	item->next = list;
	if (last->next != NULL) {
		last->next->prev = last;
	}
}

void f_list_insert_before (FList *item, FList *list) {
	FList *last = f_list_last (list);

	assert (item != NULL);
	if (list == NULL) {
		return;
	}
	last->next = item;
	item->prev = last;
	list->prev = item->prev;
	if (item->prev != NULL) {
		item->prev->next = list;
	}
}

FList *f_list_last (FList *list) {
	if (list != NULL) {
		for (; list->next != NULL; list = list->next)
			/* Do nothing */;
	}
	return list;
}

FList *f_list_next (FList *list) {
	if (list != NULL) {
		return list->next;
	}
	return NULL;
}

FList *f_list_next_filtered (FList *list, FDetectFunc dfn, void *user_data) {
	while (list != NULL && dfn (list, user_data) != 0) {
		list = list->next;
	}
	return NULL;
}

FList *f_list_previous (FList *list) {
	if (list != NULL) {
		return list->prev;
	} 
	return NULL;
}

FList *f_list_previous_filtered (FList *list, FDetectFunc dfn, void *user_data) {
	while (list != NULL && dfn (list, user_data) != 0) {
		list = list->prev;
	} 
	return NULL;
}

FList *f_list_append (FList *list, void *data) {
	return f_list_concat (list, f_list_alloc (data));
}

FList *f_list_concat (FList *list1, FList *list2) {
	if (list1 == NULL) {
		return list2;
	}
	f_list_insert_after (f_list_last (list1), list2);
	return list1;
}

FList *f_list_copy (FList *list) {
	return f_list_deep_copy (list, (FCopyFunc)f_ptrcpy, NULL);
}

FList *f_list_deep_copy (FList *list, FCopyFunc fn, void *user_data) {
	FList *newlist = NULL;

	for (; list; list = list->next) {
		newlist = f_list_append (newlist, fn (list->data, user_data));
	}
	return newlist;
}

void f_list_detach (FList *list, FCopyFunc fn, void *user_data) {
	for (; list != NULL; list = list->next) {
		list->data = fn (list->data, user_data);
	}
}

FList *f_list_detect (FList *list, FDetectFunc fn, void *user_data) {
	for (; list != NULL; list = list->next) {
		if (fn (list->data, user_data) == 0) {
			break;
		}
	}
	return list;
}

FList *f_list_filter (FList *list, FDetectFunc dfn, void *user_data) {
	FList *ret = NULL;

	for (; list != NULL; list = f_list_next_filtered (list, dfn, user_data)) {
		ret = f_list_append (ret, list->data);
	}
	return ret;
}

FList *f_list_find (FList *list, const void *ptr) {
	return f_list_search (list, ptr, (FCompareFunc)f_ptrcmp, NULL);
}

void f_list_foreach (FList *list, FVisitorFunc fn, void *user_data) {
	for (; list != NULL; list = list->next) {
		fn (list->data, user_data);
	}
}

void f_list_foreach_filtered (FList *list, FVisitorFunc fn, FDetectFunc dfn, void *user_data) {
	for (; list != NULL; list = f_list_next_filtered (list, dfn, user_data)) {
		fn (list->data, user_data);
	}
}

/* Reverse the order of a list
 *
 * The caller is responsible for freeing the old list
 */
FList *f_list_reverse (FList *list) {
	FList *ret = NULL;

	f_list_reverse_foreach (list, (FVisitorFunc)f_list_accumulator, &ret);
	return ret;
}

void f_list_reverse_foreach (FList *list, FVisitorFunc fn, void *user_data) {
	FList *last = list != NULL ? list->prev : NULL;
	for (list = f_list_last (list); list != last; list = list->prev) {
		fn (list->data, user_data);
	}
}

FList *f_list_search (FList *list, const void *ptr, FCompareFunc fn, void *user_data) {
	for (; list != NULL; list = list->next) {
		if (fn (list->data, ptr, user_data) == 0) {
			break;
		}
	}
	return list;
}

FList *f_list_uniques (FList *list, FCompareFunc fn, void *user_data) {
	FList *ret = NULL;

	for (; list != NULL; list = list->next) {
		if (f_list_search (ret, list->data, fn, user_data) == NULL) {
			ret = f_list_append (ret, list->data);
		}
	}
	return ret;
}

/* Add items to a list in sorted order. Use the given comparison function to
 * determine order.
 */
FList *f_list_add_sorted (FList *list, void *data, FCompareFunc fn, void *user_data) {
	FList *add = f_list_alloc (data);
	FList *prev = NULL;
	FList *iter = list;

	/* Find insertion point. */
	while (iter) {
		if (fn (add->data, iter->data, user_data) <= 0) break;
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
FList *f_list_remove (FList *haystack, void *needle, FCompareFunc fn, void **data) {
	FList *i = haystack;

	if(data) {
		*data = NULL;
	}

	while(i) {
		if(i->data == NULL) {
			continue;
		}
		if(fn(needle, i->data, NULL /* user_data */) == 0) {
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

/* vim: set ts=2 sw=2 noet: */
