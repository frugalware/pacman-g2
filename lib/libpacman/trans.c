/*
 *  trans.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2005, 2006 by Miklos Vajna <vmiklos@frugalware.org>
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

/* pacman-g2 */
#include "trans.h"

#include "db/fakedb.h"
#include "package/fpmpackage.h"
#include "error.h"
#include "package.h"
#include "util.h"
#include "handle.h"
#include "add.h"
#include "remove.h"
#include "sync.h"
#include "cache.h"
#include "pacman.h"
#include "versioncmp.h"

#include "util/list.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "fstdlib.h"

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int check_oldcache(void)
{
	pmdb_t *db = handle->db_local;
	time_t timestamp;

	if(_pacman_db_gettimestamp(db, &timestamp) == -1) {
		return(-1);
	}
	if(difftime(timestamp, db->cache_timestamp) != 0) {
		_pacman_log(PM_LOG_DEBUG, _("cache for '%s' repo is too old"), db->treename);
		_pacman_db_free_pkgcache(db);
	} else {
		_pacman_log(PM_LOG_DEBUG, _("cache for '%s' repo is up to date"), db->treename);
	}
	return(0);
}

pmtrans_t *_pacman_trans_new()
{
	pmtrans_t *trans = _pacman_zalloc(sizeof(pmtrans_t));

	if(trans) {
		trans->state = STATE_IDLE;
	}

	return(trans);
}

int _pacman_trans_delete(pmtrans_t *trans)
{
	ASSERT(_pacman_trans_fini(trans) == 0, return -1);

	free(trans);
	return 0;
}

int _pacman_trans_init(pmtrans_t *trans, pmtranstype_t type, unsigned int flags, pmtrans_cbs_t cbs)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	switch(type) {
		case PM_TRANS_TYPE_ADD:
		case PM_TRANS_TYPE_UPGRADE:
			trans->ops = &_pacman_add_pmtrans_opts;
		break;
		case PM_TRANS_TYPE_REMOVE:
			trans->ops = &_pacman_remove_pmtrans_opts;
		break;
		case PM_TRANS_TYPE_SYNC:
			trans->ops = &_pacman_sync_pmtrans_opts;
		break;
		default:
			trans->ops = NULL;
			// Be more verbose about the trans type
			_pacman_log(PM_LOG_ERROR,
					_("could not initialize transaction: Unknown Transaction Type %d"), type);
			return(-1);
	}

	trans->handle = handle;
	trans->type = type;
	trans->flags = flags;
	trans->cbs = cbs;
//	trans->packages = f_ptrlist_new();
	trans->targets = f_stringlist_new();
	trans->skiplist = f_stringlist_new();
	trans->triggers = f_stringlist_new();

	trans->state = STATE_INITIALIZED;

	check_oldcache();

	return(0);
}

int _pacman_trans_fini(pmtrans_t *trans)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->state != STATE_COMMITING, RET_ERR(PM_ERR_TRANS_COMMITING, -1));

	FREELISTPKGS(trans->packages);
#if 0
	{
		FVisitor visitor = {
			.fn = _pacman_syncpkg_delete,
			.data = NULL,
		};
		
		f_ptrlist_delete(trans->syncpkgs, &visitor);
	}
#endif
	f_stringlist_delete(trans->targets);
	f_stringlist_delete(trans->skiplist);
	f_stringlist_delete(trans->triggers);

	memset(trans, 0, sizeof(*trans));
	trans->state = STATE_IDLE;
	return 0;
}

static
int _pacman_trans_compute_triggers(pmtrans_t *trans)
{
	pmlist_t *lp;

	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	/* NOTE: Not the most efficient way, but will do until we add some string hash. */
	for(lp = trans->packages; lp; lp = lp->next) {
		pmpkg_t *pkg = lp->data;

		trans->triggers = f_stringlist_append_stringlist(trans->triggers, pkg->triggers);
	}
	for(lp = trans->syncpkgs; lp; lp = lp->next) {
		pmpkg_t *pkg = ((pmsyncpkg_t *)lp->data)->pkg;

		/* FIXME: might be incomplete */
		trans->triggers = f_stringlist_append_stringlist(trans->triggers, pkg->triggers);
	}
	trans->triggers = _pacman_list_remove_dupes(trans->triggers);
	/* FIXME: Sort the triggers to have a predictable execution order */

	return 0;
}

