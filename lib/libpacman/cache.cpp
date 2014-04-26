/*
 *  cache.c
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

#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* pacman-g2 */
#include "cache.h"

#include "util/list.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "fstring.h"
#include "pacman.h"
#include "util.h"
#include "package.h"
#include "group.h"
#include "db.h"
#include "handle.h"
#include "error.h"

using namespace libpacman;

static
int _pacman_db_clear_grpcache(pmdb_t *db);

static
int _pacman_db_load_grpcache(pmdb_t *db);

/* Returns a new package cache from db.
 * It frees the cache if it already exists.
 */
int _pacman_db_load_pkgcache(pmdb_t *db)
{
	pmpkg_t *info;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	_pacman_db_free_pkgcache(db);

	unsigned int inforeq = 0;
	if (db != handle->db_local)
		inforeq = INFRQ_DESC | INFRQ_DEPENDS;
	_pacman_log(PM_LOG_DEBUG, _("loading package cache (infolevel=%#x) for repository '%s'"),
	                        inforeq, db->treename);

	_pacman_db_rewind(db);
	while((info = _pacman_db_readpkg(db, inforeq)) != NULL) {
		info->origin = PKG_FROM_CACHE;
		info->data = db;
		/* add to the collective */
		db->pkgcache = _pacman_list_add_sorted(db->pkgcache, info, _pacman_pkg_cmp);
	}

	return(0);
}

void _pacman_db_free_pkgcache(pmdb_t *db)
{
	ASSERT(db != NULL, pm_errno = PM_ERR_DB_NULL; return);
	if(f_ptrlist_empty(db->pkgcache)) {
		return;
	}

	_pacman_log(PM_LOG_DEBUG, _("freeing package cache for repository '%s'"),
	                        db->treename);

	FREELISTPKGS(db->pkgcache);

	_pacman_db_clear_grpcache(db);
}

pmlist_t *_pacman_db_get_pkgcache(pmdb_t *db)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));

	if(f_ptrlist_empty(db->pkgcache)) {
		_pacman_db_load_pkgcache(db);
	}

	return(db->pkgcache);
}

int _pacman_db_add_pkgincache(pmdb_t *db, pmpkg_t *pkg)
{
	pmpkg_t *newpkg;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(pkg == NULL) {
		return(-1);
	}

	newpkg = _pacman_pkg_dup(pkg);
	if(newpkg == NULL) {
		return(-1);
	}
	_pacman_log(PM_LOG_DEBUG, _("adding entry '%s' in '%s' cache"), newpkg->name, db->treename);
	db->pkgcache = _pacman_list_add_sorted(db->pkgcache, newpkg, _pacman_pkg_cmp);

	_pacman_db_clear_grpcache(db);

	return(0);
}

int _pacman_db_remove_pkgfromcache(pmdb_t *db, pmpkg_t *pkg)
{
	pmpkg_t *data;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(pkg == NULL) {
		return(-1);
	}

	db->pkgcache = _pacman_list_remove(db->pkgcache, pkg, _pacman_pkg_cmp, (void **)&data);
	if(data == NULL) {
		/* package not found */
		return(-1);
	}

	_pacman_log(PM_LOG_DEBUG, _("removing entry '%s' from '%s' cache"), pkg->name, db->treename);
	FREEPKG(data);

	_pacman_db_clear_grpcache(db);

	return(0);
}

pmpkg_t *_pacman_db_get_pkgfromcache(pmdb_t *db, const char *target)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));
	if(_pacman_strempty(target)) {
		return NULL;
	}

	return(_pacman_pkg_isin(target, _pacman_db_get_pkgcache(db)));
}

/* return a pmlist_t of packages in "db" that provide "package"
 */
pmlist_t *_pacman_db_whatprovides(pmdb_t *db, char *package)
{
	FPtrList *pkgs = NULL;
	FStrMatcher strmatcher = { NULL };
	FMatcher packagestrmatcher = { NULL };

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));

	if(f_strmatcher_init(&strmatcher, package, F_STRMATCHER_EQUAL) == 0 &&
			_pacman_packagestrmatcher_init(&packagestrmatcher, &strmatcher, PM_PACKAGE_FLAG_PROVIDES) == 0) {
		pkgs = f_ptrlist_filter(_pacman_db_get_pkgcache(db), &packagestrmatcher);
	}
	_pacman_packagestrmatcher_fini(&packagestrmatcher);
	f_strmatcher_fini(&strmatcher);
	return(pkgs);
}

/* Returns a new group cache from db.
 */
int _pacman_db_load_grpcache(pmdb_t *db)
{
	pmlist_t *lp;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	lp = _pacman_db_get_pkgcache(db);

	_pacman_log(PM_LOG_DEBUG, _("loading group cache for repository '%s'"), db->treename);

	for(; lp; lp = lp->next) {
		pmlist_t *i;
		pmpkg_t *pkg = lp->data;

		if(!(pkg->infolevel & INFRQ_DESC)) {
			_pacman_db_read(pkg->data, pkg, INFRQ_DESC);
		}

		for(i = pkg->groups; i; i = i->next) {
			if(!_pacman_list_is_strin(i->data, db->grpcache)) {
				Group *grp = new Group();

				STRNCPY(grp->name, (char *)i->data, GRP_NAME_LEN);
				grp->packages = _pacman_list_add_sorted(grp->packages, pkg->name, _pacman_grp_cmp);
				db->grpcache = _pacman_list_add_sorted(db->grpcache, grp, _pacman_grp_cmp);
			} else {
				pmlist_t *j;

				for(j = db->grpcache; j; j = j->next) {
					Group *grp = j->data;

					if(strcmp(grp->name, i->data) == 0) {
						if(!_pacman_list_is_strin(pkg->name, grp->packages)) {
							grp->packages = _pacman_list_add_sorted(grp->packages, pkg->name, _pacman_grp_cmp);
						}
					}
				}
			}
		}
	}

	return(0);
}

int _pacman_db_clear_grpcache(pmdb_t *db)
{
	FVisitor visitor = {
		.fn = (FVisitorFunc)_pacman_grp_delete,
		.data = NULL,
	};

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	f_ptrlist_clear(db->grpcache, &visitor);
	db->grpcache = NULL;
	return 0;
}

pmlist_t *_pacman_db_get_grpcache(pmdb_t *db)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));

	if(db->grpcache == NULL) {
		_pacman_db_load_grpcache(db);
	}

	return(db->grpcache);
}

Group *_pacman_db_get_grpfromcache(pmdb_t *db, const char *target)
{
	pmlist_t *i;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));
	if(_pacman_strempty(target)) {
		return(NULL);
	}

	for(i = _pacman_db_get_grpcache(db); i; i = i->next) {
		Group *info = i->data;

		if(strcmp(info->name, target) == 0) {
			return(info);
		}
	}

	return(NULL);
}

/* vim: set ts=2 sw=2 noet: */
