/*
 *  trans_sysupgrade.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2005-2008 by Miklos Vajna <vmiklos@frugalware.org>
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

#include <dirent.h>
#include <limits.h> /* PATH_MAX */
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

/* pacman-g2 */
#include "trans.h"

#include "util/log.h"
#include "util/stringlist.h"
#include "error.h"
#include "package.h"
#include "db.h"
#include "cache.h"
#include "deps.h"
#include "conflict.h"
#include "trans.h"
#include "util.h"
#include "sync.h"
#include "versioncmp.h"
#include "handle.h"
#include "util.h"
#include "pacman.h"
#include "handle.h"
#include "server.h"

#include "sync.h"

using namespace libpacman;

static int istoonew(Package *pkg)
{
	time_t t;
	Handle *handle = pkg->database()->m_handle;

	if (!handle->upgradedelay)
		return 0;
	time(&t);
	return((pkg->date + handle->upgradedelay) > t);
}

int _pacman_trans_sysupgrade(pmtrans_t *trans)
{
	FPtrList *i, *j, *k;
	Handle *handle;
	Database *db_local;
	FPtrList *dbs_sync;

	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT((handle = trans->m_handle) != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT((db_local = handle->db_local) != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT((dbs_sync = handle->dbs_sync) != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	/* this is a sysupgrade, so that install reasons are not touched */
	handle->sysupgrade = 1;

	/* check for "recommended" package replacements */
	_pacman_log(PM_LOG_FLOW1, _("checking for package replacements"));
	for(i = dbs_sync; i; i = f_ptrlistitem_next(i)) {
		for(j = _pacman_db_get_pkgcache(f_ptrlistitem_data(i)); j; j = f_ptrlistitem_next(j)) {
			Package *spkg = f_ptrlistitem_data(j);
			for(k = spkg->replaces(); k; k = f_ptrlistitem_next(k)) {
				FPtrList *m;
				for(m = _pacman_db_get_pkgcache(db_local); m; m = f_ptrlistitem_next(m)) {
					Package *lpkg = f_ptrlistitem_data(m);
					if(!strcmp(f_stringlistitem_to_str(k), lpkg->name())) {
						_pacman_log(PM_LOG_DEBUG, _("checking replacement '%s' for package '%s'"), f_stringlistitem_to_str(k), spkg->name());
						if(_pacman_list_is_strin(lpkg->name(), handle->ignorepkg)) {
							_pacman_log(PM_LOG_WARNING, _("%s-%s: ignoring package upgrade (to be replaced by %s-%s)"),
								lpkg->name(), lpkg->version(), spkg->name(), spkg->version());
						} else {
							/* get confirmation for the replacement */
							int doreplace = 0;
							QUESTION(trans, PM_TRANS_CONV_REPLACE_PKG, lpkg, spkg, (void *)((Database *)f_ptrlistitem_data(i))->treename(), &doreplace);

							if(doreplace) {
								/* if confirmed, add this to the 'final' list, designating 'lpkg' as
								 * the package to replace.
								 */
								pmsyncpkg_t *ps;
								/* check if spkg->name is already in the packages list. */
								ps = trans->find(spkg->name());
								if(ps) {
									/* found it -- just append to the replaces list */
									lpkg->acquire();
									ps->data = f_ptrlist_add(ps->data, lpkg);
								} else {
									/* none found -- enter pkg into the final sync list */
									ps = new __pmsyncpkg_t(PM_SYNC_TYPE_REPLACE, spkg, NULL);
									if(ps == NULL) {
										pm_errno = PM_ERR_MEMORY;
										goto error;
									}
									lpkg->acquire();
									ps->data = f_ptrlist_add(NULL, lpkg);
									trans->syncpkgs = f_ptrlist_add(trans->syncpkgs, ps);
								}
								_pacman_log(PM_LOG_FLOW2, _("%s-%s elected for upgrade (to be replaced by %s-%s)"),
								          lpkg->name(), lpkg->version(), spkg->name(), spkg->version());
							}
						}
						break;
					}
				}
			}
		}
	}

	/* match installed packages with the sync dbs and compare versions */
	_pacman_log(PM_LOG_FLOW1, _("checking for package upgrades"));
	for(i = _pacman_db_get_pkgcache(db_local); i; i = f_ptrlistitem_next(i)) {
		int cmp;
		int replace=0;
		Package *local = f_ptrlistitem_data(i);
		Package *spkg = NULL;
		pmsyncpkg_t *ps;

		for(j = dbs_sync; !spkg && j; j = f_ptrlistitem_next(j)) {
			spkg = ((Database *)f_ptrlistitem_data(j))->find(local->name());
		}
		if(spkg == NULL) {
			_pacman_log(PM_LOG_DEBUG, _("'%s' not found in sync db -- skipping"), local->name());
			continue;
		}

		/* we don't care about a to-be-replaced package's newer version */
		for(j = trans->syncpkgs; j && !replace; j = f_ptrlistitem_next(j)) {
			ps = f_ptrlistitem_data(j);
			if(ps->type == PM_SYNC_TYPE_REPLACE) {
				if(_pacman_pkg_isin(spkg->name(), ps->data)) {
					replace=1;
				}
			}
		}
		if(replace) {
			_pacman_log(PM_LOG_DEBUG, _("'%s' is already elected for removal -- skipping"),
								local->name());
			continue;
		}

		/* compare versions and see if we need to upgrade */
		cmp = _pacman_versioncmp(local->version(), spkg->version());
		if(cmp > 0 && !spkg->force() && !(trans->flags & PM_TRANS_FLAG_DOWNGRADE)) {
			/* local version is newer */
			_pacman_log(PM_LOG_WARNING, _("%s-%s: local version is newer"),
					local->name(), local->version());
		} else if(cmp == 0) {
			/* versions are identical */
		} else if(_pacman_list_is_strin(local->name(), handle->ignorepkg)) {
			/* package should be ignored (IgnorePkg) */
			_pacman_log(PM_LOG_WARNING, _("%s-%s: ignoring package upgrade (%s)"),
					local->name(), local->version(), spkg->version());
		} else if(istoonew(spkg)) {
			/* package too new (UpgradeDelay) */
			_pacman_log(PM_LOG_FLOW1, _("%s-%s: delaying upgrade of package (%s)\n"),
					local->name(), local->version(), spkg->version());
		} else if(spkg->stick()) {
			_pacman_log(PM_LOG_WARNING, _("%s-%s: please upgrade manually (%s => %s)"),
					local->name(), local->version(), local->version(), spkg->version());
		} else {
			_pacman_log(PM_LOG_FLOW2, _("%s-%s elected for upgrade (%s => %s)"),
					local->name(), local->version(), local->version(), spkg->version());
			/* check if spkg->name is already in the packages list. */
			if(!trans->find(spkg->name())) {
				ps = new __pmsyncpkg_t(PM_SYNC_TYPE_UPGRADE, spkg, local);
				if(ps == NULL) {
					goto error;
				}
				trans->syncpkgs = f_ptrlist_add(trans->syncpkgs, ps);
			} else {
				/* spkg->name is already in the packages list -- just ignore it */
			}
		}
	}

	return(0);

error:
	return(-1);
}

/* vim: set ts=2 sw=2 noet: */
