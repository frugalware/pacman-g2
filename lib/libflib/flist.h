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

#define FLISTITEM_INIT(name) { \
	.next = &(name), \
	.previous = &(name) \
}

#define FLISTITEM(name) \
	FListItem name = FLISTITEM_INIT (name)

void f_listitem_init (FListItem *listitem);
void f_listitem_fini (FListItem *listitem, FVisitorFunc fn, void *user_data);

void f_listitem_delete (FListItem *listitem, FVisitorFunc fn, void *user_data);

void f_listitem_insert_after (FListItem *listitem, FListItem *listitem_ref);
void f_listitem_insert_before (FListItem *listitem, FListItem *listitem_ref);
void f_listitem_insert_range_after (FListItem *listitem_first, FListItem *listitem_last, FListItem *listitem_ref);
void f_listitem_insert_range_before (FListItem *listitem_first, FListItem *listitem_last, FListItem *listitem_ref);
void f_listitem_remove (FListItem *listitem);

typedef struct FList FList;

struct FList {
	FListItem head;
};

#define FLIST_INIT(name) { \
	.head = FLISTITEM_INIT (name.head) \
}

#define FLIST(name) \
	FList name = FLIST_INIT (name)

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

void f_list_add (FList *list, FListItem *listitem);
void f_list_add_sorted (FList *list, FListItem *listitem, FCompareFunc fn, void *user_data);
void f_list_append (FList *list, FListItem *listitem);
size_t f_list_count (FList *list);
FList *f_list_deep_copy (FList *list, FCopyFunc cfn, void *user_data);
FListItem *f_list_detect (FList *list, FDetectFunc dfn, void *user_data);
void f_list_foreach (FList *list, FVisitorFunc fn, void *user_data);
void f_list_foreach_safe (FList *list, FVisitorFunc fn, void *user_data);
int f_list_isempty (FList *list);
void f_list_remove_all_detect (FList *list, FDetectFunc dfn, void *user_data, FList *list_removed);
void f_list_rforeach (FList *list, FVisitorFunc fn, void *user_data);
void f_list_rforeach_safe (FList *list, FVisitorFunc fn, void *user_data);

#define f_list_entry(ptr, type, member) \
	f_containerof (f_identity_cast (FListItem *, ptr), type, member)

#define f_list_entry_next(ptr, type, member) \
	f_list_entry (f_identity_cast (type *, ptr)->member.next, type, member)

#define f_list_entry_previous(ptr, type, member) \
	f_list_entry (f_identity_cast (type *, ptr)->member.previous, type, member)

#define __f_foreach(it, list) \
	for (it = f_list_begin (list); \
			it != f_list_end (list); \
			it = f_identity_cast(FListItem *, it)->next)

#define f_foreach_safe(it, next, list) \
	for (it = f_list_begin (list), next = f_identity_cast (FListItem *, it)->next; \
			it != f_list_end (list); \
			it = next, next = f_identity_cast (FListItem *, it)->next)

#define f_foreach_entry(it, list, member) \
	for (it = f_list_entry (f_list_begin (list), f_typeof (*it), member); \
			&it->member != f_list_end (list); \
			it = f_list_entry_next (it, f_typeof (*it), member))

#define f_foreach_entry_safe(it, next, list, member) \
	for (it = f_list_entry (f_list_begin (list), f_typeof (*it), member), next = f_list_entry_next (it, f_typeof (*it), member); \
			&it->member != f_list_end (list); \
			it = next, next = f_list_entry_next (it, f_typeof (*it), member))

#define __f_rforeach(it, list) \
	for (it = f_list_rbegin (list); \
			it != f_list_rend (list); \
			it = f_identity_cast(FListItem *, it)->previous)

#define f_rforeach_safe(it, next, list) \
	for (it = f_list_rbegin (list), next = f_identity_cast(FListItem *, it)->previous; \
			it != f_list_rend (list); \
			it = next, next = f_identity_cast (FListItem *, it)->previous)

#define f_rforeach_entry(it, list, member) \
	for (it = f_list_entry (f_list_rbegin (list), f_typeof (*it), member); \
			&it->member != f_list_rend (list); \
			it = f_list_entry_previous (it, f_typeof(*it), member))

#define f_rforeach_entry_next(it, list, member) \
	for (it = f_list_entry (f_list_rbegin (list), f_typeof (*it), member), next = f_list_entry_previous (it, f_typeof(*it), member); \
			&it->member != f_list_rend (list); \
			it = next, next = f_list_entry_previous (it, f_typeof(*it), member))