int _pacman_trans_event(pmtrans_t *trans, unsigned char event, void *data1, void *data2)
{
	char str[LOG_STR_LEN] = "";

	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	switch(event) {
	case PM_TRANS_EVT_ADD_DONE:
		snprintf(str, LOG_STR_LEN, "installed %s (%s)",
			(char *)pacman_pkg_getinfo(data1, PM_PKG_NAME),
			(char *)pacman_pkg_getinfo(data1, PM_PKG_VERSION));
		pacman_logaction(str);
		break;
	case PM_TRANS_EVT_REMOVE_DONE:
		snprintf(str, LOG_STR_LEN, "removed %s (%s)",
			(char *)pacman_pkg_getinfo(data1, PM_PKG_NAME),
			(char *)pacman_pkg_getinfo(data1, PM_PKG_VERSION));
		pacman_logaction(str);
		break;
	case PM_TRANS_EVT_UPGRADE_DONE:
		snprintf(str, LOG_STR_LEN, "upgraded %s (%s -> %s)",
			(char *)pacman_pkg_getinfo(data1, PM_PKG_NAME),
			(char *)pacman_pkg_getinfo(data2, PM_PKG_VERSION),
			(char *)pacman_pkg_getinfo(data1, PM_PKG_VERSION));
		pacman_logaction(str);
		break;
	default:
		/* Nothing to log */
		break;
	}

	if(trans->cbs.event) {
		trans->cbs.event(event, data1, data2);
	}
	return 0;
}

/* Test for existence of a package in a pmlist_t* of pmsyncpkg_t*
 * If found, return a pointer to the respective pmsyncpkg_t*
 */
pmsyncpkg_t *_pacman_trans_find(const pmtrans_t *trans, const char *pkgname)
{
	pmlist_t *i;

	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, NULL));

	for(i = trans->syncpkgs; i != NULL ; i = i->next) {
		pmsyncpkg_t *ps = i->data;

		if(ps && !strcmp(ps->pkg->name, pkgname)) {
			return ps;
		}
	}
	return NULL;
}

int _pacman_trans_set_state(pmtrans_t *trans, int new_state)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	/* Ignore unchanged state */
	if (trans->state == new_state) {
		return(0);
	}

	if (trans->set_state != NULL) {
		if (trans->set_state(trans, new_state) == -1) {
			/* pm_errno is set by trans->state_changed() */
			return(-1);
		}
	}
	_pacman_log(PM_LOG_DEBUG, _("Changing transaction state from %d to %d"), trans->state, new_state);
	trans->state = new_state;

	return(0);
}

pmsyncpkg_t *_pacman_trans_add(pmtrans_t *trans, pmsyncpkg_t *syncpkg, int flags)
{
	pmsyncpkg_t *registered_syncpkg;

	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, NULL));
	ASSERT(syncpkg != NULL, RET_ERR(PM_ERR_TRANS_NULL, NULL));

	if((registered_syncpkg = _pacman_trans_find(trans, syncpkg->pkg_name)) != NULL) {
		/* FIXME: Try to compress syncpkg in registered_syncpkg */
		return NULL;
	}
	_pacman_log(PM_LOG_FLOW2, _("adding target '%s' to the transaction set"), syncpkg->pkg_name);
	trans->syncpkgs = _pacman_list_add(trans->syncpkgs, syncpkg);
	return syncpkg;
}

pmsyncpkg_t *_pacman_trans_add_package(pmtrans_t *trans, pmpkg_t *pkg, pmtranstype_t type, int flags)
{
	pmsyncpkg_t *syncpkg;

	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, NULL));
	ASSERT(pkg != NULL, RET_ERR(PM_ERR_TRANS_NULL, NULL));
	ASSERT((syncpkg = _pacman_syncpkg_new(type, pkg, NULL)) != NULL, return NULL);

	return _pacman_trans_add(trans, syncpkg, flags);
}

pmsyncpkg_t *_pacman_trans_add_target(pmtrans_t *trans, const char *target, pmtranstype_t type, int flags)
{
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, NULL));
	ASSERT(!f_strempty(target), RET_ERR(PM_ERR_TRANS_NULL, NULL));

	return NULL;
}

static
pmpkg_t *_pacman_filedb_load(pmdb_t *db, const char *name)
{
	struct stat buf;

	if(stat(name, &buf)) {
		pm_errno = PM_ERR_NOT_A_FILE;
		return NULL;
	}

	_pacman_log(PM_LOG_FLOW2, _("loading target '%s'"), name);
	return _pacman_fpmpackage_load(name);
}

