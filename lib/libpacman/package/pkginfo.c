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

#include "util/list.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "util.h"
#include "error.h"
#include "db.h"
#include "handle.h"
#include "cache.h"
#include "pacman.h"

/* Parses the pkginfo package description file for the current package
 *
 * Returns: 0 on success, 1 on error
 */
int _pacman_pkginfo_fread(FILE *descfile, pmpkg_t *info, int output)
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
				info->name[0] != '\0' ? info->name : "error", linenum);
		} else {
			f_strtrim(key);
			key = f_strtoupper(key);
			f_strtrim(ptr);
			if(!strcmp(key, "PKGNAME")) {
				STRNCPY(info->name, ptr, sizeof(info->name));
			} else if(!strcmp(key, "PKGVER")) {
				STRNCPY(info->version, ptr, sizeof(info->version));
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
				info->conflicts = f_stringlist_append(info->conflicts, ptr);
			} else if(!strcmp(key, "REPLACES")) {
				info->replaces = f_stringlist_append(info->replaces, ptr);
			} else if(!strcmp(key, "PROVIDES")) {
				info->provides = f_stringlist_append(info->provides, ptr);
			} else if(!strcmp(key, "BACKUP")) {
				info->backup = f_stringlist_append(info->backup, ptr);
			} else if(!strcmp(key, "TRIGGER")) {
				info->triggers = f_stringlist_append(info->triggers, ptr);
			} else {
				_pacman_log(PM_LOG_DEBUG, _("%s: syntax error in description file line %d"),
					info->name[0] != '\0' ? info->name : "error", linenum);
			}
		}
		line[0] = '\0';
	}
	return(0);
}

int _pacman_pkginfo_read(char *descfile, pmpkg_t *info, int output)
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

/* vim: set ts=2 sw=2 noet: */
