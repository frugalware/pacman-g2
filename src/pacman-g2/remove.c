/*
 *  remove.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
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
#include <string.h>
#include <libintl.h>

#include <pacman.h>
/* pacman-g2 */
#include "conf.h"
#include "list.h"
#include "log.h"
#include "remove.h"
#include "trans.h"
#include "util.h"

extern config_t *config;

extern PM_DB *db_local;

int removepkg(pmtranstype_t transtype, list_t *targets)
{
	PM_LIST *data;
	list_t *i;
	list_t *finaltargs = NULL;
	int retval = 0;

	if(targets == NULL) {
		return(0);
	}

	switch (transtype) {
	case PM_TRANS_TYPE_REMOVE:
		/* If the target is a group, ask if its packages should be removed
		 * (the library can't remove groups for now)
		 */
		for(i = targets; i; i = i->next) {
			PM_GRP *grp;

			grp = pacman_db_readgrp(db_local, i->data);
			if(grp) {
				PM_LIST *lp, *pkgnames;
				int all;

				pkgnames = pacman_grp_getinfo(grp, PM_GRP_PKGNAMES);

				MSG(NL, _(":: group %s:\n"), pacman_grp_getinfo(grp, PM_GRP_NAME));
				PM_LIST_display("   ", pkgnames);
				all = yesno(_("    Remove whole content? [Y/n] "));
				for(lp = pacman_list_first(pkgnames); lp; lp = pacman_list_next(lp)) {
					if(all || yesno(_(":: Remove %s from group %s? [Y/n] "), (char *)pacman_list_getdata(lp), i->data)) {
						finaltargs = list_add(finaltargs, strdup(pacman_list_getdata(lp)));
					}
				}
			} else {
				/* not a group, so add it to the final targets */
				finaltargs = list_add(finaltargs, strdup(i->data));
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
	for(i = finaltargs; i; i = i->next) {
		if(pacman_trans_addtarget(i->data) == -1) {
			int match = 0;

			switch (transtype) {
			case PM_TRANS_TYPE_REMOVE:
				/* check for regex */
				if(config->regex) {
					PM_LIST *k;

					for(k = pacman_db_getpkgcache(db_local); k; k = pacman_list_next(k)) {
						PM_PKG *p = pacman_list_getdata(k);
						char *pkgname = pacman_pkg_getinfo(p, PM_PKG_NAME);

						match = pacman_reg_match(pkgname, i->data);
						if(match > 0) {
							pacman_trans_addtarget(pkgname);
						}
					}
				}
				break;
			}
			if (match > 0) continue;

			ERR(NL, _("failed to add target '%s' (%s)\n"), (char *)i->data, pacman_strerror(pm_errno));
			retval = 1;
			goto cleanup;
		}
	}

	/* Step 2: prepare the transaction based on its type, targets and flags
	 */
	if(pacman_trans_prepare(&data) == -1) {
		PM_LIST *lp;

		ERR(NL, _("failed to prepare transaction (%s)\n"), pacman_strerror(pm_errno));
		switch(pm_errno) {
			case PM_ERR_UNSATISFIED_DEPS:
				for(lp = pacman_list_first(data); lp; lp = pacman_list_next(lp)) {
					PM_DEPMISS *miss = pacman_list_getdata(lp);

					switch (transtype) {
					case PM_TRANS_TYPE_REMOVE:
						MSG(NL, _("  %s: is required by %s\n"), pacman_dep_getinfo(miss, PM_DEP_TARGET),
						    pacman_dep_getinfo(miss, PM_DEP_NAME));
						break;
					}
				}
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
