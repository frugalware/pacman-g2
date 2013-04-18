/*
 *  fstdlib.c
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

#include "fstdlib.h"

void *f_malloc (size_t size)
{
	void *ptr = malloc (size);
#if 0
	if(ptr == NULL) {
		_pacman_log(PM_LOG_ERROR, _("malloc failure: could not allocate %d bytes"), size);
		RET_ERR(PM_ERR_MEMORY, NULL);
	}
#endif
	return ptr;
}

void *f_zalloc (size_t size)
{
	void *ptr = f_malloc (size);
	if (ptr != NULL)
		memset (ptr, 0, size);
	return ptr;
}

void *f_memdup (const void *ptr, size_t size) {
	void *dest = f_malloc (size);
	if (dest != NULL)
		memcpy (dest, ptr, size);
	return dest;
}

int f_ptrcmp (const void *p1, const void *p2) {
	  return p1-p2;
}

void f_ptrswap (void **p1, void **p2) {
	void *tmp = *p1;
	*p2 = *p1;
	*p1 = tmp;
}

/* vim: set ts=2 sw=2 noet: */
