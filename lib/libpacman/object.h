/*
 *  object.h
 *
 *  Copyright (c) 2014 by Michel Hermier <hermier@frugalware.org>
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
#ifndef _PACMAN_OBJECT_H
#define _PACMAN_OBJECT_H

#include <stddef.h>

namespace libpacman
{

class Object
{
public:
	virtual ~Object();

	virtual int get(unsigned val, unsigned long *data) const;
	virtual int set(unsigned val, unsigned long data);
};

}

struct __pmobject_t;
struct __pmobject_private_t;

struct __pmobject_operations_t
{
	int (*fini)(struct __pmobject_t *object);

	int (*get)(struct __pmobject_t *object, unsigned val, unsigned long *data);
	int (*set)(struct __pmobject_t *object, unsigned val, unsigned long data);
};

struct __pmobject_t
{
	const struct __pmobject_operations_t *operations;

	struct __pmobject_private_t *object_private;
};

struct __pmobject_t *_pacman_object_alloc(size_t size, const struct __pmobject_operations_t *object_operations, struct __pmobject_private_t *object_private);

int _pacman_object_delete(struct __pmobject_t *self);

int _pacman_object_init(struct __pmobject_t *self, struct __pmobject_private_t *object_private);
int _pacman_object_fini(struct __pmobject_t *self);

const struct __pmobject_operations_t *_pacman_object_operations(struct __pmobject_t *self);

int _pacman_object_get(struct __pmobject_t *self, unsigned val, unsigned long *data);
int _pacman_object_set(struct __pmobject_t *self, unsigned val, unsigned long data);

struct __pmobject_t *_pacman_objectmemory_alloc(size_t size, const struct __pmobject_operations_t *object_operations);

#endif /* _PACMAN__OBJECT_H */

/* vim: set ts=2 sw=2 noet: */
