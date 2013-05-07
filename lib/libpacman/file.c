/*
 *  file.c
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

#include "file.h"
#include "util.h"

int __pacman_file_init(struct pmfile *file, const struct pmfile_ops *ops) {
	if (ops == NULL ||
			f_object_init(&file->base, &ops->base)) {
		return EINVAL;
	}

	file->path = NULL;
	return 0;
}

int __pacman_file_fini(struct pmfile *file) {
	free(file->path);
	return 0;
}

static const
struct pmfile_ops _pacman_file_ops = {
	.base = {
		.fini = __pacman_file_fini,
	}
};

struct pmfile *_pacman_file_new(void) {
	struct pmfile *file = _pacman_zalloc(sizeof(*file));
	__pacman_file_init(file, &_pacman_file_ops);
	return file;
}

/* vim: set ts=2 sw=2 noet: */
