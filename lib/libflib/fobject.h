/*
 *  fobject.h
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
#ifndef F_OBJECT_H
#define F_OBJECT_H

typedef struct FObject FObject;
typedef struct FObjectOps FObjectOps;

struct FObjectOps {
	/* int  (*init) (FObject *object); */
	void (*fini) (FObject *object);
#if 0
	int (*get) (FObject *object, const char *property, intptr_t *data);
	int (*set) (FObject *object, const char *property, intptr_t data);
#endif
};

struct FObject {
	const struct FObjectOps *ops;
};

void f_object_delete (FObject *object);

/* Implementation details */
int f_object_init (FObject *object, const FObjectOps *ops);
int f_object_fini (FObject *object);

#endif /* F_OBJECT_H */

/* vim: set ts=2 sw=2 noet: */
