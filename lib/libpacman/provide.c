/*
 *  provide.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2007 by Miklos Vajna <vmiklos@frugalware.org>
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

#include "config.h"
#include <stdlib.h>
#include <string.h>

/* pacman-g2 */
#include "provide.h"

#include "cache.h"
#include "list.h"
#include "db.h"
#include "util.h"

/* return a pmlist_t of packages in "db" that provide "package"
 */
pmlist_t *_pacman_db_whatprovides(pmdb_t *db, char *package)
{
	pmlist_t *pkgs = NULL;
	pmlist_t *lp;

	if(db == NULL || _pacman_strempty(package)) {
		return(NULL);
	}

	for(lp = _pacman_db_get_pkgcache(db); lp; lp = lp->next) {
		pmpkg_t *info = lp->data;

		if(_pacman_list_is_strin(package, _pacman_pkg_getinfo(info, PM_PKG_PROVIDES))) {
			pkgs = _pacman_list_add(pkgs, info);
		}
	}

	return(pkgs);
}

/* vim: set ts=2 sw=2 noet: */
