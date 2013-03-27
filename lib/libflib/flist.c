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
#include <string.h>

#include "flist.h"

#include "fstdlib.h"

#if 0
/* Chained list struct */
struct FList {
	void *data;
	struct FList *prev;
	struct FList *next;
};
#endif

static
void f_list_accumulator (void *data, FList **list) {
	*list = f_list_add (*list, data);
}

FList *f_list_new()
{
	FList *list = f_malloc(sizeof(FList));

	if(list == NULL) {
		return(NULL);
	}
	list->data = NULL;
	list->prev = NULL;
	list->next = NULL;
	list->last = list;
	return(list);
}

FList *f_list_dup (FList *list, f_fn_dup fn) {
	FList *newlist = NULL;
	FList *i;

	for(i = list; i; i = i->next) {
		newlist = f_list_add(newlist, fn != NULL ? fn(i->data) : i->data);
	}

	return(newlist);
}

void f_list_free(FList *list, f_fn_free fn)
{
	FList *ptr, *it = list;

	while(it) {
		ptr = it->next;
		if(fn) {
			fn(it->data);
		}
		free(it);
		it = ptr;
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

FList *f_list_last (FList *list) {
	if (list != NULL) {
		for (; list->next != NULL; list = list->next)
			/* Do nothing */;
	}
	return list;
}

FList *f_list_detect (FList *list, f_fn_detect fn, void *user_data) {
	for (;list != NULL; list = list->next) {
		if (fn (list->data, user_data) == 0) {
			return list;
		}
	}
	return NULL;
}

FList *f_list_filter (FList *list, f_fn_detect fn, void *user_data) {
	FList *ret = NULL;

	for (; list != NULL; list = list->next) {
		if (fn (list->data, user_data) == 0) {
			ret = f_list_add (ret, list->data);
		}
	}
	return ret;
}

/* Filter out any duplicate strings in a list.
 *
 * Not the most efficient way, but simple to implement -- we assemble
 * a new list, using is_in() to check for dupes at each iteration.
 *
 */
FList *f_list_filter_dupes (FList *list, f_fn_cmp fn)
{
	FList *ret = NULL;

	for(; list != NULL; list = list->next) {
		if(f_list_detect(ret, (f_fn_detect)fn, list->data) != NULL) {
			ret = f_list_add(ret, strdup(list->data));
		}
	}
	return ret;
}

FList *f_list_find (FList *list, void *data) {
	return f_list_detect (list, (f_fn_detect)f_ptrcmp, data);
}

void f_list_foreach (FList *list, f_fn_foreach fn, void *user_data) {
	for (; list != NULL; list = list->next) {
		fn (list->data, user_data);
	}
}

/* Reverse the order of a list
 *
 * The caller is responsible for freeing the old list
 */
FList *f_list_reverse (FList *list) {
	FList *ret = NULL;

	f_list_reverse_foreach (list, (f_fn_foreach)f_list_accumulator, &ret);
	return ret;
}

void f_list_reverse_foreach (FList *list, f_fn_foreach fn, void *user_data) {
	for (list = f_list_last (list); list != NULL; list = list->prev) {
		fn (list->data, user_data);
	}
}

FList *f_list_add(FList *list, void *data)
{
	FList *ptr, *lp;

	ptr = list;
	if(ptr == NULL) {
		ptr = f_list_new();
		if(ptr == NULL) {
			return(NULL);
		}
	}

	lp = f_list_last(ptr);
	if(lp == ptr && lp->data == NULL) {
		/* nada */
	} else {
		lp->next = f_list_new();
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
FList *f_list_add_sorted(FList *list, void *data, f_fn_cmp fn)
{
	FList *add;
	FList *prev = NULL;
	FList *iter = list;

	add = f_list_new();
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
FList *f_list_remove(FList *haystack, void *needle, f_fn_cmp fn, void **data)
{
	FList *i = haystack;

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

/* vim: set ts=2 sw=2 noet: */
