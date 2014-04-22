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

int _pacman_object_init(struct __pmobject_t *self, const struct __pmobject_operations_t *operations)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(self->operations == NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	self->operations = NULL;
	return 0;
}

int _pacman_object_fini(struct __pmobject_t *self)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(self->operations != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	self->operations = NULL;
	return 0;
}

int _pacman_object_get(struct __pmobject_t *self, unsigned val, unsigned long *data)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(self->operations != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(self->operations->get != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	return self->operations->get(self, val, data);
}

int _pacman_object_set(struct __pmobject_t *self, unsigned val, unsigned long data)
{
	ASSERT(self != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(self->operations != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(self->operations->set != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	return self->operations->set(self, val, data);
}

/* vim: set ts=2 sw=2 noet: */
