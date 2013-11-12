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

#if defined(__APPLE__) || defined(__OpenBSD__)
#include <sys/syslimits.h>
#include <sys/stat.h>
#endif

#include "config.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <libintl.h>
#ifdef CYGWIN
#include <limits.h> /* PATH_MAX */
#endif

/* pacman-g2 */
#include "db.h"

#include "db/localdb.h"
#include "db/syncdb.h"
#include "util/list.h"
#include "util/log.h"
#include "util.h"
#include "error.h"
#include "server.h"
#include "handle.h"
#include "cache.h"
#include "pacman.h"

pmdb_t *_pacman_db_new(const char *root, const char* dbpath, const char *treename)
{
	pmdb_t *db = _pacman_zalloc(sizeof(pmdb_t));

	if(db == NULL) {
		return(NULL);
	}

	db->path = _pacman_malloc(strlen(root)+strlen(dbpath)+strlen(treename)+2);
	if(db->path == NULL) {
		FREE(db);
		return(NULL);
	}
	sprintf(db->path, "%s%s/%s", root, dbpath, treename);

	STRNCPY(db->treename, treename, PATH_MAX);

	return(db);
}

void _pacman_db_free(void *data)
{
	pmdb_t *db = data;

	FREELISTSERVERS(db->servers);
	free(db->path);
	free(db);

	return;
}

int _pacman_db_cmp(const void *db1, const void *db2)
{
	return(strcmp(((pmdb_t *)db1)->treename, ((pmdb_t *)db2)->treename));
}

static
int _pacman_reg_match_or_substring(const char *string, const char *pattern)
{
	int retval = _pacman_reg_match(string, pattern);

	if(retval < 0) {
		/* bad regexp */
		return -1;
	} else if(retval) {
		return 1;
	} else if (strstr(string, pattern) != NULL) {
		return 1;
	}
	return 0;
}

pmlist_t *_pacman_db_search(pmdb_t *db, pmlist_t *needles)
{
	pmlist_t *i, *j, *k, *ret = NULL;

	for(i = needles; i; i = i->next) {
		const char *targ;

		if(i->data == NULL) {
			continue;
		}
		targ = i->data;
		_pacman_log(PM_LOG_DEBUG, "searching for target '%s'\n", targ);

		for(j = _pacman_db_get_pkgcache(db); j; j = j->next) {
			pmpkg_t *pkg = j->data;
			int match = 0;

			/* check name */
			if(_pacman_reg_match_or_substring(pkg->name, targ) == 1) {
				_pacman_log(PM_LOG_DEBUG, "    search target '%s' matched '%s'", targ, pkg->name);
				match = 1;
			}

			/* check description */
			if(!match) {
				if(_pacman_reg_match_or_substring(_pacman_pkg_getinfo(pkg, PM_PKG_DESC), targ) == 1) {
					match = 1;
				}
			}

			/* check provides */
			if(!match) {
				for(k = _pacman_pkg_getinfo(pkg, PM_PKG_PROVIDES); k; k = k->next) {
					if(_pacman_reg_match_or_substring(k->data, targ) == 1) {
						match = 1;
					}
				}
			}

			if(match) {
				ret = _pacman_list_add(ret, pkg);
			}
		}
	}

	return(ret);
}

pmlist_t *_pacman_db_test(pmdb_t *db)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));

	return db->ops->test(db);
}

int _pacman_db_open(pmdb_t *db)
{
	int ret = 0;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	ret = db->ops->open(db);
	if(ret == 0 && _pacman_db_getlastupdate(db, db->lastupdate) == -1) {
		db->lastupdate[0] = '\0';
	}

	return ret;
}

int _pacman_db_close(pmdb_t *db)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	return db->ops->close(db);
}

int _pacman_db_gettimestamp(pmdb_t *db, struct tm *timestamp)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	if(db->ops->gettimestamp) {
		return db->ops->gettimestamp(db, timestamp);
	} else {
		int ret;
		char buffer[16];

		if((ret = _pacman_db_getlastupdate(db, buffer)) == 0) {
			strptime(buffer, "%Y%m%d%H%M%S", timestamp);
		}
		return ret;
	}
}

int _pacman_db_settimestamp(pmdb_t *db, const struct tm *timestamp)
{
	char buffer[16];

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", timestamp);
	return _pacman_db_setlastupdate(db, buffer);
}

