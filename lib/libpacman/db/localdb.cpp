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

#include "db/localdb_files.h"
#include "util.h"
#include "cache.h"
#include "db.h"
#include "package.h"
#include "pacman.h"
#include "error.h"
#include "handle.h"

#include "util/log.h"
#include "fstring.h"

#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h> /* PATH_MAX */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace libpacman;

LocalPackage::LocalPackage(LocalDatabase *database)
	: Package(database)
{
}

LocalPackage::~LocalPackage()
{
}

LocalDatabase *LocalPackage::database() const
{
	return static_cast<LocalDatabase *>(Package::database());
}

static
int _pacman_localpkg_file_reader(Database *db, Package *pkg, unsigned int flags, unsigned int flags_masq, const char *file, int (*reader)(Package *, FILE *))
{
	int ret = 0;

	if((flags & flags_masq) != 0) {
		FILE *fp = NULL;
		char path[PATH_MAX];

		snprintf(path, PATH_MAX, "%s/%s-%s/%s", db->path, pkg->name(), pkg->version(), file);
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

int LocalPackage::read(unsigned int flags)
{
	struct stat buf;
	char path[PATH_MAX];
	LocalDatabase *database = this->database();

	ASSERT(database != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(f_strempty(name()) || f_strempty(version())) {
		_pacman_log(PM_LOG_ERROR, _("invalid package entry provided to LocalPackage::read"));
		return(-1);
	}

	snprintf(path, PATH_MAX, "%s/%s-%s", database->path, name(), version());
	if(stat(path, &buf)) {
		/* directory doesn't exist or can't be opened */
		return(-1);
	}

	if (_pacman_localpkg_file_reader(database, this, flags, PM_LOCALPACKAGE_FLAGS_DESC, "desc", _pacman_localdb_desc_fread) == -1)
		return -1;
	if (_pacman_localpkg_file_reader(database, this, flags, PM_LOCALPACKAGE_FLAGS_DEPENDS, "depends", _pacman_localdb_depends_fread) == -1)
		return -1;
	if (_pacman_localpkg_file_reader(database, this, flags, PM_LOCALPACKAGE_FLAGS_FILES, "files", _pacman_localdb_files_fread) == -1)
		return -1;

	/* INSTALL */
	if(flags & INFRQ_SCRIPLET) {
		snprintf(path, PATH_MAX, "%s/%s-%s/install", database->path, name(), version());
		if(!stat(path, &buf)) {
			scriptlet = 1;
		}
	}
	this->flags |= flags;
	return 0;
}

int LocalPackage::remove()
{
	char path[PATH_MAX];

	snprintf(path, PATH_MAX, "%s/%s-%s", m_database->path, name(), version());
	if(_pacman_rmrf(path) == -1) {
		return(-1);
	}

	return(0);
}

int _pacman_localpackage_remove(Package *pkg, pmtrans_t *trans, int howmany, int remain)
{
	struct stat buf;
	int position = 0;
	Handle *handle = trans->m_handle;
	char line[PATH_MAX+1];

	int filenum = f_ptrlist_count(&pkg->files());
	_pacman_log(PM_LOG_FLOW1, _("removing files"));

	/* iterate through the list backwards, unlinking files */
	for(auto lp = pkg->files().rbegin(), end = pkg->files().rend(); lp != end; lp = lp->previous()) {
		int nb = 0;
		double percent = 0;
		const char *file = *lp;
		char *hash_orig = pkg->fileneedbackup(file);

		if (position != 0) {
			percent = (double)position / filenum;
		}
		if(!_pacman_strempty(hash_orig)) {
			nb = 1;
		}
		FREE(hash_orig);
		if(!nb && trans->m_type == PM_TRANS_TYPE_UPGRADE) {
			/* check noupgrade */
			if(_pacman_list_is_strin(file, &handle->noupgrade)) {
				nb = 1;
			}
		}
		snprintf(line, PATH_MAX, "%s%s", handle->root, file);
		if(lstat(line, &buf)) {
			_pacman_log(PM_LOG_DEBUG, _("file %s does not exist"), file);
			continue;
		}
		if(S_ISDIR(buf.st_mode)) {
			if(rmdir(line)) {
				/* this is okay, other packages are probably using it. */
				_pacman_log(PM_LOG_DEBUG, _("keeping directory %s"), file);
			} else {
				_pacman_log(PM_LOG_FLOW2, _("removing directory %s"), file);
			}
		} else {
			/* check the "skip list" before removing the file.
			 * see the big comment block in db_find_conflicts() for an
			 * explanation. */
			int skipit = 0;
			for(auto j = trans->skiplist.begin(), end = trans->skiplist.end(); j != end; j = j->next()) {
				if(!strcmp(file, *j)) {
					skipit = 1;
				}
			}
			if(skipit) {
				_pacman_log(PM_LOG_FLOW2, _("skipping removal of %s as it has moved to another package"),
						file);
			} else {
				/* if the file is flagged, back it up to .pacsave */
				if(nb) {
					if(trans->m_type == PM_TRANS_TYPE_UPGRADE) {
						/* we're upgrading so just leave the file as is. pacman_add() will handle it */
					} else {
						if(!(trans->flags & PM_TRANS_FLAG_NOSAVE)) {
							char newpath[PATH_MAX];
							snprintf(newpath, PATH_MAX, "%s.pacsave", line);
							rename(line, newpath);
							_pacman_log(PM_LOG_WARNING, _("%s saved as %s"), line, newpath);
						} else {
							_pacman_log(PM_LOG_FLOW2, _("unlinking %s"), file);
							if(unlink(line)) {
								_pacman_log(PM_LOG_ERROR, _("cannot remove file %s"), file);
							}
						}
					}
				} else {
					_pacman_log(PM_LOG_FLOW2, _("unlinking %s"), file);
					/* Need at here because we count only real unlinked files ? */
					PROGRESS(trans, PM_TRANS_PROGRESS_REMOVE_START, pkg->name(), (int)(percent * 100), howmany, howmany - remain + 1);
					position++;
					if(unlink(line)) {
						_pacman_log(PM_LOG_ERROR, _("cannot remove file %s"), file);
					}
				}
			}
		}
	}
	return 0;
}

LocalDatabase::LocalDatabase(libpacman::Handle *handle, const char *treename)
	  : Database(handle, treename)
{
}

LocalDatabase::~LocalDatabase()
{
}

static
LocalPackage *_pacman_localdb_pkg_new(LocalDatabase *db, const struct dirent *dirent, unsigned int inforeq)
{
	LocalPackage *pkg;
	const char *dname;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));
	ASSERT(dirent != NULL, return NULL);

	dname = dirent->d_name;
	if((pkg = new LocalPackage(db)) == NULL ||
			!pkg->set_filename(dname, 0) ||
			pkg->read(inforeq) == -1) {
		_pacman_log(PM_LOG_ERROR, _("invalid name for dabatase entry '%s'"), dname);
		pkg->release();
		pkg = NULL;
	}
	return pkg;
}

FStringList LocalDatabase::test() const
{
	struct dirent *ent;
	char path[PATH_MAX];
	struct stat buf;
	FStringList ret;

	while ((ent = readdir(m_dir)) != NULL) {
		snprintf(path, PATH_MAX, "%s/%s", this->path, ent->d_name);
		stat(path, &buf);
		if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..") || !S_ISDIR(buf.st_mode)) {
			continue;
		}
		snprintf(path, PATH_MAX, "%s/%s/desc", this->path, ent->d_name);
		if(stat(path, &buf))
		{
			snprintf(path, LOG_STR_LEN, _("%s: description file is missing"), ent->d_name);
			ret.add(path);
		}
		snprintf(path, PATH_MAX, "%s/%s/depends", this->path, ent->d_name);
		if(stat(path, &buf))
		{
			snprintf(path, LOG_STR_LEN, _("%s: dependency information is missing"), ent->d_name);
			ret.add(path);
		}
		snprintf(path, PATH_MAX, "%s/%s/files", this->path, ent->d_name);
		if(stat(path, &buf))
		{
			snprintf(path, LOG_STR_LEN, _("%s: file list is missing"), ent->d_name);
			ret.add(path);
		}
	}

	return(ret);
}

