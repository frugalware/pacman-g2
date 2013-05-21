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

#include <assert.h>

#include "flist.h"

#include "flistaccumulator.h"
#include "fstdlib.h"

/* DO NOT MAKE PUBLIC FOR NOW:
 * Require list implemantation change.
 */
static
void f_list_foreach_safe (FList *list, FVisitorFunc fn, void *user_data);

static
void *f_ptrcpy(const void *p) {
	return (void *)p;
}

FListItem *f_listitem_new (void *data) {
	FListItem *item = f_zalloc (sizeof(FList));

	if (item != NULL) {
		item->data = data;
	}
	return item;
}

static
void _f_listitem_delete (FListItem *item, FVisitor *visitor) {
	if (item != NULL) {
		f_visit (item->data, visitor);
		f_listitem_remove (item);
		f_free (item);
	}
}

/**
 * Remove the item from it's list and free it.
 */
void f_listitem_delete (FListItem *item, FVisitorFunc fn, void *user_data) {
	FVisitor visitor = {
		.fn = fn,
		.user_data = user_data
	};

	_f_listitem_delete (item, &visitor);
}

void *f_listitem_get (FListItem *item) {
	return item != NULL ? item->data : NULL;
}

void f_listitem_set (FListItem *item, void *data) {
	if (item) {
		item->data = data;
	}
}

FListItem *f_listitem_next (FListItem *item) {
	return item != NULL ? item->next : NULL;
}

FListItem *f_listitem_previous (FListItem *item) {
	return item != NULL ? item->prev : NULL;
}

/**
 * Remove an @item from the list it belongs.
 */
void f_listitem_remove (FListItem *item) {
	if (item != NULL) {
		if (item->prev != NULL) {
			item->prev->next = item->next;
		}
		if (item->next != NULL) {
			item->next->prev = item->prev;
		}
		item->prev = NULL;
		item->next = NULL;
	}
}

FList *f_list_new () {
	return NULL;
}

void f_list_delete (FList *list, FVisitorFunc fn, void *user_data) {
	FVisitor visitor = {
		.fn = fn,
		.user_data = user_data
	};

	f_list_foreach_safe (list, (FVisitorFunc)_f_listitem_delete, &visitor);
}

/**
 * Insert a @list after @item.
 */
void f_list_insert_after (FList *item, FList *list) {
	FList *last = f_list_last (list);

	assert (item != NULL);
	if (list == NULL) {
		return;
	}
	last->next = item->next;
	if (last->next != NULL) {
		last->next->prev = last;
	}
	item->next = list;
	list->prev = item;
}

/**
 * Insert a @list before @item.
 */
void f_list_insert_before (FList *item, FList *list) {
	FList *last = f_list_last (list);

	assert (item != NULL);
	if (list == NULL) {
		return;
	}
	list->prev = item->prev;
	if (list->prev != NULL) {
		list->prev->next = list;
	}
	last->next = item;
	item->prev = last;
}

FListItem *f_list_begin (FList *list) {
	return list;
}

FListItem *f_list_end (FList *list) {
	return NULL;
}

FListItem *f_list_rbegin (FList *list) {
	if (list != NULL) {
		for (; list->next != NULL; list = list->next)
			/* Do nothing */;
	}
	return list;
}

FListItem *f_list_rend (FList *list) {
	return NULL;
}

FListItem *f_list_first (FList *list) {
	return f_list_begin (list);
}

FListItem *f_list_last (FList *list) {
	return f_list_rbegin (list);
}

FList *f_list_add (FList *list, void *ptr) {
	return f_list_append (list, ptr);
}

/* Add items to a list in sorted order. Use the given comparison function to
 * determine order.
 */
