/*
 *  fptrlist.h
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2013-2014 by Michel Hermier <hermier@frugalware.org>
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
#ifndef F_PTRLIST_H
#define F_PTRLIST_H

#include "pacman.h"

#include "stdbool.h"

#include "util/fcallback.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef F_NOCOMPAT
/* Chained list struct */
struct __pmlist_t {
	struct __pmlist_t *prev;
	struct __pmlist_t *next;
	void *data;
};

typedef struct __pmlist_t FPtrList;
typedef struct __pmlist_t FPtrListItem;
#else
typedef struct FPtrList FPtrList;
typedef struct FPtrListItem FPtrListItem;
#endif

#include "util/flist.h"

#define _FREELIST(p, f) do { if(p) { FVisitor visitor = { (FVisitorFunc)f, NULL }; f_ptrlist_delete(p, &visitor); p = NULL; } } while(0)

#define FREELIST(p) _FREELIST(p, free)
#define FREELISTPTR(p) _FREELIST(p, NULL)

/* Sort comparison callback function declaration */
typedef int (*_pacman_fn_cmp)(const void *, const void *);

#define _pacman_list_new f_ptrlist_new

#define _pacman_list_add f_ptrlist_append
#define _pacman_list_count f_ptrlist_count
#define _pacman_list_empty f_ptrlist_empty
#define _pacman_list_last f_ptrlist_last

FPtrList *_pacman_list_add_sorted(FPtrList *list, void *data, _pacman_fn_cmp fn);
FPtrList *_pacman_list_remove(FPtrList *haystack, void *needle, _pacman_fn_cmp fn, void **data);
FPtrList *_pacman_list_reverse(FPtrList *list);

typedef int (*FPtrListItemComparatorFunc)(const FPtrListItem *item, const void *comparator_data);
typedef void (*FPtrListItemVisitorFunc)(FPtrListItem *item, void *visitor_data);

FPtrList *f_list_new(void);

int f_list_contains(const FPtrList *list, FPtrListItemComparatorFunc comparator, const void *comparator_data);
FPtrListItem *f_list_find(FPtrList *self, FPtrListItemComparatorFunc comparator, const void *comparator_data);
const FPtrListItem *f_list_find_const(const FPtrList *self, FPtrListItemComparatorFunc comparator, const void *comparator_data);
void f_list_foreach(const FPtrList *list, FPtrListItemVisitorFunc visitor, void *visitor_data);


FPtrListItem *f_ptrlistitem_new(void *data);
int f_ptrlistitem_delete(FPtrListItem *self, FVisitor *visitor);

void *f_ptrlistitem_data(const FPtrListItem *self);
int f_ptrlistitem_insert_after(FPtrListItem *self, FPtrListItem *previous);
FPtrListItem *f_ptrlistitem_next(FPtrListItem *self);
FPtrListItem *f_ptrlistitem_previous(FPtrListItem *self);

int f_ptrlistitem_ptrcmp(const FPtrListItem *item, const void *ptr);

FPtrList *f_ptrlist_new(void);
int f_ptrlist_delete(FPtrList *list, FVisitor *visitor);

int f_ptrlist_init(FPtrList *self);
int f_ptrlist_fini(FPtrList *self, FVisitor *visitor);

#define f_ptrlist_add f_ptrlist_append
#ifndef F_NOCOMPAT
FPtrList *f_ptrlist_append(FPtrList *list, void *data);
#else
int f_ptrlist_append(FPtrList *list, void *data);
#endif
int f_ptrlist_clear(FPtrList *list, FVisitor *visitor);
#define f_ptrlist_contains(self, comparator, comparator_data) f_list_contains(self, (FPtrListItemComparatorFunc)comparator, comparator_data)
int f_ptrlist_contains_ptr(const FPtrList *list, const void *ptr);
int f_ptrlist_count(const FPtrList *self);
bool f_ptrlist_empty(const FPtrList *self);
FPtrListItem *f_ptrlist_end(FPtrList *self);
const FPtrListItem *f_ptrlist_end_const(const FPtrList *self);
FPtrListItem *f_ptrlist_first(FPtrList *self);
const FPtrListItem *f_ptrlist_first_const(const FPtrList *self);
FPtrListItem *f_ptrlist_last(FPtrList *self);
const FPtrListItem *f_ptrlist_last_const(const FPtrList *self);
FPtrListItem *f_ptrlist_rend(FPtrList *self);
const FPtrListItem *f_ptrlist_rend_const(const FPtrList *self);

#ifdef __cplusplus
}
#endif
#endif /* F_PTRLIST_H */

/* vim: set ts=2 sw=2 noet: */
