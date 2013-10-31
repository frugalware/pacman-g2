/*
 *  stringlist.h
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
#ifndef _PACMAN_STRINGLIST_H
#define _PACMAN_STRINGLIST_H

#include "list.h"

int _pacman_list_is_strin(const char *needle, pmlist_t *haystack);
pmlist_t *_pacman_list_remove_dupes(pmlist_t *list);
pmlist_t *_pacman_list_strdup(pmlist_t *list);

#endif /* _PACMAN_STRINGLIST_H */

/* vim: set ts=2 sw=2 noet: */
