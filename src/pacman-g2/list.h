/*
 *  list.h
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
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
#ifndef _PM_LIST_H
#define _PM_LIST_H

#include <pacman.h>

#include <util/fptrlist.h>
#include <util/fstringlist.h>

typedef FPtrList list_t;

void list_free(list_t *list);
#define list_begin f_ptrlist_first
#define list_count f_ptrlist_count
#define list_data f_ptrlistitem_data
#define list_next f_ptrlistitem_next
int list_is_strin(char *needle, list_t *haystack);
void list_display(const char *title, const FStringList *list);

#define PM_LIST_display list_display
list_t *PM_LIST_remove_dupes(PM_LIST *list);

#endif /* _PM_LIST_H */

/* vim: set ts=2 sw=2 noet: */