static
int _pacman_add_addtarget(pmtrans_t *trans, const char *name)
{
	pmlist_t *i;
	pmpkg_t *pkg_new, *pkg_local;
	pmdb_t *db_local = trans->handle->db_local;

	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(!_pacman_strempty(name), RET_ERR(PM_ERR_WRONG_ARGS, -1));

	/* Check if we need to add a fake target to the transaction. */
	if(strchr(name, '|')) {
		return(_pacman_fakedb_addtarget(trans, name));
	}

	pkg_new = _pacman_filedb_load(NULL, name);
	if(pkg_new == NULL || _pacman_pkg_is_valid(pkg_new, trans, name) != 0) {
		/* pm_errno is already set by _pacman_filedb_load() */
		goto error;
	}

	pkg_local = _pacman_db_get_pkgfromcache(db_local, pkg_new->name);
	if(trans->type != PM_TRANS_TYPE_UPGRADE) {
		/* only install this package if it is not already installed */
		if(pkg_local != NULL) {
			pm_errno = PM_ERR_PKG_INSTALLED;
			goto error;
		}
	} else {
		if(trans->flags & PM_TRANS_FLAG_FRESHEN) {
			/* only upgrade/install this package if it is already installed and at a lesser version */
			if(pkg_local == NULL || _pacman_versioncmp(pkg_local->version, pkg_new->version) >= 0) {
				pm_errno = PM_ERR_PKG_CANT_FRESH;
				goto error;
			}
		}
	}

	if(trans->flags & PM_TRANS_FLAG_ALLDEPS) {
		pkg_new->reason = PM_PKG_REASON_DEPEND;
	}

	/* copy over the install reason */
	if(pkg_local) {
		pkg_new->reason = (long)_pacman_pkg_getinfo(pkg_local, PM_PKG_REASON);
	}

	/* check if an older version of said package is already in transaction packages.
	 * if so, replace it in the list */
	for(i = trans->packages; i; i = i->next) {
		pmpkg_t *pkg = i->data;
		if(strcmp(pkg->name, pkg_new->name) == 0) {
			if(_pacman_versioncmp(pkg->version, pkg_new->version) < 0) {
				_pacman_log(PM_LOG_WARNING, _("replacing older version %s-%s by %s in target list"),
				          pkg->name, pkg->version, pkg_new->version);
				f_ptrswap(&i->data, (void **)&pkg_new);
			} else {
				_pacman_log(PM_LOG_WARNING, _("newer version %s-%s is in the target list -- skipping"),
				          pkg->name, pkg->version, pkg_new->version);
			}
			FREEPKG(pkg_new);
			return(0);
		}
	}

	/* add the package to the transaction */
	trans->packages = _pacman_list_add(trans->packages, pkg_new);

	return(0);

error:
	FREEPKG(pkg_new);
	return(-1);
}

int _pacman_trans_addtarget(pmtrans_t *trans, const char *target)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->ops != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(target != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	if(_pacman_list_is_strin(target, trans->targets)) {
		RET_ERR(PM_ERR_TRANS_DUP_TARGET, -1);
	}

	if(trans->ops->addtarget != NULL) {
		if(trans->ops->addtarget(trans, target) == -1) {
			/* pm_errno is set by trans->ops->addtarget() */
			return(-1);
		}
	} else {
	if(trans->type & PM_TRANS_TYPE_ADD) {
		if(_pacman_add_addtarget(trans, target) == -1) {
			return -1;
		}
	}
	}

	trans->targets = _pacman_stringlist_append(trans->targets, target);

	return(0);
}

int _pacman_trans_prepare(pmtrans_t *trans, pmlist_t **data)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->ops != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->ops->prepare != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(data != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	*data = NULL;

	/* If there's nothing to do, return without complaining */
	if(_pacman_list_empty(trans->packages) &&
		_pacman_list_empty(trans->syncpkgs)) {
		return(0);
	}

	_pacman_trans_compute_triggers(trans);

	if(trans->ops->prepare(trans, data) == -1) {
		/* pm_errno is set by trans->ops->prepare() */
		return(-1);
	}

	_pacman_trans_set_state(trans, STATE_PREPARED);

	return(0);
}

int _pacman_trans_commit(pmtrans_t *trans, pmlist_t **data)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->ops != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->ops->commit != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	if(data!=NULL)
		*data = NULL;

	/* If there's nothing to do, return without complaining */
	if(_pacman_list_empty(trans->packages) &&
		_pacman_list_empty(trans->syncpkgs)) {
		return(0);
	}

	_pacman_trans_set_state(trans, STATE_COMMITING);

	if(trans->ops->commit(trans, data) == -1) {
		/* pm_errno is set by trans->ops->commit() */
		_pacman_trans_set_state(trans, STATE_PREPARED);
		return(-1);
	}

	_pacman_trans_set_state(trans, STATE_COMMITED);

	return(0);
}

/* vim: set ts=2 sw=2 noet: */