int LocalDatabase::open(int flags, Timestamp *timestamp)
{
	struct stat buf;

	/* make sure the database directory exists */
	if(stat(path, &buf) != 0 || !S_ISDIR(buf.st_mode)) {
		_pacman_log(PM_LOG_FLOW1, _("database directory '%s' does not exist -- try creating it"), path);
		if(_pacman_makepath(path) != 0) {
			RET_ERR(PM_ERR_SYSTEM, -1);
		}
	}
	m_dir = opendir(path);
	ASSERT(m_dir != NULL, RET_ERR(PM_ERR_DB_OPEN, -1));
	gettimestamp(timestamp);
	return 0;
}

int LocalDatabase::close()
{
	if(m_dir) {
		closedir(m_dir);
		m_dir = NULL;
	}
	return 0;
}

int LocalDatabase::rewind()
{
	if(m_dir == NULL) {
		return -1;
	}

	rewinddir(m_dir);
	return 0;
}

Package *LocalDatabase::readpkg(unsigned int inforeq)
{
	struct dirent *ent = NULL;
	struct stat sbuf;
	char path[PATH_MAX];

	while((ent = readdir(m_dir)) != NULL) {
		if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
			continue;
		}
		/* stat the entry, make sure it's a directory */
		snprintf(path, PATH_MAX, "%s/%s", this->path, ent->d_name);
		if(!stat(path, &sbuf) && S_ISDIR(sbuf.st_mode)) {
			return _pacman_localdb_pkg_new(this, ent, inforeq);
		}
	}
	return NULL;
}

