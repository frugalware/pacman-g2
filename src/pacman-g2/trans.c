/*
 *  trans.c
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

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include <libintl.h>

/* pacman-g2 */
#include "conf.h"
#include "download.h"
#include "list.h"
#include "log.h"
#include "trans.h"
#include "util.h"

#define LOG_STR_LEN 256

extern config_t *config;
extern PM_DB *db_local;
extern unsigned int maxcols;
extern list_t *pmc_syncs;

bool trans_has_usable_syncs()
{
	if(pmc_syncs == NULL || !list_count(pmc_syncs)) {
		ERR(NL, _("no usable package repositories configured.\n"));
		return false;
	}
	return true;
}

/* Callback to handle transaction events
 */
void cb_trans_evt(unsigned char event, void *data1, void *data2)
{
	char str[LOG_STR_LEN] = "";
	char out[PATH_MAX];
	int i;

	switch(event) {
		case PM_TRANS_EVT_CHECKDEPS_START:
			pm_fprintf(stderr, NL, _("checking dependencies... "));
		break;
		case PM_TRANS_EVT_FILECONFLICTS_START:
			if(config->noprogressbar) {
			MSG(NL, _("checking for file conflicts... "));
			}
		break;
		case PM_TRANS_EVT_RESOLVEDEPS_START:
			pm_fprintf(stderr, NL, _("resolving dependencies... "));
		break;
		case PM_TRANS_EVT_INTERCONFLICTS_START:
			if(config->noprogressbar) {
			MSG(NL, _("looking for inter-conflicts... "));
			}
		break;
		case PM_TRANS_EVT_FILECONFLICTS_DONE:
			if(config->noprogressbar) {
				MSG(CL, _("done.\n"));
			} else {
				MSG(NL, "");
			}
		break;
		case PM_TRANS_EVT_CHECKDEPS_DONE:
		case PM_TRANS_EVT_RESOLVEDEPS_DONE:
			MSG(CL, _("done.\n"));
		break;
		case PM_TRANS_EVT_INTERCONFLICTS_DONE:
			if(config->noprogressbar) {
				MSG(CL, _("done.\n"));
			}
		break;
		case PM_TRANS_EVT_EXTRACT_DONE:
		break;
		case PM_TRANS_EVT_ADD_START:
			if(config->noprogressbar) {
				MSG(NL, _("installing %s... "), (char *)pacman_pkg_getinfo(data1, PM_PKG_NAME));
			}
		break;
		case PM_TRANS_EVT_ADD_DONE:
			if(config->noprogressbar) {
				MSG(CL, _("done.\n"));
			}
		break;
		case PM_TRANS_EVT_REMOVE_START:
			if(config->noprogressbar) {
			MSG(NL, _("removing %s... "), (char *)pacman_pkg_getinfo(data1, PM_PKG_NAME));
			}
		break;
		case PM_TRANS_EVT_REMOVE_DONE:
			if(config->noprogressbar) {
			    MSG(CL, _("done.\n"));
			} else {
			    MSG(NL, "");
			}
		break;
		case PM_TRANS_EVT_UPGRADE_START:
			if(config->noprogressbar) {
				MSG(NL, _("upgrading %s... "), (char *)pacman_pkg_getinfo(data1, PM_PKG_NAME));
			}
		break;
		case PM_TRANS_EVT_UPGRADE_DONE:
			if(config->noprogressbar) {
				MSG(CL, _("done.\n"));
			}
		break;
		case PM_TRANS_EVT_INTEGRITY_START:
			MSG(NL, _("checking package integrity... "));
		break;
		case PM_TRANS_EVT_INTEGRITY_DONE:
			MSG(CL, _("done.\n"));
		break;
		case PM_TRANS_EVT_SCRIPTLET_INFO:
			MSG(NL, "%s\n", (char*)data1);
		break;
		case PM_TRANS_EVT_SCRIPTLET_START:
			MSG(NL, (char*)data1);
			MSG(CL, "...");
		break;
		case PM_TRANS_EVT_SCRIPTLET_DONE:
			if(!(long)data1) {
				MSG(CL, _(" done.\n"));
			} else {
				MSG(CL, _(" failed.\n"));
			}
		break;
		case PM_TRANS_EVT_PRINTURI:
			MSG(NL, "%s%s\n", (char*)data1, (char*)data2);
		break;
		case PM_TRANS_EVT_RETRIEVE_START:
			MSG(NL, _("\n:: Retrieving packages from %s...\n"), (char*)data1);
			fflush(stdout);
		break;
		case PM_TRANS_EVT_RETRIEVE_LOCAL:
			STRNCPY(str, (char*)data1, DLFNM_PROGRESS_LEN);
			MSG(NL, " %s [", str);
			STRNCPY(out, (char*)data2, maxcols-42);
			MSG(CL, "%s", out);
			for(i = strlen(out); i < (int)maxcols-43; i++) {
				MSG(CL, " ");
			}
			fputs(_("] 100%    LOCAL "), stdout);
		break;
	}
}

