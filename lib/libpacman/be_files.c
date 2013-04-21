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
#include <locale.h>
#ifdef CYGWIN
#include <limits.h> /* PATH_MAX */
#endif

/* pacman-g2 */
#include "log.h"
#include "util.h"
#include "db.h"
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
	struct dirent *ent;
	char path[PATH_MAX];
	struct stat buf;
	pmlist_t *ret = NULL;

	/* testing sync dbs is not supported */
	if (!islocal(db))
		return ret;

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
			ret = _pacman_list_add(ret, strdup(path));
		}
		snprintf(path, PATH_MAX, "%s/%s/depends", db->path, ent->d_name);
		if(stat(path, &buf))
		{
			snprintf(path, LOG_STR_LEN, _("%s: dependency information is missing"), ent->d_name);
			ret = _pacman_list_add(ret, strdup(path));
		}
		snprintf(path, PATH_MAX, "%s/%s/files", db->path, ent->d_name);
		if(stat(path, &buf))
		{
			snprintf(path, LOG_STR_LEN, _("%s: file list is missing"), ent->d_name);
			ret = _pacman_list_add(ret, strdup(path));
		}
	}
	return(ret);
}

int _pacman_db_open(pmdb_t *db)
{
	if(db == NULL) {
		RET_ERR(PM_ERR_DB_NULL, -1);
	}

	if (islocal(db)) {
		db->handle = opendir(db->path);
		if(db->handle == NULL) {
			RET_ERR(PM_ERR_DB_OPEN, -1);
		}
	} else {
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
	}
	if(_pacman_db_getlastupdate(db, db->lastupdate) == -1) {
		db->lastupdate[0] = '\0';
	}

	return(0);
}

void _pacman_db_close(pmdb_t *db)
{
	if(db == NULL) {
		return;
	}

	_pacman_log(PM_LOG_DEBUG, _("closing database '%s'"), db->treename);

	if(db->handle) {
		if (islocal(db))
			closedir(db->handle);
		else
			archive_read_finish(db->handle);
		db->handle = NULL;
	}
}

