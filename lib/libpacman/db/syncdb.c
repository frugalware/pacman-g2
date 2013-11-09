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
#include "io/archive.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "util.h"
#include "db.h"
#include "package.h"
#include "pacman.h"
#include "error.h"
#include "handle.h"

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
pmlist_t *_pacman_syncdb_test(pmdb_t *db)
{
	/* testing sync dbs is not supported */
	return _pacman_list_new();
}

static
int _pacman_syncdb_open(pmdb_t *db)
{
	char dbpath[PATH_MAX];
	snprintf(dbpath, PATH_MAX, "%s" PM_EXT_DB, db->path);
	struct stat buf;
	if(stat(dbpath, &buf) != 0) {
		// db is not there, we'll open it later
		db->handle = NULL;
		return 0;
	}
	if((db->handle = archive_read_new()) == NULL) {
		RET_ERR(PM_ERR_DB_OPEN, -1);
	}
	archive_read_support_compression_all(db->handle);
	archive_read_support_format_all(db->handle);
	if(archive_read_open_filename(db->handle, dbpath, PM_DEFAULT_BYTES_PER_BLOCK) != ARCHIVE_OK) {
		archive_read_finish(db->handle);
		RET_ERR(PM_ERR_DB_OPEN, -1);
	}
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
	char dbpath[PATH_MAX];

	snprintf(dbpath, PATH_MAX, "%s" PM_EXT_DB, db->path);
	if (db->handle)
		archive_read_finish(db->handle);
	db->handle = archive_read_new();
	archive_read_support_compression_all(db->handle);
	archive_read_support_format_all(db->handle);
	if (archive_read_open_filename(db->handle, dbpath, PM_DEFAULT_BYTES_PER_BLOCK) != ARCHIVE_OK) {
		archive_read_finish(db->handle);
		db->handle = NULL;
	}
	return 0;
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
	.read = _pacman_syncdb_read,
	.write = NULL,
	.remove = NULL,
};

/* vim: set ts=2 sw=2 noet: */