FList *f_list_add_sorted (FList *list, void *data, FCompareFunc fn, void *user_data) {
	FList *add = f_listitem_new (data);
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

FList *f_list_append (FList *list, void *ptr) {
	return f_list_concat (list, f_listitem_new (ptr));
}

FList *f_list_append_unique (FList *list, void *ptr, FCompareFunc fn, void *user_data) {
	if (f_list_find_custom (list, ptr, fn, user_data) == NULL) {
		return f_list_append (list, ptr);
	}
	return list;
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

size_t f_list_count (FList *list) {
	FListItem *it = f_list_begin (list), *end = f_list_end (list);
	size_t count = 0;

	for (; it != end; it = it->next, ++count)
		/* Do nothing */;
	return count;
}

FList *f_list_deep_copy (FList *list, FCopyFunc fn, void *user_data) {
	FListItem *it = f_list_begin (list), *end = f_list_end (list);
	FListAccumulator listaccumulator;

	f_listaccumulator_init (&listaccumulator, f_list_new ());
	for (; it != end; it = it->next) {
		f_listaccumulate (fn (it->data, user_data), &listaccumulator);
	}
	return f_listaccumulator_fini (&listaccumulator);
}

void f_list_detach (FList *list, FCopyFunc fn, void *user_data) {
	FListItem *it = f_list_begin (list), *end = f_list_end (list);

	for (; it != end; it = it->next) {
		it->data = fn (it->data, user_data);
	}
}

FListItem *f_list_detect (FList *list, FDetectFunc dfn, void *user_data) {
	FListItem *it = f_list_begin (list), *end = f_list_end (list);

	for (; it != end; it = it->next) {
		if (dfn (it->data, user_data) == 0) {
			break;
		}
	}
	return it;
}

void _f_list_exclude (FList **list, FList **excludelist, FDetectFunc dfn, void *user_data) {
	FListAccumulator listaccumulator;
	FList *item, *next = *list;

	f_listaccumulator_init (&listaccumulator, *excludelist);
	while ((item = f_list_detect (next, dfn, user_data)) != NULL) {
		next = item->next;
		if (*list == item) {
			*list = (*list)->next;
		}
		f_listitem_remove (item);
		f_listaccumulate (item, &listaccumulator);
	}
	*excludelist = f_listaccumulator_fini (&listaccumulator);
}

FList *f_list_filter (FList *list, FDetectFunc dfn, void *user_data) {
	FListAccumulator listaccumulator;
	FDetector detector = {
		.fn = dfn,
		.user_data = user_data
	};
	FVisitor visitor = {
		.fn = (FVisitorFunc)f_listaccumulate,
		.user_data = &listaccumulator
	};
	FDetectVisitor detectvisitor = {
		.detect = &detector,
		.success = &visitor,
		.fail = NULL
	};

	f_listaccumulator_init (&listaccumulator, f_list_new ());
	f_list_foreach (list, (FVisitorFunc)f_detectvisit, &detectvisitor);
	return f_listaccumulator_fini (&listaccumulator);
}

FListItem *f_list_find (FList *list, const void *data) {
	return f_list_find_custom (list, data, (FCompareFunc)f_ptrcmp, NULL);
}

FListItem *f_list_find_custom (FList *list, const void *data, FCompareFunc cfn, void *user_data) {
	FCompareDetector comparedetector = {
		.fn = cfn,
		.ptr = data,
		.user_data = user_data
	};

	return f_list_detect (list, (FDetectFunc)f_comparedetect, &comparedetector);
}

void f_list_foreach (FList *list, FVisitorFunc fn, void *user_data) {
	FListItem *it = f_list_begin (list), *end = f_list_end (list);

	for (; it != end; it = it->next) {
		fn (it->data, user_data);
	}
}

/**
 * A foreach safe in the sence you can detach the current element.
 *
 * Differs from foreach since it pass an FListItem instead of data.
 */
void f_list_foreach_safe (FList *list, FVisitorFunc fn, void *user_data) {
	FListItem *it, *next = f_list_begin (list), *end = f_list_end (list);

	for (it = next; it != end; it = next) {
		next = it->next;
		fn (it, user_data);
	}
}

/* Reverse the order of a list
 *
 * The caller is responsible for freeing the old list
 */
FList *f_list_reverse (FList *list) {
	FListAccumulator listaccumulator;

	f_listaccumulator_init (&listaccumulator, f_list_new ());
	f_list_foreach (list, (FVisitorFunc)f_listreverseaccumulate, &listaccumulator);
	return f_listaccumulator_fini (&listaccumulator);
}

void f_list_reverse_foreach (FList *list, FVisitorFunc fn, void *user_data) {
	FListItem *it = f_list_rbegin (list), *end = f_list_rend (list);

	for (; it != end; it = it->prev) {
		fn (it->data, user_data);
	}
}

FList *f_list_uniques (FList *list, FCompareFunc fn, void *user_data) {
	FListItem *it = f_list_begin (list), *end = f_list_end (list);
	FList *uniques = f_list_new ();

	for (; it != end; it = it->next) {
		uniques = f_list_append_unique (uniques, it->data, fn, user_data);
	}
	return uniques;
}

void _f_list_remove (FList **list, FListItem *item) {
	if (*list == item) {
		*list = item->next;
	}
	f_listitem_remove (item);
}

/* Remove an item in a list. Use the given comparison function to find the
 * item.
 * If the item is found, 'data' is pointing to the removed element.
 * Otherwise, it is set to NULL.
 * Return the new list (without the removed element).
 */
FList *f_list_remove_find_custom (FList *haystack, void *needle, FCompareFunc fn, void **data) {
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
