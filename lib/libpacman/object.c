/*
 *  object.c
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

#include <errno.h>
#include <stdlib.h> /* for NULL */

#include "config.h"

#include "object.h"

int __pacman_object_init(struct pmobject *object, const struct pmobject_ops *ops) {
	if (object == NULL ||
			ops == NULL) {
		return EINVAL;
	}

	object->ops = ops;
	return 0;
}

int __pacman_object_fini(struct pmobject *object) {
	if (object == NULL ||
			object->ops == NULL) {
		return EINVAL;
	}

	return 0;
}

int _pacman_object_free(struct pmobject *object) {
	if (object == NULL ||
			object->ops == NULL) {
		return EINVAL;
	}

	if (object->ops->fini != NULL) {
		object->ops->fini (object);
	}

	free (object);
	return 0;
}

/* vim: set ts=2 sw=2 noet: */
