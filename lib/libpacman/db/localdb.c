/*
 *  localdb.c
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

#include "util/log.h"
#include "util/stringlist.h"
#include "util.h"
#include "db.h"
#include "package.h"
#include "pacman.h"
#include "error.h"
#include "handle.h"

static
pmlist_t *_pacman_localdb_test(pmdb_t *db)
{
	struct dirent *ent;
	char path[PATH_MAX];
	struct stat buf;
	pmlist_t *ret = _pacman_list_new();

	while ((ent = readdir(db->handle)) != NULL) {
		snprintf(path, PATH_MAX, "%s/%s", db->path, ent->d_name);
		stat(path, &buf);
		if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..") || !S_ISDIR(buf.st_mode)) {
			continue;
		}
		snprintf(path, PATH_MAX, "%s/%s/desc", db->path, ent->d_name);
		if(stat(path, &buf))
		{
			snprintf(path, LOG_STR_LEN, _("%s: description file is missing"), ent->d_name);
			ret = _pacman_stringlist_append(ret, path);
		}
		snprintf(path, PATH_MAX, "%s/%s/depends", db->path, ent->d_name);
		if(stat(path, &buf))
		{
			snprintf(path, LOG_STR_LEN, _("%s: dependency information is missing"), ent->d_name);
			ret = _pacman_stringlist_append(ret, path);
		}
		snprintf(path, PATH_MAX, "%s/%s/files", db->path, ent->d_name);
		if(stat(path, &buf))
		{
			snprintf(path, LOG_STR_LEN, _("%s: file list is missing"), ent->d_name);
			ret = _pacman_stringlist_append(ret, path);
		}
	}

	return(ret);
}

static
int _pacman_localdb_open(pmdb_t *db)
{
	db->handle = opendir(db->path);
	ASSERT(db->handle != NULL, RET_ERR(PM_ERR_DB_OPEN, -1));

	return 0;
}

static
int _pacman_localdb_close(pmdb_t *db)
{
	if(db->handle) {
		closedir(db->handle);
		db->handle = NULL;
	}
	return 0;
}

static
int _pacman_localdb_rewind(pmdb_t *db)
{
	if(db->handle == NULL) {
		return -1;
	}

	rewinddir(db->handle);
	return 0;
}

static
void _pacman_localdb_write_bool(const char *entry, int value, FILE *stream)
{
	if(value != 0) {
		fprintf(stream, "%%%s%%\n\n", entry);
	}
}

static
void _pacman_localdb_write_string(const char *entry, const char *value, FILE *stream)
{
	if(!_pacman_strempty(value)) {
		fprintf(stream, "%%%s%%\n%s\n\n", entry, value);
	}
}

static
void _pacman_localdb_write_stringlist(const char *entry, const pmlist_t *values, FILE *stream)
{
	const pmlist_t *lp;

	if(values != NULL) {
		fprintf(stream, "%%%s%%\n", entry);
		for(lp = values; lp; lp = lp->next) {
			fprintf(stream, "%s\n", (char *)lp->data);
		}
		fputc('\n', stream);
	}
}

int _pacman_localdb_write(pmdb_t *db, pmpkg_t *info, unsigned int inforeq)
{
	FILE *fp = NULL;
	char path[PATH_MAX];
	mode_t oldmask;
	int retval = 0;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(info == NULL) {
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
		_pacman_localdb_write_string("NAME", info->name, fp);
		_pacman_localdb_write_string("VERSION", info->version, fp);
		if(info->desc[0]) {
			_pacman_localdb_write_stringlist("DESC", info->desc_localized, fp);
		}
		_pacman_localdb_write_stringlist("GROUPS", info->groups, fp);
		_pacman_localdb_write_string("URL", info->url, fp);
		_pacman_localdb_write_stringlist("LICENSE", info->license, fp);
		_pacman_localdb_write_string("ARCH", info->arch, fp);
		_pacman_localdb_write_string("BUILDDATE", info->builddate, fp);
		_pacman_localdb_write_string("BUILDTYPE", info->buildtype, fp);
		_pacman_localdb_write_string("INSTALLDATE", info->installdate, fp);
		_pacman_localdb_write_string("PACKAGER", info->packager, fp);
		if(info->size) {
			fprintf(fp, "%%SIZE%%\n"
				"%ld\n\n", info->size);
		}
		if(info->reason) {
			fprintf(fp, "%%REASON%%\n"
				"%d\n\n", info->reason);
		}
		_pacman_localdb_write_stringlist("TRIGGERS", info->triggers, fp);
		fclose(fp);
		fp = NULL;
	}

	/* FILES */
	if(inforeq & INFRQ_FILES) {
		snprintf(path, PATH_MAX, "%s/%s-%s/files", db->path, info->name, info->version);
		if((fp = fopen(path, "w")) == NULL) {
			_pacman_log(PM_LOG_ERROR, _("db_write: could not open file %s/files"), db->treename);
			retval = -1;
			goto cleanup;
		}
		_pacman_localdb_write_stringlist("FILES", info->files, fp);
		_pacman_localdb_write_stringlist("BACKUP", info->backup, fp);
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
		_pacman_localdb_write_stringlist("DEPENDS", info->depends, fp);
		_pacman_localdb_write_stringlist("REQUIREDBY", info->requiredby, fp);
		_pacman_localdb_write_stringlist("CONFLICTS", info->conflicts, fp);
		_pacman_localdb_write_stringlist("PROVIDES", info->provides, fp);
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

const pmdb_ops_t _pacman_localdb_ops = {
	.test = _pacman_localdb_test,
	.open = _pacman_localdb_open,
	.close = _pacman_localdb_close,
	.rewind = _pacman_localdb_rewind,
	.write = _pacman_localdb_write,
};

/* vim: set ts=2 sw=2 noet: */
