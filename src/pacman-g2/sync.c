/*
 *  sync.c
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

#if defined(__APPLE__) || defined(__OpenBSD__)
#include <sys/syslimits.h>
#include <sys/stat.h>
#endif

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <libintl.h>
#ifdef CYGWIN
#include <limits.h> /* PATH_MAX */
#endif

#include <pacman.h>
/* pacman-g2 */
#include "util.h"
#include "log.h"
#include "download.h"
#include "list.h"
#include "package.h"
#include "trans.h"
#include "sync.h"
#include "conf.h"

extern config_t *config;

extern list_t *pmc_syncs;

static int sync_synctree(int level, list_t *syncs)
{
	list_t *i;
	int success = 0, ret;

	for(i = syncs; i; i = i->next) {
		PM_DB *db = i->data;

		ret = pacman_db_update((level < 2 ? 0 : 1), db);
		if(ret == -1) {
			if(pm_errno == PM_ERR_DB_SYNC) {
				ERR(NL, _("failed to synchronize %s\n"), (char*)pacman_db_getinfo(db, PM_DB_TREENAME));
			} else {
				ERR(NL, _("failed to update %s (%s)\n"), (char*)pacman_db_getinfo(db,
							PM_DB_TREENAME), pacman_strerror(pm_errno));
			}
			success--;
		} else if(ret == 1) {
			MSG(NL, _(" %s is up to date\n"), (char *)pacman_db_getinfo(db, PM_DB_TREENAME));
		}
	}

	return(success);
}

static int sync_search(list_t *syncs, list_t *targets)
{
	list_t *i, *j;
	PM_LIST *ret;

	for(i = syncs; i; i = i->next) {
		PM_DB *db = i->data;
		PM_LIST *lp;

		for(j = targets; j; j = j->next) {
			pacman_set_option(PM_OPT_NEEDLES, (long)j->data);
		}
		ret = pacman_db_search(db);
		if(ret == NULL) {
			continue;
		}
		for(lp = ret; lp; lp = pacman_list_next(lp)) {
			PM_PKG *pkg = pacman_list_getdata(lp);

			char *group = (char *)pacman_list_getdata(pacman_pkg_getinfo(pkg,PM_PKG_GROUPS));
			printf("%s/%s %s %s%s%s\n    ",
					(char *)pacman_db_getinfo(db, PM_DB_TREENAME),
					(char *)pacman_pkg_getinfo(pkg, PM_PKG_NAME),
					(char *)pacman_pkg_getinfo(pkg, PM_PKG_VERSION),
					(group ? "(" : ""), (group ? group : ""), (group ? ")" : ""));
			indentprint((char *)pacman_pkg_getinfo(pkg, PM_PKG_DESC), 4);
			printf("\n");
		}
	}

	return(0);
}

static int sync_group(int level, list_t *syncs, list_t *targets)
{
	list_t *i, *j;

	if(targets) {
		for(i = targets; i; i = i->next) {
			for(j = syncs; j; j = j->next) {
				PM_DB *db = j->data;
				PM_GRP *grp = pacman_db_readgrp(db, i->data);

				if(grp) {
					MSG(NL, "%s\n", (char *)pacman_grp_getinfo(grp, PM_GRP_NAME));
					PM_LIST_display("   ", pacman_grp_getinfo(grp, PM_GRP_PKGNAMES));
				}
			}
		}
	} else {
		for(j = syncs; j; j = j->next) {
			PM_DB *db = j->data;
			PM_LIST *lp;

			for(lp = pacman_db_getgrpcache(db); lp; lp = pacman_list_next(lp)) {
				PM_GRP *grp = pacman_list_getdata(lp);

				MSG(NL, "%s\n", (char *)pacman_grp_getinfo(grp, PM_GRP_NAME));
				if(grp && level > 1) {
					PM_LIST_display("   ", pacman_grp_getinfo(grp, PM_GRP_PKGNAMES));
				}
			}
		}
	}

	return(0);
}

