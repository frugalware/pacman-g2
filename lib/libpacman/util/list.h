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

typedef int (*FVisitorFunc)(void *ptr, void *visitor_data);

typedef int (*flist_compar_t)(const pmlist_t *item, const void *compar_data);
typedef int (*flist_visitor_t)(pmlist_t *item, void *visitor_data);

/* Chained list struct */
struct __pmlist_t {
	void *data;
	struct __pmlist_t *prev;
	struct __pmlist_t *next;
	struct __pmlist_t *last; /* Quick access to last item in list */
};

#define _FREELIST(p, f) do { if(p) { f_ptrlist_free(p, (FVisitorFunc)f, NULL); p = NULL; } } while(0)
#define FREELIST(p) _FREELIST(p, free)
#define FREELISTPTR(p) _FREELIST(p, NULL)

/* Sort comparison callback function declaration */
typedef int (*_pacman_fn_cmp)(const void *, const void *);

pmlist_t *_pacman_list_new(void);

int f_list_contains(const pmlist_t *list, flist_compar_t compar, const void *compar_data);
int _pacman_list_count(const pmlist_t *list);
int _pacman_list_empty(const pmlist_t *list);
int _pacman_list_is_in(void *needle, const pmlist_t *haystack);
void f_list_foreach(const pmlist_t *list, flist_visitor_t visitor, void *visitor_data);

pmlist_t *_pacman_list_add(pmlist_t *list, void *data);
pmlist_t *_pacman_list_add_sorted(pmlist_t *list, void *data, _pacman_fn_cmp fn);
pmlist_t *_pacman_list_remove(pmlist_t *haystack, void *needle, _pacman_fn_cmp fn, void **data);
pmlist_t *_pacman_list_last(pmlist_t *list);
pmlist_t *_pacman_list_reverse(pmlist_t *list);

void f_ptrlist_free(pmlist_t *list, FVisitorFunc visitor, void *visitor_data);

void f_ptrlist_clear(pmlist_t *list, FVisitorFunc visitor, void *visitor_data);

#endif /* _PACMAN_LIST_H */

/* vim: set ts=2 sw=2 noet: */