void cb_trans_conv(unsigned char event, void *data1, void *data2, void *data3, int *response)
{
	char str[LOG_STR_LEN] = "";

	switch(event) {
		case PM_TRANS_CONV_INSTALL_IGNOREPKG:
			if(config->noask) {
				if(config->ask & PM_TRANS_CONV_INSTALL_IGNOREPKG) {
					*response = 1;
				} else {
					*response = 0;
				}
			} else {
				snprintf(str, LOG_STR_LEN, _(":: %s requires %s, but it is in IgnorePkg. Install anyway? [Y/n] "),
				         (char *)pacman_pkg_getinfo(data1, PM_PKG_NAME),
				         (char *)pacman_pkg_getinfo(data2, PM_PKG_NAME));
				*response = yesno(str);
			}
		break;
		case PM_TRANS_CONV_REMOVE_HOLDPKG:
			if(config->noask) {
				if(config->ask & PM_TRANS_CONV_REMOVE_HOLDPKG) {
					*response = 1;
				} else {
					*response = 0;
				}
			} else {
				snprintf(str, LOG_STR_LEN, _(":: %s is designated as a HoldPkg.  Remove anyway? [Y/n] "),
				         (char *)pacman_pkg_getinfo(data1, PM_PKG_NAME));
				*response = yesno(str);
			}
		break;
		case PM_TRANS_CONV_REPLACE_PKG:
			if(config->noask) {
				if(config->ask & PM_TRANS_CONV_REPLACE_PKG) {
					*response = 1;
				} else {
					*response = 0;
				}
			} else {
				snprintf(str, LOG_STR_LEN, _(":: Replace %s with %s/%s? [Y/n] "),
				         (char *)pacman_pkg_getinfo(data1, PM_PKG_NAME),
				         (char *)data3,
				         (char *)pacman_pkg_getinfo(data2, PM_PKG_NAME));
				*response = yesno(str);
			}
		break;
		case PM_TRANS_CONV_CONFLICT_PKG:
			if(config->noask) {
				if(config->ask & PM_TRANS_CONV_CONFLICT_PKG) {
					*response = 1;
				} else {
					*response = 0;
				}
			} else {
				snprintf(str, LOG_STR_LEN, _(":: %s conflicts with %s. Remove %s? [Y/n] "),
				         (char *)data1,
				         (char *)data2,
				         (char *)data2);
				*response = yesno(str);
			}
		break;
		case PM_TRANS_CONV_LOCAL_NEWER:
			if(config->noask) {
				if(config->ask & PM_TRANS_CONV_LOCAL_NEWER) {
					*response = 1;
				} else {
					*response = 0;
				}
			} else {
				if(!config->op_s_downloadonly) {
					snprintf(str, LOG_STR_LEN, _(":: %s-%s: local version is newer. Upgrade anyway? [Y/n] "),
				         (char *)pacman_pkg_getinfo(data1, PM_PKG_NAME),
				         (char *)pacman_pkg_getinfo(data1, PM_PKG_VERSION));
					*response = yesno(str);
				} else {
					*response = 1;
				}
			}
		break;
		case PM_TRANS_CONV_LOCAL_UPTODATE:
			if(config->noask) {
				if(config->ask & PM_TRANS_CONV_LOCAL_UPTODATE) {
					*response = 1;
				} else {
					*response = 0;
				}
			} else {
				if(!config->op_s_downloadonly) {
					snprintf(str, LOG_STR_LEN, _(":: %s-%s: local version is up to date. Upgrade anyway? [Y/n] "),
				         (char *)pacman_pkg_getinfo(data1, PM_PKG_NAME),
				         (char *)pacman_pkg_getinfo(data1, PM_PKG_VERSION));
					*response = yesno(str);
				} else {
					*response = 1;
				}
			}
		break;
		case PM_TRANS_CONV_CORRUPTED_PKG:
			if(config->noask) {
				if(config->ask & PM_TRANS_CONV_CORRUPTED_PKG) {
					*response = 1;
				} else {
					*response = 0;
				}
			} else {
				if(!config->noconfirm) {
					snprintf(str, LOG_STR_LEN, _(":: Archive %s is corrupted. Do you want to delete it? [Y/n] "),
				         (char *)data1);
					*response = yesno(str);
				} else {
					*response = 1;
				}
			}
		break;
	}
}

