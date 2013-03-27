/*
 *  value.c
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

#include <errno.h>

#include "fvalue.h"

#include "fstdlib.h" /* for f_zalloc */

struct FValueImpl {
	enum FValueType type;
	void *value; /* or intptr_t ? */
};

#if 0
int f_value_init (FValue *value, const FValueOps *ops) {
	if (value == NULL ||
			ops == NULL) {
		return EINVAL;
	}

	value->ops = ops;
	return 0;
}

int f_value_fini (FValue *value) {
	if (value == NULL ||
			value->ops == NULL) {
		return EINVAL;
	}

	return 0;
}
#endif

FValue f_value_new (enum FValueType type, void *data) {
	FValue value = F_VALUE_INIT;

	if ((value.impl = f_zalloc(sizeof(*value.impl))) != NULL) {
//		typeops->init (&value.impl->value, type, data);
	}

	return value;
}

void f_value_delete (FValue value) {
	if (value.impl != NULL) {
#if 0
		if (value->ops == NULL &&
				value->ops->fini != NULL) {
			value->ops->fini (value);
		}
#endif
		free (value.impl);
	}
}

void f_value_get (const FValue value, enum FValueType type, void **data) {
	/* IMPLEMENT ME */
	abort ();
}

void f_value_set (FValue *value, enum FValueType type, void *data) {
	/* IMPLEMENT ME */
	abort ();
}

/* vim: set ts=2 sw=2 noet: */
