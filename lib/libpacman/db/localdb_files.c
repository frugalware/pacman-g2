/*
 *  localdb_files.c
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

static
int _pacman_db_read_lines(FStringList **list, char *s, size_t size, FILE *fp)
{
	int lines = 0;

	while(fgets(s, size, fp) && !_pacman_strempty(_pacman_strtrim(s))) {
		*list = f_stringlist_append(*list, s);
		lines++;
	}
	return lines;
}

int _pacman_localdb_desc_fread(pmpkg_t *info, FILE *fp)
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

int _pacman_localdb_depends_fread(pmpkg_t *info, FILE *fp)
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

int _pacman_localdb_files_fread(pmpkg_t *info, FILE *fp)
{
	char line[512];
	int sline = sizeof(line)-1;

	while(!feof(fp)) {
		if(fgets(line, 256, fp) == NULL) {
			break;
		}
		_pacman_strtrim(line);
		if(!strcmp(line, "%FILES%")) {
			while(fgets(line, sline, fp) && !_pacman_strempty(_pacman_strtrim(line))) {
				char *ptr;

				if((ptr = strchr(line, '|'))) {
					/* just ignore the content after the pipe for now */
					*ptr = '\0';
				}
				info->files = f_stringlist_append(info->files, line);
			}
		} else if(!strcmp(line, "%BACKUP%")) {
			while(fgets(line, sline, fp) && !_pacman_strempty(_pacman_strtrim(line))) {
				info->backup = f_stringlist_append(info->backup, line);
			}
		}
	}
	return 0;
}

/* vim: set ts=2 sw=2 noet: */
