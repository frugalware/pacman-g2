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

static
void _pacman_localdb_write_item_list (FILE *fp, const char *item_name, pmlist_t *item_list) {
	if (item_list != NULL) {
		pmlist_t *i = item_list;

		fprintf(fp, "%%%s%%\n", item_name);
		f_foreach (i, item_list) {
			fprintf(fp, "%s\n", (char *)i->data);
		}
		fprintf(fp, "\n");
	}	
}

int _pacman_localdb_write (pmdb_t *db, pmpkg_t *info, unsigned int inforeq) {
	FILE *fp = NULL;
	char path[PATH_MAX];
	mode_t oldmask;
	int retval = 0;
	int local = 1; /* FIXME: REMOVE */

	if(db == NULL || info == NULL) {
		return(-1);
	}

	snprintf(path, PATH_MAX, "%s/%s-%s", db->path, info->name, info->version);
	oldmask = umask(0000);
	mkdir(path, 0755);
	/* make sure we have a sane umask */
	umask(0022);

	/* DESC */
	if(inforeq & INFRQ_DESC) {
		snprintf(path, PATH_MAX, "%s/%s-%s/desc", db->path, info->name, info->version);
		if((fp = fopen(path, "w")) == NULL) {
			_pacman_log(PM_LOG_ERROR, _("db_write: could not open file %s/desc"), db->treename);
			retval = 1;
			goto cleanup;
		}
		fprintf(fp, "%%NAME%%\n%s\n\n"
			"%%VERSION%%\n%s\n\n", info->name, info->version);
		if(info->desc[0]) {
			_pacman_localdb_write_item_list(fp, "DESC", info->desc_localized);
		}
		_pacman_localdb_write_item_list(fp, "GROUPS", info->groups);
		if(local) {
			if(info->url[0]) {
				fprintf(fp, "%%URL%%\n"
					"%s\n\n", info->url);
			}
			_pacman_localdb_write_item_list(fp, "LICENSE", info->license);
			if(info->arch[0]) {
				fprintf(fp, "%%ARCH%%\n"
					"%s\n\n", info->arch);
			}
			if(info->builddate[0]) {
				fprintf(fp, "%%BUILDDATE%%\n"
					"%s\n\n", info->builddate);
			}
			if(info->buildtype[0]) {
				fprintf(fp, "%%BUILDTYPE%%\n"
					"%s\n\n", info->buildtype);
			}
			if(info->installdate[0]) {
				fprintf(fp, "%%INSTALLDATE%%\n"
					"%s\n\n", info->installdate);
			}
			if(info->packager[0]) {
				fprintf(fp, "%%PACKAGER%%\n"
					"%s\n\n", info->packager);
			}
			if(info->size) {
				fprintf(fp, "%%SIZE%%\n"
					"%ld\n\n", info->size);
			}
			if(info->reason) {
				fprintf(fp, "%%REASON%%\n"
					"%d\n\n", info->reason);
			}
		} else {
			if(info->size) {
				fprintf(fp, "%%CSIZE%%\n"
					"%ld\n\n", info->size);
			}
			if(info->usize) {
				fprintf(fp, "%%USIZE%%\n"
					"%ld\n\n", info->usize);
			}
			if(info->sha1sum) {
				fprintf(fp, "%%SHA1SUM%%\n"
					"%s\n\n", info->sha1sum);
			} else if(info->md5sum) {
				fprintf(fp, "%%MD5SUM%%\n"
					"%s\n\n", info->md5sum);
			}
		}
		fclose(fp);
		fp = NULL;
	}

	/* FILES */
	if(local && (inforeq & INFRQ_FILES)) {
		snprintf(path, PATH_MAX, "%s/%s-%s/files", db->path, info->name, info->version);
		if((fp = fopen(path, "w")) == NULL) {
			_pacman_log(PM_LOG_ERROR, _("db_write: could not open file %s/files"), db->treename);
			retval = -1;
			goto cleanup;
		}
		_pacman_localdb_write_item_list(fp, "FILES", info->files);
		_pacman_localdb_write_item_list(fp, "BACKUP", info->backup);
		fclose(fp);
		fp = NULL;
	}

	/* DEPENDS */
	if(inforeq & INFRQ_DEPENDS) {
		snprintf(path, PATH_MAX, "%s/%s-%s/depends", db->path, info->name, info->version);
		if((fp = fopen(path, "w")) == NULL) {
			_pacman_log(PM_LOG_ERROR, _("db_write: could not open file %s/depends"), db->treename);
			retval = -1;
			goto cleanup;
		}
		_pacman_localdb_write_item_list(fp, "DEPENDS", info->depends);
		if (local) {
			_pacman_localdb_write_item_list(fp, "REQUIREDBY", info->requiredby);
		}
		_pacman_localdb_write_item_list(fp, "CONFLICTS", info->conflicts);
		_pacman_localdb_write_item_list(fp, "PROVIDES", info->provides);
		if(!local) {
			_pacman_localdb_write_item_list(fp, "REPLACES", info->replaces);
			if(info->force) {
				fprintf(fp, "%%FORCE%%\n"
					"\n");
			}
			if(info->stick) {
				fprintf(fp, "%%STICK%%\n"
					"\n");
			}
		}
		fclose(fp);
		fp = NULL;
	}

	/* INSTALL */
	/* nothing needed here (script is automatically extracted) */

cleanup:
	umask(oldmask);

	if(fp) {
		fclose(fp);
	}

	return(retval);
}

/* vim: set ts=2 sw=2 noet: */
