/*
 *  fdb.c
 *
 *  Copyright (c) 2006 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2006-2008 by Miklos Vajna <vmiklos@frugalware.org>
 *  Copyright (c) 2013 by Michel Hermier <hermier@frugalware.org>
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

#include <assert.h>

#include <fstringlist.h>

#include "fdb.h"

#include "util.h"

int _pacman_fdb_open (pmdb_t *db) {
	char dbpath[PATH_MAX];

	assert (db != NULL);

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
		_pacman_fdb_close (db);
		RET_ERR(PM_ERR_DB_OPEN, -1);
	}
	return 0;
}

void _pacman_fdb_close (pmdb_t *db) {
	assert (db != NULL);

	if(db->handle) {
		archive_read_finish(db->handle);
		db->handle = NULL;
	}
}

void _pacman_fdb_rewind(pmdb_t *db) {
	assert (db != NULL);

	_pacman_fdb_close (db);
	_pacman_fdb_open (db);
}

/* vim: set ts=2 sw=2 noet: */