/* FIXME: log10() want float .. */
void cb_trans_progress(unsigned char event, const char *pkgname, int percent, int count, int remaining)
{
	static int prevpercent=0; /* for less progressbar output */
	int i, hash;
	unsigned int maxpkglen, progresslen = maxcols - 57;
    const char *ptr = "";
	char *pkgname_short = NULL;

	if(config->noprogressbar) {
		goto cleanup;
	}

	if (!pkgname)
		goto cleanup;
	if ((percent > 100) || (percent < 0) || (percent == prevpercent))
		goto cleanup;

	prevpercent=percent;
	switch (event) {
		case PM_TRANS_PROGRESS_ADD_START:
			ptr = _("installing");
		break;

		case PM_TRANS_PROGRESS_UPGRADE_START:
			ptr = _("upgrading");
		break;

		case PM_TRANS_PROGRESS_REMOVE_START:
			ptr = _("removing");
		break;

		case PM_TRANS_PROGRESS_INTERCONFLICTS_START:
			ptr = _("looking for inter-conflicts");
		break;

		case PM_TRANS_PROGRESS_CONFLICTS_START:
			ptr = _("checking for file conflicts");
		break;
	}
	hash=percent*progresslen/100;

	// if the package name is too long, then slice the ending
	maxpkglen=46-strlen(ptr)-(3+2*(int)log10(count));
	pkgname_short = strdup(pkgname);
	if(strlen(pkgname_short)>maxpkglen)
		pkgname_short[maxpkglen-1]='\0';

	switch (event) {
	case PM_TRANS_PROGRESS_ADD_START:
	case PM_TRANS_PROGRESS_UPGRADE_START:
	case PM_TRANS_PROGRESS_REMOVE_START:
		putchar('(');
		for(i=0;i<(int)log10(count)-(int)log10(remaining);i++)
			putchar(' ');
		printf("%d/%d) %s %s ", remaining, count, ptr, pkgname_short);
		if (strlen(pkgname_short)<maxpkglen)
			for (i=maxpkglen-strlen(pkgname_short)-1; i>0; i--)
				putchar(' ');
		break;

	case PM_TRANS_PROGRESS_INTERCONFLICTS_START:
	case PM_TRANS_PROGRESS_CONFLICTS_START:
		printf("%s (", ptr);
		for(i=0;i<(int)log10(count)-(int)log10(remaining);i++)
			putchar(' ');
		printf("%d/%d) ", remaining, count);
		for (i=maxpkglen; i>0; i--)
			putchar(' ');
		break;
	}

	printf("[");
	for (i = progresslen; i > 0; i--) {
		if (i >= (int)(progresslen - hash))
			printf("#");
		else
			printf(" ");
	}
	MSG(CL, "] %3d%%\r", percent);
	/* avoid adding a new line for the last package */
	if(percent == 100 && (event == PM_TRANS_PROGRESS_ADD_START || event ==
			PM_TRANS_PROGRESS_UPGRADE_START) && remaining != count) {
		MSG(NL, "");
	}

cleanup:

	FREE(pkgname_short);
}

