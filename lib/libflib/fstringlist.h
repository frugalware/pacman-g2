/*
 *  stringlist.h
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
#ifndef F_STRINGLIST_H
#define F_STRINGLIST_H

#include "flist.h"

typedef struct FStringListItem FStringListItem;

typedef struct FStringList FStringList;

FList *f_stringlist_append (FList *list, const char *str);
FList *f_stringlist_deep_copy (FList *list);
void   f_stringlist_detach (FList *list);
FList *f_stringlist_find (FList *list, const char *str);
char  *f_stringlist_join (FList *list, const char *sep);
FList *f_stringlist_remove_all (FList *list, const char *str);
FList *f_stringlist_uniques (FList *list);

#endif /* F_STRINGLIST_H */

/* vim: set ts=2 sw=2 noet: */
