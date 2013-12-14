/*
 *  list.h
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
#ifndef _PACMAN_LIST_H
#define _PACMAN_LIST_H

#include "pacman.h"

#include "util/fcallback.h"

typedef struct __pmlist_t FList;
typedef struct __pmlist_t FListItem;

typedef int (*FListItemComparatorFunc)(const FListItem *item, const void *comparator_data);
typedef void (*FListItemVisitorFunc)(FListItem *item, void *visitor_data);

/* Chained list struct */
struct __pmlist_t {
	struct __pmlist_t *prev;
	struct __pmlist_t *next;
	void *data;
	struct __pmlist_t *last; /* Quick access to last item in list */
};

#define _FREELIST(p, f) do { if(p) { FVisitor visitor = { .fn = (FVisitorFunc)f, .data = NULL, }; f_ptrlist_delete(p, &visitor); p = NULL; } } while(0)
#define FREELIST(p) _FREELIST(p, free)
#define FREELISTPTR(p) _FREELIST(p, NULL)

/* Sort comparison callback function declaration */
typedef int (*_pacman_fn_cmp)(const void *, const void *);

#define _pacman_list_new f_ptrlist_new

#define _pacman_list_add f_ptrlist_append
#define _pacman_list_count f_ptrlist_count
#define _pacman_list_empty f_ptrlist_empty

void f_listitem_delete(FListItem *item, FVisitor *visitor);

int f_list_contains(const FList *list, FListItemComparatorFunc comparator, const void *comparator_data);
int f_list_count(const FList *list);
int f_list_empty(const FList *list);
FListItem *f_list_find(const FList *list, FListItemComparatorFunc comparator, const void *comparator_data);
void f_list_foreach(const FList *list, FListItemVisitorFunc visitor, void *visitor_data);

pmlist_t *f_ptrlist_append(pmlist_t *list, void *data);
pmlist_t *_pacman_list_add_sorted(pmlist_t *list, void *data, _pacman_fn_cmp fn);
int f_list_all_match(const FList *list, const FMatcher *matcher);
int f_list_any_match(const FList *list, const FMatcher *matcher);
pmlist_t *_pacman_list_remove(pmlist_t *haystack, void *needle, _pacman_fn_cmp fn, void **data);
pmlist_t *_pacman_list_last(pmlist_t *list);
pmlist_t *_pacman_list_reverse(pmlist_t *list);

typedef struct __pmlist_t FPtrList;
typedef struct __pmlist_t FPtrListItem;

FPtrListItem *f_ptrlistitem_new(void *ptr);
void f_ptrlistitem_delete(FListItem *item, FVisitor *visitor);

int f_ptrlistitem_match(const FListItem *item, const FMatcher *matcher);
int f_ptrlistitem_ptrcmp(const FListItem *item, const void *ptr);

FPtrList *f_ptrlist_new(void);
void f_ptrlist_delete(FPtrList *list, FVisitor *visitor);

int f_ptrlist_all_match(const FPtrList *list, const FMatcher *matcher);
int f_ptrlist_any_match(const FPtrList *list, const FMatcher *matcher);
void f_ptrlist_clear(FPtrList *list, FVisitor *visitor);
#define f_ptrlist_contains f_list_contains
int f_ptrlist_contains_ptr(const FPtrList *list, const void *ptr);
#define f_ptrlist_count f_list_count
#define f_ptrlist_empty f_list_empty

#endif /* _PACMAN_LIST_H */

/* vim: set ts=2 sw=2 noet: */
