/*
 *  fcallbacks.h
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
#ifndef F_CALLBACKS_H
#define F_CALLBACKS_H

#include "util.h"

typedef int (*FComparatorFunc)(const void *ptr, const void *visitor_data);
typedef void (*FVisitorFunc)(void *ptr, void *visitor_data);

typedef struct __FComparator FComparator;
typedef struct __FVisitor FVisitor;

struct __FComparator {
	FComparatorFunc fn;
	void *data;
};

struct __FVisitor {
	FVisitorFunc fn;
	void *data;
};

static inline
int f_compare(void *ptr, const FComparator *comparator) {
	ASSERT(comparator != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(comparator->fn != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	return comparator->fn(ptr, comparator->data);
}

static inline
void f_visit(void *ptr, const FVisitor *visitor) {
	ASSERT(visitor != NULL, return);
	ASSERT(visitor->fn != NULL, return);

	visitor->fn(ptr, visitor->data);
}

#endif /* F_CALLBACKS_H */

/* vim: set ts=2 sw=2 noet: */