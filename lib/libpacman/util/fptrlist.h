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

#include "util/flist.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef F_NOCOMPAT
typedef struct __pmlist_t FPtrList;
typedef struct __pmlist_t FPtrListItem;
#else
typedef struct FPtrList FPtrList;
typedef struct FPtrListItem FPtrListItem;
#endif

FPtrListItem *f_ptrlistitem_new(void *data);
int f_ptrlistitem_delete(FListItem *self, FVisitor *visitor);

void *f_ptrlistitem_data(const FPtrListItem *self);
#define f_ptrlistitem_next(self) ((FPtrListItem *)f_listitem_next((FListItem *)(self)))
#define f_ptrlistitem_previous(self) ((FPtrListItem *)f_listitem_previous((FListItem *)(self)))

int f_ptrlistitem_ptrcmp(const FPtrListItem *item, const void *ptr);

FPtrList *f_ptrlist_new(void);
int f_ptrlist_delete(FPtrList *list, FVisitor *visitor);

int f_ptrlist_init(FPtrList *self);
int f_ptrlist_fini(FPtrList *self, FVisitor *visitor);

FList *f_ptrlist_as_FList(FPtrList *self);
const FList *f_ptrlist_as_FList_const(const FPtrList *self);

#define f_ptrlist_add f_ptrlist_append
#ifndef F_NOCOMPAT
FPtrList *f_ptrlist_append(FPtrList *list, void *data);
#else
int f_ptrlist_append(FPtrList *list, void *data);
#endif
int f_ptrlist_clear(FPtrList *list, FVisitor *visitor);
#define f_ptrlist_contains(self, comparator, comparator_data) f_list_contains(f_ptrlist_as_FList_const(self), (FListItemComparatorFunc)comparator, comparator_data)
int f_ptrlist_contains_ptr(const FPtrList *list, const void *ptr);
#define f_ptrlist_count f_list_count
#define f_ptrlist_empty f_list_empty
#define f_ptrlist_end(list) ((FPtrListItem *)f_ptrlist_end_const(list))
#define f_ptrlist_end_const(list) ((const FPtrListItem *)f_list_end_const(f_ptrlist_as_FList_const(list)))
#define f_ptrlist_first(self) ((FPtrListItem *)f_ptrlist_first_const(self))
#define f_ptrlist_first_const(self) ((const FPtrListItem *)f_list_first_const(f_ptrlist_as_FList_const(self)))
#define f_ptrlist_last(self) ((FPtrListItem *)f_ptrlist_last_const(self))
#define f_ptrlist_last_const(self) ((const FPtrListItem *)f_list_last_const(f_ptrlist_as_FList_const(self)))

#ifdef __cplusplus
}
#endif
#endif /* F_PTRLIST_H */

/* vim: set ts=2 sw=2 noet: */
