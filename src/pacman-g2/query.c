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

#include "pacman-g2.h"
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

int querypkg(FStringList *targets)
{
	PM_PKG *info = NULL;
	PM_LIST *ret;
	int errors = 0;

	if(config->op_q_search) {
		for(FPtrListIterator *i = f_ptrlist_first(targets), *end = f_ptrlist_end(targets); i != end; i = f_ptrlistitem_next(i)) {
			pacman_set_option(PM_OPT_NEEDLES, (long)list_data(i));
		}
		ret = pacman_db_search(db_local);
		if(pacman_list_count(ret) == 0) {
			return(1);
		}
		for(pmlist_iterator_t *j = pacman_list_begin(ret), *end = pacman_list_end(ret); j != end; j = pacman_list_next(j)) {
			PM_PKG *pkg = pacman_list_getdata(j);

			printf("local/%s %s-%s [Desc: %s]\n",
					(char *)pacman_list_getdata(pacman_list_begin(pacman_pkg_getinfo(pkg, PM_PKG_GROUPS))),
					(char *)pacman_pkg_getinfo(pkg, PM_PKG_NAME),
					(char *)pacman_pkg_getinfo(pkg, PM_PKG_VERSION),
					(char *)pacman_pkg_getinfo(pkg, PM_PKG_DESC));
		}
		return(0);
	}

	if(config->op_q_test) {
		ret = pacman_db_test(db_local);
		if(ret == NULL) {
			printf(_(":: the database seems to be consistent\n"));
			return(0);
		}
		for(pmlist_iterator_t *j = pacman_list_begin(ret), *end = pacman_list_end(ret); j != end; j = pacman_list_next(j)) {
			char *str = pacman_list_getdata(j);
			printf(":: %s\n", str);
		}
		return(0);
	}

	if(config->op_q_foreign && !trans_has_usable_syncs()) {
		return(1);
	}

	for(FPtrListIterator *targ = f_ptrlist_first(targets), *end = f_ptrlist_end(targets); targ != end; targ = f_ptrlistitem_next(targ)) {
		char *package = list_data(targ);

		/* looking for groups */
		if(config->group) {
			if(targets == NULL) {
				pmlist_t *cache = pacman_db_getgrpcache(db_local);
				for(pmlist_iterator_t *lp = pacman_list_begin(cache), *end = pacman_list_end(cache); lp != end; lp = pacman_list_next(lp)) {
					PM_GRP *grp = pacman_list_getdata(lp);
					char *grpname = pacman_grp_getinfo(grp, PM_GRP_NAME);
					pmlist_t *pkgnames = pacman_grp_getinfo(grp, PM_GRP_PKGNAMES);

					for(pmlist_iterator_t *lq = pacman_list_begin(pkgnames), *end = pacman_list_end(pkgnames); lq != end; lq = pacman_list_next(lq)) {
						MSG(NL, "%s %s\n", grpname, (char *)pacman_list_getdata(lq));
					}
				}
			} else {
				PM_GRP *grp = pacman_db_readgrp(db_local, package);
				if(grp) {
					pmlist_t *pkgnames = pacman_grp_getinfo(grp, PM_GRP_PKGNAMES);
					for(pmlist_iterator_t *lq = pacman_list_begin(pkgnames), *end = pacman_list_end(pkgnames); lq != end; lq = pacman_list_next(lq)) {
						MSG(NL, "%s %s\n", package, (char *)pacman_list_getdata(lq));
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
			if((info = pacman_pkg_load(package)) == NULL) {
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
				PM_LIST *data;
			if((data = pacman_pkg_getowners(package)) == NULL) {
				ERR(NL, _("No package owns %s\n"), package);
				errors++;
			} else {
				for(pmlist_iterator_t *lp = pacman_list_begin(data), *end = pacman_list_end(data); lp != end; lp = pacman_list_next(lp)) {
					PM_PKG *pkg = pacman_list_getdata(lp);
					printf(_("%s %s is an owner of %s\n"), (char *)pacman_pkg_getinfo(pkg, PM_PKG_NAME),
							(char *)pacman_pkg_getinfo(pkg, PM_PKG_VERSION), package);
				}
			}
			continue;
		}

		/* find packages in the db */
		if(package == NULL) {
			/* Do not allow -Qc , -Qi , -Ql without package arg .. */
			if(config->op_q_changelog || config->op_q_info || config->op_q_list) {
				ERR(NL, _("This query option require an package name as argument\n"));
				return(1);
			}

			/* -Qd is not valid */
			if(config->op_q_orphans_deps && !config->op_q_orphans) {
				ERR(NL, _("Invalid query option , use 'pacman-g2 -Qed'\n"));
				return(1);
			}

			/* no target */
			pmlist_t *cache = pacman_db_getpkgcache(db_local);
			for(pmlist_iterator_t *lp = pacman_list_begin(cache), *end = pacman_list_end(cache); lp != end; lp = pacman_list_next(lp)) {
				PM_PKG *tmpp = pacman_list_getdata(lp);
				char *pkgname, *pkgver;

				pkgname = pacman_pkg_getinfo(tmpp, PM_PKG_NAME);
				pkgver = pacman_pkg_getinfo(tmpp, PM_PKG_VERSION);
				if(config->op_q_orphans || config->op_q_foreign || config->op_q_fsck) {
					info = pacman_db_readpkg(db_local, pkgname);
					if(info == NULL) {
						/* something weird happened */
						ERR(NL, _("package \"%s\" not found\n"), pkgname);
						return(1);
					}
					if(config->op_q_foreign) {
						int match = 0;
						for(pmlist_iterator_t *i = pacman_list_begin(pmc_syncs), *end = pacman_list_end(pmc_syncs); i != end; i = pacman_list_next(i)) {
							PM_DB *db = list_data(i);
							pmlist_t *cache = pacman_db_getpkgcache(db);
							for(pmlist_iterator_t *j = pacman_list_begin(cache), *end = pacman_list_end(cache); j != end; j = pacman_list_next(j)) {
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
					if(config->op_q_orphans) {
						int reason;
						if(config->op_q_orphans_deps) {
							reason = PM_PKG_REASON_EXPLICIT;
						} else {
							reason = PM_PKG_REASON_DEPEND;
						}

						if(pacman_pkg_getinfo(info, PM_PKG_REQUIREDBY) == NULL
						   && (long)pacman_pkg_getinfo(info, PM_PKG_REASON) == reason) {
							MSG(NL, "%s %s\n", pkgname, pkgver);
						}
					}
					if(config->op_q_fsck) {
						pkg_fsck(tmpp);
					}
				} else {
					MSG(NL, "%s %s\n", pkgname, pkgver);
				}
			}
		} else {
			/* Do not allow -Qe , -Qm with package arg */
			if(config->op_q_orphans || config->op_q_foreign) {
				ERR(NL, _("This query option cannot have an package argument\n"));
				return(1);
			}

			info = pacman_db_readpkg(db_local, package);
			if(info == NULL) {
				ERR(NL, _("package \"%s\" not found\n"), package);
				return(2);
			}

			/* find a target */
			if(config->op_q_changelog || config->op_q_info || config->op_q_list || config->op_q_fsck) {
				if(config->op_q_changelog) {
					char *dbpath, changelog[PATH_MAX];
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
				if(config->op_q_fsck) {
					pkg_fsck(info);
				}
			} else {
                               char *pkgname = pacman_pkg_getinfo(info, PM_PKG_NAME);
                               char *pkgver = pacman_pkg_getinfo(info, PM_PKG_VERSION);
                               MSG(NL, "%s %s\n", pkgname, pkgver);

			}
		}
	}

	return(errors);
}

/* vim: set ts=2 sw=2 noet: */