void _pacman_db_rewind(pmdb_t *db)
{
	if(db == NULL || (islocal(db) && db->handle == NULL)) {
		return;
	}

	if (islocal(db)) {
		rewinddir(db->handle);
	} else {
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
	}
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
		/* search for a specific package (by name only) */
		if (islocal(db)) {
			rewinddir(db->handle);
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
			// seek to start
			_pacman_db_rewind(db->handle);

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
	pkg->reason = PM_PKG_REASON_EXPLICIT;
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

static char *_pacman_db_read_fgets(pmdb_t *db, char *line, size_t size, FILE *fp)
{
	if (islocal(db))
		return fgets(line, size, fp);
	else
		return _pacman_archive_fgets(line, size, db->handle);
}

static int _pacman_db_read_desc(pmdb_t *db, unsigned int inforeq, pmpkg_t *info)
{
	FILE *fp = NULL;
	char path[PATH_MAX];
	char line[512];
	int sline = sizeof(line)-1;
	pmlist_t *i;

	if(inforeq & INFRQ_DESC) {
		if (islocal(db)) {
			snprintf(path, PATH_MAX, "%s/%s-%s/desc", db->path, info->name, info->version);
			fp = fopen(path, "r");
			if(fp == NULL) {
				_pacman_log(PM_LOG_DEBUG, "%s (%s)", path, strerror(errno));
				goto error;
			}
		}
		while(!islocal(db) || !feof(fp)) {
			if(_pacman_db_read_fgets(db, line, 256, fp) == NULL) {
				break;
			}
			_pacman_strtrim(line);
			if(!strcmp(line, "%DESC%")) {
				while(_pacman_db_read_fgets(db, line, sline, fp) && strlen(_pacman_strtrim(line))) {
					info->desc_localized = _pacman_list_add(info->desc_localized, strdup(line));
				}
				STRNCPY(info->desc, (char*)info->desc_localized->data, sizeof(info->desc));
				for (i = info->desc_localized; i; i = i->next) {
					if (!strncmp(i->data, handle->language, strlen(handle->language)) &&
							*((char*)i->data+strlen(handle->language)) == ' ') {
						STRNCPY(info->desc, (char*)i->data+strlen(handle->language)+1, sizeof(info->desc));
					}
				}
				_pacman_strtrim(info->desc);
			} else if(!strcmp(line, "%GROUPS%")) {
				while(_pacman_db_read_fgets(db, line, sline, fp) && strlen(_pacman_strtrim(line))) {
					info->groups = _pacman_list_add(info->groups, strdup(line));
				}
			} else if(!strcmp(line, "%URL%")) {
				if(_pacman_db_read_fgets(db, info->url, sizeof(info->url), fp) == NULL) {
					goto error;
				}
				_pacman_strtrim(info->url);
			} else if(!strcmp(line, "%LICENSE%")) {
				while(_pacman_db_read_fgets(db, line, sline, fp) && strlen(_pacman_strtrim(line))) {
					info->license = _pacman_list_add(info->license, strdup(line));
				}
			} else if(!strcmp(line, "%ARCH%")) {
				if(_pacman_db_read_fgets(db, info->arch, sizeof(info->arch), fp) == NULL) {
					goto error;
				}
				_pacman_strtrim(info->arch);
			} else if(!strcmp(line, "%BUILDDATE%")) {
				if(_pacman_db_read_fgets(db, info->builddate, sizeof(info->builddate), fp) == NULL) {
					goto error;
				}
				_pacman_strtrim(info->builddate);
			} else if(!strcmp(line, "%BUILDTYPE%")) {
				if(_pacman_db_read_fgets(db, info->buildtype, sizeof(info->buildtype), fp) == NULL) {
					goto error;
				}
				_pacman_strtrim(info->buildtype);
			} else if(!strcmp(line, "%INSTALLDATE%")) {
				if(_pacman_db_read_fgets(db, info->installdate, sizeof(info->installdate), fp) == NULL) {
					goto error;
				}
				_pacman_strtrim(info->installdate);
			} else if(!strcmp(line, "%PACKAGER%")) {
				if(_pacman_db_read_fgets(db, info->packager, sizeof(info->packager), fp) == NULL) {
					goto error;
				}
				_pacman_strtrim(info->packager);
			} else if(!strcmp(line, "%REASON%")) {
				char tmp[32];
				if(_pacman_db_read_fgets(db, tmp, sizeof(tmp), fp) == NULL) {
					goto error;
				}
				_pacman_strtrim(tmp);
				info->reason = atol(tmp);
			} else if(!strcmp(line, "%SIZE%") || !strcmp(line, "%CSIZE%")) {
				/* NOTE: the CSIZE and SIZE fields both share the "size" field
				 *       in the pkginfo_t struct.  This can be done b/c CSIZE
				 *       is currently only used in sync databases, and SIZE is
				 *       only used in local databases.
				 */
				char tmp[32];
				if(_pacman_db_read_fgets(db, tmp, sizeof(tmp), fp) == NULL) {
					goto error;
				}
				_pacman_strtrim(tmp);
				info->size = atol(tmp);
			} else if(!strcmp(line, "%USIZE%")) {
				/* USIZE (uncompressed size) tag only appears in sync repositories,
				 * not the local one. */
				char tmp[32];
				if(_pacman_db_read_fgets(db, tmp, sizeof(tmp), fp) == NULL) {
					goto error;
				}
				_pacman_strtrim(tmp);
				info->usize = atol(tmp);
			} else if(!strcmp(line, "%SHA1SUM%")) {
				/* SHA1SUM tag only appears in sync repositories,
				 * not the local one. */
				if(_pacman_db_read_fgets(db, info->sha1sum, sizeof(info->sha1sum), fp) == NULL) {
					goto error;
				}
			} else if(!strcmp(line, "%MD5SUM%")) {
				/* MD5SUM tag only appears in sync repositories,
				 * not the local one. */
				if(_pacman_db_read_fgets(db, info->md5sum, sizeof(info->md5sum), fp) == NULL) {
					goto error;
				}
			/* XXX: these are only here as backwards-compatibility for pacman
			 * sync repos.... in pacman-g2, they have been moved to DEPENDS.
			 * Remove this when we move to pacman-g2 repos.
			 */
			} else if(!strcmp(line, "%REPLACES%")) {
				/* the REPLACES tag is special -- it only appears in sync repositories,
				 * not the local one. */
				while(_pacman_db_read_fgets(db, line, sline, fp) && strlen(_pacman_strtrim(line))) {
					info->replaces = _pacman_list_add(info->replaces, strdup(line));
				}
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
		if (fp)
			fclose(fp);
		fp = NULL;
	}

	return(0);

error:
	if(fp) {
		fclose(fp);
	}
	return(-1);
}

static int _pacman_db_read_depends(pmdb_t *db, unsigned int inforeq, pmpkg_t *info)
{
	FILE *fp = NULL;
	char path[PATH_MAX];
	char line[512];
	int sline = sizeof(line)-1;

	if(inforeq & INFRQ_DEPENDS) {
		if (islocal(db)) {
			snprintf(path, PATH_MAX, "%s/%s-%s/depends", db->path, info->name, info->version);
			fp = fopen(path, "r");
			if(fp == NULL) {
				_pacman_log(PM_LOG_WARNING, "%s (%s)", path, strerror(errno));
				goto error;
			}
		}
		while(!islocal(db) || !feof(fp)) {
			if(_pacman_db_read_fgets(db, line, 256, fp) == NULL) {
				break;
			}
			_pacman_strtrim(line);
			if(!strcmp(line, "%DEPENDS%")) {
				while(_pacman_db_read_fgets(db, line, sline, fp) && strlen(_pacman_strtrim(line))) {
					info->depends = _pacman_list_add(info->depends, strdup(line));
				}
			} else if(!strcmp(line, "%REQUIREDBY%")) {
				while(_pacman_db_read_fgets(db, line, sline, fp) && strlen(_pacman_strtrim(line))) {
					info->requiredby = _pacman_list_add(info->requiredby, strdup(line));
				}
			} else if(!strcmp(line, "%CONFLICTS%")) {
				while(_pacman_db_read_fgets(db, line, sline, fp) && strlen(_pacman_strtrim(line))) {
					info->conflicts = _pacman_list_add(info->conflicts, strdup(line));
				}
			} else if(!strcmp(line, "%PROVIDES%")) {
				while(_pacman_db_read_fgets(db, line, sline, fp) && strlen(_pacman_strtrim(line))) {
					info->provides = _pacman_list_add(info->provides, strdup(line));
				}
			} else if(!strcmp(line, "%REPLACES%")) {
				/* the REPLACES tag is special -- it only appears in sync repositories,
				 * not the local one. */
				while(_pacman_db_read_fgets(db, line, sline, fp) && strlen(_pacman_strtrim(line))) {
					info->replaces = _pacman_list_add(info->replaces, strdup(line));
				}
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
		if (fp)
			fclose(fp);
		fp = NULL;
	}

	return(0);

error:
	if(fp) {
		fclose(fp);
	}
	return(-1);
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

	if(db == NULL) {
		RET_ERR(PM_ERR_DB_NULL, -1);
	}

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
				while(fgets(line, sline, fp) && strlen(_pacman_strtrim(line))) {
					if((ptr = strchr(line, '|'))) {
						/* just ignore the content after the pipe for now */
						*ptr = '\0';
					}
					info->files = _pacman_list_add(info->files, strdup(line));
				}
			} else if(!strcmp(line, "%BACKUP%")) {
				while(fgets(line, sline, fp) && strlen(_pacman_strtrim(line))) {
					info->backup = _pacman_list_add(info->backup, strdup(line));
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
void _pacman_db_write_item_list (FILE *fp, const char *item_name, pmlist_t *item_list) {
	if (item_list != NULL) {
		pmlist_t *i = item_list;

		fprintf(fp, "%%%s%%\n", item_name);
		for(i = item_list; i; i = i->next) {
			fprintf(fp, "%s\n", (char *)i->data);
		}
		fprintf(fp, "\n");
	}	
}

int _pacman_db_write(pmdb_t *db, pmpkg_t *info, unsigned int inforeq)
{
	FILE *fp = NULL;
	char path[PATH_MAX];
	mode_t oldmask;
	pmlist_t *lp = NULL;
	int retval = 0;
	int local = 0;

	if(db == NULL || info == NULL) {
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
		fprintf(fp, "%%NAME%%\n%s\n\n"
			"%%VERSION%%\n%s\n\n", info->name, info->version);
		if(info->desc[0]) {
			_pacman_db_write_item_list(fp, "DESC", info->desc_localized);
		}
		_pacman_db_write_item_list(fp, "GROUPS", info->groups);
		if(local) {
			if(info->url[0]) {
				fprintf(fp, "%%URL%%\n"
					"%s\n\n", info->url);
			}
			_pacman_db_write_item_list(fp, "LICENSE", info->license);
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
		_pacman_db_write_item_list(fp, "FILES", info->files);
		_pacman_db_write_item_list(fp, "BACKUP", info->backup);
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
		_pacman_db_write_item_list(fp, "DEPENDS", info->depends);
		if (local) {
			_pacman_db_write_item_list(fp, "REQUIREDBY", info->requiredby);
		}
		_pacman_db_write_item_list(fp, "CONFLICTS", info->conflicts);
		_pacman_db_write_item_list(fp, "PROVIDES", info->provides);
		if(!local) {
			_pacman_db_write_item_list(fp, "REPLACES", info->replaces);
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

int _pacman_db_remove(pmdb_t *db, pmpkg_t *info)
{
	char path[PATH_MAX];

	if(db == NULL || info == NULL) {
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

	if(db == NULL || ts == NULL) {
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
int _pacman_db_setlastupdate(pmdb_t *db, char *ts)
{
	FILE *fp;
	char file[PATH_MAX];

	if(db == NULL || ts == NULL || strlen(ts) == 0) {
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