Package *LocalDatabase::scan(const char *target, unsigned int inforeq)
{
	struct dirent *ent = NULL;
	struct stat sbuf;
	char path[PATH_MAX];
	char name[PKG_FULLNAME_LEN];
	char *ptr = NULL;

	ASSERT(!_pacman_strempty(target), RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	// Search from start
	rewind();

	/* search for a specific package (by name only) */
	while((ent = readdir(m_dir)) != NULL) {
		if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
			continue;
		}
		/* stat the entry, make sure it's a directory */
		snprintf(path, PATH_MAX, "%s/%s", this->path, ent->d_name);
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
			return _pacman_localdb_pkg_new(this, ent, inforeq);
		}
	}
	return(NULL);
}

static
void _pacman_localdb_write_string(const char *entry, const char *value, FILE *stream)
{
	if(!_pacman_strempty(value)) {
		fprintf(stream, "%%%s%%\n%s\n\n", entry, value);
	}
}

static
void _pacman_localdb_write_stringlist(const char *entry, const FStringList &values, FILE *stream)
{
	if(!values.empty()) {
		fprintf(stream, "%%%s%%\n", entry);
		for(auto lp = values.begin(), end = values.end(); lp != end; lp = lp->next()) {
			fprintf(stream, "%s\n", *lp);
		}
		fputc('\n', stream);
	}
}

