/*
 *  fvalue.h
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
#ifndef F_VALUE_H
#define F_VALUE_H

#include <stdlib.h> /* for NULL */

typedef struct FValue FValue;
typedef struct FValueOps FValueOps;

enum FValueType {
	F_VALUETYPE_INVALID = -1,
	F_VALUETYPE_VOID = 0,
	F_VALUETYPE_OBJECT = 1,
	F_VALUETYPE_LIST = 2,
	/* F_VALUETYPE_USERDEFINED = 128, */
};

struct FValue {
	struct FValueImpl *impl;
};

struct FValueOps {
	int  (*init) (void **value, enum FValueType type, void *data);
	void (*fini) (void *value);
};

#define F_VALUE_INIT { .impl = NULL, }

FValue f_value_new (enum FValueType type, void *data);
void f_value_delete (FValue value);

void f_value_get (const FValue value, enum FValueType type, void **data);
void f_value_set (FValue *value, enum FValueType type, void *data);

/* Implementation details */
//int f_value_init (FValue *value, const FValueOps *ops);
//int f_value_fini (FValue *value);

#endif /* F_VALUE_H */

/* vim: set ts=2 sw=2 noet: */