static int sync_info(list_t *syncs, list_t *targets)
{
	list_t *i, *j;

	if(targets) {
		for(i = targets; i; i = i->next) {
			int found = 0;

			for(j = syncs; j && !found; j = j->next) {
				PM_DB *db = j->data;
				PM_LIST *lp;

				for(lp = pacman_db_getpkgcache(db); !found && lp; lp = pacman_list_next(lp)) {
					PM_PKG *pkg = pacman_list_getdata(lp);

					if(!strcmp(pacman_pkg_getinfo(pkg, PM_PKG_NAME), i->data)) {
						dump_pkg_sync(pkg, (char *)pacman_db_getinfo(db, PM_DB_TREENAME));
						MSG(NL, "\n");
						found = 1;
					}
				}
			}
			if(!found) {
				ERR(NL, _("package \"%s\" was not found.\n"), (char *)i->data);
				break;
			}
		}
	} else {
		for(j = syncs; j; j = j->next) {
			PM_DB *db = j->data;
			PM_LIST *lp;
			
			for(lp = pacman_db_getpkgcache(db); lp; lp = pacman_list_next(lp)) {
				dump_pkg_sync(pacman_list_getdata(lp), (char *)pacman_db_getinfo(db, PM_DB_TREENAME));
				MSG(NL, "\n");
			}
		}
	}

	return(0);
}

static int sync_list(list_t *syncs, list_t *targets)
{
	list_t *i;
	list_t *ls = NULL;

	if(targets) {
		for(i = targets; i; i = i->next) {
			list_t *j;
			PM_DB *db = NULL;

			for(j = syncs; j && !db; j = j->next) {
				PM_DB *d = j->data;

				if(strcmp(i->data, (char *)pacman_db_getinfo(d, PM_DB_TREENAME)) == 0) {
					db = d;
				}
			}

			if(db == NULL) {
				ERR(NL, _("repository \"%s\" was not found.\n"), (char *)i->data);
				FREELISTPTR(ls);
				return(1);
			}

			ls = list_add(ls, sync);
		}
	} else {
		ls = syncs;
	}

	for(i = ls; i; i = i->next) {
		PM_LIST *lp;
		PM_DB *db = i->data;

		for(lp = pacman_db_getpkgcache(db); lp; lp = pacman_list_next(lp)) {
			PM_PKG *pkg = pacman_list_getdata(lp);

			MSG(NL, "%s %s %s\n", (char *)pacman_db_getinfo(db, PM_DB_TREENAME),
					(char *)pacman_pkg_getinfo(pkg, PM_PKG_NAME),
					(char *)pacman_pkg_getinfo(pkg, PM_PKG_VERSION));
		}
	}

	if(targets) {
		FREELISTPTR(ls);
	}

	return(0);
}

