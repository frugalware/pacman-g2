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

pmlist_t *_pacman_db_test(pmdb_t *db)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));

	if (islocal(db))
		return _pacman_localdb_test(db);
	else
		return _pacman_syncdb_test(db);
}

int _pacman_db_open(pmdb_t *db)
{
	int ret = 0;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	if (islocal(db)) {
		ret = _pacman_localdb_open(db);
		if(ret != 0)
			return ret;
	} else {
		ret = _pacman_syncdb_open(db);
		if(ret != 0)
			return ret;
	}
	if(ret == 0 && _pacman_db_getlastupdate(db, db->lastupdate) == -1) {
		db->lastupdate[0] = '\0';
	}

	return ret;
}

int _pacman_db_close(pmdb_t *db)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	if (islocal(db))
		return _pacman_localdb_close(db);
	else
		return _pacman_syncdb_close(db);
}

int _pacman_db_rewind(pmdb_t *db)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	if (islocal(db))
		return _pacman_localdb_rewind(db);
	else
		return _pacman_syncdb_rewind(db);
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

static int _pacman_db_read_lines(pmlist_t **list, char *s, size_t size, FILE *fp)
{
	int lines = 0;

	while(fgets(s, size, fp) && !_pacman_strempty(_pacman_strtrim(s))) {
		*list = _pacman_stringlist_append(*list, s);
		lines++;
	}
	return lines;
}

static
int _pacman_db_desc_fread(pmpkg_t *info, unsigned int inforeq, FILE *fp)
{
	char line[512];
	int sline = sizeof(line)-1;
	pmlist_t *i;

	while(!feof(fp)) {
		if(fgets(line, 256, fp) == NULL) {
			break;
		}
		_pacman_strtrim(line);
		if(!strcmp(line, "%DESC%")) {
			_pacman_db_read_lines(&info->desc_localized, line, sline, fp);
			STRNCPY(info->desc, (char*)info->desc_localized->data, sizeof(info->desc));
			for (i = info->desc_localized; i; i = i->next) {
				if (!strncmp(i->data, handle->language, strlen(handle->language)) &&
						*((char*)i->data+strlen(handle->language)) == ' ') {
					STRNCPY(info->desc, (char*)i->data+strlen(handle->language)+1, sizeof(info->desc));
				}
			}
			_pacman_strtrim(info->desc);
		} else if(!strcmp(line, "%GROUPS%")) {
			_pacman_db_read_lines(&info->groups, line, sline, fp);
		} else if(!strcmp(line, "%URL%")) {
			if(fgets(info->url, sizeof(info->url), fp) == NULL) {
				goto error;
			}
			_pacman_strtrim(info->url);
		} else if(!strcmp(line, "%LICENSE%")) {
			_pacman_db_read_lines(&info->license, line, sline, fp);
		} else if(!strcmp(line, "%ARCH%")) {
			if(fgets(info->arch, sizeof(info->arch), fp) == NULL) {
				goto error;
			}
			_pacman_strtrim(info->arch);
		} else if(!strcmp(line, "%BUILDDATE%")) {
			if(fgets(info->builddate, sizeof(info->builddate), fp) == NULL) {
				goto error;
			}
			_pacman_strtrim(info->builddate);
		} else if(!strcmp(line, "%BUILDTYPE%")) {
			if(fgets(info->buildtype, sizeof(info->buildtype), fp) == NULL) {
				goto error;
			}
			_pacman_strtrim(info->buildtype);
		} else if(!strcmp(line, "%INSTALLDATE%")) {
			if(fgets(info->installdate, sizeof(info->installdate), fp) == NULL) {
				goto error;
			}
			_pacman_strtrim(info->installdate);
		} else if(!strcmp(line, "%PACKAGER%")) {
			if(fgets(info->packager, sizeof(info->packager), fp) == NULL) {
				goto error;
			}
			_pacman_strtrim(info->packager);
		} else if(!strcmp(line, "%REASON%")) {
			char tmp[32];
			if(fgets(tmp, sizeof(tmp), fp) == NULL) {
				goto error;
			}
			_pacman_strtrim(tmp);
			info->reason = atol(tmp);
		} else if(!strcmp(line, "%TRIGGERS%")) {
			_pacman_db_read_lines(&info->triggers, line, sline, fp);
		} else if(!strcmp(line, "%SIZE%") || !strcmp(line, "%CSIZE%")) {
			/* NOTE: the CSIZE and SIZE fields both share the "size" field
			 *       in the pkginfo_t struct.  This can be done b/c CSIZE
			 *       is currently only used in sync databases, and SIZE is
			 *       only used in local databases.
			 */
			char tmp[32];
			if(fgets(tmp, sizeof(tmp), fp) == NULL) {
				goto error;
			}
			_pacman_strtrim(tmp);
			info->size = atol(tmp);
		} else if(!strcmp(line, "%USIZE%")) {
			/* USIZE (uncompressed size) tag only appears in sync repositories,
			 * not the local one. */
			char tmp[32];
			if(fgets(tmp, sizeof(tmp), fp) == NULL) {
				goto error;
			}
			_pacman_strtrim(tmp);
			info->usize = atol(tmp);
		} else if(!strcmp(line, "%SHA1SUM%")) {
			/* SHA1SUM tag only appears in sync repositories,
			 * not the local one. */
			if(fgets(info->sha1sum, sizeof(info->sha1sum), fp) == NULL) {
				goto error;
			}
		} else if(!strcmp(line, "%MD5SUM%")) {
			/* MD5SUM tag only appears in sync repositories,
			 * not the local one. */
			if(fgets(info->md5sum, sizeof(info->md5sum), fp) == NULL) {
				goto error;
			}
		/* XXX: these are only here as backwards-compatibility for pacman
		 * sync repos.... in pacman-g2, they have been moved to DEPENDS.
		 * Remove this when we move to pacman-g2 repos.
		 */
		} else if(!strcmp(line, "%REPLACES%")) {
			/* the REPLACES tag is special -- it only appears in sync repositories,
			 * not the local one. */
			_pacman_db_read_lines(&info->replaces, line, sline, fp);
		} else if(!strcmp(line, "%FORCE%")) {
			/* FORCE tag only appears in sync repositories,
			 * not the local one. */
			info->force = 1;
		} else if(!strcmp(line, "%STICK%")) {
			/* STICK tag only appears in sync repositories,
			 * not the local one. */
			info->stick = 1;
		}
	}
	return 0;

error:
	return -1;
}

