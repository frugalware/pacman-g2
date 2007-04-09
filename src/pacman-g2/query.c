/*
 *  query.c
 * 
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2007 by Miklos Vajna <vmiklos@frugalware.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <libintl.h>

#include <pacman.h>
/* pacman-g2 */
#include "list.h"
#include "package.h"
#include "query.h"
#include "log.h"
#include "conf.h"
#include "sync.h"
#include "util.h"

extern config_t *config;
extern PM_DB *db_local;
extern list_t *pmc_syncs;

static int query_fileowner(PM_DB *db, char *filename)
{
	struct stat buf;
	int gotcha = 0;
	char rpath[PATH_MAX];
	PM_LIST *lp;
	char *root;

	if(db == NULL) {
		return(0);
	}
	if(filename == NULL || strlen(filename) == 0) {
		ERR(NL, _("no file was specified for --owns\n"));
		return(1);
	}

	if(stat(filename, &buf) == -1 || realpath(filename, rpath) == NULL) {
		ERR(NL, _("can't find %s.\n"), filename);
		return(1);
	}

	if(S_ISDIR(buf.st_mode)) {
		/* this is a directory and the db has a / suffix for dirs - add it here so
		 * that we'll find dirs, too */
		rpath[strlen(rpath)] = '/';
		rpath[strlen(rpath)+1] = '\0';
	}

	pacman_get_option(PM_OPT_ROOT, (long *)&root);

	for(lp = pacman_db_getpkgcache(db); lp; lp = pacman_list_next(lp)) {
		PM_PKG *info;
		PM_LIST *i;

		info = pacman_list_getdata(lp);

		for(i = pacman_pkg_getinfo(info, PM_PKG_FILES); i; i = pacman_list_next(i)) {
			char path[PATH_MAX];

			snprintf(path, PATH_MAX, "%s%s", root, (char *)pacman_list_getdata(i));
			if(!strcmp(path, rpath)) {
				printf(_("%s %s is an owner of %s\n"), (char *)pacman_pkg_getinfo(info, PM_PKG_NAME),
				       (char *)pacman_pkg_getinfo(info, PM_PKG_VERSION), filename);
				if(rpath[strlen(rpath)-1] != '/') {
					/* we are searching for a file and multiple packages won't contain
					 * the same file */
					return(0);
				}
				gotcha = 1;
			}
		}
	}
	if(!gotcha) {
		ERR(NL, _("No package owns %s\n"), filename);
		return(1);
	}

	return(0);
}

