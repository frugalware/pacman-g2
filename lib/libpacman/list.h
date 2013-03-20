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

/* Chained list struct */
typedef struct __pmlist_t {
	void *data;
	struct __pmlist_t *prev;
	struct __pmlist_t *next;
	struct __pmlist_t *last; /* Quick access to last item in list */
} pmlist_t;

#define _FREELIST(p, f) do { if(p) { _pacman_list_free(p, f); p = NULL; } } while(0)
#define FREELIST(p) _FREELIST(p, free)
#define FREELISTPTR(p) _FREELIST(p, NULL)

/**
 * Detection comparison callback function declaration.
 * 
 * If detection is successful callback must return 0, or any other
 * values in case of failure (So it can be equivalent to a cmp).
 */
typedef int (*_pacman_fn_detect)(const void *, void *);
typedef void *(*_pacman_fn_dup)(const void *);
typedef void (*_pacman_fn_free)(void *);
typedef void (*_pacman_fn_foreach)(void *, void *);
/* Sort comparison callback function declaration */
typedef int (*_pacman_fn_cmp)(const void *, const void *);

pmlist_t *_pacman_list_new(void);
pmlist_t *_pacman_list_dup(pmlist_t *list, _pacman_fn_dup fn);
void _pacman_list_free(pmlist_t *list, _pacman_fn_free fn);

pmlist_t *_pacman_list_detect(pmlist_t *list, _pacman_fn_detect fn, void *user_data);
pmlist_t *_pacman_list_find(pmlist_t *list, void *data);
void _pacman_list_foreach(pmlist_t *list, _pacman_fn_foreach fn, void *user_data);

pmlist_t *_pacman_list_add(pmlist_t *list, void *data);
pmlist_t *_pacman_list_add_sorted(pmlist_t *list, void *data, _pacman_fn_cmp fn);
pmlist_t *_pacman_list_remove(pmlist_t *haystack, void *needle, _pacman_fn_cmp fn, void **data);
int _pacman_list_count(pmlist_t *list);
int _pacman_list_is_in(void *needle, pmlist_t *haystack);
int _pacman_list_is_strin(const char *needle, pmlist_t *haystack);
pmlist_t *_pacman_list_last(pmlist_t *list);
pmlist_t *_pacman_list_remove_dupes(pmlist_t *list);
pmlist_t *_pacman_list_reverse(pmlist_t *list);

pmlist_t *_pacman_strlist_dup(pmlist_t *list);

#endif /* _PACMAN_LIST_H */

/* vim: set ts=2 sw=2 noet: */
