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

/* pacman-g2 */
#include "db/localdb.h"

#include "util.h"
#include "db.h"
#include "package.h"
#include "pacman.h"
#include "error.h"
#include "handle.h"

#include "db/localdb_files.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "fstring.h"

#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h> /* PATH_MAX */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static
int _pacman_localdb_write(pmdb_t *db, pmpkg_t *info, unsigned int inforeq);

static
int _pacman_localdb_remove(pmdb_t *db, pmpkg_t *info);

static
int _pacman_localpkg_file_reader(pmdb_t *db, pmpkg_t *pkg, unsigned int flags, unsigned int flags_masq, const char *file, int (*reader)(pmpkg_t *, FILE *))
{
	int ret = 0;

	if(flags & flags_masq) {
		FILE *fp = NULL;
		char path[PATH_MAX];

		snprintf(path, PATH_MAX, "%s/%s-%s/%s", db->path, pkg->name, pkg->version, file);
		fp = fopen(path, "r");
		if(fp == NULL) {
			_pacman_log(PM_LOG_WARNING, "%s (%s)", path, strerror(errno));
			return -1;
		}
		if(reader(pkg, fp) == -1)
			return -1;
		fclose(fp);
	}
	return ret;
}

static int
_pacman_localpkg_read(pmpkg_t *pkg, unsigned int flags)
{
	pmdb_t *db;
	struct stat buf;
	char path[PATH_MAX];

	ASSERT(pkg != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT((db = pkg->database) != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(f_strempty(pkg->name) || f_strempty(pkg->version)) {
		_pacman_log(PM_LOG_ERROR, _("invalid package entry provided to _pacman_localdb_read"));
		return(-1);
	}

	snprintf(path, PATH_MAX, "%s/%s-%s", db->path, pkg->name, pkg->version);
	if(stat(path, &buf)) {
		/* directory doesn't exist or can't be opened */
		return(-1);
	}

	if (_pacman_localpkg_file_reader(db, pkg, flags, PM_LOCALPACKAGE_FLAGS_DESC, "desc", _pacman_localdb_desc_fread) == -1)
		return -1;
	if (_pacman_localpkg_file_reader(db, pkg, flags, PM_LOCALPACKAGE_FLAGS_DEPENDS, "depends", _pacman_localdb_depends_fread) == -1)
		return -1;
	if (_pacman_localpkg_file_reader(db, pkg, flags, PM_LOCALPACKAGE_FLAGS_FILES, "files", _pacman_localdb_files_fread) == -1)
		return -1;

	/* INSTALL */
	if(flags & INFRQ_SCRIPLET) {
		snprintf(path, PATH_MAX, "%s/%s-%s/install", db->path, pkg->name, pkg->version);
		if(!stat(path, &buf)) {
			pkg->scriptlet = 1;
		}
		pkg->flags |= PM_LOCALPACKAGE_FLAGS_SCRIPLET;
	}
	return 0;
}

static int
_pacman_localpkg_write(pmpkg_t *pkg, unsigned int flags)
{
	return _pacman_localdb_write(pkg->database, pkg, flags);
}

static int
_pacman_localpkg_remove(pmpkg_t *pkg)
{
	return _pacman_localdb_remove(pkg->database, pkg);
}

static const
pmpkg_ops_t _pacman_localpkg_operations = {
	.destroy = NULL,

	.read = _pacman_localpkg_read,
	.write = _pacman_localpkg_write,
	.remove = _pacman_localpkg_remove,
};

static
int _pacman_localpkg_init(pmpkg_t *pkg, pmdb_t *db)
{
	ASSERT(pkg != NULL, RET_ERR(PM_ERR_PKG_INVALID, -1));
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	pkg->operations = &_pacman_localpkg_operations;
	pkg->database = db;
	return 0;
}

static
pmpkg_t *_pacman_localdb_pkg_new(pmdb_t *db, const struct dirent *dirent, unsigned int inforeq)
{
	pmpkg_t *pkg;
	const char *dname;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));
	ASSERT(dirent != NULL, return NULL);

	dname = dirent->d_name;
	if((pkg = _pacman_pkg_new_from_filename(dname, 0)) == NULL ||
		_pacman_localpkg_init(pkg, db) != 0 ||
		_pacman_db_read(db, pkg, inforeq) == -1) {
		_pacman_log(PM_LOG_ERROR, _("invalid name for dabatase entry '%s'"), dname);
		FREEPKG(pkg);
	}
	return pkg;
}

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
int _pacman_localdb_open(pmdb_t *db, int flags, time_t *timestamp)
{
	struct stat buf;

	/* make sure the database directory exists */
	if(stat(db->path, &buf) != 0 || !S_ISDIR(buf.st_mode)) {
		_pacman_log(PM_LOG_FLOW1, _("database directory '%s' does not exist -- try creating it"), db->path);
		if(_pacman_makepath(db->path) != 0) {
			RET_ERR(PM_ERR_SYSTEM, -1);
		}
	}
	db->handle = opendir(db->path);
	ASSERT(db->handle != NULL, RET_ERR(PM_ERR_DB_OPEN, -1));
	_pacman_db_gettimestamp(db, timestamp);
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
pmpkg_t *_pacman_localdb_readpkg(pmdb_t *db, unsigned int inforeq)
{
	struct dirent *ent = NULL;
	struct stat sbuf;
	char path[PATH_MAX];

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));

	while((ent = readdir(db->handle)) != NULL) {
		if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
			continue;
		}
		/* stat the entry, make sure it's a directory */
		snprintf(path, PATH_MAX, "%s/%s", db->path, ent->d_name);
		if(!stat(path, &sbuf) && S_ISDIR(sbuf.st_mode)) {
			return _pacman_localdb_pkg_new(db, ent, inforeq);
		}
	}
	return NULL;
}

