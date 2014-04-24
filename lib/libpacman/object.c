/*
 *  object.c
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

#include "config.h"

#include "object.h"

#include "util.h"

#include <fstdlib.h>

int _pacman_object_delete(struct __pmobject_t *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(self->operations == NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	if(self->operations->fini != NULL) {
		self->operations->fini(self);
	}
	f_free(self);
	return 0;
}

int _pacman_object_init(struct __pmobject_t *self, struct __pmobject_private_t *object_private)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	self->object_private = object_private;
	return 0;
}

int _pacman_object_fini(struct __pmobject_t *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(self->operations != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	self->operations = NULL;
	return 0;
}

const struct __pmobject_operations_t *_pacman_object_operations(struct __pmobject_t *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
	ASSERT(self->operations != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return self->operations;
}

int _pacman_object_get(struct __pmobject_t *self, unsigned val, unsigned long *data)
{
	const struct __pmobject_operations_t *object_operations;

	ASSERT((object_operations = _pacman_object_operations(self)) != NULL, return -1);
	ASSERT(object_operations->get != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	return object_operations->get(self, val, data);
}

int _pacman_object_set(struct __pmobject_t *self, unsigned val, unsigned long data)
{
	const struct __pmobject_operations_t *object_operations;

	ASSERT((object_operations = _pacman_object_operations(self)) != NULL, return -1);
	ASSERT(object_operations->set != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	return object_operations->set(self, val, data);
}

struct __pmobject_t *_pacman_objectmemory_alloc(size_t size, const struct __pmobject_operations_t *operations)
{
	struct __pmobject_t *ret;

	ASSERT(size >= sizeof(*ret), RET_ERR(PM_ERR_WRONG_ARGS, NULL));
	ASSERT(operations == NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	if((ret = f_zalloc(size)) != NULL) {
		ret->operations = operations;
	}
	return ret;
}

/* vim: set ts=2 sw=2 noet: */
