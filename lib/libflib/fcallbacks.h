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

/* Sort comparison callback function declaration */
typedef int   (*FCompareFunc) (const void *ptr1, const void *ptr2, void *user_data);

typedef void *(*FCopyFunc) (const void *ptr, void *user_data);

/**
 * Detection comparison callback function declaration.
 * 
 * If detection is successful callback must return 0, or any other
 * values in case of failure (So it can be equivalent to a cmp).
 */
typedef int   (*FDetectFunc) (const void *ptr, void *user_data);

typedef void  (*FVisitorFunc) (void *ptr, void *user_data);

typedef struct FDetector FDetector;

struct FDetector {
	FDetectFunc fn;
	void *user_data;
};

int f_detect (const void *ptr, FDetector *detector);

typedef struct FVisitor FVisitor;

struct FVisitor {
	FVisitorFunc fn;
	void *user_data;
};

void f_visit (void *ptr, FVisitor *visitor);

typedef struct FDetectVisitor FDetectVisitor;

struct FDetectVisitor {
	FDetector *detect;
	FVisitor *success;
	FVisitor *fail;
};

void f_detectvisit (void *ptr, FDetectVisitor *conditionalvisitor);

#endif /* F_CALLBACKS_H */

/* vim: set ts=2 sw=2 noet: */
