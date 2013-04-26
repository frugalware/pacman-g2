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

#include "flist.h"

typedef FList pmlist_t;

#define _FREELIST(ptr, fn) do { if(ptr) { _pacman_list_free(ptr, fn); ptr = NULL; } } while(0)
#define FREELIST(p) _FREELIST(p, free)
#define FREELISTPTR(p) _FREELIST(p, NULL)

typedef FCompareFunc _pacman_fn_cmp;
typedef FDetectFunc _pacman_fn_detect;
typedef FCopyFunc _pacman_fn_dup;
typedef FVisitorFunc _pacman_fn_free;
typedef FVisitorFunc _pacman_fn_foreach;

#define _pacman_list_new f_list_new
#define _pacman_list_free(list, fn) f_list_delete ((list), (FVisitorFunc)(fn), NULL)

#define _pacman_list_first f_list_first
#define _pacman_list_last f_list_last

#define _pacman_list_detect f_list_detect
#define _pacman_list_filter f_list_filter
#define _pacman_list_find f_list_find
#define _pacman_list_foreach f_list_foreach
#define _pacman_list_reverse f_list_reverse
#define _pacman_list_reverse_foreach f_list_reverse_foreach

#define _pacman_list_add f_list_append
#define _pacman_list_add_sorted(list, ptr, fn) f_list_add_sorted ((list), (ptr), (FCompareFunc)(fn), NULL)
#define _pacman_list_remove(list, ptr, fn, data) f_list_remove((list), (ptr), (FCompareFunc)(fn), (data))
#define _pacman_list_count f_list_count

#include "fstringlist.h"

#define _pacman_strlist_dup f_stringlist_deep_copy
#define _pacman_strlist_find f_stringlist_find
#define _pacman_list_remove_dupes f_stringlist_filter_dupes

#endif /* _PACMAN_LIST_H */

/* vim: set ts=2 sw=2 noet: */