static int _pacman_db_read_desc(pmdb_t *db, unsigned int inforeq, pmpkg_t *info)
{
	int ret = 0;

	if(inforeq & INFRQ_DESC) {
		FILE *fp = NULL;
		char path[PATH_MAX];

		if (islocal(db)) {
			snprintf(path, PATH_MAX, "%s/%s-%s/desc", db->path, info->name, info->version);
			fp = fopen(path, "r");
			if(fp == NULL) {
				_pacman_log(PM_LOG_DEBUG, "%s (%s)", path, strerror(errno));
				return -1;
			}
		} else {
			fp = _pacman_archive_read_fropen(db->handle);
		}
		ret = _pacman_db_desc_fread(info, inforeq, fp);
		if (fp)
			fclose(fp);
	}
	return ret;
}

static
int _pacman_db_depends_fread(pmpkg_t *info, unsigned int inforeq, FILE *fp)
{
	char line[512];
	int sline = sizeof(line)-1;

	while(!feof(fp)) {
		if(fgets(line, 256, fp) == NULL) {
			break;
		}
		_pacman_strtrim(line);
		if(!strcmp(line, "%DEPENDS%")) {
			_pacman_db_read_lines(&info->depends, line, sline, fp);
		} else if(!strcmp(line, "%REQUIREDBY%")) {
			_pacman_db_read_lines(&info->requiredby, line, sline, fp);
		} else if(!strcmp(line, "%CONFLICTS%")) {
			_pacman_db_read_lines(&info->conflicts, line, sline, fp);
		} else if(!strcmp(line, "%PROVIDES%")) {
			_pacman_db_read_lines(&info->provides, line, sline, fp);
		} else if(!strcmp(line, "%REPLACES%")) {
			/* the REPLACES tag is special -- it only appears in sync repositories,
			 * not the local one. */
			_pacman_db_read_lines(&info->replaces, line, sline, fp);
		} else if(!strcmp(line, "%FORCE%")) {
			/* FORCE tag only appears in sync repositories,
			 * not the local one. */
			info->force = 1;
		} else if(!strcmp(line, "%STICK%")) {
			/* STICK tag only appears in sync repositories,
			 * not the local one. */
			info->stick = 1;
		}
	}
	return 0;
}

