/*
 *  flistaccumulator.h
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

#ifndef F_LISTACCUMULATOR_H
#define F_LISTACCUMULATOR_H

#include "flist.h"

typedef struct FPtrListAccumulator FPtrListAccumulator;

struct FPtrListAccumulator {
	FPtrList *head;
	FPtrList *last;
};

void f_ptrlistaccumulator_init (FPtrListAccumulator *listaccumulator, FPtrList *list);
FPtrList *f_ptrlistaccumulator_fini (FPtrListAccumulator *listaccumulator);

void f_ptrlistaccumulator_accumulate (FPtrListAccumulator *listaccumulator, void *data);
void f_ptrlistaccumulator_reverse_accumulate (FPtrListAccumulator *listaccumulator, void *data);

void f_ptrlistaccumulate (void *data, FPtrListAccumulator *listaccumulator);
void f_ptrlistreverseaccumulate (void *data, FPtrListAccumulator *listaccumulator);

#endif /* F_LISTACCUMULATOR_H */

/* vim: set ts=2 sw=2 noet: */
