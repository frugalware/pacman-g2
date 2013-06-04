/*
 *  flist.h
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

#include <fcallbacks.h>

/* FIXME: Make it a struct FListItem */
typedef struct FList FListItem;

FListItem *f_listitem_new (void *data);
void f_listitem_delete (FListItem *item, FVisitorFunc fn, void *user_data);

void *f_listitem_get (FListItem *item);
void f_listitem_set (FListItem *item, void *data);

FListItem *f_listitem_next (FListItem *item);
FListItem *f_listitem_previous (FListItem *item);
void f_listitem_remove (FListItem *item);

typedef struct FList FList;

/* FIXME: Make private as soon as possible */
/* Chained list struct */
struct FList {
	FList *prev;
	FList *next;
	void *data;
};

FList *f_list_new (void);
void   f_list_delete (FList *list, FVisitorFunc fn, void *user_data);
void   f_list_init (FList *list);

void   f_list_insert_after (FList *item, FList *list);
void   f_list_insert_before (FList *item, FList *list);

FListItem *f_list_begin (FList *list);
FListItem *f_list_end (FList *list);
FListItem *f_list_rbegin (FList *list);
FListItem *f_list_rend (FList *list);

FListItem *f_list_first (FList *list);
FListItem *f_list_last (FList *list);

int f_list_add (FList *list, FListItem *listitem);
FList *_f_list_add_sorted (FList *list, void *data, FCompareFunc fn, void *user_data);
FList *_f_list_append (FList *list, void *data);
FList *_f_list_append_unique (FList *list, void *data, FCompareFunc fn, void *user_data);
FList *f_list_concat (FList *list1, FList *list2);
FList *f_list_copy (FList *list);
size_t f_list_count (FList *list);
FList *f_list_deep_copy (FList *list, FCopyFunc fn, void *user_data);
void   f_list_detach (FList *list, FCopyFunc fn, void *user_data);
FListItem *f_list_detect (FList *list, FDetectFunc dfn, void *user_data);
//void   f_list_exclude (FList *list, FList *excludelist, FDetectFunc dfn, void *user_data);
FList *f_list_filter (FList *list, FDetectFunc fn, void *user_data);
FListItem *_f_list_find (FList *list, const void *data);
FListItem *_f_list_find_custom (FList *list, const void *data, FCompareFunc cfn, void *user_data);
void   f_list_foreach (FList *list, FVisitorFunc fn, void *user_data);
FList *f_list_reverse (FList *list);
void   f_list_reverse_foreach (FList *list, FVisitorFunc fn, void *user_data);
FList *f_list_uniques (FList *list, FCompareFunc fn, void *user_data);

/* FIXME: To be removed */
void   _f_list_exclude (FList **list, FList **excludelist, FDetectFunc dfn, void *user_data);
void   _f_list_remove (FList **list, FListItem *item);
FList *f_list_remove_find_custom (FList *haystack, void *needle, FCompareFunc fn, void **data);

#define f_foreach(it, list) \
	for (it = f_list_begin (list); it != f_list_end (list); it = f_listitem_next (it))

#define f_rforeach(it, list) \
	for (it = f_list_rbegin (list); it != f_list_rend (list); it = f_listitem_prev (it))

#endif /* F_LIST_H */

/* vim: set ts=2 sw=2 noet: */
