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

#include "pacman.h"

#include "util/fcallback.h"

#ifndef F_NOCOMPAT
typedef struct __pmlist_t FList;
typedef struct __pmlist_t FListItem;

/* Chained list struct */
struct __pmlist_t {
	struct __pmlist_t *prev;
	struct __pmlist_t *next;
	void *data;
};
#else
typedef struct FList FList;
typedef struct FListItem FListItem;

struct FListItem {
	FListItem *next;
	FListItem *previous;
};

struct FList {
	FListItem as_FListItem;
};
#endif

typedef int (*FListItemComparatorFunc)(const FListItem *item, const void *comparator_data);
typedef void (*FListItemVisitorFunc)(FListItem *item, void *visitor_data);

int f_listitem_delete(FListItem *self, FVisitor *visitor);

int f_listitem_insert_after(FListItem *self, FListItem *previous);

FList *f_list_new(void);
int f_list_delete(FList *self, FVisitor *visitor);

int f_list_init(FList *self);
int f_list_fini(FList *self, FVisitor *visitor);

#define f_list_add f_list_append
int f_list_all_match(const FList *list, const FMatcher *matcher);
int f_list_any_match(const FList *list, const FMatcher *matcher);
int f_list_append(FList *self, FListItem *item);
int f_list_append_unique(FList *self, FListItem *item, FListItemComparatorFunc comparator);
int f_list_clear(FList *self, FVisitor *visitor);
int f_list_contains(const FList *list, FListItemComparatorFunc comparator, const void *comparator_data);
int f_list_count(const FList *list);
int f_list_empty(const FList *list);
FListItem *f_list_end(FList *self);
const FListItem *f_list_end_const(const FList *self);
FListItem *f_list_find(FList *self, FListItemComparatorFunc comparator, const void *comparator_data);
const FListItem *f_list_find_const(const FList *self, FListItemComparatorFunc comparator, const void *comparator_data);
FListItem *f_list_first(FList *self);
const FListItem *f_list_first_const(const FList *self);
void f_list_foreach(const FList *list, FListItemVisitorFunc visitor, void *visitor_data);
FListItem *f_list_last(FList *self);
const FListItem *f_list_last_const(const FList *self);
FListItem *f_list_rend(FList *self);
const FListItem *f_list_rend_const(const FList *self);

#endif /* F_LIST_H */

/* vim: set ts=2 sw=2 noet: */
