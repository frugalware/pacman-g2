/*
 *  package.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005, 2006 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2005, 2006. 2007 by Miklos Vajna <vmiklos@frugalware.org>
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
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <libintl.h>
#include <locale.h>
#include <sys/utsname.h>

/* pacman-g2 */
#include "package.h"

#include "io/archive.h"
#include "package/pkginfo.h"
#include "util/list.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "util.h"
#include "error.h"
#include "db.h"
#include "handle.h"
#include "cache.h"
#include "pacman.h"

pmpkg_t *_pacman_pkg_new(const char *name, const char *version)
{
	pmpkg_t* pkg = NULL;

	if((pkg = (pmpkg_t *)_pacman_zalloc(sizeof(pmpkg_t))) == NULL) {
		return NULL;
	}

	if(!_pacman_strempty(name)) {
		STRNCPY(pkg->name, name, PKG_NAME_LEN);
	}
	if(!_pacman_strempty(version)) {
		STRNCPY(pkg->version, version, PKG_VERSION_LEN);
	}
	pkg->reason = PM_PKG_REASON_EXPLICIT;

	return(pkg);
}

pmpkg_t *_pacman_pkg_new_from_filename(const char *filename, int witharch)
{
	pmpkg_t *pkg;

	ASSERT(pkg = _pacman_pkg_new(NULL, NULL), return NULL);

	if(_pacman_pkg_splitname(filename, pkg->name, pkg->version, witharch) == -1) {
		_pacman_pkg_free(pkg);
		pkg = NULL;
	}
	return pkg;
}

pmpkg_t *_pacman_pkg_dup(pmpkg_t *pkg)
{
	pmpkg_t* newpkg = _pacman_malloc(sizeof(pmpkg_t));

	if(newpkg == NULL) {
		return(NULL);
	}

	STRNCPY(newpkg->name, pkg->name, PKG_NAME_LEN);
	STRNCPY(newpkg->version, pkg->version, PKG_VERSION_LEN);
	STRNCPY(newpkg->desc, pkg->desc, PKG_DESC_LEN);
	STRNCPY(newpkg->url, pkg->url, PKG_URL_LEN);
	STRNCPY(newpkg->builddate, pkg->builddate, PKG_DATE_LEN);
	STRNCPY(newpkg->buildtype, pkg->buildtype, PKG_DATE_LEN);
	STRNCPY(newpkg->installdate, pkg->installdate, PKG_DATE_LEN);
	STRNCPY(newpkg->packager, pkg->packager, PKG_PACKAGER_LEN);
	STRNCPY(newpkg->md5sum, pkg->md5sum, PKG_MD5SUM_LEN);
	STRNCPY(newpkg->sha1sum, pkg->sha1sum, PKG_SHA1SUM_LEN);
	STRNCPY(newpkg->arch, pkg->arch, PKG_ARCH_LEN);
	newpkg->size       = pkg->size;
	newpkg->usize      = pkg->usize;
	newpkg->force      = pkg->force;
	newpkg->stick      = pkg->stick;
	newpkg->scriptlet  = pkg->scriptlet;
	newpkg->reason     = pkg->reason;
	newpkg->license    = _pacman_list_strdup(pkg->license);
	newpkg->desc_localized = _pacman_list_strdup(pkg->desc_localized);
	newpkg->requiredby = _pacman_list_strdup(pkg->requiredby);
	newpkg->conflicts  = _pacman_list_strdup(pkg->conflicts);
	newpkg->files      = _pacman_list_strdup(pkg->files);
	newpkg->backup     = _pacman_list_strdup(pkg->backup);
	newpkg->depends    = _pacman_list_strdup(pkg->depends);
	newpkg->removes    = _pacman_list_strdup(pkg->removes);
	newpkg->groups     = _pacman_list_strdup(pkg->groups);
	newpkg->provides   = _pacman_list_strdup(pkg->provides);
	newpkg->replaces   = _pacman_list_strdup(pkg->replaces);
	newpkg->triggers = _pacman_list_strdup(pkg->triggers);

	/* internal */
	newpkg->origin     = pkg->origin;
	newpkg->data = (newpkg->origin == PKG_FROM_FILE) ? strdup(pkg->data) : pkg->data;
	newpkg->infolevel  = pkg->infolevel;

	return(newpkg);
}

