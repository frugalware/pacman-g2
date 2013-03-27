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

#include "config.h"

#include <errno.h>
#include <stdlib.h> /* for NULL */

#include "fobject.h"

int f_object_init (FObject *object, const FObjectOps *ops) {
	if (object == NULL ||
			ops == NULL) {
		return EINVAL;
	}

	object->ops = ops;
	return 0;
}

int f_object_fini (FObject *object) {
	if (object == NULL ||
			object->ops == NULL) {
		return EINVAL;
	}

	return 0;
}

void f_object_delete (FObject *object) {
	if (object != NULL) {
		if (object->ops == NULL &&
				object->ops->fini != NULL) {
			object->ops->fini (object);
		}
		free (object);
	}
}

/* vim: set ts=2 sw=2 noet: */
