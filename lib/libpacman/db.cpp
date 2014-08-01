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

/* pacman-g2 */
#include "db.h"

#include "util.h"
#include "error.h"
#include "server.h"
#include "handle.h"
#include "cache.h"
#include "pacman_p.h"

#include "db/localdb.h"
#include "db/syncdb.h"
#include "io/ftp.h"
#include "util/list.h"
#include "util/log.h"
#include "util/time.h"
#include "fstdlib.h"
#include "fstring.h"

#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h> /* PATH_MAX */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

using namespace libpacman;

static int _pacman_db_getlastupdate(Database *db, char *ts);
static int _pacman_db_setlastupdate(Database *db, const char *ts);

static
FILE *_pacman_db_fopen_lastupdate(const Database *db, const char *mode)
{
	char path[PATH_MAX];

	snprintf(path, sizeof(path), "%s%s/%s.lastupdate", db->m_handle->root, db->m_handle->dbpath, db->treename);
	return fopen(path, mode);
}

Database::Database(Handle *handle, const char *treename)
	: m_handle(handle)
{
	path = f_zalloc(strlen(handle->root)+strlen(handle->dbpath)+strlen(treename)+2);
//	if(path == NULL) {
//		return(NULL);
//	}
	sprintf(path, "%s%s/%s", handle->root, handle->dbpath, treename);

	STRNCPY(this->treename, treename, PATH_MAX);
}

Database::~Database()
{
	FREELISTSERVERS(servers);
	free(path);
}

int _pacman_db_cmp(const void *db1, const void *db2)
{
	return(strcmp(((Database *)db1)->treename, ((Database *)db2)->treename));
}

pmlist_t *Database::search(pmlist_t *needles)
{
	pmlist_t *i, *j, *ret = NULL;

	for(i = needles; i; i = i->next) {
		FStrMatcher strmatcher = { NULL };
		FMatcher packagestrmatcher = { NULL };
		const char *targ = i->data;

		if(f_strempty(targ)) {
			continue;
		}
		_pacman_log(PM_LOG_DEBUG, "searching for target '%s'\n", targ);
		if(f_strmatcher_init(&strmatcher, targ, F_STRMATCHER_ALL_IGNORE_CASE) == 0 &&
				_pacman_packagestrmatcher_init(&packagestrmatcher, &strmatcher, PM_PACKAGE_FLAG_NAME | PM_PACKAGE_FLAG_DESCRIPTION | PM_PACKAGE_FLAG_PROVIDES) == 0) {
			for(j = _pacman_db_get_pkgcache(this); j; j = j->next) {
				Package *pkg = j->data;

				if(f_match(pkg, &packagestrmatcher)) {
					ret = f_ptrlist_append(ret, pkg);
				}
			}
		}
		_pacman_packagestrmatcher_fini(&packagestrmatcher);
		f_strmatcher_fini(&strmatcher);
	}

	return(ret);
}

pmlist_t *Database::test() const
{
	/* testing sync dbs is not supported */
	return _pacman_list_new();
}

int Database::open(int flags)
{
	ASSERT(flags == 0, RET_ERR(PM_ERR_DB_OPEN, -1)); /* No flags are supported for now */

	return open(flags, &cache_timestamp);
}

int Database::open(int flags, Timestamp *timestamp)
{
	return -1;
}

int Database::close()
{
	return -1;
}

int Database::gettimestamp(Timestamp *timestamp)
{
	ASSERT(timestamp != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	char buffer[PM_FMT_MDTM_MAX];

	if(_pacman_db_getlastupdate(this, buffer) == 0 &&
		_pacman_ftp_strpmdtm(buffer, timestamp != NULL ? &timestamp->m_value : NULL) != NULL) {
		return 0;
	}
	return -1;
}

/* A NULL timestamp means now per f_localtime definition.
 */
int Database::settimestamp(const Timestamp *timestamp)
{
	char buffer[PM_FMT_MDTM_MAX];

	_pacman_ftp_strfmdtm(buffer, sizeof(buffer), timestamp != NULL ? &timestamp->m_value : NULL);
	return _pacman_db_setlastupdate(this, buffer);
}

int Database::rewind()
{
	return -1;
}

Package *Database::readpkg(unsigned int inforeq)
{
	return NULL;
}

int Database::write(Package *info, unsigned int inforeq)
{
	ASSERT(info != NULL, RET_ERR(PM_ERR_PKG_INVALID, -1));
	RET_ERR(PM_ERR_WRONG_ARGS, -1); // Not supported
}

pmlist_t *Database::getowners(const char *filename)
{
	return NULL;
}

/* Reads dbpath/treename.lastupdate and populates *ts with the contents.
 * *ts should be malloc'ed and should be at least 15 bytes.
 *
 * Returns 0 on success, -1 on error.
 */
int _pacman_db_getlastupdate(Database *db, char *ts)
{
	FILE *fp;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(ts == NULL) {
		return(-1);
	}

	/* get the last update time, if it's there */
	if((fp = _pacman_db_fopen_lastupdate(db, "r")) == NULL) {
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
int _pacman_db_setlastupdate(Database *db, const char *ts)
{
	FILE *fp;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(_pacman_strempty(ts)) {
		return(-1);
	}

	if((fp = _pacman_db_fopen_lastupdate(db, "w")) == NULL) {
		return(-1);
	}
	if(fputs(ts, fp) <= 0) {
		fclose(fp);
		return(-1);
	}
	fclose(fp);
	return(0);
}

/* vim: set ts=2 sw=2 noet: */
