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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libintl.h>
/* pacman-g2 */
#include "error.h"
#include "package.h"
#include "util.h"
#include "log.h"
#include "list.h"
#include "handle.h"
#include "add.h"
#include "remove.h"
#include "sync.h"
#include "cache.h"
#include "pacman.h"

#include "trans_sysupgrade.h"

pmsyncpkg_t *__pacman_trans_pkg_new (int type, pmpkg_t *spkg, void *data)
{
	pmsyncpkg_t *trans_pkg = _pacman_malloc(sizeof(pmsyncpkg_t));

	if (trans_pkg == NULL) {
		return(NULL);
	}

	trans_pkg->type = type;
	trans_pkg->pkg = spkg;
	trans_pkg->data = data;

	return(trans_pkg);
}

void __pacman_trans_pkg_delete (pmsyncpkg_t *trans_pkg)
{
	if(trans_pkg == NULL) {
		return;
	}

	if(trans_pkg->type == PM_SYNC_TYPE_REPLACE) {
		FREELISTPKGS(trans_pkg->data);
	} else {
		FREEPKG(trans_pkg->data);
	}
	free(trans_pkg);
}

static
int check_oldcache(pmtrans_t *trans)
{
	pmdb_t *db_local = trans->handle->db_local;
	char lastupdate[16] = "";

	if(_pacman_db_getlastupdate(db_local, lastupdate) == -1) {
		return(-1);
	}
	if(strlen(db_local->lastupdate) && strcmp(lastupdate, db_local->lastupdate) != 0) {
		_pacman_log(PM_LOG_DEBUG, _("cache for '%s' repo is too old"), db_local->treename);
		_pacman_db_free_pkgcache(db_local);
	} else {
		_pacman_log(PM_LOG_DEBUG, _("cache for '%s' repo is up to date"), db_local->treename);
	}
	return(0);
}

pmsyncpkg_t *__pacman_trans_get_trans_pkg(pmtrans_t *trans, const char *package) {
	pmlist_t *i;
	pmsyncpkg_t *syncpkg;

	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, NULL));

	for(i = trans->packages; i != NULL ; i = i->next) {
		syncpkg = i->data;
		if(syncpkg && !strcmp(syncpkg->pkg->name, package)) {
			return(syncpkg);
		}
	}

	return(NULL);
}

static
void __pacman_trans_fini(struct pmobject *obj) {
	pmlist_t *i;
	pmtrans_t *trans = (pmtrans_t *)obj;

	FREELIST(trans->targets);
	FREELISTPKGS(trans->_packages);
	for(i = trans->packages; i; i = i->next) {
		__pacman_trans_pkg_delete (i->data);
		i->data = NULL;
	}
	FREELIST(trans->packages);
	FREELIST(trans->skiplist);
}

static const
struct pmobject_ops _pacman_trans_ops = {
	.fini = __pacman_trans_fini,
};

pmtrans_t *_pacman_trans_new()
{
	pmtrans_t *trans = _pacman_zalloc(sizeof(pmtrans_t));

	if(trans) {
		__pacman_object_init (&trans->base, &_pacman_trans_ops);
		trans->state = STATE_IDLE;
	}

	return(trans);
}

void _pacman_trans_free(pmtrans_t *trans) {
	_pacman_object_free (&trans->base);
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
	trans->state = STATE_INITIALIZED;

	check_oldcache(trans);

	return(0);
}

int _pacman_trans_sysupgrade(pmtrans_t *trans)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	return(_pacman_sync_sysupgrade(trans, handle->db_local, handle->dbs_sync));
}

static int add_faketarget(pmtrans_t *trans, const char *name)
{
	char *ptr, *p;
	char *str = NULL;
	pmpkg_t *dummy = NULL;

	dummy = _pacman_pkg_new(NULL, NULL);
	if(dummy == NULL) {
		RET_ERR(PM_ERR_MEMORY, -1);
	}

	/* Format: field1=value1|field2=value2|...
	 * Valid fields are "name", "version" and "depend"
	 */
	str = strdup(name);
	ptr = str;
	while((p = strsep(&ptr, "|")) != NULL) {
		char *q;
		if(p[0] == 0) {
			continue;
		}
		q = strchr(p, '=');
		if(q == NULL) { /* not a valid token */
			continue;
		}
		if(strncmp("name", p, q-p) == 0) {
			STRNCPY(dummy->name, q+1, PKG_NAME_LEN);
		} else if(strncmp("version", p, q-p) == 0) {
			STRNCPY(dummy->version, q+1, PKG_VERSION_LEN);
		} else if(strncmp("depend", p, q-p) == 0) {
			dummy->depends = _pacman_list_add(dummy->depends, strdup(q+1));
		} else {
			_pacman_log(PM_LOG_ERROR, _("could not parse token %s"), p);
		}
	}
	FREE(str);
	if(dummy->name[0] == 0 || dummy->version[0] == 0) {
		FREEPKG(dummy);
		RET_ERR(PM_ERR_PKG_INVALID_NAME, -1);
	}

	/* add the package to the transaction */
	trans->_packages = _pacman_list_add(trans->_packages, dummy);

	return(0);
}

