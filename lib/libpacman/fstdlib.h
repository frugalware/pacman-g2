/*
 *  fstdlib.h
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
#ifndef _F_STDLIB_H
#define _F_STDLIB_H

#include <stdint.h>
#include <stdlib.h>

#include "util/log.h"
#include "util.h"

static inline
void *f_malloc(size_t size)
{
	void *ptr = malloc(size);
	if(ptr == NULL) {
		_pacman_log(PM_LOG_ERROR, _("malloc failure: could not allocate %d bytes"), size);
		RET_ERR(PM_ERR_MEMORY, NULL);
	}
	return ptr;
}

static inline
void *f_zalloc(size_t size)
{
	void *ptr = f_malloc(size);
	if(ptr != NULL)
		memset(ptr, 0, size);
	return ptr;
}

static inline
int f_ptrcmp(const void *ptr1, const void *ptr2) {
	return (uintptr_t)ptr1 - (uintptr_t)ptr2;
}

static inline
void f_ptrswap(void **ptr1, void **ptr2)
{
	void *tmp = *ptr2;
	*ptr2 = *ptr1;
	*ptr1 = tmp;
}

#endif /* _F_STDLIB_H */

/* vim: set ts=2 sw=2 noet: */
