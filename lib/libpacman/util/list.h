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
#ifndef _PACMAN_LIST_H
#define _PACMAN_LIST_H

#include "pacman.h"

#include "util/flist.h"
#include "util/fptrlist.h"

#define _FREELIST(p, f) do { if(p) { FVisitor visitor = { .fn = (FVisitorFunc)f, .data = NULL, }; f_ptrlist_delete(p, &visitor); p = NULL; } } while(0)
#define FREELIST(p) _FREELIST(p, free)
#define FREELISTPTR(p) _FREELIST(p, NULL)

/* Sort comparison callback function declaration */
typedef int (*_pacman_fn_cmp)(const void *, const void *);

#define _pacman_list_new f_ptrlist_new

#define _pacman_list_add f_ptrlist_append
#define _pacman_list_count f_ptrlist_count
#define _pacman_list_empty f_ptrlist_empty

pmlist_t *_pacman_list_add_sorted(pmlist_t *list, void *data, _pacman_fn_cmp fn);
pmlist_t *_pacman_list_remove(pmlist_t *haystack, void *needle, _pacman_fn_cmp fn, void **data);
pmlist_t *_pacman_list_last(pmlist_t *list);
pmlist_t *_pacman_list_reverse(pmlist_t *list);

#endif /* _PACMAN_LIST_H */

/* vim: set ts=2 sw=2 noet: */
