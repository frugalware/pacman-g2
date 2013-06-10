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

void f_listitem_init (FListItem *listitem) {
	assert (listitem != NULL);

	listitem->next = listitem->previous = listitem;
}

void f_listitem_fini (FListItem *listitem, FVisitorFunc fn, void *user_data) {
	FVisitor visitor = {
		.fn = fn,
		.user_data = user_data
	};

	f_visit (listitem, &visitor);
	f_listitem_remove (listitem);
}

void f_listitem_delete (FListItem *listitem, FVisitorFunc fn, void *user_data) {
	assert (listitem != NULL);

	f_listitem_fini (listitem, fn, user_data);
	f_free (listitem);
}

/**
 * Insert a @listitem after @listitem_ref.
 */
void f_listitem_insert_after (FListItem *listitem, FListItem *listitem_ref) {
	f_listitem_insert_range_after (listitem, listitem, listitem_ref);
}

/**
 * Insert a @listitem before @listitem_ref.
 */
void f_listitem_insert_before (FListItem *listitem, FListItem *listitem_ref) {
	f_listitem_insert_range_before (listitem, listitem, listitem_ref);
}

/**
 * Insert a @listitem_first to @listitem_last after @listitem_ref.
 */
void f_listitem_insert_range_after (FListItem *listitem_first, FListItem *listitem_last, FListItem *listitem_ref) {
	assert (listitem_first != NULL);
	assert (listitem_last != NULL);
	assert (listitem_ref != NULL);

	listitem_first->previous = listitem_ref;
	listitem_last->next = listitem_ref->next;
	listitem_first->previous->next = listitem_first;
	listitem_last->next->previous = listitem_last;
}

/**
 * Insert a @listitem_first to @listitem_last before @listitem_ref.
 */
void f_listitem_insert_range_before (FListItem *listitem_first, FListItem *listitem_last, FListItem *listitem_ref) {
	assert (listitem_first != NULL);
	assert (listitem_last != NULL);
	assert (listitem_ref != NULL);

	listitem_first->previous = listitem_ref->previous;
	listitem_last->next = listitem_ref;
	listitem_first->previous->next = listitem_first;
	listitem_last->next->previous = listitem_last;
}

/**
 * Remove a @listitem from the list it belongs.
 */
void f_listitem_remove (FListItem *listitem) {
	assert (listitem != NULL);

	listitem->next->previous = listitem->previous;
	listitem->previous->next = listitem->next;
	f_listitem_init (listitem);
}

static
FListItem *f_list_head (FList *list) {
	assert (list != NULL);

	return &list->head;
}

void f_list_init (FList *list) {
	f_listitem_init (f_list_head (list));
}

void f_list_fini (FList *list, FVisitorFunc fn, void *user_data) {
}

FList *f_list_new () {
	FList *list = f_zalloc (sizeof (*list));

	if (list != NULL) {
		f_list_init (list);
	}
	return list;
}

void f_list_delete (FList *list, FVisitorFunc fn, void *user_data) {
	f_list_fini (list, fn, user_data);
	f_free (list);
}

FListItem *f_list_begin (FList *list) {
	return f_list_head (list)->next;
}

FListItem *f_list_end (FList *list) {
	return f_list_head (list);
}

FListItem *f_list_rbegin (FList *list) {
	return f_list_head (list)->previous;
}

FListItem *f_list_rend (FList *list) {
	return f_list_head (list);
}

FListItem *f_list_first (FList *list) {
	FListItem *first = f_list_begin (list);

	return first != f_list_end (list) ? first : NULL;
}

FListItem *f_list_last (FList *list) {
	FListItem *last = f_list_rbegin (list);

	return last != f_list_rend (list) ? last : NULL;
}

void f_list_add (FList *list, FListItem *listitem) {
	f_list_append (list, listitem);
}

void f_list_add_sorted (FList *list, FListItem *listitem, FCompareFunc cfn, void *user_data) {
	FListItem *it;

	__f_foreach (it, list) {
		if (cfn (it, listitem, user_data) > 0) {
			break;
		}
	}
	f_listitem_insert_before (listitem, it);
}

void f_list_append (FList *list, FListItem *listitem) {
	f_listitem_insert_before (listitem, f_list_end (list));
}

size_t f_list_count (FList *list) {
	size_t count = 0;
	FListItem *it;

	__f_foreach (it, list) {
		++count;
	}
	return count;
}

FList *f_list_deep_copy (FList *list, FCopyFunc cfn, void *user_data) {
	FList *list_deep_copy = f_list_new ();
	FListItem *it;

	assert (cfn != NULL);

	if (list_deep_copy != NULL) {
		__f_foreach (it, list) {
			f_list_append (list_deep_copy, cfn (it, user_data));
		}
	} 
	return list_deep_copy;
}