void _pacman_pkg_free(void *data)
{
	pmpkg_t *pkg = data;

	if(pkg == NULL) {
		return;
	}

	FREELIST(pkg->license);
	FREELIST(pkg->desc_localized);
	FREELIST(pkg->files);
	FREELIST(pkg->backup);
	FREELIST(pkg->depends);
	FREELIST(pkg->removes);
	FREELIST(pkg->conflicts);
	FREELIST(pkg->requiredby);
	FREELIST(pkg->groups);
	FREELIST(pkg->provides);
	FREELIST(pkg->replaces);
	FREELIST(pkg->triggers);
	if(pkg->origin == PKG_FROM_FILE) {
		FREE(pkg->data);
	}
	free(pkg);
}

/* Helper function for comparing packages
 */
int _pacman_pkg_cmp(const void *p1, const void *p2)
{
	return(strcmp(((pmpkg_t *)p1)->name, ((pmpkg_t *)p2)->name));
}

int _pacman_pkg_is_valid(const pmpkg_t *pkg, const pmtrans_t *trans, const char *pkgfile)
{
	struct utsname name;

	if(_pacman_strempty(pkg->name)) {
		_pacman_log(PM_LOG_ERROR, _("missing package name in %s"), pkgfile);
		goto pkg_error;
	}
	if(_pacman_strempty(pkg->version)) {
		_pacman_log(PM_LOG_ERROR, _("missing package version in %s"), pkgfile);
		goto pkg_error;
	}
	if(strchr(pkg->version, '-') != strrchr(pkg->version, '-')) {
		_pacman_log(PM_LOG_ERROR, _("version contains additional hyphens in %s"), pkgfile);
		goto invalid_name_error;
	}
	if (trans != NULL && !(trans->flags & PM_TRANS_FLAG_NOARCH)) {
		if(_pacman_strempty(pkg->arch)) {
			_pacman_log(PM_LOG_ERROR, _("missing package architecture in %s"), pkgfile);
			goto pkg_error;
		}

		uname (&name);
		if(strncmp(name.machine, pkg->arch, strlen(pkg->arch))) {
			_pacman_log(PM_LOG_ERROR, _("wrong package architecture in %s"), pkgfile);
			goto arch_error;
		}
	}
	return 0;

invalid_name_error:
	pm_errno = PM_ERR_PKG_INVALID_NAME;
	return -1;

arch_error:
	pm_errno = PM_ERR_WRONG_ARCH;
	return -1;

pkg_error:
	pm_errno = PM_ERR_PKG_INVALID;
	return -1;
}

