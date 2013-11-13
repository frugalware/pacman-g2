/*
 *  syncdb.c
 *
 *  Copyright (c) 2006 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2006-2008 by Miklos Vajna <vmiklos@frugalware.org>
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
#ifdef __sun__
#include <strings.h>
#endif
#include <sys/stat.h>
#include <dirent.h>
#include <libintl.h>
#include <locale.h>
#ifdef CYGWIN
#include <limits.h> /* PATH_MAX */
#endif

/* pacman-g2 */
#include "db/syncdb.h"

#include "db/localdb_files.h"
#include "db/syncdb.h"
#include "io/archive.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "util.h"
#include "cache.h"
#include "db.h"
#include "package.h"
#include "pacman.h"
#include "error.h"
#include "handle.h"
#include "server.h"

static
int suffixcmp(const char *str, const char *suffix)
{
	int len = strlen(str), suflen = strlen(suffix);
	if (len < suflen)
		return -1;
	else
		return strcmp(str + len - suflen, suffix);
}

static
pmpkg_t *_pacman_syncdb_pkg_new(pmdb_t *db, const struct archive_entry *entry, unsigned int inforeq)
{
	pmpkg_t *pkg;
	const char *dname;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));
	ASSERT(entry != NULL, return NULL);

	dname = archive_entry_pathname((struct archive_entry *)entry);
	if((pkg = _pacman_pkg_new_from_filename(dname, 0)) == NULL ||
		_pacman_db_read(db, inforeq, pkg) == -1) {
		_pacman_log(PM_LOG_ERROR, _("invalid name for dabatase entry '%s'"), dname);
		FREEPKG(pkg);
	}
	return pkg;
}

static
pmlist_t *_pacman_syncdb_test(pmdb_t *db)
{
	/* testing sync dbs is not supported */
	return _pacman_list_new();
}

int _pacman_syncdb_update(pmdb_t *db, int force)
{
	char path[PATH_MAX], dirpath[PATH_MAX];
	pmlist_t *files = NULL;
	time_t newmtime = (time_t) -1;
	char lastupdate[16] = "";
	int ret, updated=0;

	if(!force) {
		/* get the lastupdate time */
		_pacman_db_getlastupdate(db, lastupdate);
		if(_pacman_strempty(lastupdate)) {
			_pacman_log(PM_LOG_DEBUG, _("failed to get lastupdate time for %s (no big deal)\n"), db->treename);
		}
	}

	/* build a one-element list */
	snprintf(path, PATH_MAX, "%s" PM_EXT_DB, db->treename);
	files = _pacman_stringlist_append(files, path);

	snprintf(path, PATH_MAX, "%s%s", handle->root, handle->dbpath);

	ret = _pacman_downloadfiles_forreal(db->servers, path, files, lastupdate, &newmtime, 0);
	FREELIST(files);
	if(ret != 0) {
		if(ret == -1) {
			_pacman_log(PM_LOG_DEBUG, _("failed to sync db: %s [%d]\n"),  pacman_strerror(ret), ret);
			pm_errno = PM_ERR_DB_SYNC;
		}
		return 1; /* Means up2date */
	} else {
		if(newmtime != ((time_t) -1)) {
			_pacman_log(PM_LOG_DEBUG, _("sync: new mtime for %s: %s\n"), db->treename, newmtime);
			updated = 1;
		}
		snprintf(dirpath, PATH_MAX, "%s%s/%s", handle->root, handle->dbpath, db->treename);
		snprintf(path, PATH_MAX, "%s%s/%s" PM_EXT_DB, handle->root, handle->dbpath, db->treename);

		/* remove the old dir */
		_pacman_rmrf(dirpath);

		/* Cache needs to be rebuild */
		_pacman_db_free_pkgcache(db);

		if(updated) {
			_pacman_db_settimestamp(db, &newmtime);
		}
	}
	return 0;
}

static
int _pacman_syncdb_open(pmdb_t *db, int flags, time_t *timestamp)
{
	struct stat buf;
	char dbpath[PATH_MAX];

	snprintf(dbpath, PATH_MAX, "%s" PM_EXT_DB, db->path);
	if(stat(dbpath, &buf) != 0) {
		// db is not there, we'll open it later
		db->handle = NULL;
		return 0;
	}
	if((db->handle = _pacman_archive_read_open_all_file(dbpath)) == NULL) {
		RET_ERR(PM_ERR_DB_OPEN, -1);
	}
	_pacman_db_gettimestamp(db, timestamp);
	return 0;
}

