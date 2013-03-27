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
	void *data;
	FList *prev;
	FList *next;
	FList *last;
};

/* Sort comparison callback function declaration */
typedef int   (*f_fn_cmp) (const void *, const void *);
/**
 * Detection comparison callback function declaration.
 * 
 * If detection is successful callback must return 0, or any other
 * values in case of failure (So it can be equivalent to a cmp).
 */
typedef int   (*f_fn_detect) (const void *, void *);
typedef void *(*f_fn_dup) (const void *);
typedef void  (*f_fn_foreach) (void *, void *);
typedef void  (*f_fn_free) (void *);

FList *f_list_new (void);
FList *f_list_dup (FList *list, f_fn_dup fn);
void   f_list_free (FList *list, f_fn_free fn);

size_t f_list_count (FList *list);
FList *f_list_first (FList *list);
FList *f_list_last (FList *list);

FList *f_list_detect (FList *list, f_fn_detect fn, void *user_data);
FList *f_list_filter (FList *list, f_fn_detect fn, void *user_data);
FList *f_list_filter_dupes (FList *list, f_fn_cmp fn);
FList *f_list_find (FList *list, void *data);
void   f_list_foreach (FList *list, f_fn_foreach fn, void *user_data);
FList *f_list_reverse (FList *list);
void   f_list_reverse_foreach (FList *list, f_fn_foreach fn, void *user_data);

FList *f_list_add (FList *list, void *data);
FList *f_list_add_sorted (FList *list, void *data, f_fn_cmp fn);
FList *f_list_remove (FList *haystack, void *needle, f_fn_cmp fn, void **data);

#endif /* F_LIST_H */

/* vim: set ts=2 sw=2 noet: */
