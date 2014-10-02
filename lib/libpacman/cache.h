/*
 *  cache.h
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
#ifndef _PACMAN_CACHE_H
#define _PACMAN_CACHE_H

#include "package.h"
#include "group.h"
#include "db.h"

/* packages */
int _pacman_db_load_pkgcache(libpacman::Database *db);
void _pacman_db_free_pkgcache(libpacman::Database *db);
int _pacman_db_add_pkgincache(libpacman::Database *db, libpacman::Package *pkg);
int _pacman_db_remove_pkgfromcache(libpacman::Database *db, libpacman::Package *pkg);
libpacman::package_set &_pacman_db_get_pkgcache(libpacman::Database *db);
/* groups */
libpacman::group_set &_pacman_db_get_grpcache(libpacman::Database *db);
libpacman::Group *_pacman_db_get_grpfromcache(libpacman::Database *db, const char *target);

#endif /* _PACMAN_CACHE_H */

/* vim: set ts=2 sw=2 noet: */