FListItem *f_list_detect (FList *list, FDetectFunc dfn, void *user_data) {
	assert (list != NULL);
	if (dfn != NULL) {
		FListItem *it;

		__f_foreach (it, list) {
			if (dfn (it, user_data) == 0) {
				return it;
			}
		}
	}
	return NULL;
}

void f_list_foreach (FList *list, FVisitorFunc fn, void *user_data) {
	assert (list != NULL);
	if (fn != NULL) {
		FListItem *it;

		__f_foreach (it, list) {
			fn (it, user_data);
		}
	}
}

void f_list_foreach_safe (FList *list, FVisitorFunc fn, void *user_data) {
	assert (list != NULL);
	if (fn != NULL) {
		FListItem *it, *next;

		f_foreach_safe (it, next, list) {
			fn (it, user_data);
		}
	}
}

void f_list_rforeach (FList *list, FVisitorFunc fn, void *user_data) {
	assert (list != NULL);
	if (fn != NULL) {
		FListItem *it;

		__f_rforeach (it, list) {
			fn (it, user_data);
		}
	}
}

void f_list_rforeach_safe (FList *list, FVisitorFunc fn, void *user_data) {
	assert (list != NULL);
	if (fn != NULL) {
		FListItem *it, *next;

		f_rforeach_safe (it, next, list) {
			fn (it, user_data);
		}
	}
}

/* DO NOT MAKE PUBLIC FOR NOW:
 * Require list implemantation change.
 */
static
void f_ptrlist_foreach_safe (FPtrList *ptrlist, FVisitorFunc fn, void *user_data);

static
void *f_ptrcpy(const void *p) {
	return (void *)p;
}

static
FPtrListItem *f_ptrlistitem_of_listitem (FListItem *listitem) {
	return f_list_entry (listitem, FPtrListItem, base);
}

static
void f_ptrlistitem_visit (FListItem *listitem, FVisitorFunc fn, void *user_data) {
	FVisitor visitor = {
		.fn = fn,
		.user_data = user_data
	};

	assert (listitem != NULL);

	f_visit (f_ptrlistitem_of_listitem (listitem)->data, &visitor);
}

FPtrListItem *f_ptrlistitem_new (void *data) {
	FPtrListItem *ptrlistitem = f_zalloc (sizeof (*ptrlistitem));

	if (ptrlistitem != NULL) {
		ptrlistitem->data = data;
	}
	return ptrlistitem;
}

static
void _f_ptrlistitem_delete (FPtrListItem *item, FVisitor *visitor) {
	if (item != NULL) {
		f_visit (item->data, visitor);
		f_ptrlistitem_remove (item);
		f_free (item);
	}
}

/**
 * Remove the item from it's list and free it.
 */
void f_ptrlistitem_delete (FPtrListItem *item, FVisitorFunc fn, void *user_data) {
	FVisitor visitor = {
		.fn = fn,
		.user_data = user_data
	};

	_f_ptrlistitem_delete (item, &visitor);
}

void *f_ptrlistitem_get (FPtrListItem *ptrlistitem) {
	return ptrlistitem != NULL ? ptrlistitem->data : NULL;
}

void f_ptrlistitem_set (FPtrListItem *ptrlistitem, void *data) {
	assert (ptrlistitem != NULL);

	ptrlistitem->data = data;
}

/**
 * Remove an @item from the list it belongs.
 */
