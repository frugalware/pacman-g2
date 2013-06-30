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

#include <sys/stat.h>
#include <assert.h>
#include <dirent.h>
#include <unistd.h>

#include <fstringlist.h>

#include "localdb.h"

#include "error.h"
#include "util.h"

int _pacman_localdb_open (pmdb_t *db) {
	assert (db != NULL);

	db->handle = opendir(db->path);
	if(db->handle == NULL) {
		RET_ERR(PM_ERR_DB_OPEN, -1);
	}
}

void _pacman_localdb_close (pmdb_t *db) {
	assert (db != NULL);

	if(db->handle) {
		closedir(db->handle);
		db->handle = NULL;
	}
}

void _pacman_localdb_rewind(pmdb_t *db)
{
	assert (db != NULL);

	if (db->handle != NULL) {
		rewinddir(db->handle);
	}
}

pmlist_t *_pacman_localdb_test(pmdb_t *db) {
	struct dirent *ent;
	char path[PATH_MAX];
	struct stat buf;
	pmlist_t *ret = f_ptrlist_new ();

	assert (db != NULL);

	while ((ent = readdir(db->handle)) != NULL) {
		snprintf(path, PATH_MAX, "%s/%s", db->path, ent->d_name);
		stat(path, &buf);
		if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..") || !S_ISDIR(buf.st_mode)) {
			continue;
		}
		snprintf(path, PATH_MAX, "%s/%s/desc", db->path, ent->d_name);
		if(stat(path, &buf))
		{
			snprintf (path, sizeof (*path), _("%s: description file is missing"), ent->d_name);
			ret = f_stringlist_append (ret, path);
		}
		snprintf(path, PATH_MAX, "%s/%s/depends", db->path, ent->d_name);
		if(stat(path, &buf))
		{
			snprintf (path, sizeof (*path), _("%s: dependency information is missing"), ent->d_name);
			ret = f_stringlist_append (ret, path);
		}
		snprintf(path, PATH_MAX, "%s/%s/files", db->path, ent->d_name);
		if(stat(path, &buf))
		{
			snprintf (path, sizeof (*path), _("%s: file list is missing"), ent->d_name);
			ret = f_stringlist_append (ret, path);
		}
	}
	return ret;
}

/* vim: set ts=2 sw=2 noet: */