int _pacman_db_rewind(pmdb_t *db)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	return db->ops->rewind(db);
}

pmpkg_t *_pacman_db_readpkg(pmdb_t *db, unsigned int inforeq)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));

	return db->ops->readpkg(db, inforeq);
}

pmpkg_t *_pacman_db_scan(pmdb_t *db, const char *target, unsigned int inforeq)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));

	return db->ops->scan(db, target, inforeq);
}

int _pacman_db_read(pmdb_t *db, unsigned int inforeq, pmpkg_t *info)
{
	int ret;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(info == NULL || info->name[0] == 0 || info->version[0] == 0) {
		_pacman_log(PM_LOG_ERROR, _("invalid package entry provided to _pacman_db_read"));
		return(-1);
	}

	if((ret = db->ops->read(db, info, inforeq)) == 0) {
		info->infolevel |= inforeq;
	}
	return ret;
}

int _pacman_db_write(pmdb_t *db, pmpkg_t *info, unsigned int inforeq)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(info != NULL, RET_ERR(PM_ERR_PKG_INVALID, -1));

	if(db->ops->write != NULL) {
		return db->ops->write(db, info, inforeq);
	} else {
		RET_ERR(PM_ERR_WRONG_ARGS, -1); // Not supported
	}
}

int _pacman_db_remove(pmdb_t *db, pmpkg_t *info)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(info != NULL, RET_ERR(PM_ERR_PKG_INVALID, -1));

	if(db->ops->remove != NULL) {
		return db->ops->remove(db, info);
	} else {
		RET_ERR(PM_ERR_WRONG_ARGS, -1); // Not supported
	}
}

/* Reads dbpath/treename.lastupdate and populates *ts with the contents.
 * *ts should be malloc'ed and should be at least 15 bytes.
 *
 * Returns 0 on success, -1 on error.
 */
int _pacman_db_getlastupdate(pmdb_t *db, char *ts)
{
	FILE *fp;
	char file[PATH_MAX];

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(ts == NULL) {
		return(-1);
	}

	snprintf(file, PATH_MAX, "%s%s/%s.lastupdate", handle->root, handle->dbpath, db->treename);

	/* get the last update time, if it's there */
	if((fp = fopen(file, "r")) == NULL) {
		return(-1);
	} else {
		char line[256];
		if(fgets(line, sizeof(line), fp)) {
			STRNCPY(ts, line, 15); /* YYYYMMDDHHMMSS */
			ts[14] = '\0';
		} else {
			fclose(fp);
			return(-1);
		}
	}
	fclose(fp);
	return(0);
}

/* Writes the dbpath/treename.lastupdate with the contents of *ts
 *
 * Returns 0 on success, -1 on error.
 */
int _pacman_db_setlastupdate(pmdb_t *db, const char *ts)
{
	FILE *fp;
	char file[PATH_MAX];

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(_pacman_strempty(ts)) {
		return(-1);
	}

	snprintf(file, PATH_MAX, "%s%s/%s.lastupdate", handle->root, handle->dbpath, db->treename);

	if((fp = fopen(file, "w")) == NULL) {
		return(-1);
	}
	if(fputs(ts, fp) <= 0) {
		fclose(fp);
		return(-1);
	}
	fclose(fp);

	return(0);
}

pmdb_t *_pacman_db_register(const char *treename, pacman_cb_db_register callback)
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
		for(i = handle->dbs_sync; i; i = i->next) {
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
	if(strcmp(treename, "local") == 0) {
		db->ops = &_pacman_localdb_ops;
	} else {
		db->ops = &_pacman_syncdb_ops;
	}

	_pacman_log(PM_LOG_DEBUG, _("opening database '%s'"), db->treename);
	if(_pacman_db_open(db) == -1) {
		_pacman_db_free(db);
		RET_ERR(PM_ERR_DB_OPEN, NULL);
	}

	/* Only call callback on NEW registration. */
	if(callback) callback(treename, db);

	if(strcmp(treename, "local") == 0) {
		handle->db_local = db;
	} else {
		handle->dbs_sync = _pacman_list_add(handle->dbs_sync, db);
	}

	return(db);
}
/* vim: set ts=2 sw=2 noet: */
