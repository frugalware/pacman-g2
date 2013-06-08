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

#include <fcallbacks.h>
#include <fstddef.h>

typedef struct FListItem FListItem;

struct FListItem {
	FListItem *next;
	FListItem *previous;
};

void f_listitem_init (FListItem *listitem);
void f_listitem_fini (FListItem *listitem, FVisitorFunc fn, void *user_data);

void f_listitem_delete (FListItem *listitem, FVisitorFunc fn, void *user_data);

FListItem *f_listitem_next (FListItem *listitem);
FListItem *f_listitem_previous (FListItem *listitem);

void f_listitem_insert_after (FListItem *listitem, FListItem *listitem_ref);
void f_listitem_insert_before (FListItem *listitem, FListItem *listitem_ref);
void f_listitem_insert_range_after (FListItem *listitem_first, FListItem *listitem_last, FListItem *listitem_ref);
void f_listitem_insert_range_before (FListItem *listitem_first, FListItem *listitem_last, FListItem *listitem_ref);
void f_listitem_remove (FListItem *listitem);

typedef struct FList FList;

struct FList {
	FListItem head;
};

void f_list_init (FList *list);
void f_list_fini (FList *list, FVisitorFunc fn, void *user_data);

FList *f_list_new (void);
void f_list_delete (FList *list, FVisitorFunc fn, void *user_data);

FListItem *f_list_begin (FList *list);
FListItem *f_list_end (FList *list);
FListItem *f_list_rbegin (FList *list);
FListItem *f_list_rend (FList *list);

FListItem *f_list_first (FList *list);
FListItem *f_list_last (FList *list);

int f_list_add (FList *list, FListItem *listitem);
int f_list_append (FList *list, FListItem *listitem);

#define f_list_entry(ptr, type, member) \
	f_containerof (ptr, type, member)
#if 0
#define f_foreach(it, list) \
	for (it = f_list_begin (f_identity_cast(FList *, list)); \
			it != f_list_end (list); \
			it = f_identity_cast(FListItem *, it->next))
#endif
#define f_foreach_entry(it, list, member) \
	for (it = f_list_entry (f_list_begin (list), f_typeof (*it), member); \
			&it->member != f_list_end (list); \
			it = f_list_entry (it->member.next, f_typeof (*it), member))
#if 0
#define f_rforeach(it, list) \
	for (it = f_list_rbegin (f_identity_cast(FList *, list)); \
			it != f_list_rend (FList *, list); \
			it = f_identity_cast(FListItem *, it->next))
#endif
#define f_rforeach_entry(it, list, member) \
	for (it = f_list_entry (f_list_rbegin (list), f_typeof (*it), member); \
			&it->member != f_list_rend (list); \
			it = f_list_entry (it->member.previous, f_typeof(*it), member))

typedef struct FPtrListItem FPtrListItem;

struct FPtrListItem {
	union {
		FListItem base;
		struct { /* For compatibility */
			FPtrListItem *next;
			FPtrListItem *prev;
		};
	};
	void *data;
};

FPtrListItem *f_ptrlistitem_new (void *data);
void f_ptrlistitem_delete (FPtrListItem *item, FVisitorFunc fn, void *user_data);

void *f_ptrlistitem_get (FPtrListItem *item);
void f_ptrlistitem_set (FPtrListItem *item, void *data);

FPtrListItem *f_ptrlistitem_next (FPtrListItem *item);
FPtrListItem *f_ptrlistitem_previous (FPtrListItem *item);
void f_ptrlistitem_remove (FPtrListItem *item);

/* FIXME: Make private as soon as possible */
typedef struct FPtrListItem FPtrList;

FPtrList *f_ptrlist_new (void);
void   f_ptrlist_delete (FPtrList *list, FVisitorFunc fn, void *user_data);
void   f_ptrlist_init (FPtrList *list);

void   f_ptrlist_insert_after (FPtrList *item, FPtrList *list);
void   f_ptrlist_insert_before (FPtrList *item, FPtrList *list);

FPtrListItem *f_ptrlist_begin (FPtrList *list);
FPtrListItem *f_ptrlist_end (FPtrList *list);
FPtrListItem *f_ptrlist_rbegin (FPtrList *list);
FPtrListItem *f_ptrlist_rend (FPtrList *list);

FPtrListItem *f_ptrlist_first (FPtrList *list);
FPtrListItem *f_ptrlist_last (FPtrList *list);

FPtrList *f_ptrlist_add_sorted (FPtrList *list, void *data, FCompareFunc fn, void *user_data);
FPtrList *f_ptrlist_append (FPtrList *list, void *data);
FPtrList *f_ptrlist_append_unique (FPtrList *list, void *data, FCompareFunc fn, void *user_data);
FPtrList *f_ptrlist_concat (FPtrList *list1, FPtrList *list2);
FPtrList *f_ptrlist_copy (FPtrList *list);
size_t f_ptrlist_count (FPtrList *list);
FPtrList *f_ptrlist_deep_copy (FPtrList *list, FCopyFunc fn, void *user_data);
void   f_ptrlist_detach (FPtrList *list, FCopyFunc fn, void *user_data);
FPtrListItem *f_ptrlist_detect (FPtrList *list, FDetectFunc dfn, void *user_data);
//void   f_ptrlist_exclude (FPtrList *list, FPtrList *excludelist, FDetectFunc dfn, void *user_data);
FPtrList *f_ptrlist_filter (FPtrList *list, FDetectFunc fn, void *user_data);
FPtrListItem *f_ptrlist_find (FPtrList *list, const void *data);
FPtrListItem *f_ptrlist_find_custom (FPtrList *list, const void *data, FCompareFunc cfn, void *user_data);
void   f_ptrlist_foreach (FPtrList *list, FVisitorFunc fn, void *user_data);
FPtrList *f_ptrlist_reverse (FPtrList *list);
void   f_ptrlist_reverse_foreach (FPtrList *list, FVisitorFunc fn, void *user_data);
FPtrList *f_ptrlist_uniques (FPtrList *list, FCompareFunc fn, void *user_data);

/* FIXME: To be removed */
void   _f_ptrlist_exclude (FPtrList **list, FPtrList **excludelist, FDetectFunc dfn, void *user_data);
void   _f_ptrlist_remove (FPtrList **list, FPtrListItem *item);
FPtrList *f_ptrlist_remove_find_custom (FPtrList *haystack, void *needle, FCompareFunc fn, void **data);

#define f_foreach(it, list) \
	for (it = f_ptrlist_begin (list); it != f_ptrlist_end (list); it = f_ptrlistitem_next (it))

#define f_rforeach(it, list) \
	for (it = f_ptrlist_rbegin (list); it != f_ptrlist_rend (list); it = f_ptrlistitem_prev (it))

#endif /* F_LIST_H */

/* vim: set ts=2 sw=2 noet: */
