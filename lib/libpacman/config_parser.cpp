/*
 *  config_parser.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005 by Christian Hamar <krics@linuxforum.hu>
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

#include <limits.h> /* PATH_MAX */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <stdarg.h>

/* pacman-g2 */
#include "pacman_p.h"

#include "db/syncdb.h"
#include "hash/md5.h"
#include "hash/sha1.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "fstring.h"
#include "error.h"
#include "deps.h"
#include "versioncmp.h"
#include "package.h"
#include "group.h"
#include "util.h"
#include "db.h"
#include "cache.h"
#include "conflict.h"
#include "handle.h"
#include "server.h"

#define min(X, Y)  ((X) < (Y) ? (X) : (Y))

using namespace libpacman;

int _pacman_parse_config(Handle *handle, const char *file, pacman_cb_db_register callback, const char *this_section)
{
	FILE *fp = NULL;
	char line[PATH_MAX+1];
	char *ptr = NULL;
	char *key = NULL;
	int linenum = 0;
	char section[256] = "";
	Database *db = NULL;

	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	fp = fopen(file, "r");
	if(fp == NULL) {
		return(0);
	}

	if(!_pacman_strempty(this_section)) {
		strncpy(section, this_section, min(255, strlen(this_section)));
		if(!strcmp(section, "local")) {
			RET_ERR(PM_ERR_CONF_LOCAL, -1);
		}
		if(strcmp(section, "options")) {
			db = handle->createDatabase(section, callback);
		}
	} else {
		FREELIST(handle->ignorepkg);
		FREELIST(handle->holdpkg);
	}

	while(fgets(line, PATH_MAX, fp)) {
		linenum++;
		f_strtrim(line);
		if(line[0] == '\0' || line[0] == '#') {
			continue;
		}
		if((ptr = strchr(line, '#'))) {
			*ptr = '\0';
		}
		if(line[0] == '[' && line[strlen(line)-1] == ']') {
			/* new config section */
			ptr = line;
			ptr++;
			strncpy(section, ptr, min(255, strlen(ptr)-1));
			section[min(255, strlen(ptr)-1)] = '\0';
			_pacman_log(PM_LOG_DEBUG, _("config: new section '%s'\n"), section);
			if(!strlen(section)) {
				RET_ERR(PM_ERR_CONF_BAD_SECTION, -1);
			}
			if(!strcmp(section, "local")) {
				RET_ERR(PM_ERR_CONF_LOCAL, -1);
			}
			if(strcmp(section, "options")) {
				db = handle->createDatabase(section, callback);
				if(db == NULL) {
					/* pm_errno is set by pacman_db_register */
					return(-1);
				}
			}
		} else {
			/* directive */
			ptr = line;
			key = strsep(&ptr, "=");
			if(key == NULL) {
				RET_ERR(PM_ERR_CONF_BAD_SYNTAX, -1);
			}
			f_strtrim(key);
			key = f_strtoupper(key);
			if(!strlen(section) && strcmp(key, "INCLUDE")) {
				RET_ERR(PM_ERR_CONF_DIRECTIVE_OUTSIDE_SECTION, -1);
			}
			if(ptr == NULL) {
				if(!strcmp(key, "NOPASSIVEFTP")) {
					pacman_set_option(PM_OPT_NOPASSIVEFTP, (long)1);
				} else if(!strcmp(key, "USESYSLOG")) {
					pacman_set_option(PM_OPT_USESYSLOG, (long)1);
					_pacman_log(PM_LOG_DEBUG, _("config: usesyslog\n"));
				} else if(!strcmp(key, "ILOVECANDY")) {
					pacman_set_option(PM_OPT_CHOMP, (long)1);
				} else {
					RET_ERR(PM_ERR_CONF_BAD_SYNTAX, -1);
				}
			} else {
				f_strtrim(ptr);
				if(!strcmp(key, "INCLUDE")) {
					char conf[PATH_MAX];
					strncpy(conf, ptr, PATH_MAX);
					_pacman_log(PM_LOG_DEBUG, _("config: including %s\n"), conf);
					_pacman_parse_config(handle, conf, callback, section);
				} else if(!strcmp(section, "options")) {
					if(!strcmp(key, "NOUPGRADE")) {
						char *p = ptr;
						char *q;
						while((q = strchr(p, ' '))) {
							*q = '\0';
							if(pacman_set_option(PM_OPT_NOUPGRADE, (long)p) == -1) {
								/* pm_errno is set by pacman_set_option */
								return(-1);
							}
							_pacman_log(PM_LOG_DEBUG, _("config: noupgrade: %s\n"), p);
							p = q;
							p++;
						}
						if(pacman_set_option(PM_OPT_NOUPGRADE, (long)p) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
						_pacman_log(PM_LOG_DEBUG, _("config: noupgrade: %s\n"), p);
					} else if(!strcmp(key, "NOEXTRACT")) {
						char *p = ptr;
						char *q;
						while((q = strchr(p, ' '))) {
							*q = '\0';
							if(pacman_set_option(PM_OPT_NOEXTRACT, (long)p) == -1) {
								/* pm_errno is set by pacman_set_option */
								return(-1);
							}
							_pacman_log(PM_LOG_DEBUG, _("config: noextract: %s\n"), p);
							p = q;
							p++;
						}
						if(pacman_set_option(PM_OPT_NOEXTRACT, (long)p) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
						_pacman_log(PM_LOG_DEBUG, _("config: noextract: %s\n"), p);
					} else if(!strcmp(key, "IGNOREPKG")) {
						char *p = ptr;
						char *q;
						while((q = strchr(p, ' '))) {
							*q = '\0';
							if(pacman_set_option(PM_OPT_IGNOREPKG, (long)p) == -1) {
								/* pm_errno is set by pacman_set_option */
								return(-1);
							}
							_pacman_log(PM_LOG_DEBUG, _("config: ignorepkg: %s\n"), p);
							p = q;
							p++;
						}
						if(pacman_set_option(PM_OPT_IGNOREPKG, (long)p) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
						_pacman_log(PM_LOG_DEBUG, _("config: ignorepkg: %s\n"), p);
					} else if(!strcmp(key, "HOLDPKG")) {
						char *p = ptr;
						char *q;
						while((q = strchr(p, ' '))) {
							*q = '\0';
							if(pacman_set_option(PM_OPT_HOLDPKG, (long)p) == -1) {
								/* pm_errno is set by pacman_set_option */
								return(-1);
							}
							_pacman_log(PM_LOG_DEBUG, _("config: holdpkg: %s\n"), p);
							p = q;
							p++;
						}
						if(pacman_set_option(PM_OPT_HOLDPKG, (long)p) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
						_pacman_log(PM_LOG_DEBUG, _("config: holdpkg: %s\n"), p);
					} else if(!strcmp(key, "DBPATH")) {
						/* shave off the leading slash, if there is one */
						if(*ptr == '/') {
							ptr++;
						}
						if(pacman_set_option(PM_OPT_DBPATH, (long)ptr) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
						_pacman_log(PM_LOG_DEBUG, _("config: dbpath: %s\n"), ptr);
					} else if(!strcmp(key, "CACHEDIR")) {
						/* shave off the leading slash, if there is one */
						if(*ptr == '/') {
							ptr++;
						}
						if(pacman_set_option(PM_OPT_CACHEDIR, (long)ptr) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
						_pacman_log(PM_LOG_DEBUG, _("config: cachedir: %s\n"), ptr);
					} else if (!strcmp(key, "LOGFILE")) {
						if(pacman_set_option(PM_OPT_LOGFILE, (long)ptr) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
						_pacman_log(PM_LOG_DEBUG, _("config: log file: %s\n"), ptr);
					} else if (!strcmp(key, "XFERCOMMAND")) {
						if(pacman_set_option(PM_OPT_XFERCOMMAND, (long)ptr) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
					} else if (!strcmp(key, "UPGRADEDELAY")) {
						/* The config value is in days, we use seconds */
						_pacman_log(PM_LOG_DEBUG, _("config: UpgradeDelay: %i\n"), (60*60*24) * atol(ptr));
						if(pacman_set_option(PM_OPT_UPGRADEDELAY, (60*60*24) * atol(ptr)) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
					} else if (!strcmp(key, "OLDDELAY")) {
						/* The config value is in days, we use seconds */
						_pacman_log(PM_LOG_DEBUG, _("config: OldDelay: %i\n"), (60*60*24) * atol(ptr));
						if(pacman_set_option(PM_OPT_OLDDELAY, (60*60*24) * atol(ptr)) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
					} else if (!strcmp(key, "PROXYSERVER")) {
						if(pacman_set_option(PM_OPT_PROXYHOST, (long)ptr) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
					} else if (!strcmp(key, "PROXYPORT")) {
						if(pacman_set_option(PM_OPT_PROXYPORT, (long)atoi(ptr)) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
					} else if (!strcmp(key, "MAXTRIES")) {
						if(pacman_set_option(PM_OPT_MAXTRIES, (long)atoi(ptr)) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
					} else {
						RET_ERR(PM_ERR_CONF_BAD_SYNTAX, -1);
					}
				} else {
					if(!strcmp(key, "SERVER")) {
						/* add to the list */
						_pacman_log(PM_LOG_DEBUG, _("config: %s: server: %s\n"), section, ptr);
						if(!db->add_server(ptr)) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
					} else {
						RET_ERR(PM_ERR_CONF_BAD_SYNTAX, -1);
					}
				}
				line[0] = '\0';
			}
		}
	}
	fclose(fp);

	return(0);
}

/* @} */

/* vim: set ts=2 sw=2 noet: */