static int _pacman_db_read_depends(pmdb_t *db, unsigned int inforeq, pmpkg_t *info)
{
	int ret = 0;

	if(inforeq & INFRQ_DEPENDS) {
		FILE *fp = NULL;
		char path[PATH_MAX];

		if (islocal(db)) {
			snprintf(path, PATH_MAX, "%s/%s-%s/depends", db->path, info->name, info->version);
			fp = fopen(path, "r");
			if(fp == NULL) {
				_pacman_log(PM_LOG_WARNING, "%s (%s)", path, strerror(errno));
				return -1;
			}
		} else {
			fp = _pacman_archive_read_fropen(db->handle);
		}
		ret = _pacman_db_depends_fread(info, inforeq, fp);
		if (fp)
			fclose(fp);
	}
	return ret;
}

static int suffixcmp(const char *str, const char *suffix)
{
	int len = strlen(str), suflen = strlen(suffix);
	if (len < suflen)
		return -1;
	else
		return strcmp(str + len - suflen, suffix);
}

int _pacman_db_read(pmdb_t *db, unsigned int inforeq, pmpkg_t *info)
{
	FILE *fp = NULL;
	struct stat buf;
	char path[PATH_MAX];
	char line[512];
	int sline = sizeof(line)-1;
	char *ptr;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	if(info == NULL || info->name[0] == 0 || info->version[0] == 0) {
		_pacman_log(PM_LOG_ERROR, _("invalid package entry provided to _pacman_db_read"));
		return(-1);
	}

	snprintf(path, PATH_MAX, "%s/%s-%s", db->path, info->name, info->version);
	if(islocal(db) && stat(path, &buf)) {
		/* directory doesn't exist or can't be opened */
		return(-1);
	}

	if (islocal(db)) {
		if (_pacman_db_read_desc(db, inforeq, info) == -1)
			return -1;

		if (_pacman_db_read_depends(db, inforeq, info) == -1)
			return -1;
	} else {
		int descdone = 0, depsdone = 0;
		while (!descdone || !depsdone) {
			struct archive_entry *entry = NULL;
			if (archive_read_next_header(db->handle, &entry) != ARCHIVE_OK)
				return -1;
			const char *pathname = archive_entry_pathname(entry);
			if (!suffixcmp(pathname, "/desc")) {
				if (_pacman_db_read_desc(db, inforeq, info) == -1)
					return -1;
				descdone = 1;
			}
			if (!suffixcmp(pathname, "/depends")) {
				if (_pacman_db_read_depends(db, inforeq, info) == -1)
					return -1;
				depsdone = 1;
			}
		}
	}

	/* FILES */
	if(inforeq & INFRQ_FILES) {
		snprintf(path, PATH_MAX, "%s/%s-%s/files", db->path, info->name, info->version);
		fp = fopen(path, "r");
		if(fp == NULL) {
			_pacman_log(PM_LOG_WARNING, "%s (%s)", path, strerror(errno));
			goto error;
		}
		while(fgets(line, 256, fp)) {
			_pacman_strtrim(line);
			if(!strcmp(line, "%FILES%")) {
				while(fgets(line, sline, fp) && !_pacman_strempty(_pacman_strtrim(line))) {
					if((ptr = strchr(line, '|'))) {
						/* just ignore the content after the pipe for now */
						*ptr = '\0';
					}
					info->files = _pacman_stringlist_append(info->files, line);
				}
			} else if(!strcmp(line, "%BACKUP%")) {
				while(fgets(line, sline, fp) && !_pacman_strempty(_pacman_strtrim(line))) {
					info->backup = _pacman_stringlist_append(info->backup, line);
				}
			}
		}
		fclose(fp);
		fp = NULL;
	}

	/* INSTALL */
	if(inforeq & INFRQ_SCRIPLET) {
		snprintf(path, PATH_MAX, "%s/%s-%s/install", db->path, info->name, info->version);
		if(!stat(path, &buf)) {
			info->scriptlet = 1;
		}
	}

	/* internal */
	info->infolevel |= inforeq;

	return(0);

error:
	if(fp) {
		fclose(fp);
	}
	return(-1);
}