pmpkg_t *_pacman_pkg_load(const char *pkgfile)
{
	char *expath;
	int i, ret;
	int config = 0;
	int filelist = 0;
	int scriptcheck = 0;
	register struct archive *archive;
	struct archive_entry *entry;
	pmpkg_t *info = NULL;

	if((archive = _pacman_archive_read_open_all_file(pkgfile)) == NULL) {
		RET_ERR(PM_ERR_PKG_OPEN, NULL);
	}
	info = _pacman_pkg_new(NULL, NULL);
	if(info == NULL) {
		archive_read_finish (archive);
		RET_ERR(PM_ERR_MEMORY, NULL);
	}

	for(i = 0; (ret = archive_read_next_header (archive, &entry)) == ARCHIVE_OK; i++) {
		if(config && filelist && scriptcheck) {
			/* we have everything we need */
			break;
		}
		if(!strcmp(archive_entry_pathname (entry), ".PKGINFO")) {
			FILE *pkginfo = _pacman_archive_read_fropen(archive);

			/* parse the info file */
			if(_pacman_pkginfo_fread(pkginfo, info, 0) == -1) {
				_pacman_log(PM_LOG_ERROR, _("could not parse the package description file"));
				pm_errno = PM_ERR_PKG_INVALID;
				fclose(pkginfo);
				goto error;
			}
			fclose(pkginfo);
			if(_pacman_pkg_is_valid(info, handle->trans, pkgfile) != 0) {
				goto error;
			}
			config = 1;
			continue;
		} else if(!strcmp(archive_entry_pathname (entry), "._install") || !strcmp(archive_entry_pathname (entry),  ".INSTALL")) {
			info->scriptlet = 1;
			scriptcheck = 1;
		} else if(!strcmp(archive_entry_pathname (entry), ".FILELIST")) {
			/* Build info->files from the filelist */
			FILE *filelist;
			char *str;

			if((str = (char *)malloc(PATH_MAX)) == NULL) {
				RET_ERR(PM_ERR_MEMORY, (pmpkg_t *)-1);
			}
			filelist = _pacman_archive_read_fropen(archive);
			while(!feof(filelist)) {
				if(fgets(str, PATH_MAX, filelist) == NULL) {
					continue;
				}
				f_strtrim(str);
				info->files = _pacman_stringlist_append(info->files, str);
			}
			FREE(str);
			fclose(filelist);
			continue;
		} else {
			scriptcheck = 1;
			if(!filelist) {
				/* no .FILELIST present in this package..  build the filelist the */
				/* old-fashioned way, one at a time */
				expath = strdup(archive_entry_pathname (entry));
				info->files = _pacman_list_add(info->files, expath);
			}
		}

		if(archive_read_data_skip (archive)) {
			_pacman_log(PM_LOG_ERROR, _("bad package file in %s"), pkgfile);
			goto error;
		}
		expath = NULL;
	}
	archive_read_finish (archive);

	if(!config) {
		_pacman_log(PM_LOG_ERROR, _("missing package info file in %s"), pkgfile);
		goto error;
	}

	/* internal */
	info->origin = PKG_FROM_FILE;
	info->data = strdup(pkgfile);
	info->infolevel = 0xFF;

	return(info);

error:
	FREEPKG(info);
	if(!ret) {
		archive_read_finish (archive);
	}
	pm_errno = PM_ERR_PKG_CORRUPTED;

	return(NULL);
}

/* Test for existence of a package in a pmlist_t*
 * of pmpkg_t*
 */
pmpkg_t *_pacman_pkg_isin(const char *needle, pmlist_t *haystack)
{
	pmlist_t *lp;

	if(needle == NULL || haystack == NULL) {
		return(NULL);
	}

	for(lp = haystack; lp; lp = lp->next) {
		pmpkg_t *info = lp->data;

		if(info && !strcmp(info->name, needle)) {
			return(lp->data);
		}
	}
	return(NULL);
}

int _pacman_pkg_splitname(const char *target, char *name, char *version, int witharch)
{
	char *tmp;
	char *p, *q;

	if ((tmp = _pacman_basename(target)) == NULL) {
		return -1;
	}
	/* trim file extension (if any) */
	if((p = strstr(tmp, PM_EXT_PKG))) {
		*p = 0;
	}
	if(witharch) {
		/* trim architecture */
		if((p = strrchr(tmp, '-'))) {
			*p = 0;
		}
	}

	p = tmp + strlen(tmp);

	for(q = --p; *q && *q != '-'; q--);
	if(*q != '-' || q == tmp) {
		return(-1);
	}
	for(p = --q; *p && *p != '-'; p--);
	if(*p != '-' || p == tmp) {
		return(-1);
	}
	if(version) {
		STRNCPY(version, p+1, PKG_VERSION_LEN);
	}
	*p = 0;

	if(name) {
		STRNCPY(name, tmp, PKG_NAME_LEN);
	}

	return(0);
}

