/*
 *  fptrlist.c
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

#include "util/fptrlist_p.h"

#include "fstdlib.h"

FPtrListItem *f_ptrlistitem_new(void *ptr)
{
	FPtrListItem *item = f_zalloc(sizeof(*item));

	if(item != NULL) {
		item->data = ptr;
	}
	return item;
}

int f_ptrlistitem_delete(FListItem *self, FVisitor *visitor)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	return f_listitem_delete(self, visitor);
}

void *f_ptrlistitem_data(const FPtrListItem *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return self->data;
}

int f_ptrlistitem_match(const FPtrListItem *item, const FMatcher *matcher) {
	return f_match(item->data, matcher);
}

int f_ptrlistitem_ptrcmp(const FPtrListItem *item, const void *ptr) {
	return f_ptrcmp(item->data, ptr);
}

FPtrList *f_ptrlist_new(void)
{
	FPtrList *self;

	ASSERT((self = f_zalloc(sizeof(*self))) != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	f_ptrlist_init(self);
	return self;
}

int f_ptrlist_delete(FPtrList *self, FVisitor *visitor)
{
	return f_ptrlist_fini(self, visitor);
}

int f_ptrlist_init(FPtrList *self)
{
	return f_list_init(f_ptrlist_as_FList(self));
}

int f_ptrlist_fini(FPtrList *self, FVisitor *visitor)
{
	return f_ptrlist_clear(self, visitor);
}

FList *f_ptrlist_as_FList(FPtrList *self)
{
	return (FList *)f_ptrlist_as_FList_const(self);
}

const FList *f_ptrlist_as_FList_const(const FPtrList *self)
{
#ifndef F_NOCOMPAT
	return (FList *)self;
#else
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return &self->as_FList;
#endif
}

int f_ptrlist_all_match(const FPtrList *list, const FMatcher *matcher)
{
	FMatcher itemmatcher = {
		.fn = (FMatcherFunc)f_ptrlistitem_match,
		.data = matcher,
	};

	return f_list_all_match(f_ptrlist_as_FList_const(list), &itemmatcher);
}

int f_ptrlist_any_match(const FPtrList *list, const FMatcher *matcher)
{
	FMatcher itemmatcher = {
		.fn = (FMatcherFunc)f_ptrlistitem_match,
		.data = matcher,
	};

	return f_list_any_match(f_ptrlist_as_FList_const(list), &itemmatcher);
}

#ifndef F_NOCOMPAT
FPtrList *f_ptrlist_append(FPtrList *list, void *data)
{
	pmlist_t *ptr, *lp;

	ptr = list;
	if(ptr == NULL) {
		ptr = f_ptrlist_new();
		if(ptr == NULL) {
			return(NULL);
		}
	}

	lp = f_ptrlist_last(ptr);
	if(lp == ptr && lp->data == NULL) {
		/* nada */
	} else {
		lp->next = f_ptrlist_new();
		if(lp->next == NULL) {
			return(NULL);
		}
		lp->next->prev = lp;
		lp = lp->next;
	}

	lp->data = data;

	return(ptr);
}
#else
int f_ptrlist_append(FPtrList *self, void *data)
{
}
#endif

int f_ptrlist_clear(FPtrList *list, FVisitor *visitor)
{
	FPtrList *next;

	while(list != NULL) {
		next = list->next;
		if(visitor != NULL) {
			f_visit(list->data, visitor);
		}
		free(list);
		list = next;
	}
	return 0;
}

int f_ptrlist_contains_ptr(const FPtrList *list, const void *ptr)
{
	return f_ptrlist_contains(list, f_ptrlistitem_ptrcmp, ptr);
}

FPtrList *f_ptrlist_filter(const FPtrList *list, const FMatcher *matcher)
{
	FPtrList *retlist = NULL;

	for(; list != NULL; list = list->next) {
		if(f_match(list->data, matcher)) {
			retlist = f_ptrlist_append(retlist, list->data);
		}
	}
	return retlist;
}
