/*
 *  fcallbacks.c
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

#include <stddef.h>

#include "fcallbacks.h"

int f_detect (const void *ptr, FDetector *detector) {
	return detector->fn (ptr, detector->user_data);
}

void f_visit (void *ptr, FVisitor *visitor) {
	if (visitor != NULL && visitor->fn != NULL) {	
		visitor->fn (ptr, visitor->user_data);
	}
}

void f_detectvisit (void *ptr, FDetectVisitor *detectvisitor) {
	if (detectvisitor != NULL) {
		f_visit (ptr, f_detect (ptr, detectvisitor->detect) == 0 ?
				detectvisitor->success : detectvisitor->fail);
	}
}

/* vim: set ts=2 sw=2 noet: */
