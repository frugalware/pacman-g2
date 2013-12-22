/*
 *  flist.c
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

#include "util/flist.h"

#include "fstdlib.h"

#include <assert.h>

int f_listitem_delete(FListItem *self, FVisitor *visitor)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	f_visit(self, visitor);
	free(self);
	return 0;
}

FList *f_list_new()
{
	return NULL;
}

int f_list_all_match(const FList *list, const FMatcher *matcher)
{
	const FListItem *it;

	if(f_list_empty(list)) {
		return 0;
	}

	for(it = list; it != NULL; it = it->next) {
		if(f_match(it, matcher) == 0) {
			return 0;
		}
	}
	return 1;
}

int f_list_any_match(const FList *list, const FMatcher *matcher)
{
	const FListItem *it;

	for(it = list; it != NULL; it = it->next) {
		if(f_match(it, matcher) != 0) {
			return 1;
		}
	}
	return 0;
}

int f_list_contains(const FList *list, FListItemComparatorFunc comparator, const void *comparator_data)
{
	return f_list_find(list, comparator, comparator_data) != NULL;
}

int f_list_count(const FList *list)
{
	int i;

	for(i = 0; list; list = list->next, i++);
	return i;
}

int f_list_empty(const FList *list)
{
	return list == NULL;
}

FListItem *f_list_find(const FList *list, FListItemComparatorFunc comparator, const void *comparator_data)
{
	for(; list != NULL; list = list->next) {
		if(comparator(list, comparator_data) == 0) {
			break;
		}
	}
	return (FListItem *)list;
}

FListItem *f_list_first(FList *self)
{
	return (FListItem *)f_list_first_const(self);
}

const FListItem *f_list_first_const(const FList *self)
{
	return (FListItem *)self;
}

void f_list_foreach(const FList *list, FListItemVisitorFunc visitor, void *visitor_data)
{
	for(; list != NULL; list = list->next) {
		visitor((FListItem *)list, visitor_data);
	}
}

FListItem *f_list_last(FList *self)
{
	return (FListItem *)f_list_last_const(self);
}

const FListItem *f_list_last_const(const FList *self)
{
	if(self == NULL) {
		return(NULL);
	}

	assert(self->last != NULL);
	return(self->last);
}
