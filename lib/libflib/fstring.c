/*
 *  stringlist.c
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

#include <assert.h>
#include <string.h>

#include "fstring.h"

char *f_strdup (const char *s) {
	return s != NULL ? strdup (s) : NULL;
}

char *f_strcpy (char *dest, const char *src) {
	assert (dest != NULL);

	if (f_strisempty (src) != 0) {
		return strcpy (dest, src);
	} else {
		dest[0] = '\0';
		return dest;
	}
}

char *f_strncpy (char *dest, const char *src, size_t n) {
	assert (dest != NULL);

	if (f_strisempty (src) != 0) {
		return strncpy (dest, src, n);
	} else {
		dest[0] = '\0';
		return dest;
	}
}

size_t f_strlen (const char *s) {
	return s != NULL ? strlen (s) : 0;
}

int f_strisempty (const char *s) {
	return (s == NULL || s[0] == '\0') ? 0 : -1;
}

/* vim: set ts=2 sw=2 noet: */