static
pmpkg_t *_pacman_localdb_scan(pmdb_t *db, const char *target, unsigned int inforeq)
{
	struct dirent *ent = NULL;
	struct stat sbuf;
	char path[PATH_MAX];
	char name[PKG_FULLNAME_LEN];
	char *ptr = NULL;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));
	ASSERT(!_pacman_strempty(target), RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	// Search from start
	_pacman_db_rewind(db);

	/* search for a specific package (by name only) */
	while((ent = readdir(db->handle)) != NULL) {
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
			return _pacman_localdb_pkg_new(db, ent, inforeq);
		}
	}
	return(NULL);
}

static
int _pacman_localdb_file_reader(pmdb_t *db, pmpkg_t *info, unsigned int inforeq, unsigned int inforeq_masq, const char *file, int (*reader)(pmpkg_t *, FILE *))
{
	int ret = 0;

	if(inforeq & inforeq_masq) {
		FILE *fp = NULL;
		char path[PATH_MAX];

		snprintf(path, PATH_MAX, "%s/%s-%s/%s", db->path, info->name, info->version, file);
		fp = fopen(path, "r");
		if(fp == NULL) {
			_pacman_log(PM_LOG_WARNING, "%s (%s)", path, strerror(errno));
			return -1;
		}
		if(reader(info, fp) == -1)
			return -1;
		fclose(fp);
	}
	return ret;
}

static
int _pacman_localdb_read(pmdb_t *db, pmpkg_t *info, unsigned int inforeq)
{
	struct stat buf;
	char path[PATH_MAX];

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(info == NULL || info->name[0] == 0 || info->version[0] == 0) {
		_pacman_log(PM_LOG_ERROR, _("invalid package entry provided to _pacman_localdb_read"));
		return(-1);
	}

	snprintf(path, PATH_MAX, "%s/%s-%s", db->path, info->name, info->version);
	if(stat(path, &buf)) {
		/* directory doesn't exist or can't be opened */
		return(-1);
	}

	if (_pacman_localdb_file_reader(db, info, inforeq, INFRQ_DESC, "desc", _pacman_localdb_desc_fread) == -1)
		return -1;
	if (_pacman_localdb_file_reader(db, info, inforeq, INFRQ_DEPENDS, "depends", _pacman_localdb_depends_fread) == -1)
		return -1;
	if (_pacman_localdb_file_reader(db, info, inforeq, INFRQ_FILES, "files", _pacman_localdb_files_fread) == -1)
		return -1;

	/* INSTALL */
	if(inforeq & INFRQ_SCRIPLET) {
		snprintf(path, PATH_MAX, "%s/%s-%s/install", db->path, info->name, info->version);
		if(!stat(path, &buf)) {
			info->scriptlet = 1;
		}
		info->flags |= PM_LOCALPACKAGE_FLAGS_SCRIPLET;
	}
	return 0;
}

/*
static
void _pacman_localdb_write_bool(const char *entry, int value, FILE *stream)
{
	if(value != 0) {
		fprintf(stream, "%%%s%%\n\n", entry);
	}
}
*/

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

static
int _pacman_localdb_write(pmdb_t *db, pmpkg_t *info, unsigned int inforeq)
{
	FILE *fp = NULL;
	char path[PATH_MAX];
	mode_t oldmask;
	int retval = 0;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(info != NULL, RET_ERR(PM_ERR_PKG_INVALID, -1));

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

static
int _pacman_localdb_remove(pmdb_t *db, pmpkg_t *info)
{
	char path[PATH_MAX];

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(info != NULL, RET_ERR(PM_ERR_PKG_INVALID, -1));

	snprintf(path, PATH_MAX, "%s/%s-%s", db->path, info->name, info->version);
	if(_pacman_rmrf(path) == -1) {
		return(-1);
	}

	return(0);
}

const pmdb_ops_t _pacman_localdb_ops = {
	.test = _pacman_localdb_test,
	.open = _pacman_localdb_open,
	.close = _pacman_localdb_close,
	.rewind = _pacman_localdb_rewind,
	.readpkg = _pacman_localdb_readpkg,
	.scan = _pacman_localdb_scan,
	.read = _pacman_localdb_read,
	.write = _pacman_localdb_write,
	.remove = _pacman_localdb_remove,
};

/* vim: set ts=2 sw=2 noet: */