typedef struct FPtrListItem FPtrListItem;
typedef struct FPtrListItem FPtrList;

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

void f_ptrlistitem_init (FPtrListItem *ptrlistitem, void *data);
void f_ptrlistitem_fini (FPtrListItem *ptrlistitem, FVisitorFunc fn, void *user_data, FPtrList **ptrlist);

FPtrListItem *f_ptrlistitem_new (void *data);
void f_ptrlistitem_delete (FPtrListItem *item, FVisitorFunc fn, void *user_data, FPtrList **ptrlist);

void *f_ptrlistitem_get (FPtrListItem *item);
void f_ptrlistitem_set (FPtrListItem *item, void *data);

void f_ptrlistitem_remove (FPtrListItem *item, FPtrList **ptrlist);

FPtrList *f_ptrlist_new (void);
void f_ptrlist_delete (FPtrList *ptrlist, FVisitorFunc fn, void *user_data);
void f_ptrlist_init (FPtrList *ptrlist);

void f_ptrlist_insert_after (FPtrList *item, FPtrList *ptrlist);
void f_ptrlist_insert_before (FPtrList *item, FPtrList *ptrlist);

FPtrListItem *f_ptrlist_begin (FPtrList *ptrlist);
FPtrListItem *f_ptrlist_end (FPtrList *ptrlist);
FPtrListItem *f_ptrlist_rbegin (FPtrList *ptrlist);
FPtrListItem *f_ptrlist_rend (FPtrList *ptrlist);

FPtrListItem *f_ptrlist_first (FPtrList *ptrlist);
FPtrListItem *f_ptrlist_last (FPtrList *ptrlist);

FPtrList *f_ptrlist_add_sorted (FPtrList *ptrlist, void *data, FCompareFunc fn, void *user_data);
FPtrList *f_ptrlist_append (FPtrList *ptrlist, void *data);
FPtrList *f_ptrlist_append_unique (FPtrList *ptrlist, void *data, FCompareFunc fn, void *user_data);
FPtrList *f_ptrlist_concat (FPtrList *ptrlist1, FPtrList *ptrlist2);
FPtrList *f_ptrlist_copy (FPtrList *ptrlist);
size_t f_ptrlist_count (FPtrList *ptrlist);
FPtrList *f_ptrlist_deep_copy (FPtrList *ptrlist, FCopyFunc fn, void *user_data);
void   f_ptrlist_detach (FPtrList *ptrlist, FCopyFunc fn, void *user_data);
FPtrListItem *f_ptrlist_detect (FPtrList *ptrlist, FDetectFunc dfn, void *user_data);
FPtrListItem *f_ptrlist_find (FPtrList *ptrlist, const void *data);
FPtrListItem *f_ptrlist_find_custom (FPtrList *ptrlist, const void *data, FCompareFunc cfn, void *user_data);
void f_ptrlist_foreach (FPtrList *ptrlist, FVisitorFunc fn, void *user_data);
int f_ptrlist_isempty (FPtrList *ptrlist);
//void f_ptrlist_remove_all_detect (FPtrList *ptrlist, FDetectFunc dfn, void *user_data, FPtrList *ptrlist_removed);
void _f_ptrlist_remove_all_detect (FPtrList **ptrlist, FDetectFunc dfn, void *user_data, FPtrList **ptrlist_removed);
FPtrList *f_ptrlist_reverse (FPtrList *ptrlist);
void f_ptrlist_rforeach (FPtrList *ptrlist, FVisitorFunc fn, void *user_data);
FPtrList *f_ptrlist_uniques (FPtrList *ptrlist, FCompareFunc fn, void *user_data);

#define f_ptrlist_entry(ptr) f_list_entry (ptr, FPtrListItem, base)
#define f_ptrlist_entry_next(ptr) f_list_entry_next (ptr, FPtrListItem, base)
#define f_ptrlist_entry_previous(ptr) f_list_entry_previous (ptr, FPtrListItem, base)

/* FIXME: To be removed */
#define f_foreach(it, list) \
	for (it = f_ptrlist_begin (list); it != f_ptrlist_end (list); it = f_ptrlist_entry_next (it))

#define f_rforeach(it, list) \
	for (it = f_ptrlist_rbegin (list); it != f_ptrlist_rend (list); it = f_ptrlist_entry_previous (it))

#endif /* F_LIST_H */

/* vim: set ts=2 sw=2 noet: */
