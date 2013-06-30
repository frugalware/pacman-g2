/*
 *  db.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2006 by David Kimpe <dnaku@frugalware.org>
 *  Copyright (c) 2005, 2006 by Miklos Vajna <vmiklos@frugalware.org>
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

#if defined(__APPLE__) || defined(__OpenBSD__)
#include <sys/syslimits.h>
#include <sys/stat.h>
#endif

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#ifdef CYGWIN
#include <limits.h> /* PATH_MAX */
#endif

#include <fstring.h>

/* pacman-g2 */
#include "db.h"

#include "log.h"
#include "util.h"
#include "error.h"
#include "server.h"
#include "handle.h"
#include "cache.h"
#include "pacman.h"

void _pacman_db_init (pmdb_t *db, char *root, char* dbpath, const char *treename) {
	size_t path_size;

	assert (db != NULL);

	snprintf (db->path, PATH_MAX, "%s%s/", root, dbpath);
	path_size = f_strlen (db->path);
	db->treename = strncat (&db->path[path_size], treename, PATH_MAX - path_size - 1);
	db->pkgcache = f_ptrlist_new ();
	db->grpcache = f_ptrlist_new ();
	db->servers = f_ptrlist_new ();
}

void _pacman_db_fini (pmdb_t *db) {
	assert (db != NULL);

	_pacman_log(PM_LOG_FLOW1, _("unregistering database '%s'"), db->treename);

	/* Cleanup */
	_pacman_db_free_pkgcache(db);
	_pacman_db_close(db);
	FREELISTSERVERS(db->servers);
}

pmdb_t *_pacman_db_new (char *root, char* dbpath, const char *treename) {
	pmdb_t *db = _pacman_malloc (sizeof (*db));

	if (db != NULL) {
		_pacman_db_init (db, root, dbpath, treename);
	}
	return db;
}

void _pacman_db_free(pmdb_t *db)
{
	_pacman_db_fini (db);	
	free(db);
}

int _pacman_db_cmp(const void *db1, const void *db2)
{
	return(strcmp(((pmdb_t *)db1)->treename, ((pmdb_t *)db2)->treename));
}

static
int _pacman_reg_match_or_strstr(const char *string, const char *pattern) {
	if (_pacman_reg_match(string, pattern) > 0 ||
			strstr(string, pattern) != NULL) {
		return 0;
	}
	return 1;
}

pmlist_t *_pacman_db_search(pmdb_t *db, pmlist_t *needles)
{
	pmlist_t *i, *j, *ret = NULL;

	f_foreach (i, needles) {
		/* FIXME: precompile regex once per loop, and handle bad regexp more gracefully */
		const char *targ;

		targ = i->data;
		_pacman_log(PM_LOG_DEBUG, "searching for target '%s'\n", targ);

		for(j = _pacman_db_get_pkgcache(db); j; j = j->next) {
			pmpkg_t *pkg = j->data;

			if (_pacman_reg_match_or_strstr(pkg->name, targ) == 0 ||
					_pacman_reg_match_or_strstr(_pacman_pkg_getinfo(pkg, PM_PKG_DESC), targ) == 0 ||
					f_ptrlist_detect(_pacman_pkg_getinfo(pkg, PM_PKG_PROVIDES), (FDetectFunc)_pacman_reg_match_or_strstr, (void *)targ)) {
				_pacman_log(PM_LOG_DEBUG, "    search target '%s' matched '%s'", targ, pkg->name);
				ret = _pacman_list_add(ret, pkg);
			}
		}
	}

	return(ret);
}

pmdb_t *_pacman_db_register(const char *treename)
{
	struct stat buf;
	pmdb_t *db;
	char path[PATH_MAX];

	if(strcmp(treename, "local") == 0) {
		if(handle->db_local != NULL) {
			_pacman_log(PM_LOG_WARNING, _("attempt to re-register the 'local' DB\n"));
			RET_ERR(PM_ERR_DB_NOT_NULL, NULL);
		}
	} else {
		pmlist_t *i;

		f_foreach (i, handle->dbs_sync) {
			pmdb_t *sdb = i->data;
			if(strcmp(treename, sdb->treename) == 0) {
				_pacman_log(PM_LOG_DEBUG, _("attempt to re-register the '%s' database, using existing\n"), sdb->treename);
				return sdb;
			}
		}
	}

	_pacman_log(PM_LOG_FLOW1, _("registering database '%s'"), treename);

	/* make sure the database directory exists */
	snprintf(path, PATH_MAX, "%s%s/%s", handle->root, handle->dbpath, treename);
	if(!strcmp(treename, "local") && (stat(path, &buf) != 0 || !S_ISDIR(buf.st_mode))) {
		_pacman_log(PM_LOG_FLOW1, _("database directory '%s' does not exist -- try creating it"), path);
		if(_pacman_makepath(path) != 0) {
			RET_ERR(PM_ERR_SYSTEM, NULL);
		}
	}

	db = _pacman_db_new(handle->root, handle->dbpath, treename);
	if(db == NULL) {
		RET_ERR(PM_ERR_DB_CREATE, NULL);
	}

	_pacman_log(PM_LOG_DEBUG, _("opening database '%s'"), db->treename);
	if(_pacman_db_open(db) == -1) {
		_pacman_db_free(db);
		RET_ERR(PM_ERR_DB_OPEN, NULL);
	}

	if(strcmp(treename, "local") == 0) {
		handle->db_local = db;
	} else {
		handle->dbs_sync = _pacman_list_add(handle->dbs_sync, db);
	}

	return(db);
}

/* return a pmlist_t of packages in "db" that provide "package"
 */
pmlist_t *_pacman_db_whatprovides(pmdb_t *db, const char *package)
{
	pmlist_t *pkgs = NULL;
	pmlist_t *lp;

	if(db == NULL || package == NULL || strlen(package) == 0) {
		return(NULL);
	}

	for(lp = _pacman_db_get_pkgcache(db); lp; lp = lp->next) {
		pmpkg_t *info = lp->data;

		if (f_stringlist_find (_pacman_pkg_getinfo(info, PM_PKG_PROVIDES), package)) {
			pkgs = _pacman_list_add(pkgs, info);
		}
	}

	return(pkgs);
}

/* vim: set ts=2 sw=2 noet: */