int trans_commit(pmtranstype_t transtype, list_t *targets)
{
	PM_LIST *data;
	list_t *i;
	list_t *finaltargs = NULL;
	int retval = 0;

	if(targets == NULL) {
		return(0);
	}

	switch (transtype) {
	case PM_TRANS_TYPE_ADD:
	case PM_TRANS_TYPE_UPGRADE:
		/* Check for URL targets and process them
		 */
		for(i = targets; i; i = list_next(i)) {
			if(strstr(list_data(i), "://")) {
				char *str = pacman_fetch_pkgurl(list_data(i));
				if(str == NULL) {
					return(1);
				} else {
					finaltargs = list_add(finaltargs, str);
				}
			} else {
				finaltargs = list_add(finaltargs, strdup(list_data(i)));
			}
		}
		break;
	case PM_TRANS_TYPE_REMOVE:
		/* If the target is a group, ask if its packages should be removed
		 * (the library can't remove groups for now)
		 */
		for(i = targets; i; i = list_next(i)) {
			PM_GRP *grp;

			grp = pacman_db_readgrp(db_local, list_data(i));
			if(grp) {
				PM_LIST *lp, *pkgnames;
				int all;

				pkgnames = pacman_grp_getinfo(grp, PM_GRP_PKGNAMES);

				MSG(NL, _(":: group %s:\n"), pacman_grp_getinfo(grp, PM_GRP_NAME));
				PM_LIST_display("   ", pkgnames);
				all = yesno(_("    Remove whole content? [Y/n] "));
				for(lp = pacman_list_first(pkgnames); lp; lp = pacman_list_next(lp)) {
					if(all || yesno(_(":: Remove %s from group %s? [Y/n] "), (char *)pacman_list_getdata(lp), list_data(i))) {
						finaltargs = list_add(finaltargs, strdup(pacman_list_getdata(lp)));
					}
				}
			} else {
				/* not a group, so add it to the final targets */
				finaltargs = list_add(finaltargs, strdup(list_data(i)));
			}
		}
		break;
	}

	/* Step 1: create a new transaction
	 */
	if(pacman_trans_init(transtype, config->flags,
				cb_trans_evt, cb_trans_conv, cb_trans_progress) == -1) {
		ERR(NL, _("failed to init transaction (%s)\n"), pacman_strerror(pm_errno));
		if(pm_errno == PM_ERR_HANDLE_LOCK) {
			MSG(NL, _("       if you're sure a package manager is not already running,\n"
						"       you can remove %s%s\n"), config->root, PM_LOCK);
		}
		FREELIST(finaltargs);
		return(1);
	}
	/* and add targets to it */
	MSG(NL, _("loading package data... "));
	for(i = finaltargs; i; i = list_next(i)) {
		if(pacman_trans_addtarget(pacman_get_trans(), transtype, list_data(i), config->flags) == -1) {
			int match = 0;

			switch (transtype) {
			case PM_TRANS_TYPE_REMOVE:
				/* check for regex */
				if(config->regex) {
					PM_LIST *k;

					for(k = pacman_db_getpkgcache(db_local); k; k = pacman_list_next(k)) {
						PM_PKG *p = pacman_list_getdata(k);
						char *pkgname = pacman_pkg_getinfo(p, PM_PKG_NAME);

						match = pacman_reg_match(pkgname, list_data(i));
						if(match > 0 &&
								pacman_trans_addtarget(pacman_get_trans(), transtype, pkgname, config->flags) == -1) {
							match = 0;
						}
					}
				}
				break;
			}
			if (match > 0) continue;
			ERR(NL, _("failed to add target '%s' (%s)\n"), (char *)list_data(i), pacman_strerror(pm_errno));
			retval = 1;
			goto cleanup;
		}
	}
	MSG(CL, _("done.\n"));

	/* Step 2: prepare the transaction based on its type, targets and flags
	 */
	if(pacman_trans_prepare(&data) == -1) {
		long long *pkgsize, *freespace;
		PM_LIST *lp;

		ERR(NL, _("failed to prepare transaction (%s)\n"), pacman_strerror(pm_errno));
		switch(pm_errno) {
			case PM_ERR_UNSATISFIED_DEPS:
				for(lp = pacman_list_first(data); lp; lp = pacman_list_next(lp)) {
					PM_DEPMISS *miss = pacman_list_getdata(lp);

					switch (transtype) {
					case PM_TRANS_TYPE_ADD:
					case PM_TRANS_TYPE_UPGRADE:
						MSG(NL, _(":: %s: requires %s"), pacman_dep_getinfo(miss, PM_DEP_TARGET),
						                              pacman_dep_getinfo(miss, PM_DEP_NAME));
						switch((long)pacman_dep_getinfo(miss, PM_DEP_MOD)) {
							case PM_DEP_MOD_EQ: MSG(CL, "=%s", pacman_dep_getinfo(miss, PM_DEP_VERSION));  break;
							case PM_DEP_MOD_GE: MSG(CL, ">=%s", pacman_dep_getinfo(miss, PM_DEP_VERSION)); break;
							case PM_DEP_MOD_LE: MSG(CL, "<=%s", pacman_dep_getinfo(miss, PM_DEP_VERSION)); break;
						}
						MSG(CL, "\n");
						break;
					case PM_TRANS_TYPE_REMOVE:
						MSG(NL, _("  %s: is required by %s\n"), pacman_dep_getinfo(miss, PM_DEP_TARGET),
						    pacman_dep_getinfo(miss, PM_DEP_NAME));
						break;

					}
				}
			break;
			case PM_ERR_CONFLICTING_DEPS:
				for(lp = pacman_list_first(data); lp; lp = pacman_list_next(lp)) {
					PM_DEPMISS *miss = pacman_list_getdata(lp);
					MSG(NL, _(":: %s: conflicts with %s"),
						pacman_dep_getinfo(miss, PM_DEP_TARGET), pacman_dep_getinfo(miss, PM_DEP_NAME));
				}
			break;
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
				MSG(NL, _("\nerrors occurred, no packages were upgraded.\n"));
			break;
			case PM_ERR_DISK_FULL:
				lp = pacman_list_first(data);
				pkgsize = pacman_list_getdata(lp);
				lp = pacman_list_next(lp);
				freespace = pacman_list_getdata(lp);
					MSG(NL, _(":: %.1f MB required, have %.1f MB"),
						(double)(*pkgsize / 1048576.0), (double)(*freespace / 1048576.0));
			break;
			default:
			break;
		}
		pacman_list_free(data);
		retval = 1;
		goto cleanup;
	}

	switch (transtype) {
	case PM_TRANS_TYPE_REMOVE:
		/* Warn user in case of dangerous operation
		 */
		if(config->flags & PM_TRANS_FLAG_RECURSE || config->flags & PM_TRANS_FLAG_CASCADE) {
			PM_LIST *lp;
			/* list transaction targets */
			i = NULL;
			for(lp = pacman_list_first(pacman_trans_getinfo(PM_TRANS_PACKAGES)); lp; lp = pacman_list_next(lp)) {
				PM_PKG *pkg = pacman_list_getdata(lp);
				i = list_add(i, strdup(pacman_pkg_getinfo(pkg, PM_PKG_NAME)));
			}
			list_display(_("\nTargets:"), i);
			FREELIST(i);
			/* get confirmation */
			if(yesno(_("\nDo you want to remove these packages? [Y/n] ")) == 0) {
				retval = 1;
				goto cleanup;
			}
			MSG(NL, "\n");
		}
		break;
	}

	/* Step 3: actually perform the transaction
	 */
	if(pacman_trans_commit(NULL) == -1) {
		ERR(NL, _("failed to commit transaction (%s)\n"), pacman_strerror(pm_errno));
		retval = 1;
		goto cleanup;
	}

	/* Step 4: release transaction resources
	 */
cleanup:
	FREELIST(finaltargs);
	if(pacman_trans_release() == -1) {
		ERR(NL, _("failed to release transaction (%s)\n"), pacman_strerror(pm_errno));
		retval = 1;
	}

	return(retval);
}

/* vim: set ts=2 sw=2 noet: */