int syncpkg(list_t *targets)
{
	int confirm = 0;
	int retval = 0;
	list_t *i;
	PM_LIST *packages, *data, *lp;

	if(pmc_syncs == NULL || !list_count(pmc_syncs)) {
		ERR(NL, _("no usable package repositories configured.\n"));
		return(1);
	}

	if(config->op_s_clean) {
		int level;
		level = (config->op_s_clean == 1) ? 0 : 1;
		if(!level) {
			if(!yesno(_("Do you want to remove old packages from cache? [Y/n] ")))
				return(0);
			MSG(NL, _("removing old packages from cache... "));
			retval = pacman_sync_cleancache(level);
		} else {
			if(!yesno(_("Do you want to remove all packages from cache? [Y/n] ")))
				return(0);
			MSG(NL, _("removing all packages from cache... "));
			retval = pacman_sync_cleancache(level);
		}
		if(retval == 0) {
			MSG(CL, _("done.\n"));
		} else {
			ERR(NL, _("failed to clean the cache (%s)\n"), pacman_strerror(pm_errno));
			return(1);
		}
	}

	if(config->op_s_sync) {
		/* grab a fresh package list */
		MSG(NL, _(":: Synchronizing package databases...\n"));
		pacman_logaction(_("synchronizing package lists"));
		if(sync_synctree(config->op_s_sync, pmc_syncs)) {
			return(1);
		}
	}

	if(config->op_s_search) {
		return(sync_search(pmc_syncs, targets));
	}

	if(config->group) {
		return(sync_group(config->group, pmc_syncs, targets));
	}

	if(config->op_s_info) {
		return(sync_info(pmc_syncs, targets));
	}

	if(config->op_q_list) {
		return(sync_list(pmc_syncs, targets));
	}

	/* Step 1: create a new transaction...
	 */
	if(pacman_trans_init(PM_TRANS_TYPE_SYNC, config->flags, cb_trans_evt, cb_trans_conv, cb_trans_progress) == -1) {
		ERR(NL, _("failed to init transaction (%s)\n"), pacman_strerror(pm_errno));
		if(pm_errno == PM_ERR_HANDLE_LOCK) {
			MSG(NL, _("       if you're sure a package manager is not already running,\n"
			        "       you can remove %s%s\n"), config->root, PM_LOCK);
		}
		return(1);
	}

	if(config->op_s_upgrade) {
		MSG(NL, _(":: Starting local database upgrade...\n"));
		pacman_logaction(_("starting full system upgrade"));
		if(pacman_trans_sysupgrade() == -1) {
			ERR(NL, "%s\n", pacman_strerror(pm_errno));
			retval = 1;
			goto cleanup;
		}

		/* check if pacman-g2 itself is one of the packages to upgrade.  If so, we
		 * we should upgrade ourselves first and then re-exec as the new version.
		 *
		 * this can prevent some of the "syntax error" problems users can have
		 * when sysupgrade'ing with an older version of pacman-g2.
		 */
		data = pacman_trans_getinfo(PM_TRANS_PACKAGES);
		for(lp = pacman_list_first(data); lp; lp = pacman_list_next(lp)) {
			PM_SYNCPKG *sync = pacman_list_getdata(lp);
			PM_PKG *spkg = pacman_sync_getinfo(sync, PM_SYNC_PKG);
			if(!strcmp("pacman-g2", pacman_pkg_getinfo(spkg, PM_PKG_NAME)) && pacman_list_count(data) > 1) {
				MSG(NL, _("\n:: pacman-g2 has detected a newer version of the \"pacman-g2\" package.\n"));
				MSG(NL, _(":: It is recommended that you allow pacman-g2 to upgrade itself\n"));
				MSG(NL, _(":: first, then you can re-run the operation with the newer version.\n"));
				MSG(NL, "::\n");
				if(yesno(_(":: Upgrade pacman-g2 first? [Y/n] "))) {
					if(pacman_trans_release() == -1) {
						ERR(NL, _("failed to release transaction (%s)\n"), pacman_strerror(pm_errno));
						retval = 1;
						goto cleanup;
					}
					if(pacman_trans_init(PM_TRANS_TYPE_SYNC, config->flags, cb_trans_evt, cb_trans_conv, cb_trans_progress) == -1) {
						ERR(NL, _("failed to init transaction (%s)\n"), pacman_strerror(pm_errno));
						if(pm_errno == PM_ERR_HANDLE_LOCK) {
							MSG(NL, _("       if you're sure a package manager is not already running,\n"
			        				"       you can remove %s%s\n"), config->root, PM_LOCK);
						}
						return(1);
					}
					if(pacman_trans_addtarget("pacman-g2") == -1) {
						ERR(NL, _("could not add target '%s': %s\n"), (char *)i->data, pacman_strerror(pm_errno));
						retval = 1;
						goto cleanup;
					}
					break;
				}
			}
		}
	} else {
		/* process targets */
		for(i = targets; i; i = i->next) {
			char *targ = i->data;
			if(pacman_trans_addtarget(targ) == -1) {
				PM_GRP *grp = NULL;
				list_t *j;
				int found=0;
				if(pm_errno == PM_ERR_TRANS_DUP_TARGET) {
					/* just ignore duplicate targets */
					continue;
				}
				if(pm_errno != PM_ERR_PKG_NOT_FOUND) {
					ERR(NL, _("could not add target '%s': %s\n"), (char *)i->data, pacman_strerror(pm_errno));
					retval = 1;
					goto cleanup;
				}
				/* target not found: check if it's a group */
				for(j = pmc_syncs; j; j = j->next) {
					PM_DB *db = j->data;
					grp = pacman_db_readgrp(db, targ);
					if(grp) {
						PM_LIST *pmpkgs;
						list_t *k, *pkgs;
						found++;
						MSG(NL, _(":: group %s:\n"), targ);
						pmpkgs = pacman_grp_getinfo(grp, PM_GRP_PKGNAMES);
						/* remove dupe entries in case a package exists in multiple repos */
						/*   (the dupe function takes a PM_LIST* and returns a list_t*) */
						pkgs = PM_LIST_remove_dupes(pmpkgs);
						list_display("   ", pkgs);
						if(yesno(_(":: Install whole content? [Y/n] "))) {
							for(k = pkgs; k; k = k->next) {
								targets = list_add(targets, strdup(k->data));
							}
						} else {
							for(k = pkgs; k; k = k->next) {
								char *pkgname = k->data;
								if(yesno(_(":: Install %s from group %s? [Y/n] "), pkgname, targ)) {
									targets = list_add(targets, strdup(pkgname));
								}
							}
						}
						FREELIST(pkgs);
					}
				}
				if(!found) {
					/* targ not found in sync db, searching for providers... */
					PM_LIST *k = NULL;
					PM_PKG *pkg;
					char *pname;
					for(j = pmc_syncs; j && !k; j = j->next) {
						PM_DB *db = j->data;
						k = pacman_db_whatprovides(db, targ);
						pkg = (PM_PKG*)pacman_list_getdata(pacman_list_first(k));
						pname = (char*)pacman_pkg_getinfo(pkg, PM_PKG_NAME);
					}
					if(pname != NULL) {
						/* targ is provided by pname */
						targets = list_add(targets, strdup(pname));
					} else {
						ERR(NL, _("could not add target '%s': not found in sync db\n"), targ);
						retval = 1;
						goto cleanup;
					}
				}
			}
		}
	}

	/* Step 2: "compute" the transaction based on targets and flags
	 */
	if(pacman_trans_prepare(&data) == -1) {
		long long *pkgsize, *freespace;
		ERR(NL, _("failed to prepare transaction (%s)\n"), pacman_strerror(pm_errno));
		switch(pm_errno) {
			case PM_ERR_UNSATISFIED_DEPS:
				for(lp = pacman_list_first(data); lp; lp = pacman_list_next(lp)) {
					PM_DEPMISS *miss = pacman_list_getdata(lp);
					MSG(NL, ":: %s: %s %s", pacman_dep_getinfo(miss, PM_DEP_TARGET),
					    (long)pacman_dep_getinfo(miss, PM_DEP_TYPE) == PM_DEP_TYPE_DEPEND ? _("requires") : _("is required by"),
					    pacman_dep_getinfo(miss, PM_DEP_NAME));
					switch((long)pacman_dep_getinfo(miss, PM_DEP_MOD)) {
						case PM_DEP_MOD_EQ: MSG(CL, "=%s", pacman_dep_getinfo(miss, PM_DEP_VERSION)); break;
						case PM_DEP_MOD_GE: MSG(CL, ">=%s", pacman_dep_getinfo(miss, PM_DEP_VERSION)); break;
						case PM_DEP_MOD_LE: MSG(CL, "<=%s", pacman_dep_getinfo(miss, PM_DEP_VERSION)); break;
					}
					MSG(CL, "\n");
				}
				pacman_list_free(data);
			break;
			case PM_ERR_CONFLICTING_DEPS:
				for(lp = pacman_list_first(data); lp; lp = pacman_list_next(lp)) {
					PM_DEPMISS *miss = pacman_list_getdata(lp);

					MSG(NL, _(":: %s: conflicts with %s"),
						pacman_dep_getinfo(miss, PM_DEP_TARGET), pacman_dep_getinfo(miss, PM_DEP_NAME));
				}
				pacman_list_free(data);
			break;
			case PM_ERR_DISK_FULL:
				lp = pacman_list_first(data);
				pkgsize = pacman_list_getdata(lp);
				lp = pacman_list_next(lp);
				freespace = pacman_list_getdata(lp);
					MSG(NL, _(":: %.1f MB required, have %.1f MB"),
						(double)(*pkgsize / 1048576.0), (double)(*freespace / 1048576.0));
				pacman_list_free(data);
			break;
			default:
			break;
		}
		retval = 1;
		goto cleanup;
	}

	packages = pacman_trans_getinfo(PM_TRANS_PACKAGES);
	if(packages == NULL) {
		/* nothing to do: just exit without complaining */
		goto cleanup;
	}

	/* list targets and get confirmation */
	if(!((unsigned long)pacman_trans_getinfo(PM_TRANS_FLAGS) & PM_TRANS_FLAG_PRINTURIS)) {
		list_t *list_install = NULL;
		list_t *list_remove = NULL;
		char *str;
		unsigned long totalsize = 0;
		unsigned long totalusize = 0;
		double mb, umb;

		for(lp = pacman_list_first(packages); lp; lp = pacman_list_next(lp)) {
			PM_SYNCPKG *sync = pacman_list_getdata(lp);
			PM_PKG *pkg = pacman_sync_getinfo(sync, PM_SYNC_PKG);
			char *pkgname, *pkgver;

			if((long)pacman_sync_getinfo(sync, PM_SYNC_TYPE) == PM_SYNC_TYPE_REPLACE) {
				PM_LIST *j, *data;
				data = pacman_sync_getinfo(sync, PM_SYNC_DATA);
				for(j = pacman_list_first(data); j; j = pacman_list_next(j)) {
					PM_PKG *p = pacman_list_getdata(j);
					char *pkgname = pacman_pkg_getinfo(p, PM_PKG_NAME);
					if(!list_is_strin(pkgname, list_remove)) {
						list_remove = list_add(list_remove, strdup(pkgname));
					}
				}
			}

			pkgname = pacman_pkg_getinfo(pkg, PM_PKG_NAME);
			pkgver = pacman_pkg_getinfo(pkg, PM_PKG_VERSION);
			totalsize += (long)pacman_pkg_getinfo(pkg, PM_PKG_SIZE);
			totalusize += (long)pacman_pkg_getinfo(pkg, PM_PKG_USIZE);

			asprintf(&str, "%s-%s", pkgname, pkgver);
			list_install = list_add(list_install, str);
		}
		if(list_remove) {
			MSG(NL, _("\nRemove:  "));
			str = buildstring(list_remove);
			indentprint(str, 9);
			MSG(CL, "\n");
			FREELIST(list_remove);
			FREE(str);
		}
		mb = (double)(totalsize / 1048576.0);
		umb = (double)(totalusize / 1048576.0);
		/* round up to 0.1 */
		if(mb < 0.1) {
			mb = 0.1;
		}
		if(umb > 0 && umb < 0.1) {
			umb = 0.1;
		}
		MSG(NL, _("\nTargets: "));
		str = buildstring(list_install);
		indentprint(str, 9);
		MSG(NL, _("\nTotal Package Size:   %.1f MB\n"), mb);
		if(umb > 0) {
		  MSG(NL, _("\nTotal Uncompressed Package Size:   %.1f MB\n"), umb);
		}
		FREELIST(list_install);
		FREE(str);

		if(config->op_s_downloadonly) {
			if(config->noconfirm) {
				MSG(NL, _("\nBeginning download...\n"));
				confirm = 1;
			} else {
				MSG(NL, "\n");
				confirm = yesno(_("Proceed with download? [Y/n] "));
			}
		} else {
			/* don't get any confirmation if we're called from makepkg */
			if(config->op_d_resolve) {
				confirm = 1;
			} else {
				if(config->noconfirm) {
					MSG(NL, _("\nBeginning upgrade process...\n"));
					confirm = 1;
				} else {
					MSG(NL, "\n");
					confirm = yesno(_("Proceed with upgrade? [Y/n] "));
				}
			}
		}
		if(!confirm) {
			goto cleanup;
		}
	}

	/* Step 3: actually perform the installation
	 */
	if(pacman_trans_commit(&data) == -1) {
		ERR(NL, _("failed to commit transaction (%s)\n"), pacman_strerror(pm_errno));
		switch(pm_errno) {
			case PM_ERR_FILE_CONFLICTS:
				for(lp = pacman_list_first(data); lp; lp = pacman_list_next(lp)) {
					PM_CONFLICT *conflict = pacman_list_getdata(lp);
					switch((long)pacman_conflict_getinfo(conflict, PM_CONFLICT_TYPE)) {
						case PM_CONFLICT_TYPE_TARGET:
							MSG(NL, _("%s%s exists in \"%s\" (target) and \"%s\" (target)"),
											config->root,
							        (char *)pacman_conflict_getinfo(conflict, PM_CONFLICT_FILE),
							        (char *)pacman_conflict_getinfo(conflict, PM_CONFLICT_TARGET),
							        (char *)pacman_conflict_getinfo(conflict, PM_CONFLICT_CTARGET));
						break;
						case PM_CONFLICT_TYPE_FILE:
							MSG(NL, _("%s: %s%s exists in filesystem"),
							        (char *)pacman_conflict_getinfo(conflict, PM_CONFLICT_TARGET),
											config->root,
							        (char *)pacman_conflict_getinfo(conflict, PM_CONFLICT_FILE));
						break;
					}
				}
				pacman_list_free(data);
				MSG(NL, _("\nerrors occurred, no packages were upgraded.\n"));
			break;
			case PM_ERR_PKG_CORRUPTED:
				for(lp = pacman_list_first(data); lp; lp = pacman_list_next(lp)) {
					MSG(NL, "%s", (char*)pacman_list_getdata(lp));
				}
				pacman_list_free(data);
				MSG(NL, _("\nerrors occurred, no packages were upgraded.\n"));
			break;
			default:
			break;
		}
		retval = 1;
		goto cleanup;
	}

	/* Step 4: release transaction resources
	 */
cleanup:
	if(pacman_trans_release() == -1) {
		ERR(NL, _("failed to release transaction (%s)\n"), pacman_strerror(pm_errno));
		retval = 1;
	}

	return(retval);
}

/* vim: set ts=2 sw=2 noet: */
