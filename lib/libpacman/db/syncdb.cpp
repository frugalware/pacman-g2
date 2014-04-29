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

#include <sys/stat.h>
#include <dirent.h>
#include <limits.h> /* PATH_MAX */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* pacman-g2 */
#include "db/syncdb.h"

#include "db/localdb_files.h"
#include "db/syncdb.h"
#include "io/archive.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "util/time.h"
#include "fstring.h"
#include "util.h"
#include "cache.h"
#include "db.h"
#include "package.h"
#include "pacman.h"
#include "error.h"
#include "handle.h"
#include "server.h"

using namespace libpacman;

static
int suffixcmp(const char *str, const char *suffix)
{
	int len = strlen(str), suflen = strlen(suffix);
	if (len < suflen)
		return -1;
	else
		return strcmp(str + len - suflen, suffix);
}

SyncPackage::~SyncPackage()
{
}

static
int _pacman_syncpkg_file_reader(Database *db, Package *pkg, unsigned int flags, unsigned int flags_masq, int (*reader)(Package *, FILE *))
{
	int ret = 0;

	if(flags & flags_masq) {
		FILE *fp = _pacman_archive_read_fropen(db->handle);

		ASSERT(fp != NULL, RET_ERR(PM_ERR_MEMORY, -1));
		ret = reader(pkg, fp);
		fclose(fp);
	}
	return ret;
}

int SyncPackage::read(unsigned int flags)
{
	int descdone = 0, depsdone = 0;

	ASSERT(database != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(name[0] == 0 || version[0] == 0) {
		_pacman_log(PM_LOG_ERROR, _("invalid package entry provided to _pacman_syncdb_read"));
		return(-1);
	}

	while (!descdone || !depsdone) {
		struct archive_entry *entry = NULL;
		if (archive_read_next_header(database->handle, &entry) != ARCHIVE_OK)
			return -1;
		const char *pathname = archive_entry_pathname(entry);
		if (!suffixcmp(pathname, "/desc")) {
			if(_pacman_syncpkg_file_reader(database, this, flags, PM_LOCALPACKAGE_FLAGS_DESC, _pacman_localdb_desc_fread) == -1)
				return -1;
			descdone = 1;
		}
		if (!suffixcmp(pathname, "/depends")) {
			if(_pacman_syncpkg_file_reader(database, this, flags, PM_LOCALPACKAGE_FLAGS_DEPENDS, _pacman_localdb_depends_fread) == -1)
				return -1;
			depsdone = 1;
		}
	}
	return 0;
}

SyncDatabase::SyncDatabase(libpacman::Handle *handle, const char *treename)
	  : Database(handle, treename, &_pacman_syncdb_ops)
{
}

SyncDatabase::~SyncDatabase()
{
}

static
Package *_pacman_syncdb_pkg_new(Database *db, const struct archive_entry *entry, unsigned int inforeq)
{
	Package *pkg;
	const char *dname;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));
	ASSERT(entry != NULL, return NULL);

	dname = archive_entry_pathname((struct archive_entry *)entry);
	if((pkg = _pacman_pkg_new_from_filename(dname, 0, db)) == NULL ||
		db->read(pkg, inforeq) == -1) {
		_pacman_log(PM_LOG_ERROR, _("invalid name for dabatase entry '%s'"), dname);
		FREEPKG(pkg);
	}
	return pkg;
}

int _pacman_syncdb_update(Database *db, int force)
{
	char path[PATH_MAX], dirpath[PATH_MAX];
	pmlist_t *files = NULL;
	time_t newmtime = PM_TIME_INVALID;
	time_t timestamp = PM_TIME_INVALID;
	int ret, updated=0;

	if(!force) {
		db->gettimestamp(&timestamp);
		if(timestamp == PM_TIME_INVALID) {
			_pacman_log(PM_LOG_DEBUG, _("failed to get timestamp for %s (no big deal)\n"), db->treename);
		}
	}

	/* build a one-element list */
	snprintf(path, PATH_MAX, "%s" PM_EXT_DB, db->treename);
	files = _pacman_stringlist_append(files, path);

	snprintf(path, PATH_MAX, "%s%s", handle->root, handle->dbpath);

	ret = _pacman_downloadfiles_forreal(db->servers, path, files, &timestamp, &newmtime, 0);
	FREELIST(files);
	if(ret != 0) {
		if(ret == -1) {
			_pacman_log(PM_LOG_DEBUG, _("failed to sync db: %s [%d]\n"),  pacman_strerror(ret), ret);
			pm_errno = PM_ERR_DB_SYNC;
		}
		return 1; /* Means up2date */
	} else {
		if(newmtime != PM_TIME_INVALID) {
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
			db->settimestamp(&newmtime);
		}
	}
	return 0;
}

int SyncDatabase::open(int flags, time_t *timestamp)
{
	struct stat buf;
	char dbpath[PATH_MAX];

	snprintf(dbpath, PATH_MAX, "%s" PM_EXT_DB, path);
	if(stat(dbpath, &buf) != 0) {
		// db is not there, we'll open it later
		handle = NULL;
		return 0;
	}
	if((handle = _pacman_archive_read_open_all_file(dbpath)) == NULL) {
		//return 0 here to be able to update the damaged db with pacman -Syy
		RET_ERR(PM_ERR_DB_OPEN, 0);
	}
	if(timestamp != NULL) {
		gettimestamp(timestamp);
	}
	return 0;
}

int SyncDatabase::close()
{
	if(handle) {
		archive_read_finish(handle);
		handle = NULL;
	}
	return 0;
}

int SyncDatabase::rewind()
{
	close();
	return open(0, NULL);
}

Package *SyncDatabase::readpkg(unsigned int inforeq)
{
	struct archive_entry *entry = NULL;

	if(!handle) {
		rewind();
	}
	if(!handle) {
		return NULL;
	}

	for(;;) {
		if(archive_read_next_header(handle, &entry) != ARCHIVE_OK) {
			return NULL;
		}
		// make sure it's a directory
		const char *pathname = archive_entry_pathname(entry);
		if (pathname[strlen(pathname)-1] == '/') {
			return _pacman_syncdb_pkg_new(this, entry, inforeq);
		}
	}
	return NULL;
}

Package *SyncDatabase::scan(const char *target, unsigned int inforeq)
{
	char name[PKG_FULLNAME_LEN];
	char *ptr = NULL;
	struct archive_entry *entry = NULL;

	ASSERT(!_pacman_strempty(target), RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	// Search from start
	rewind();

	/* search for a specific package (by name only) */
	while (archive_read_next_header(handle, &entry) == ARCHIVE_OK) {
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
			return _pacman_syncdb_pkg_new(this, entry, inforeq);
		}
	}
	return(NULL);
}

static
int _pacman_syncdb_file_reader(Database *db, Package *info, unsigned int inforeq, unsigned int inforeq_masq, int (*reader)(Package *, FILE *))
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
int _pacman_syncdb_read(Database *db, Package *info, unsigned int inforeq)
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
	.read = _pacman_syncdb_read,
};

/* vim: set ts=2 sw=2 noet: */
