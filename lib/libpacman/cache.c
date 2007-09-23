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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <libintl.h>
#include <dirent.h>
/* pacman-g2 */
#include "log.h"
#include "pacman.h"
#include "list.h"
#include "util.h"
#include "package.h"
#include "group.h"
#include "db.h"
#include "handle.h"
#include "error.h"
#include "cache.h"

/* Returns a new package cache from db.
 * It frees the cache if it already exists.
 */
int _pacman_db_load_pkgcache(pmdb_t *db)
{
	pmpkg_t *info;

	if(db == NULL) {
		return(-1);
	}

	_pacman_db_free_pkgcache(db);

	_pacman_log(PM_LOG_DEBUG, _("loading package cache (infolevel=%#x) for repository '%s'"),
	                        0, db->treename);

	_pacman_db_rewind(db);
	while((info = _pacman_db_scan(db, NULL, 0)) != NULL) {
		info->origin = PKG_FROM_CACHE;
		info->data = db;
		/* add to the collective */
		db->pkgcache = _pacman_list_add_sorted(db->pkgcache, info, _pacman_pkg_cmp);
	}

	return(0);
}

void _pacman_db_free_pkgcache(pmdb_t *db)
{
	if(db == NULL || db->pkgcache == NULL) {
		return;
	}

	_pacman_log(PM_LOG_DEBUG, _("freeing package cache for repository '%s'"),
	                        db->treename);

	FREELISTPKGS(db->pkgcache);

	if(db->grpcache) {
		_pacman_db_free_grpcache(db);
	}
}

pmlist_t *_pacman_db_get_pkgcache(pmdb_t *db)
{
	if(db == NULL) {
		return(NULL);
	}

	if(db->pkgcache == NULL) {
		_pacman_db_load_pkgcache(db);
	}

	return(db->pkgcache);
}

int _pacman_db_add_pkgincache(pmdb_t *db, pmpkg_t *pkg)
{
	pmpkg_t *newpkg;

	if(db == NULL || pkg == NULL) {
		return(-1);
	}

	newpkg = _pacman_pkg_dup(pkg);
	if(newpkg == NULL) {
		return(-1);
	}
	_pacman_log(PM_LOG_DEBUG, _("adding entry '%s' in '%s' cache"), newpkg->name, db->treename);
	db->pkgcache = _pacman_list_add_sorted(db->pkgcache, newpkg, _pacman_pkg_cmp);

	_pacman_db_free_grpcache(db);

	return(0);
}

int _pacman_db_remove_pkgfromcache(pmdb_t *db, pmpkg_t *pkg)
{
	pmpkg_t *data;

	if(db == NULL || pkg == NULL) {
		return(-1);
	}

	db->pkgcache = _pacman_list_remove(db->pkgcache, pkg, _pacman_pkg_cmp, (void **)&data);
	if(data == NULL) {
		/* package not found */
		return(-1);
	}

	_pacman_log(PM_LOG_DEBUG, _("removing entry '%s' from '%s' cache"), pkg->name, db->treename);
	FREEPKG(data);

	_pacman_db_free_grpcache(db);

	return(0);
}

pmpkg_t *_pacman_db_get_pkgfromcache(pmdb_t *db, const char *target)
{
	if(db == NULL) {
		return(NULL);
	}

	return(_pacman_pkg_isin(target, _pacman_db_get_pkgcache(db)));
}

/* Returns a new group cache from db.
 */
int _pacman_db_load_grpcache(pmdb_t *db)
{
	pmlist_t *lp;

	if(db == NULL) {
		return(-1);
	}

	if(db->pkgcache == NULL) {
		_pacman_db_load_pkgcache(db);
	}

	_pacman_log(PM_LOG_DEBUG, _("loading group cache for repository '%s'"), db->treename);

	for(lp = db->pkgcache; lp; lp = lp->next) {
		pmlist_t *i;
		pmpkg_t *pkg = lp->data;

		if(!(pkg->infolevel & INFRQ_DESC)) {
			_pacman_db_read(pkg->data, INFRQ_DESC, pkg);
		}

		for(i = pkg->groups; i; i = i->next) {
			if(!_pacman_list_is_strin(i->data, db->grpcache)) {
				pmgrp_t *grp = _pacman_grp_new();

				STRNCPY(grp->name, (char *)i->data, GRP_NAME_LEN);
				grp->packages = _pacman_list_add_sorted(grp->packages, pkg->name, _pacman_grp_cmp);
				db->grpcache = _pacman_list_add_sorted(db->grpcache, grp, _pacman_grp_cmp);
			} else {
				pmlist_t *j;

				for(j = db->grpcache; j; j = j->next) {
					pmgrp_t *grp = j->data;

					if(strcmp(grp->name, i->data) == 0) {
						if(!_pacman_list_is_strin(pkg->name, grp->packages)) {
							grp->packages = _pacman_list_add_sorted(grp->packages, (char *)pkg->name, _pacman_grp_cmp);
						}
					}
				}
			}
		}
	}

	return(0);
}

