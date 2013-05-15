/*
 *  flistaccumulator.c
 *
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

#include <assert.h>

#include "flistaccumulator.h"

#include "flist.h"

void f_listaccumulator_init (FListAccumulator *listaccumulator, FList *list) {
	listaccumulator->head = list;
	listaccumulator->last = f_list_last (list);
}

FList *f_listaccumulator_fini (FListAccumulator *listaccumulator) {
	FList *ret = listaccumulator->head;

	listaccumulator->head = listaccumulator->last = NULL;
	return ret;
}

void f_listaccumulator_accumulate (FListAccumulator *listaccumulator, void *data) {
	f_listaccumulator (data, listaccumulator);
}

void f_listaccumulator_reverse_accumulate (FListAccumulator *listaccumulator, void *data) {
	f_listreverseaccumulator (data, listaccumulator);
}

void f_listaccumulator (void *data, FListAccumulator *listaccumulator) {
	FList *item = f_list_alloc (data);

	if (listaccumulator->head != NULL) {
		f_list_insert_after (listaccumulator->last, item);
		listaccumulator->last = listaccumulator->last->next;
	} else {
		listaccumulator->head = listaccumulator->last = item;
	}
}

void f_listreverseaccumulator (void *data, FListAccumulator *listaccumulator) {
	FList *item = f_list_alloc (data);

	if (listaccumulator->head != NULL) {
		f_list_insert_before (listaccumulator->head, item);
		listaccumulator->head = listaccumulator->head->prev;
	} else {
		listaccumulator->head = listaccumulator->last = item;
	}
}

/* vim: set ts=2 sw=2 noet: */