void f_ptrlistitem_remove (FPtrListItem *item) {
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

static
FList *f_ptrlist_as_list (FPtrList *ptrlist) {
	assert (ptrlist != NULL);

	return &ptrlist->base;
}

void f_ptrlist_init (FPtrList *ptrlist) {
	f_list_init (f_ptrlist_as_list (ptrlist));
}

void f_ptrlist_fini (FPtrList *ptrlist, FVisitorFunc fn, void *user_data) {
	FVisitor visitor = {
		.fn = fn,
		.user_data = user_data
	};

	f_list_fini (f_ptrlist_as_list (ptrlist), (FVisitorFunc)f_ptrlistitem_visit, &visitor);
}

FPtrList *f_ptrlist_new () {
	return NULL;
}

void f_ptrlist_delete (FPtrList *ptrlist, FVisitorFunc fn, void *user_data) {
	FVisitor visitor = {
		.fn = fn,
		.user_data = user_data
	};

	f_ptrlist_foreach_safe (ptrlist, (FVisitorFunc)_f_ptrlistitem_delete, &visitor);
}

/**
 * Insert a @list after @item.
 */
void f_ptrlist_insert_after (FPtrList *item, FPtrList *ptrlist) {
	FPtrList *last = f_ptrlist_last (ptrlist);

	assert (item != NULL);
	if (ptrlist == NULL) {
		return;
	}
	last->next = item->next;
	if (last->next != NULL) {
		last->next->prev = last;
	}
	item->next = ptrlist;
	ptrlist->prev = item;
}

/**
 * Insert a @list before @item.
 */
void f_ptrlist_insert_before (FPtrList *item, FPtrList *ptrlist) {
	FPtrList *last = f_ptrlist_last (ptrlist);

	assert (item != NULL);
	if (ptrlist == NULL) {
		return;
	}
	ptrlist->prev = item->prev;
	if (ptrlist->prev != NULL) {
		ptrlist->prev->next = ptrlist;
	}
	last->next = item;
	item->prev = last;
}

FPtrListItem *f_ptrlist_begin (FPtrList *ptrlist) {
	return ptrlist;
}

FPtrListItem *f_ptrlist_end (FPtrList *ptrlist) {
	return NULL;
}

FPtrListItem *f_ptrlist_rbegin (FPtrList *ptrlist) {
	if (ptrlist != NULL) {
		for (; ptrlist->next != NULL; ptrlist = ptrlist->next)
			/* Do nothing */;
	}
	return ptrlist;
}

FPtrListItem *f_ptrlist_rend (FPtrList *ptrlist) {
	return NULL;
}

FPtrListItem *f_ptrlist_first (FPtrList *ptrlist) {
	return f_ptrlist_begin (ptrlist);
}

FPtrListItem *f_ptrlist_last (FPtrList *ptrlist) {
	return f_ptrlist_rbegin (ptrlist);
}

/* Add items to a list in sorted order. Use the given comparison function to
 * determine order.
 */
FPtrList *f_ptrlist_add_sorted (FPtrList *ptrlist, void *data, FCompareFunc fn, void *user_data) {
	FPtrList *add = f_ptrlistitem_new (data);
	FPtrList *prev = NULL;
	FPtrList *iter = ptrlist;

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
		ptrlist = add;           /*  Start or empty, new list head.  */
	}

	return(ptrlist);
}

FPtrList *f_ptrlist_append (FPtrList *ptrlist, void *ptr) {
	return f_ptrlist_concat (ptrlist, f_ptrlistitem_new (ptr));
}

FPtrList *f_ptrlist_append_unique (FPtrList *ptrlist, void *ptr, FCompareFunc fn, void *user_data) {
	if (f_ptrlist_find_custom (ptrlist, ptr, fn, user_data) == NULL) {
		return f_ptrlist_append (ptrlist, ptr);
	}
	return ptrlist;
}

FPtrList *f_ptrlist_concat (FPtrList *ptrlist1, FPtrList *ptrlist2) {
	if (ptrlist1 == NULL) {
		return ptrlist2;
	}
	f_ptrlist_insert_after (f_ptrlist_last (ptrlist1), ptrlist2);
	return ptrlist1;
}

FPtrList *f_ptrlist_copy (FPtrList *ptrlist) {
	return f_ptrlist_deep_copy (ptrlist, (FCopyFunc)f_ptrcpy, NULL);
}

size_t f_ptrlist_count (FPtrList *ptrlist) {
	size_t count = 0;
	FPtrListItem *it;

	f_foreach (it, ptrlist) {
	 	++count;
	}
	return count;
}

FPtrList *f_ptrlist_deep_copy (FPtrList *ptrlist, FCopyFunc cfn, void *user_data) {
	FPtrList *ptrlist_deep_copy = f_ptrlist_new ();
	FPtrListItem *it;

	assert (cfn != NULL);

//	if (ptrlist_deep_copy != NULL) {
		f_foreach (it, ptrlist) {
			ptrlist_deep_copy = f_ptrlist_append (ptrlist_deep_copy, cfn (it->data, user_data));
		}
//	}
	return ptrlist_deep_copy;
}

void f_ptrlist_detach (FPtrList *ptrlist, FCopyFunc fn, void *user_data) {
	FPtrListItem *it;

	assert (fn != NULL);

	f_foreach (it, ptrlist) {
		it->data = fn (it->data, user_data);
	}
}

FPtrListItem *f_ptrlist_detect (FPtrList *ptrlist, FDetectFunc dfn, void *user_data) {
	if (dfn != NULL) {
		FPtrListItem *it;

		f_foreach (it, ptrlist) {
			if (dfn (it->data, user_data) == 0) {
				return it;
			}
		}
	}
	return NULL;
}

