/*
 *  fcallbacks.h
 *
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
#ifndef F_CALLBACKS_H
#define F_CALLBACKS_H

#include "fstdlib.h"
#include "util.h"

typedef void (*FVisitorFunc)(void *ptr, void *visitor_data);

typedef struct FVisitor FVisitor;

struct FVisitor {
#ifdef __cplusplus
	FVisitor()
		: fn(NULL), data(NULL)
	{ }

	FVisitor(FVisitorFunc _fn, void *_data)
		: fn(_fn), data(_data)
	{ }
#endif

	FVisitorFunc fn;
	void *data;
};

static inline
int f_visitor_init(FVisitor *self, FVisitorFunc fn, void *data)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	self->fn = fn;
	self->data = data;
	return 0;
}

static inline
int f_visitor_fini(FVisitor *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	self->fn = NULL;
	self->data = NULL;
	return 0;
}

static inline
FVisitor *f_visitor_new(FVisitorFunc fn, void *data)
{
	FVisitor *self;

	ASSERT((self = (FVisitor *)f_zalloc(sizeof(*self))) != NULL, return NULL);
	f_visitor_init(self, fn, data);
	return self;
}

static inline
int f_visitor_delete(FVisitor *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	f_visitor_fini(self);
	free(self);
	return 0;
}

static inline
void f_visit(void *ptr, const FVisitor *visitor) {
	ASSERT(visitor != NULL, return);
	ASSERT(visitor->fn != NULL, return);

	visitor->fn(ptr, visitor->data);
}

#endif /* F_CALLBACKS_H */

/* vim: set ts=2 sw=2 noet: */