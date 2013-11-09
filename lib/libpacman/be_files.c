/*
 *  be_files.c
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

#include "db/localdb.h"
#include "db/localdb_files.h"
#include "db/syncdb.h"
#include "io/archive.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "util.h"
#include "db.h"
#include "package.h"
#include "pacman.h"
#include "error.h"
#include "handle.h"

static inline int islocal(pmdb_t *db)
{
	if (handle->db_local)
		return db == handle->db_local;
	else
		return strcmp(db->treename, "local") == 0;
}

pmpkg_t *_pacman_db_scan(pmdb_t *db, const char *target, unsigned int inforeq)
{
	struct dirent *ent = NULL;
	struct stat sbuf;
	char path[PATH_MAX];
	char name[PKG_FULLNAME_LEN];
	char *ptr = NULL;
	int found = 0;
	pmpkg_t *pkg;

	char dbpath[PATH_MAX];
	snprintf(dbpath, PATH_MAX, "%s" PM_EXT_DB, db->path);
	struct archive_entry *entry = NULL;

	if(db == NULL) {
		RET_ERR(PM_ERR_DB_NULL, NULL);
	}

	if(target != NULL) {
		// Search from start
		_pacman_db_rewind(db);

		/* search for a specific package (by name only) */
		if (islocal(db)) {
			while(!found && (ent = readdir(db->handle)) != NULL) {
				if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
					continue;
				}
				/* stat the entry, make sure it's a directory */
				snprintf(path, PATH_MAX, "%s/%s", db->path, ent->d_name);
				if(stat(path, &sbuf) || !S_ISDIR(sbuf.st_mode)) {
					continue;
				}
				STRNCPY(name, ent->d_name, PKG_FULLNAME_LEN);
				/* truncate the string at the second-to-last hyphen, */
				/* which will give us the package name */
				if((ptr = rindex(name, '-'))) {
					*ptr = '\0';
				}
				if((ptr = rindex(name, '-'))) {
					*ptr = '\0';
				}
				if(!strcmp(name, target)) {
					found = 1;
				}
			}
		} else {
			while (!found && archive_read_next_header(db->handle, &entry) == ARCHIVE_OK) {
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
					found = 1;
				}
			}
		}
		if(!found) {
			return(NULL);
		}
	} else {
		/* normal iteration */
		int isdir = 0;
		while(!isdir) {
			if (islocal(db)) {
				ent = readdir(db->handle);
				if(ent == NULL) {
					return(NULL);
				}
				if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
					isdir = 0;
					continue;
				}
				/* stat the entry, make sure it's a directory */
				snprintf(path, PATH_MAX, "%s/%s", db->path, ent->d_name);
				if(!stat(path, &sbuf) && S_ISDIR(sbuf.st_mode)) {
					isdir = 1;
				}
			} else {
				if (!db->handle)
					_pacman_db_rewind(db);
				if (!db->handle || archive_read_next_header(db->handle, &entry) != ARCHIVE_OK) {
					return NULL;
				}
				// make sure it's a directory
				const char *pathname = archive_entry_pathname(entry);
				if (pathname[strlen(pathname)-1] == '/')
					isdir = 1;
			}
		}
	}

	pkg = _pacman_pkg_new(NULL, NULL);
	if(pkg == NULL) {
		return(NULL);
	}
	char *dname;
	if (islocal(db)) {
		dname = strdup(ent->d_name);
	} else {
		dname = strdup(archive_entry_pathname(entry));
		dname[strlen(dname)-1] = '\0'; // drop trailing slash
	}
	if(_pacman_pkg_splitname(dname, pkg->name, pkg->version, 0) == -1) {
		_pacman_log(PM_LOG_ERROR, _("invalid name for dabatase entry '%s'"), dname);
		FREE(dname);
		return(NULL);
	}
	FREE(dname);
	if(_pacman_db_read(db, inforeq, pkg) == -1) {
		FREEPKG(pkg);
	}

	return(pkg);
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

/* reads dbpath/.lastupdate and populates *ts with the contents.
 * *ts should be malloc'ed and should be at least 15 bytes.
 *
 * Returns 0 on success, 1 on error
 *
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

/* writes the dbpath/.lastupdate with the contents of *ts
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

/* vim: set ts=2 sw=2 noet: */