void _pacman_db_free_grpcache(pmdb_t *db)
{
	pmlist_t *lg;

	if(db == NULL || db->grpcache == NULL) {
		return;
	}

	for(lg = db->grpcache; lg; lg = lg->next) {
		pmgrp_t *grp = lg->data;

		FREELISTPTR(grp->packages);
		FREEGRP(lg->data);
	}
	FREELIST(db->grpcache);
}

pmlist_t *_pacman_db_get_grpcache(pmdb_t *db)
{
	if(db == NULL) {
		return(NULL);
	}

	if(db->grpcache == NULL) {
		_pacman_db_load_grpcache(db);
	}

	return(db->grpcache);
}

pmgrp_t *_pacman_db_get_grpfromcache(pmdb_t *db, const char *target)
{
	pmlist_t *i;

	if(db == NULL || target == NULL || strlen(target) == 0) {
		return(NULL);
	}

	for(i = _pacman_db_get_grpcache(db); i; i = i->next) {
		pmgrp_t *info = i->data;

		if(strcmp(info->name, target) == 0) {
			return(info);
		}
	}

	return(NULL);
}

int _pacman_sync_cleancache(int level)
{
	char dirpath[PATH_MAX];

	snprintf(dirpath, PATH_MAX, "%s%s", handle->root, handle->cachedir);

	if(!level) {
		/* incomplete cleanup: we keep latest packages and partial downloads */
		DIR *dir;
		struct dirent *ent;
		pmlist_t *cache = NULL;
		pmlist_t *clean = NULL;
		pmlist_t *i, *j;

		dir = opendir(dirpath);
		if(dir == NULL) {
			RET_ERR(PM_ERR_NO_CACHE_ACCESS, -1);
		}
		rewinddir(dir);
		while((ent = readdir(dir)) != NULL) {
			if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
				continue;
			}
			cache = _pacman_list_add(cache, strdup(ent->d_name));
		}
		closedir(dir);

		for(i = cache; i; i = i->next) {
			char *str = i->data;
			char name[256], version[64];

			if(strstr(str, PM_EXT_PKG) == NULL) {
				clean = _pacman_list_add(clean, strdup(str));
				continue;
			}
			/* we keep partially downloaded files */
			if(strstr(str, PM_EXT_PKG ".part")) {
				continue;
			}
			if(_pacman_pkg_splitname(str, name, version, 1) != 0) {
				clean = _pacman_list_add(clean, strdup(str));
				continue;
			}
			for(j = i->next; j; j = j->next) {
				char *s = j->data;
				char n[256], v[64];

				if(strstr(s, PM_EXT_PKG) == NULL) {
					continue;
				}
				if(strstr(s, PM_EXT_PKG ".part")) {
					continue;
				}
				if(_pacman_pkg_splitname(s, n, v, 1) != 0) {
					continue;
				}
				if(!strcmp(name, n)) {
					char *ptr = (pacman_pkg_vercmp(version, v) < 0) ? str : s;
					if(!_pacman_list_is_strin(ptr, clean)) {
						clean = _pacman_list_add(clean, strdup(ptr));
					}
				}
			}
		}
		FREELIST(cache);

		for(i = clean; i; i = i->next) {
			char path[PATH_MAX];

			snprintf(path, PATH_MAX, "%s/%s", dirpath, (char *)i->data);
			unlink(path);
		}
		FREELIST(clean);
	} else {
		/* full cleanup */

		if(_pacman_rmrf(dirpath)) {
			RET_ERR(PM_ERR_CANT_REMOVE_CACHE, -1);
		}

		if(_pacman_makepath(dirpath)) {
			RET_ERR(PM_ERR_CANT_CREATE_CACHE, -1);
		}
	}

	return(0);
}

/* vim: set ts=2 sw=2 noet: */