static
void _pacman_db_write_bool(const char *entry, int value, FILE *stream)
{
	if(value != 0) {
		fprintf(stream, "%%%s%%\n\n", entry);
	}
}

static
void _pacman_db_write_string(const char *entry, const char *value, FILE *stream)
{
	if(!_pacman_strempty(value)) {
		fprintf(stream, "%%%s%%\n%s\n\n", entry, value);
	}
}

static
void _pacman_db_write_stringlist(const char *entry, const pmlist_t *values, FILE *stream)
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
	int local = 0;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(info == NULL) {
		return(-1);
	}

	snprintf(path, PATH_MAX, "%s/%s-%s", db->path, info->name, info->version);
	oldmask = umask(0000);
	mkdir(path, 0755);
	/* make sure we have a sane umask */
	umask(0022);

	if(islocal(db)) {
		local = 1;
	}

	/* DESC */
	if(inforeq & INFRQ_DESC) {
		snprintf(path, PATH_MAX, "%s/%s-%s/desc", db->path, info->name, info->version);
		if((fp = fopen(path, "w")) == NULL) {
			_pacman_log(PM_LOG_ERROR, _("db_write: could not open file %s/desc"), db->treename);
			retval = 1;
			goto cleanup;
		}
		_pacman_db_write_string("NAME", info->name, fp);
		_pacman_db_write_string("VERSION", info->version, fp);
		if(info->desc[0]) {
			_pacman_db_write_stringlist("DESC", info->desc_localized, fp);
		}
		_pacman_db_write_stringlist("GROUPS", info->groups, fp);
		if(local) {
			_pacman_db_write_string("URL", info->url, fp);
			_pacman_db_write_stringlist("LICENSE", info->license, fp);
			_pacman_db_write_string("ARCH", info->arch, fp);
			_pacman_db_write_string("BUILDDATE", info->builddate, fp);
			_pacman_db_write_string("BUILDTYPE", info->buildtype, fp);
			_pacman_db_write_string("INSTALLDATE", info->installdate, fp);
			_pacman_db_write_string("PACKAGER", info->packager, fp);
			if(info->size) {
				fprintf(fp, "%%SIZE%%\n"
					"%ld\n\n", info->size);
			}
			if(info->reason) {
				fprintf(fp, "%%REASON%%\n"
					"%d\n\n", info->reason);
			}
			_pacman_db_write_stringlist("TRIGGERS", info->triggers, fp);
		} else {
			if(info->size) {
				fprintf(fp, "%%CSIZE%%\n"
					"%ld\n\n", info->size);
			}
			if(info->usize) {
				fprintf(fp, "%%USIZE%%\n"
					"%ld\n\n", info->usize);
			}
			_pacman_db_write_string("SHA1SUM", info->sha1sum, fp);
			_pacman_db_write_string("MD5SUM", info->md5sum, fp);
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
		_pacman_db_write_stringlist("FILES", info->files, fp);
		_pacman_db_write_stringlist("BACKUP", info->backup, fp);
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
		_pacman_db_write_stringlist("DEPENDS", info->depends, fp);
		if(local) {
			_pacman_db_write_stringlist("REQUIREDBY", info->requiredby, fp);
		}
		_pacman_db_write_stringlist("CONFLICTS", info->conflicts, fp);
		_pacman_db_write_stringlist("PROVIDES", info->provides, fp);
		if(!local) {
			_pacman_db_write_stringlist("REPLACES", info->replaces, fp);
			_pacman_db_write_bool("FORCE", info->force, fp);
			_pacman_db_write_bool("STICK", info->stick, fp);
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

int _pacman_db_write(pmdb_t *db, pmpkg_t *info, unsigned int inforeq)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(info == NULL) {
		return(-1);
	}

	if(islocal(db)) {
		return _pacman_localdb_write(db, info, inforeq);
	} else {
		RET_ERR(PM_ERR_WRONG_ARGS, -1); // Not supported anymore
	}

}

int _pacman_db_remove(pmdb_t *db, pmpkg_t *info)
{
	char path[PATH_MAX];

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	if(info == NULL) {
		RET_ERR(PM_ERR_DB_NULL, -1);
	}

	snprintf(path, PATH_MAX, "%s/%s-%s", db->path, info->name, info->version);
	if(_pacman_rmrf(path) == -1) {
		return(-1);
	}

	return(0);
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
