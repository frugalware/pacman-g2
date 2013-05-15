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
#ifndef F_STDLIB_H
#define F_STDLIB_H

#include <stdlib.h>

void f_free (void *ptr);
void *f_malloc (size_t size);
void *f_zalloc (size_t size);

void *f_memdup (const void *ptr, size_t size);

int f_ptrcmp (const void *p1, const void *p2);
int f_ptreq (const void *p1, const void *p2);
int f_ptrneq (const void *p1, const void *p2);
void f_ptrswap (void **p1, void **p2);

#endif /* F_STDLIB_H */

/* vim: set ts=2 sw=2 noet: */
