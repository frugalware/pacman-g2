/*
 *  list.h
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
#ifndef F_LIST_H
#define F_LIST_H

#include <stddef.h>

typedef struct FList FList;

/* FIXME: Make private as soon as possible */
/* Chained list struct */
struct FList {
	FList *prev;
	FList *next;
	void *data;
};

/* Sort comparison callback function declaration */
typedef int   (*FCompareFunc) (const void *ptr1, const void *ptr2, void *user_data);
typedef void *(*FCopyFunc) (const void *ptr, void *user_data);
/**
 * Detection comparison callback function declaration.
 * 
 * If detection is successful callback must return 0, or any other
 * values in case of failure (So it can be equivalent to a cmp).
 */
typedef int   (*FDetectFunc) (const void *ptr, void *user_data);
typedef void  (*FVisitorFunc) (void *ptr, void *user_data);

FList *f_list_new (void);
void   f_list_delete (FList *list, FVisitorFunc fn, void *user_data);

FList *f_list_alloc (void *data);
void   f_list_free (FList *item, FVisitorFunc fn, void *user_data);
void  *f_list_get (FList *item);
void   f_list_set (FList *item, void *data);
void   f_list_insert_after (FList *item, FList *list);
void   f_list_insert_before (FList *item, FList *list);
void   f_list_remove (FList *item);

size_t f_list_count (FList *list);
FList *f_list_first (FList *list);
FList *f_list_last (FList *list);
FList *f_list_next (FList *list);
FList *f_list_next_filtered (FList *list, FDetectFunc dfn, void *user_data);
FList *f_list_previous (FList *list);
FList *f_list_previous_filtered (FList *list, FDetectFunc dfn, void *user_data);

FList *f_list_append (FList *list, void *data);
FList *f_list_concat (FList *list1, FList *list2);
FList *f_list_copy (FList *list);
FList *f_list_deep_copy (FList *list, FCopyFunc fn, void *user_data);
void   f_list_detach (FList *list, FCopyFunc fn, void *user_data);
FList *f_list_detect (FList *list, FDetectFunc fn, void *user_data);
FList *f_list_filter (FList *list, FDetectFunc fn, void *user_data);
FList *f_list_find (FList *list, const void *data);
FList *f_list_find_custom (FList *list, const void *data, FCompareFunc cfn, void *user_data);
void   f_list_foreach (FList *list, FVisitorFunc fn, void *user_data);
void   f_list_foreach_filtered (FList *list, FVisitorFunc fn, FDetectFunc dfn, void *user_data);
FList *f_list_reverse (FList *list);
void   f_list_reverse_foreach (FList *list, FVisitorFunc fn, void *user_data);
FList *f_list_uniques (FList *list, FCompareFunc fn, void *user_data);

FList *f_list_add_sorted (FList *list, void *data, FCompareFunc fn, void *user_data);
FList *f_list_remove_find_custom (FList *haystack, void *needle, FCompareFunc fn, void **data);

#endif /* F_LIST_H */

/* vim: set ts=2 sw=2 noet: */
