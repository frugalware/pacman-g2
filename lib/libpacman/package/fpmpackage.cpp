/*
 *  fpmpackage.c
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

/* pacman-g2 */
#include "fpmpackage.h"

#include "util.h"
#include "error.h"
#include "db.h"
#include "handle.h"
#include "cache.h"
#include "pacman.h"

#include "io/archive.h"
#include "util/list.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "fstring.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

using namespace libpacman;

/* Parses the pkginfo package description file for the current package
 *
 * Returns: 0 on success, 1 on error
 */
int _pacman_pkginfo_fread(FILE *descfile, Package *info, int output)
{
	char line[PATH_MAX];
	char* ptr = NULL;
	char* key = NULL;
	int linenum = 0;

	while(!feof(descfile)) {
		fgets(line, PATH_MAX, descfile);
		linenum++;
		f_strtrim(line);
		if(_pacman_strempty(line) || line[0] == '#') {
			continue;
		}
		if(output) {
			_pacman_log(PM_LOG_DEBUG, "%s", line);
		}
		ptr = line;
		key = strsep(&ptr, "=");
		if(key == NULL || ptr == NULL) {
			_pacman_log(PM_LOG_DEBUG, _("%s: syntax error in description file line %d"),
				info->name()[0] != '\0' ? info->name() : "error", linenum);
		} else {
			f_strtrim(key);
			key = f_strtoupper(key);
			f_strtrim(ptr);
			if(!strcmp(key, "PKGNAME")) {
				STRNCPY(info->m_name, ptr, sizeof(info->m_name));
			} else if(!strcmp(key, "PKGVER")) {
				STRNCPY(info->m_version, ptr, sizeof(info->m_version));
			} else if(!strcmp(key, "PKGDESC")) {
				info->desc_localized = f_stringlist_append(info->desc_localized, ptr);
				if(_pacman_list_count(info->desc_localized) == 1) {
					STRNCPY(info->desc, ptr, sizeof(info->desc));
				} else if (!strncmp(ptr, handle->language, strlen(handle->language))) {
					STRNCPY(info->desc, ptr+strlen(handle->language)+1, sizeof(info->desc));
				}
			} else if(!strcmp(key, "GROUP")) {
				info->groups = f_stringlist_append(info->groups, ptr);
			} else if(!strcmp(key, "URL")) {
				STRNCPY(info->url, ptr, sizeof(info->url));
			} else if(!strcmp(key, "LICENSE")) {
				info->license = f_stringlist_append(info->license, ptr);
			} else if(!strcmp(key, "BUILDDATE")) {
				STRNCPY(info->builddate, ptr, sizeof(info->builddate));
			} else if(!strcmp(key, "BUILDTYPE")) {
				STRNCPY(info->buildtype, ptr, sizeof(info->buildtype));
			} else if(!strcmp(key, "INSTALLDATE")) {
				STRNCPY(info->installdate, ptr, sizeof(info->installdate));
			} else if(!strcmp(key, "PACKAGER")) {
				STRNCPY(info->packager, ptr, sizeof(info->packager));
			} else if(!strcmp(key, "ARCH")) {
				STRNCPY(info->arch, ptr, sizeof(info->arch));
			} else if(!strcmp(key, "SIZE")) {
				char tmp[32];
				STRNCPY(tmp, ptr, sizeof(tmp));
				info->size = atol(tmp);
			} else if(!strcmp(key, "USIZE")) {
				char tmp[32];
				STRNCPY(tmp, ptr, sizeof(tmp));
				info->usize = atol(tmp);
			} else if(!strcmp(key, "DEPEND")) {
				info->depends = f_stringlist_append(info->depends, ptr);
			} else if(!strcmp(key, "REMOVE")) {
				info->removes = f_stringlist_append(info->removes, ptr);
			} else if(!strcmp(key, "CONFLICT")) {
				info->m_conflicts = f_stringlist_append(info->m_conflicts, ptr);
			} else if(!strcmp(key, "REPLACES")) {
				info->replaces = f_stringlist_append(info->replaces, ptr);
			} else if(!strcmp(key, "PROVIDES")) {
				info->m_provides = f_stringlist_append(info->m_provides, ptr);
			} else if(!strcmp(key, "BACKUP")) {
				info->backup = f_stringlist_append(info->backup, ptr);
			} else if(!strcmp(key, "TRIGGER")) {
				info->triggers = f_stringlist_append(info->triggers, ptr);
			} else {
				_pacman_log(PM_LOG_DEBUG, _("%s: syntax error in description file line %d"),
					info->name()[0] != '\0' ? info->name() : "error", linenum);
			}
		}
		line[0] = '\0';
	}
	return(0);
}

int _pacman_pkginfo_read(char *descfile, Package *info, int output)
{
	FILE* fp = NULL;
	int ret;

	if((fp = fopen(descfile, "r")) == NULL) {
		_pacman_log(PM_LOG_ERROR, _("could not open file %s"), descfile);
		return(-1);
	}

	ret = _pacman_pkginfo_fread(fp, info, output);

	fclose(fp);
	unlink(descfile);

	return ret;
}

Package *_pacman_fpmpackage_load(const char *pkgfile)
{
	char *expath;
	int i, ret;
	int config = 0;
	int filelist = 0;
	int scriptcheck = 0;
	register struct archive *archive;
	struct archive_entry *entry;
	Package *info = NULL;

	if((archive = _pacman_archive_read_open_all_file(pkgfile)) == NULL) {
		RET_ERR(PM_ERR_PKG_OPEN, NULL);
	}
	info = new Package(NULL, NULL);
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
				RET_ERR(PM_ERR_MEMORY, (Package *)-1);
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
	delete info;
	if(!ret) {
		archive_read_finish (archive);
	}
	pm_errno = PM_ERR_PKG_CORRUPTED;

	return(NULL);
}

/* vim: set ts=2 sw=2 noet: */