int LocalDatabase::write(Package *info, unsigned int inforeq)
{
	FILE *fp = NULL;
	char path[PATH_MAX];
	mode_t oldmask;
	int retval = 0;

	ASSERT(info != NULL, RET_ERR(PM_ERR_PKG_INVALID, -1));

	snprintf(path, PATH_MAX, "%s/%s-%s", this->path, info->name(), info->version());
	oldmask = umask(0000);
	mkdir(path, 0755);
	/* make sure we have a sane umask */
	umask(0022);

	/* DESC */
	if(inforeq & INFRQ_DESC) {
		snprintf(path, PATH_MAX, "%s/%s-%s/desc", this->path, info->name(), info->version());
		if((fp = fopen(path, "w")) == NULL) {
			_pacman_log(PM_LOG_ERROR, _("db_write: could not open file %s/desc"), treename());
			retval = 1;
			goto cleanup;
		}
		_pacman_localdb_write_string("NAME", info->name(), fp);
		_pacman_localdb_write_string("VERSION", info->version(), fp);
		if(info->description()[0]) {
			_pacman_localdb_write_stringlist("DESC", info->desc_localized, fp);
		}
		_pacman_localdb_write_stringlist("GROUPS", info->groups(), fp);
		_pacman_localdb_write_string("URL", info->url(), fp);
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
		if(info->reason()) {
			fprintf(fp, "%%REASON%%\n"
				"%d\n\n", info->reason());
		}
		_pacman_localdb_write_stringlist("TRIGGERS", info->triggers(), fp);
		fclose(fp);
		fp = NULL;
	}

	/* FILES */
	if(inforeq & INFRQ_FILES) {
		snprintf(path, PATH_MAX, "%s/%s-%s/files", this->path, info->name(), info->version());
		if((fp = fopen(path, "w")) == NULL) {
			_pacman_log(PM_LOG_ERROR, _("db_write: could not open file %s/files"), treename());
			retval = -1;
			goto cleanup;
		}
		_pacman_localdb_write_stringlist("FILES", info->files(), fp);
		_pacman_localdb_write_stringlist("BACKUP", info->backup(), fp);
		fclose(fp);
		fp = NULL;
	}

	/* DEPENDS */
	if(inforeq & INFRQ_DEPENDS) {
		snprintf(path, PATH_MAX, "%s/%s-%s/depends", this->path, info->name(), info->version());
		if((fp = fopen(path, "w")) == NULL) {
			_pacman_log(PM_LOG_ERROR, _("db_write: could not open file %s/depends"), treename());
			retval = -1;
			goto cleanup;
		}
		_pacman_localdb_write_stringlist("DEPENDS", info->depends(), fp);
		_pacman_localdb_write_stringlist("REQUIREDBY", info->requiredby(), fp);
		_pacman_localdb_write_stringlist("CONFLICTS", info->conflicts(), fp);
		_pacman_localdb_write_stringlist("PROVIDES", info->provides(), fp);
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

FPtrList LocalDatabase::getowners(const char *filename)
{
	struct stat buf;
	int gotcha = 0;
	char rpath[PATH_MAX];
	FPtrList ret;

	if(stat(filename, &buf) == -1 || realpath(filename, rpath) == NULL) {
		RET_ERR(PM_ERR_PKG_OPEN, ret);
	}

	if(S_ISDIR(buf.st_mode)) {
		/* this is a directory and the db has a / suffix for dirs - add it here so
		 * that we'll find dirs, too */
		rpath[strlen(rpath)+1] = '\0';
		rpath[strlen(rpath)] = '/';
	}

	FPtrList &cache = _pacman_db_get_pkgcache(this);
	for(auto lp = cache.begin(), end = cache.end(); lp != end; lp = lp->next()) {
		Package *info = (Package *)*lp;

		auto &files = info->files();
		for(auto i = files.begin(), end = files.end(); i != end; i = i->next()) {
			char path[PATH_MAX];

			snprintf(path, PATH_MAX, "%s%s", m_handle->root, *i);
			if(!strcmp(path, rpath)) {
				ret.add(info);
				if(rpath[strlen(rpath)-1] != '/') {
					/* we are searching for a file and multiple packages won't contain
					 * the same file */
					return ret;
				}
				gotcha = 1;
			}
		}
	}
	if(!gotcha) {
		RET_ERR(PM_ERR_NO_OWNER, ret);
	}

	return ret;
}

/* vim: set ts=2 sw=2 noet: */