void *_pacman_pkg_getinfo(pmpkg_t *pkg, unsigned char parm)
{
	void *data = NULL;

	/* Update the cache package entry if needed */
	if(pkg->origin == PKG_FROM_CACHE) {
		switch(parm) {
			/* Desc entry */
			case PM_PKG_DESC:
			case PM_PKG_GROUPS:
			case PM_PKG_URL:
			case PM_PKG_LICENSE:
			case PM_PKG_ARCH:
			case PM_PKG_BUILDDATE:
			case PM_PKG_INSTALLDATE:
			case PM_PKG_PACKAGER:
			case PM_PKG_SIZE:
			case PM_PKG_USIZE:
			case PM_PKG_REASON:
			case PM_PKG_MD5SUM:
			case PM_PKG_SHA1SUM:
			case PM_PKG_REPLACES:
			case PM_PKG_FORCE:
				if(!(pkg->infolevel & INFRQ_DESC)) {
					_pacman_log(PM_LOG_DEBUG, _("loading DESC info for '%s'"), pkg->name);
					_pacman_db_read(pkg->data, pkg, INFRQ_DESC);
				}
			break;
			/* Depends entry */
			case PM_PKG_DEPENDS:
			case PM_PKG_REQUIREDBY:
			case PM_PKG_CONFLICTS:
			case PM_PKG_PROVIDES:
				if(!(pkg->infolevel & INFRQ_DEPENDS)) {
					_pacman_log(PM_LOG_DEBUG, "loading DEPENDS info for '%s'", pkg->name);
					_pacman_db_read(pkg->data, pkg, INFRQ_DEPENDS);
				}
			break;
			/* Files entry */
			case PM_PKG_FILES:
			case PM_PKG_BACKUP:
				if(pkg->data == handle->db_local && !(pkg->infolevel & INFRQ_FILES)) {
					_pacman_log(PM_LOG_DEBUG, _("loading FILES info for '%s'"), pkg->name);
					_pacman_db_read(pkg->data, pkg, INFRQ_FILES);
				}
			break;
			/* Scriptlet */
			case PM_PKG_SCRIPLET:
				if(pkg->data == handle->db_local && !(pkg->infolevel & INFRQ_SCRIPLET)) {
					_pacman_log(PM_LOG_DEBUG, _("loading SCRIPLET info for '%s'"), pkg->name);
					_pacman_db_read(pkg->data, pkg, INFRQ_SCRIPLET);
				}
			break;
		}
	}

	switch(parm) {
		case PM_PKG_NAME:        data = pkg->name; break;
		case PM_PKG_VERSION:     data = pkg->version; break;
		case PM_PKG_DESC:        data = pkg->desc; break;
		case PM_PKG_GROUPS:      data = pkg->groups; break;
		case PM_PKG_URL:         data = pkg->url; break;
		case PM_PKG_ARCH:        data = pkg->arch; break;
		case PM_PKG_BUILDDATE:   data = pkg->builddate; break;
		case PM_PKG_BUILDTYPE:   data = pkg->buildtype; break;
		case PM_PKG_INSTALLDATE: data = pkg->installdate; break;
		case PM_PKG_PACKAGER:    data = pkg->packager; break;
		case PM_PKG_SIZE:        data = (void *)(long)pkg->size; break;
		case PM_PKG_USIZE:       data = (void *)(long)pkg->usize; break;
		case PM_PKG_REASON:      data = (void *)(long)pkg->reason; break;
		case PM_PKG_LICENSE:     data = pkg->license; break;
		case PM_PKG_REPLACES:    data = pkg->replaces; break;
		case PM_PKG_FORCE:       data = (void *)(long)pkg->force; break;
		case PM_PKG_STICK:       data = (void *)(long)pkg->stick; break;
		case PM_PKG_MD5SUM:      data = pkg->md5sum; break;
		case PM_PKG_SHA1SUM:     data = pkg->sha1sum; break;
		case PM_PKG_DEPENDS:     data = pkg->depends; break;
		case PM_PKG_REMOVES:     data = pkg->removes; break;
		case PM_PKG_REQUIREDBY:  data = pkg->requiredby; break;
		case PM_PKG_PROVIDES:    data = pkg->provides; break;
		case PM_PKG_CONFLICTS:   data = pkg->conflicts; break;
		case PM_PKG_FILES:       data = pkg->files; break;
		case PM_PKG_BACKUP:      data = pkg->backup; break;
		case PM_PKG_SCRIPLET:    data = (void *)(long)pkg->scriptlet; break;
		case PM_PKG_DATA:        data = pkg->data; break;
		case PM_PKG_TRIGGERS:    data = pkg->triggers; break;
		default:
			data = NULL;
		break;
	}

	return(data);
}

