/*
 *  dirname.c
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

#include <libgen.h> // Must be before string.h

#include "fstring.h"

#include <limits.h>

char *f_dirname(const char *path)
{
	static char buffer[PATH_MAX];

	ASSERT(!_pacman_strempty(path), RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	STRNCPY(buffer, path, sizeof(buffer));
	return dirname(buffer);
}

/* vim: set ts=2 sw=2 noet: */