static
int _pacman_syncdb_close(pmdb_t *db)
{
	if(db->handle) {
		archive_read_finish(db->handle);
		db->handle = NULL;
	}
	return 0;
}

static
int _pacman_syncdb_rewind(pmdb_t *db)
{
	_pacman_syncdb_close(db);
	return _pacman_syncdb_open(db, 0, NULL);
}

static
pmpkg_t *_pacman_syncdb_readpkg(pmdb_t *db, unsigned int inforeq)
{
	struct archive_entry *entry = NULL;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));

	if(!db->handle) {
		_pacman_db_rewind(db);
	}
	if(!db->handle) {
		return NULL;
	}

	for(;;) {
		if(archive_read_next_header(db->handle, &entry) != ARCHIVE_OK) {
			return NULL;
		}
		// make sure it's a directory
		const char *pathname = archive_entry_pathname(entry);
		if (pathname[strlen(pathname)-1] == '/') {
			return _pacman_syncdb_pkg_new(db, entry, inforeq);
		}
	}
	return NULL;
}

static
pmpkg_t *_pacman_syncdb_scan(pmdb_t *db, const char *target, unsigned int inforeq)
{
	char name[PKG_FULLNAME_LEN];
	char *ptr = NULL;
	struct archive_entry *entry = NULL;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));
	ASSERT(!_pacman_strempty(target), RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	// Search from start
	_pacman_db_rewind(db);

	/* search for a specific package (by name only) */
	while (archive_read_next_header(db->handle, &entry) == ARCHIVE_OK) {
		// make sure it's a directory
		const char *pathname = archive_entry_pathname(entry);
		if (pathname[strlen(pathname)-1] != '/')
			continue;
		STRNCPY(name, pathname, PKG_FULLNAME_LEN);
		// truncate the string at the second-to-last hyphen,
		// which will give us the package name
		if((ptr = rindex(name, '-'))) {
			*ptr = '\0';
		}
		if((ptr = rindex(name, '-'))) {
			*ptr = '\0';
		}
		if(!strcmp(name, target)) {
			return _pacman_syncdb_pkg_new(db, entry, inforeq);
		}
	}
	return(NULL);
}

static
int _pacman_syncdb_file_reader(pmdb_t *db, pmpkg_t *info, unsigned int inforeq, unsigned int inforeq_masq, int (*reader)(pmpkg_t *, FILE *))
{
	int ret = 0;

	if(inforeq & inforeq_masq) {
		FILE *fp = _pacman_archive_read_fropen(db->handle);

		ASSERT(fp != NULL, RET_ERR(PM_ERR_MEMORY, -1));
		ret = reader(info, fp);
		fclose(fp);
	}
	return ret;
}

static
int _pacman_syncdb_read(pmdb_t *db, pmpkg_t *info, unsigned int inforeq)
{
	int descdone = 0, depsdone = 0;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(info == NULL || info->name[0] == 0 || info->version[0] == 0) {
		_pacman_log(PM_LOG_ERROR, _("invalid package entry provided to _pacman_syncdb_read"));
		return(-1);
	}

	while (!descdone || !depsdone) {
		struct archive_entry *entry = NULL;
		if (archive_read_next_header(db->handle, &entry) != ARCHIVE_OK)
			return -1;
		const char *pathname = archive_entry_pathname(entry);
		if (!suffixcmp(pathname, "/desc")) {
			if(_pacman_syncdb_file_reader(db, info, inforeq, INFRQ_DESC, _pacman_localdb_desc_fread) == -1)
				return -1;
			descdone = 1;
		}
		if (!suffixcmp(pathname, "/depends")) {
			if(_pacman_syncdb_file_reader(db, info, inforeq, INFRQ_DEPENDS, _pacman_localdb_depends_fread) == -1)
				return -1;
			depsdone = 1;
		}
	}
	return 0;
}

const pmdb_ops_t _pacman_syncdb_ops = {
	.test = _pacman_syncdb_test,
	.open = _pacman_syncdb_open,
	.close = _pacman_syncdb_close,
	.rewind = _pacman_syncdb_rewind,
	.readpkg = _pacman_syncdb_readpkg,
	.scan = _pacman_syncdb_scan,
	.read = _pacman_syncdb_read,
	.write = NULL,
	.remove = NULL,
};

/* vim: set ts=2 sw=2 noet: */