void _f_ptrlist_exclude (FPtrList **ptrlist, FPtrList **excludelist, FDetectFunc dfn, void *user_data) {
	FPtrListAccumulator listaccumulator;
	FPtrList *item, *next = *ptrlist;

	f_ptrlistaccumulator_init (&listaccumulator, *excludelist);
	while ((item = f_ptrlist_detect (next, dfn, user_data)) != NULL) {
		next = item->next;
		if (*ptrlist == item) {
			*ptrlist = (*ptrlist)->next;
		}
		f_ptrlistitem_remove (item);
		f_ptrlistaccumulate (item, &listaccumulator);
	}
	*excludelist = f_ptrlistaccumulator_fini (&listaccumulator);
}

FPtrList *f_ptrlist_filter (FPtrList *list, FDetectFunc dfn, void *user_data) {
	FPtrListAccumulator listaccumulator;
	FDetector detector = {
		.fn = dfn,
		.user_data = user_data
	};
	FVisitor visitor = {
		.fn = (FVisitorFunc)f_ptrlistaccumulate,
		.user_data = &listaccumulator
	};
	FDetectVisitor detectvisitor = {
		.detect = &detector,
		.success = &visitor,
		.fail = NULL
	};

	f_ptrlistaccumulator_init (&listaccumulator, f_ptrlist_new ());
	f_ptrlist_foreach (list, (FVisitorFunc)f_detectvisit, &detectvisitor);
	return f_ptrlistaccumulator_fini (&listaccumulator);
}

FPtrListItem *f_ptrlist_find (FPtrList *ptrlist, const void *data) {
	return f_ptrlist_find_custom (ptrlist, data, (FCompareFunc)f_ptrcmp, NULL);
}

FPtrListItem *f_ptrlist_find_custom (FPtrList *ptrlist, const void *data, FCompareFunc cfn, void *user_data) {
	FCompareDetector comparedetector = {
		.fn = cfn,
		.ptr = data,
		.user_data = user_data
	};

	return f_ptrlist_detect (ptrlist, (FDetectFunc)f_comparedetect, &comparedetector);
}

void f_ptrlist_foreach (FPtrList *ptrlist, FVisitorFunc fn, void *user_data) {
	FPtrListItem *it = f_ptrlist_begin (ptrlist), *end = f_ptrlist_end (ptrlist);

	for (; it != end; it = it->next) {
		fn (it->data, user_data);
	}
}

/**
 * A foreach safe in the sence you can detach the current element.
 *
 * Differs from foreach since it pass an FPtrListItem instead of data.
 */
void f_ptrlist_foreach_safe (FPtrList *ptrlist, FVisitorFunc fn, void *user_data) {
	FPtrListItem *it, *next = f_ptrlist_begin (ptrlist), *end = f_ptrlist_end (ptrlist);

	for (it = next; it != end; it = next) {
		next = it->next;
		fn (it, user_data);
	}
}

/* Reverse the order of a list
 *
 * The caller is responsible for freeing the old list
 */
FPtrList *f_ptrlist_reverse (FPtrList *ptrlist) {
	FPtrListAccumulator listaccumulator;

	f_ptrlistaccumulator_init (&listaccumulator, f_ptrlist_new ());
	f_ptrlist_foreach (ptrlist, (FVisitorFunc)f_ptrlistreverseaccumulate, &listaccumulator);
	return f_ptrlistaccumulator_fini (&listaccumulator);
}

void f_ptrlist_reverse_foreach (FPtrList *ptrlist, FVisitorFunc fn, void *user_data) {
	FPtrListItem *it = f_ptrlist_rbegin (ptrlist), *end = f_ptrlist_rend (ptrlist);

	for (; it != end; it = it->prev) {
		fn (it->data, user_data);
	}
}

FPtrList *f_ptrlist_uniques (FPtrList *ptrlist, FCompareFunc fn, void *user_data) {
	FPtrListItem *it = f_ptrlist_begin (ptrlist), *end = f_ptrlist_end (ptrlist);
	FPtrList *uniques = f_ptrlist_new ();

	for (; it != end; it = it->next) {
		uniques = f_ptrlist_append_unique (uniques, it->data, fn, user_data);
	}
	return uniques;
}

void _f_ptrlist_remove (FPtrList **ptrlist, FPtrListItem *item) {
	if (*ptrlist == item) {
		*ptrlist = item->next;
	}
	f_ptrlistitem_remove (item);
}

/* Remove an item in a list. Use the given comparison function to find the
 * item.
 * If the item is found, 'data' is pointing to the removed element.
 * Otherwise, it is set to NULL.
 * Return the new list (without the removed element).
 */
FPtrList *f_ptrlist_remove_find_custom (FPtrList *haystack, void *needle, FCompareFunc fn, void **data) {
	FPtrList *i = haystack;

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