pmlist_t *_pacman_pkg_getowners(char *filename)
{
	struct stat buf;
	int gotcha = 0;
	char rpath[PATH_MAX];
	pmlist_t *lp, *ret = NULL;

	if(stat(filename, &buf) == -1 || realpath(filename, rpath) == NULL) {
		RET_ERR(PM_ERR_PKG_OPEN, NULL);
	}

	if(S_ISDIR(buf.st_mode)) {
		/* this is a directory and the db has a / suffix for dirs - add it here so
		 * that we'll find dirs, too */
		rpath[strlen(rpath)+1] = '\0';
		rpath[strlen(rpath)] = '/';
	}

	for(lp = _pacman_db_get_pkgcache(handle->db_local); lp; lp = lp->next) {
		pmpkg_t *info;
		pmlist_t *i;

		info = lp->data;

		for(i = _pacman_pkg_getinfo(info, PM_PKG_FILES); i; i = i->next) {
			char path[PATH_MAX];

			snprintf(path, PATH_MAX, "%s%s", handle->root, (char *)i->data);
			if(!strcmp(path, rpath)) {
				ret = _pacman_list_add(ret, info);
				if(rpath[strlen(rpath)-1] != '/') {
					/* we are searching for a file and multiple packages won't contain
					 * the same file */
					return(ret);
				}
				gotcha = 1;
			}
		}
	}
	if(!gotcha) {
		RET_ERR(PM_ERR_NO_OWNER, NULL);
	}

	return(ret);
}

int _pacman_pkg_filename(char *str, size_t size, const pmpkg_t *pkg)
{
	return snprintf(str, size, "%s-%s-%s%s",
			pkg->name, pkg->version, pkg->arch, PM_EXT_PKG);
}

/* Look for a filename in a pmpkg_t.backup list.  If we find it,
 * then we return the md5 or sha1 hash (parsed from the same line)
 */
char *_pacman_pkg_fileneedbackup(const pmpkg_t *pkg, const char *file)
{
	const pmlist_t *lp;

	ASSERT(pkg != NULL, RET_ERR(PM_ERR_PKG_INVALID, NULL)); // Should be PM_ERR_PKG_NULL
	ASSERT(!_pacman_strempty(file), RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	/* run through the backup list and parse out the md5 or sha1 hash for our file */
	for(lp = pkg->backup; lp; lp = lp->next) {
		char *str = strdup(lp->data);
		char *ptr;

		/* tab delimiter */
		ptr = strchr(str, '\t');
		if(ptr == NULL) {
			free(str);
			continue;
		}
		*ptr = '\0';
		ptr++;
		/* now str points to the filename and ptr points to the md5 or sha1 hash */
		if(!strcmp(file, str)) {
			char *hash = strdup(ptr);
			free(str);
			return hash;
		}
		free(str);
	}

	return(NULL);
}

/* vim: set ts=2 sw=2 noet: */