int querypkg(list_t *targets)
{
	PM_PKG *info = NULL;
	list_t *targ;
	list_t *i;
	PM_LIST *j, *ret;
	char *package = NULL;
	int done = 0, errors = 0;

	if(config->op_q_search) {
		for(i = targets; i; i = i->next) {
			pacman_set_option(PM_OPT_NEEDLES, (long)i->data);
		}
		ret = pacman_db_search(db_local);
		if(ret == NULL) {
			return(1);
		}
		for(j = ret; j; j = pacman_list_next(j)) {
			PM_PKG *pkg = pacman_list_getdata(j);

			printf("local/%s/%s %s\n    ",
					(char*)pacman_list_getdata(pacman_pkg_getinfo(pkg, PM_PKG_GROUPS)),
					(char *)pacman_pkg_getinfo(pkg, PM_PKG_NAME),
					(char *)pacman_pkg_getinfo(pkg, PM_PKG_VERSION));
			indentprint((char *)pacman_pkg_getinfo(pkg, PM_PKG_DESC), 4);
			printf("\n");
		}
		return(0);
	}

	if(config->op_q_foreign) {
		if(pmc_syncs == NULL || !list_count(pmc_syncs)) {
			ERR(NL, _("no usable package repositories configured.\n"));
			return(1);
		}
	}

	for(targ = targets; !done; targ = (targ ? targ->next : NULL)) {
		if(targets == NULL) {
			done = 1;
		} else {
			if(targ->next == NULL) {
				done = 1;
			}
			package = targ->data;
		}

		/* looking for groups */
		if(config->group) {
			PM_LIST *lp;
			if(targets == NULL) {
				for(lp = pacman_db_getgrpcache(db_local); lp; lp = pacman_list_next(lp)) {
					PM_GRP *grp = pacman_list_getdata(lp);
					PM_LIST *i, *pkgnames;
					char *grpname;

					grpname = pacman_grp_getinfo(grp, PM_GRP_NAME);
					pkgnames = pacman_grp_getinfo(grp, PM_GRP_PKGNAMES);

					for(i = pkgnames; i; i = pacman_list_next(i)) {
						MSG(NL, "%s %s\n", grpname, (char *)pacman_list_getdata(i));
					}
				}
			} else {
				PM_GRP *grp = pacman_db_readgrp(db_local, package);
				if(grp) {
					PM_LIST *i, *pkgnames = pacman_grp_getinfo(grp, PM_GRP_PKGNAMES);
					for(i = pkgnames; i; i = pacman_list_next(i)) {
						MSG(NL, "%s %s\n", package, (char *)pacman_list_getdata(i));
					}
				} else {
					ERR(NL, _("group \"%s\" was not found\n"), package);
					return(2);
				}
			}
			continue;
		}

		/* output info for a .tar.gz package */
		if(config->op_q_isfile) {
			if(package == NULL) {
				ERR(NL, _("no package file was specified for --file\n"));
				return(1);
			}
			if(pacman_pkg_load(package, &info) == -1) {
				ERR(NL, _("failed to load package '%s' (%s)\n"), package, pacman_strerror(pm_errno));
				return(1);
			}
			if(config->op_q_info) {
				dump_pkg_full(info, 0);
				MSG(NL, "\n");
			}
			if(config->op_q_list) {
				dump_pkg_files(info);
			}
			if(!config->op_q_info && !config->op_q_list) {
				MSG(NL, "%s %s\n", (char *)pacman_pkg_getinfo(info, PM_PKG_NAME),
				                   (char *)pacman_pkg_getinfo(info, PM_PKG_VERSION));
			}
			FREEPKG(info);
			continue;
		}

		/* determine the owner of a file */
		if(config->op_q_owns) {
			errors += query_fileowner(db_local, package);
			continue;
		}

		/* find packages in the db */
		if(package == NULL) {
			PM_LIST *lp;
			/* no target */
			for(lp = pacman_db_getpkgcache(db_local); lp; lp = pacman_list_next(lp)) {
				PM_PKG *tmpp = pacman_list_getdata(lp);
				char *pkgname, *pkgver;

				pkgname = pacman_pkg_getinfo(tmpp, PM_PKG_NAME);
				pkgver = pacman_pkg_getinfo(tmpp, PM_PKG_VERSION);

				if(config->op_q_list || config->op_q_orphans || config->op_q_foreign) {
					info = pacman_db_readpkg(db_local, pkgname);
					if(info == NULL) {
						/* something weird happened */
						ERR(NL, _("package \"%s\" not found\n"), pkgname);
						return(1);
					}
					if(config->op_q_foreign) {
						int match = 0;
						for(i = pmc_syncs; i; i = i->next) {
							sync_t *sync = (sync_t *)i->data;
							for(j = pacman_db_getpkgcache(sync->db); j; j = pacman_list_next(j)) {
								PM_PKG *pkg = pacman_list_getdata(j);
								char *haystack;
								char *needle;
								haystack = strdup(pacman_pkg_getinfo(pkg, PM_PKG_NAME));
								needle = strdup(pacman_pkg_getinfo(info, PM_PKG_NAME));
								if(!strcmp(haystack, needle)) {
									match = 1;
								}
								FREE(haystack);
								FREE(needle);
							}
						}
						if(match==0) {
							MSG(NL, "%s %s\n", pkgname, pkgver);
						}
					}
					if(config->op_q_list) {
						dump_pkg_files(info);
					}
					if(config->op_q_orphans) {
						if(pacman_pkg_getinfo(info, PM_PKG_REQUIREDBY) == NULL
						   && (long)pacman_pkg_getinfo(info, PM_PKG_REASON) == PM_PKG_REASON_DEPEND) {
							MSG(NL, "%s %s\n", pkgname, pkgver);
						}
					} 
				} else {
					MSG(NL, "%s %s\n", pkgname, pkgver);
				}
			}
		} else {
			char *pkgname, *pkgver, changelog[PATH_MAX];

			info = pacman_db_readpkg(db_local, package);
			if(info == NULL) {
				ERR(NL, _("package \"%s\" not found\n"), package);
				return(2);
			}

			/* find a target */
			if(config->op_q_changelog || config->op_q_info || config->op_q_list) {
				if(config->op_q_changelog) {
					char *dbpath;
					pacman_get_option(PM_OPT_DBPATH, (long *)&dbpath);
					snprintf(changelog, PATH_MAX, "%s%s/%s/%s-%s/changelog",
						config->root, dbpath,
						(char*)pacman_db_getinfo(db_local, PM_DB_TREENAME),
						(char*)pacman_pkg_getinfo(info, PM_PKG_NAME),
						(char*)pacman_pkg_getinfo(info, PM_PKG_VERSION));
					dump_pkg_changelog(changelog, (char*)pacman_pkg_getinfo(info, PM_PKG_NAME));
				}
				if(config->op_q_info) {
					dump_pkg_full(info, config->op_q_info);
				}
				if(config->op_q_list) {
					dump_pkg_files(info);
				}
			} else if(config->op_q_orphans) {
					if(pacman_pkg_getinfo(info, PM_PKG_REQUIREDBY) == NULL) {
						MSG(NL, "%s %s\n", pkgname, pkgver);
					}
			} else {
				pkgname = pacman_pkg_getinfo(info, PM_PKG_NAME);
				pkgver = pacman_pkg_getinfo(info, PM_PKG_VERSION);
				MSG(NL, "%s %s\n", pkgname, pkgver);
			}
		}
	}

	return(errors);
}

/* vim: set ts=2 sw=2 noet: */