int _pacman_trans_addtarget(pmtrans_t *trans, const char *target, __pmtrans_pkg_type_t type, unsigned int flags)
{
	char *pkg_name;
	pmpkg_t *info = NULL;
	pmpkg_t *pkg_local = NULL;
	pmdb_t *db_local;

	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->ops != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(target != NULL && strlen(target) != 0, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	db_local = trans->handle->db_local;
	pkg_name = target;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	if(_pacman_list_is_strin(target, trans->targets)) {
		RET_ERR(PM_ERR_TRANS_DUP_TARGET, -1);
	}

	if(trans->ops->addtarget != NULL) {
		if (trans->ops->addtarget(trans, target) == -1) {
			/* pm_errno is set by trans->ops->addtarget() */
			return(-1);
		}
	} else {

	if (type & _PACMAN_TRANS_PKG_TYPE_ADD) {
		pmlist_t *i;
		pmpkg_t *local;
		struct stat buf;

		/* Check if we need to add a fake target to the transaction. */
		if(strchr(pkg_name, '|')) {
			return(add_faketarget(trans, pkg_name));
		}

		if(stat(pkg_name, &buf)) {
			pm_errno = PM_ERR_NOT_A_FILE;
			goto error;
		}

		_pacman_log(PM_LOG_FLOW2, _("loading target '%s'"), pkg_name);
		info = _pacman_pkg_load(pkg_name);
		if(info == NULL) {
			/* pm_errno is already set by pkg_load() */
			goto error;
		}

		/* no additional hyphens in version strings */
		if(strchr(_pacman_pkg_getinfo(info, PM_PKG_VERSION), '-') !=
				strrchr(_pacman_pkg_getinfo(info, PM_PKG_VERSION), '-')) {
			pm_errno = PM_ERR_PKG_INVALID_NAME;
			goto error;
		}

		pkg_local = _pacman_db_get_pkgfromcache(db_local, _pacman_pkg_getinfo(info, PM_PKG_NAME));
		if(trans->type != PM_TRANS_TYPE_UPGRADE) {
			/* only install this package if it is not already installed */
			if(pkg_local) {
				pm_errno = PM_ERR_PKG_INSTALLED;
				goto error;
			}
		} else {
			if(trans->flags & PM_TRANS_FLAG_FRESHEN) {
				/* only upgrade/install this package if it is already installed and at a lesser version */
				if(pkg_local == NULL || _pacman_versioncmp(pkg_local->version, info->version) >= 0) {
					pm_errno = PM_ERR_PKG_CANT_FRESH;
					goto error;
				}
			}
		}

		/* check if an older version of said package is already in transaction packages.
		* if so, replace it in the list */
		for(i = trans->_packages; i; i = i->next) {
			pmpkg_t *pkg = i->data;
			if(strcmp(pkg->name, _pacman_pkg_getinfo(info, PM_PKG_NAME)) == 0) {
				if(_pacman_versioncmp(pkg->version, info->version) < 0) {
					pmpkg_t *newpkg;
					_pacman_log(PM_LOG_WARNING, _("replacing older version %s-%s by %s in target list"),
						pkg->name, pkg->version, info->version);
					if((newpkg = _pacman_pkg_load(pkg_name)) == NULL) {
						/* pm_errno is already set by pkg_load() */
						goto error;
					}
					FREEPKG(i->data);
					i->data = newpkg;
				} else {
					_pacman_log(PM_LOG_WARNING, _("newer version %s-%s is in the target list -- skipping"),
						pkg->name, pkg->version, info->version);
				}
				return(0);
			}
		}

		if(trans->flags & PM_TRANS_FLAG_ALLDEPS) {
			info->reason = PM_PKG_REASON_DEPEND;
		}

		/* copy over the install reason */
		local =  _pacman_db_get_pkgfromcache(db_local, info->name);
		if(local) {
			info->reason = (long)_pacman_pkg_getinfo(local, PM_PKG_REASON);
		}
		goto out;
	}

	if (type & _PACMAN_TRANS_PKG_TYPE_REMOVE) {
		if(_pacman_pkg_isin(pkg_name, trans->_packages)) {
			RET_ERR(PM_ERR_TRANS_DUP_TARGET, -1);
		}

		if((info = _pacman_db_scan(db_local, pkg_name, INFRQ_ALL)) == NULL) {
			_pacman_log(PM_LOG_ERROR, _("could not find %s in database"), pkg_name);
			RET_ERR(PM_ERR_PKG_NOT_FOUND, -1);
		}

		/* ignore holdpkgs on upgrade */
		if((trans == handle->trans) && _pacman_list_is_strin(info->name, handle->holdpkg)) {
			int resp = 0;
			QUESTION(trans, PM_TRANS_CONV_REMOVE_HOLDPKG, info, NULL, NULL, &resp);
			if(!resp) {
				RET_ERR(PM_ERR_PKG_HOLD, -1);
			}
		}

	}
out:
	_pacman_log(PM_LOG_FLOW2, _("adding %s in the targets list"), info->name);
	trans->_packages = _pacman_list_add(trans->_packages, info);

	}

	trans->targets = _pacman_list_add(trans->targets, strdup(target));
	return(0);

error:
	FREEPKG(info);
	return(-1);
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
	trans->state = new_state;

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
	if(trans->_packages == NULL && trans->packages == NULL) {
		return(0);
	}

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
	if(trans->_packages == NULL && trans->packages == NULL) {
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
