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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#ifdef CYGWIN
#include <limits.h> /* PATH_MAX */
#endif
#include <dirent.h>
#include <libintl.h>
/* pacman-g2 */
#include "log.h"
#include "error.h"
#include "list.h"
#include "package.h"
#include "db.h"
#include "cache.h"
#include "deps.h"
#include "conflict.h"
#include "provide.h"
#include "trans.h"
#include "util.h"
#include "sync.h"
#include "versioncmp.h"
#include "handle.h"
#include "util.h"
#include "pacman.h"
#include "md5.h"
#include "sha1.h"
#include "handle.h"
#include "server.h"

#include "trans_sysupgrade.h"

#include "sync.h"

static int istoonew(pmpkg_t *pkg)
{
	time_t t;
	if (!handle->upgradedelay)
		return 0;
	time(&t);
	return((pkg->date + handle->upgradedelay) > t);
}

int _pacman_sync_sysupgrade(pmtrans_t *trans, pmdb_t *db_local, pmlist_t *dbs_sync)
{
	pmlist_t *i, *j, *k;

	/* this is a sysupgrade, so that install reasons are not touched */
	handle->sysupgrade = 1;

	/* check for "recommended" package replacements */
	_pacman_log(PM_LOG_FLOW1, _("checking for package replacements"));
	for(i = dbs_sync; i; i = i->next) {
		for(j = _pacman_db_get_pkgcache(i->data); j; j = j->next) {
			pmpkg_t *spkg = j->data;
			for(k = _pacman_pkg_getinfo(spkg, PM_PKG_REPLACES); k; k = k->next) {
				pmlist_t *m;
				for(m = _pacman_db_get_pkgcache(db_local); m; m = m->next) {
					pmpkg_t *lpkg = m->data;
					if(!strcmp(k->data, lpkg->name)) {
						_pacman_log(PM_LOG_DEBUG, _("checking replacement '%s' for package '%s'"), k->data, spkg->name);
						if(_pacman_list_is_strin(lpkg->name, handle->ignorepkg)) {
							_pacman_log(PM_LOG_WARNING, _("%s-%s: ignoring package upgrade (to be replaced by %s-%s)"),
								lpkg->name, lpkg->version, spkg->name, spkg->version);
						} else {
							/* get confirmation for the replacement */
							int doreplace = 0;
							QUESTION(trans, PM_TRANS_CONV_REPLACE_PKG, lpkg, spkg, ((pmdb_t *)i->data)->treename, &doreplace);

							if(doreplace) {
								/* if confirmed, add this to the 'final' list, designating 'lpkg' as
								 * the package to replace.
								 */
								pmsyncpkg_t *ps;
								pmpkg_t *dummy = _pacman_pkg_new(lpkg->name, NULL);
								if(dummy == NULL) {
									pm_errno = PM_ERR_MEMORY;
									goto error;
								}
								dummy->requiredby = _pacman_list_strdup(lpkg->requiredby);
								/* check if spkg->name is already in the packages list. */
								ps = find_pkginsync(spkg->name, trans->packages);
								if(ps) {
									/* found it -- just append to the replaces list */
									ps->data = _pacman_list_add(ps->data, dummy);
								} else {
									/* none found -- enter pkg into the final sync list */
									ps = _pacman_sync_new(PM_SYNC_TYPE_REPLACE, spkg, NULL);
									if(ps == NULL) {
										FREEPKG(dummy);
										pm_errno = PM_ERR_MEMORY;
										goto error;
									}
									ps->data = _pacman_list_add(NULL, dummy);
									trans->packages = _pacman_list_add(trans->packages, ps);
								}
								_pacman_log(PM_LOG_FLOW2, _("%s-%s elected for upgrade (to be replaced by %s-%s)"),
								          lpkg->name, lpkg->version, spkg->name, spkg->version);
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
	for(i = _pacman_db_get_pkgcache(db_local); i; i = i->next) {
		int cmp;
		int replace=0;
		pmpkg_t *local = i->data;
		pmpkg_t *spkg = NULL;
		pmsyncpkg_t *ps;

		for(j = dbs_sync; !spkg && j; j = j->next) {
			spkg = _pacman_db_get_pkgfromcache(j->data, local->name);
		}
		if(spkg == NULL) {
			_pacman_log(PM_LOG_DEBUG, _("'%s' not found in sync db -- skipping"), local->name);
			continue;
		}

		/* we don't care about a to-be-replaced package's newer version */
		for(j = trans->packages; j && !replace; j=j->next) {
			ps = j->data;
			if(ps->type == PM_SYNC_TYPE_REPLACE) {
				if(_pacman_pkg_isin(spkg->name, ps->data)) {
					replace=1;
				}
			}
		}
		if(replace) {
			_pacman_log(PM_LOG_DEBUG, _("'%s' is already elected for removal -- skipping"),
								local->name);
			continue;
		}

		/* compare versions and see if we need to upgrade */
		cmp = _pacman_versioncmp(local->version, spkg->version);
		if(cmp > 0 && !_pacman_pkg_getinfo(spkg, PM_PKG_FORCE) && !(trans->flags & PM_TRANS_FLAG_DOWNGRADE)) {
			/* local version is newer */
			_pacman_log(PM_LOG_WARNING, _("%s-%s: local version is newer"),
				local->name, local->version);
		} else if(cmp == 0) {
			/* versions are identical */
		} else if(_pacman_list_is_strin(local->name, handle->ignorepkg)) {
			/* package should be ignored (IgnorePkg) */
			_pacman_log(PM_LOG_WARNING, _("%s-%s: ignoring package upgrade (%s)"),
				local->name, local->version, spkg->version);
		} else if(istoonew(spkg)) {
			/* package too new (UpgradeDelay) */
			_pacman_log(PM_LOG_FLOW1, _("%s-%s: delaying upgrade of package (%s)\n"),
					local->name, local->version, spkg->version);
		} else if(_pacman_pkg_getinfo(spkg, PM_PKG_STICK)) {
			_pacman_log(PM_LOG_WARNING, _("%s-%s: please upgrade manually (%s => %s)"),
				local->name, local->version, local->version, spkg->version);
		} else {
			_pacman_log(PM_LOG_FLOW2, _("%s-%s elected for upgrade (%s => %s)"),
				local->name, local->version, local->version, spkg->version);
			/* check if spkg->name is already in the packages list. */
			if(!find_pkginsync(spkg->name, trans->packages)) {
				pmpkg_t *dummy = _pacman_pkg_new(local->name, local->version);
				if(dummy == NULL) {
					goto error;
				}
				ps = _pacman_sync_new(PM_SYNC_TYPE_UPGRADE, spkg, dummy);
				if(ps == NULL) {
					FREEPKG(dummy);
					goto error;
				}
				trans->packages = _pacman_list_add(trans->packages, ps);
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
