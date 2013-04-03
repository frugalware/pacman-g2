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

/* pacman-g2 */
#include "trans.h"

#include "deps.h"
#include "error.h"
#include "package.h"
#include "util.h"
#include "log.h"
#include "handle.h"
#include "add.h"
#include "remove.h"
#include "sync.h"
#include "cache.h"
#include "pacman.h"

#include "trans_sysupgrade.h"

pmsyncpkg_t *__pacman_trans_pkg_new(int type, pmpkg_t *spkg)
{
	pmsyncpkg_t *trans_pkg = _pacman_zalloc(sizeof(pmsyncpkg_t));

	if (trans_pkg == NULL) {
		return(NULL);
	}

	trans_pkg->type = type;
	trans_pkg->pkg_new = spkg;

	return(trans_pkg);
}

void __pacman_trans_pkg_delete(pmsyncpkg_t *trans_pkg)
{
	if(trans_pkg == NULL) {
		return;
	}

#if 0
	/* FIXME: Enable when pmtrans_t::_packages is gone */
	FREEPKG(trans_pkg->pkg_new);
	FREEPKG(trans_pkg->pkg_local);
#endif
	FREELISTPKGS(trans_pkg->replaces);
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
		if(syncpkg && !strcmp(syncpkg->pkg_new->name, package)) {
			return(syncpkg);
		}
	}

	return(NULL);
}

static
void __pacman_trans_fini(struct pmobject *obj) {
	pmtrans_t *trans = (pmtrans_t *)obj;

	FREELIST(trans->targets);
	_pacman_list_free(trans->packages, (_pacman_fn_free)__pacman_trans_pkg_delete);
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

	return(_pacman_sync_sysupgrade(trans));
}

static
pmpkg_t *fakepkg_create(const char *name)
{
	char *ptr, *p;
	char *str = NULL;
	pmpkg_t *dummy = NULL;

	dummy = _pacman_pkg_new(NULL, NULL);
	if(dummy == NULL) {
		goto error;
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
		pm_errno = PM_ERR_PKG_INVALID_NAME;
		goto error;
	}
	return(dummy);

error:
	FREEPKG(dummy);
	return NULL;
}

int _pacman_trans_addtarget(pmtrans_t *trans, const char *target, pmtranstype_t type, unsigned int flags)
{
	char *pkg_name;
	pmsyncpkg_t *trans_pkg = __pacman_trans_pkg_new(type, NULL);
	pmdb_t *db_local;

	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->ops != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(target != NULL && strlen(target) != 0, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(trans_pkg != NULL, return -1); /* pm_errno is allready set by __pacman_trans_pkg_new */

	db_local = trans->handle->db_local;
	pkg_name = target;
	trans_pkg->flags = flags;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	if(_pacman_strlist_find(trans->targets, target)) {
		RET_ERR(PM_ERR_TRANS_DUP_TARGET, -1);
	}

	if(trans->ops->addtarget != NULL) {
		if (trans->ops->addtarget(trans, target) == -1) {
			/* pm_errno is set by trans->ops->addtarget() */
			return(-1);
		}
	} else {

	if (type & PM_TRANS_TYPE_ADD) {
		pmlist_t *i;
		struct stat buf;

		/* Check if we need to add a fake target to the transaction. */
		if(strchr(pkg_name, '|')) {
			trans_pkg->pkg_new = fakepkg_create(pkg_name);
			if (trans_pkg->pkg_new) {
				goto out;
			}
			goto error;
		}

		if (stat(pkg_name, &buf) == 0) {
			_pacman_log(PM_LOG_FLOW2, _("loading target '%s'"), pkg_name);
			trans_pkg->pkg_new = _pacman_pkg_load(pkg_name);
			if(trans_pkg->pkg_new == NULL) {
				/* pm_errno is already set by pkg_load() */
				goto error;
			}
		}

		if (trans_pkg->pkg_new == NULL) {
			pm_errno = PM_ERR_PKG_NOT_FOUND;
			goto error;
		}

		trans_pkg->pkg_local = _pacman_db_get_pkgfromcache(db_local, _pacman_pkg_getinfo(trans_pkg->pkg_new, PM_PKG_NAME));
		if (trans_pkg->pkg_local != NULL) {
			int cmp = _pacman_versioncmp(trans_pkg->pkg_local->version, trans_pkg->pkg_new->version);

			if (type == PM_TRANS_TYPE_ADD) {
				/* only install this package if it is not already installed */
				pm_errno = PM_ERR_PKG_INSTALLED;
				goto error;
			}
			if (trans->flags & PM_TRANS_FLAG_FRESHEN && cmp >= 0) {
				/* only upgrade/install this package if it is at a lesser version */
				pm_errno = PM_ERR_PKG_CANT_FRESH;
				goto error;
			}
		} else {
			if(trans->flags & PM_TRANS_FLAG_FRESHEN) {
				/* only upgrade/install this package if it is already installed */
				pm_errno = PM_ERR_PKG_CANT_FRESH;
				goto error;
			}
		}

		/* check if an older version of said package is already in transaction packages.
		* if so, replace it in the list */
		for(i = trans->_packages; i; i = i->next) {
			pmpkg_t *pkg = i->data;
			if(strcmp(pkg->name, _pacman_pkg_getinfo(trans_pkg->pkg_new, PM_PKG_NAME)) == 0) {
				if(_pacman_versioncmp(pkg->version, trans_pkg->pkg_new->version) < 0) {
					pmpkg_t *newpkg;
					_pacman_log(PM_LOG_WARNING, _("replacing older version %s-%s by %s in target list"),
						pkg->name, pkg->version, trans_pkg->pkg_new->version);
					if((newpkg = _pacman_pkg_load(pkg_name)) == NULL) {
						/* pm_errno is already set by pkg_load() */
						goto error;
					}
					FREEPKG(i->data);
					i->data = newpkg;
				} else {
					_pacman_log(PM_LOG_WARNING, _("newer version %s-%s is in the target list -- skipping"),
						pkg->name, pkg->version, trans_pkg->pkg_new->version);
				}
				return(0);
			}
		}

		/* copy over the install reason */
		if (trans_pkg->pkg_local) {
			trans_pkg->pkg_new->reason = (long)_pacman_pkg_getinfo(trans_pkg->pkg_local, PM_PKG_REASON);
		}
		if (trans->flags & PM_TRANS_FLAG_ALLDEPS) {
			trans_pkg->pkg_new->reason = PM_PKG_REASON_DEPEND;
		}
		if (trans_pkg->flags & PM_TRANS_FLAG_EXPLICIT) {
			trans_pkg->pkg_new->reason = PM_PKG_REASON_EXPLICIT;
		}
		goto out;
	}

	if (type & PM_TRANS_TYPE_REMOVE) {
		if(_pacman_pkg_isin(pkg_name, trans->_packages)) {
			RET_ERR(PM_ERR_TRANS_DUP_TARGET, -1);
		}

		if((trans_pkg->pkg_new = _pacman_db_scan(db_local, pkg_name, INFRQ_ALL)) == NULL) {
			_pacman_log(PM_LOG_ERROR, _("could not find %s in database"), pkg_name);
			RET_ERR(PM_ERR_PKG_NOT_FOUND, -1);
		}

		/* ignore holdpkgs on upgrade */
		if((trans == handle->trans) && _pacman_strlist_find(handle->holdpkg, trans_pkg->pkg_new->name)) {
			int resp = 0;
			QUESTION(trans, PM_TRANS_CONV_REMOVE_HOLDPKG, trans_pkg->pkg_new, NULL, NULL, &resp);
			if(!resp) {
				RET_ERR(PM_ERR_PKG_HOLD, -1);
			}
		}

	}
out:
	_pacman_log(PM_LOG_FLOW2, _("adding %s in the targets list"), trans_pkg->pkg_new->name);
	trans->_packages = _pacman_list_add(trans->_packages, trans_pkg->pkg_new);
	trans->packages = _pacman_list_add(trans->packages, trans_pkg);
	}

	trans->targets = _pacman_list_add(trans->targets, strdup(target));
	return(0);

error:
	__pacman_trans_pkg_delete(trans_pkg);
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
	pmdb_t *db_local;

	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->ops != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(data != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	*data = NULL;
	db_local = trans->handle->db_local;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	/* If there's nothing to do, return without complaining */
	if(trans->_packages == NULL && trans->packages == NULL) {
		return(0);
	}

	if(trans->ops->prepare != NULL) {
		if(trans->ops->prepare(trans, data) == -1) {
			/* pm_errno is set by trans->ops->prepare() */
			return(-1);
		}
	} else {

	pmlist_t *lp;
	pmlist_t *rmlist = NULL;
	char rm_fname[PATH_MAX];
	pmpkg_t *info = NULL;

	if (trans->type & PM_TRANS_TYPE_ADD) {
		/* Check dependencies */
		if(!(trans->flags & PM_TRANS_FLAG_NODEPS)) {
			EVENT(trans, PM_TRANS_EVT_CHECKDEPS_START, NULL, NULL);
			/* look for unsatisfied dependencies */
			_pacman_log(PM_LOG_FLOW1, _("looking for unsatisfied dependencies"));
			lp = _pacman_checkdeps(trans, db_local, trans->type, trans->_packages);
			if(lp != NULL) {
				if(data) {
					*data = lp;
				} else {
					FREELIST(lp);
				}
				RET_ERR(PM_ERR_UNSATISFIED_DEPS, -1);
			}

			/* no unsatisfied deps, so look for conflicts */
			_pacman_log(PM_LOG_FLOW1, _("looking for conflicts"));
			lp = _pacman_checkconflicts(trans, db_local, trans->_packages);
			if(lp != NULL) {
				if(data) {
					*data = lp;
				} else {
					FREELIST(lp);
				}
				RET_ERR(PM_ERR_CONFLICTING_DEPS, -1);
			}

			/* re-order w.r.t. dependencies */
			_pacman_log(PM_LOG_FLOW1, _("sorting by dependencies"));
			lp = _pacman_sortbydeps(trans->_packages, PM_TRANS_TYPE_ADD);
			/* free the old alltargs */
			FREELISTPTR(trans->_packages);
			trans->_packages = lp;
			EVENT(trans, PM_TRANS_EVT_CHECKDEPS_DONE, NULL, NULL);
		}

		/* Cleaning up */
		EVENT(trans, PM_TRANS_EVT_CLEANUP_START, NULL, NULL);
		_pacman_log(PM_LOG_FLOW1, _("cleaning up"));
		for (lp=trans->_packages; lp!=NULL; lp=lp->next) {
			info=(pmpkg_t *)lp->data;
			for (rmlist=info->removes; rmlist!=NULL; rmlist=rmlist->next) {
				snprintf(rm_fname, PATH_MAX, "%s%s", handle->root, (char *)rmlist->data);
				remove(rm_fname);
			}
		}
		EVENT(trans, PM_TRANS_EVT_CLEANUP_DONE, NULL, NULL);

		/* Check for file conflicts */
		if(!(trans->flags & PM_TRANS_FLAG_FORCE)) {
			pmlist_t *skiplist = NULL;

			EVENT(trans, PM_TRANS_EVT_FILECONFLICTS_START, NULL, NULL);

			_pacman_log(PM_LOG_FLOW1, _("looking for file conflicts"));
			lp = _pacman_db_find_conflicts(db_local, trans, handle->root, &skiplist);
			if(lp != NULL) {
				if(data) {
					*data = lp;
				} else {
					FREELIST(lp);
				}
				FREELIST(skiplist);
				RET_ERR(PM_ERR_FILE_CONFLICTS, -1);
			}

			/* copy the file skiplist into the transaction */
			trans->skiplist = skiplist;

			EVENT(trans, PM_TRANS_EVT_FILECONFLICTS_DONE, NULL, NULL);
		}

#ifndef __sun__
		if(_pacman_check_freespace(trans, data) == -1) {
			/* pm_errno is set by check_freespace */
			return(-1);
		}
#endif
	}

	if(!(trans->flags & (PM_TRANS_FLAG_NODEPS)) && (trans->type != PM_TRANS_TYPE_UPGRADE)) {
		EVENT(trans, PM_TRANS_EVT_CHECKDEPS_START, NULL, NULL);

		_pacman_log(PM_LOG_FLOW1, _("looking for unsatisfied dependencies"));
		lp = _pacman_checkdeps(trans, db_local, trans->type, trans->_packages);
		if(lp != NULL) {
			if(trans->flags & PM_TRANS_FLAG_CASCADE) {
				while(lp) {
					pmlist_t *i;
					for(i = lp; i; i = i->next) {
						pmdepmissing_t *miss = (pmdepmissing_t *)i->data;
						pmpkg_t *info = _pacman_db_scan(db_local, miss->depend.name, INFRQ_ALL);
						if(info) {
							_pacman_log(PM_LOG_FLOW2, _("pulling %s in the targets list"), info->name);
							trans->_packages = _pacman_list_add(trans->_packages, info);
						} else {
							_pacman_log(PM_LOG_ERROR, _("could not find %s in database -- skipping"),
								miss->depend.name);
						}
					}
					FREELIST(lp);
					lp = _pacman_checkdeps(trans, db_local, trans->type, trans->_packages);
				}
			} else {
				if(data) {
					*data = lp;
				} else {
					FREELIST(lp);
				}
				RET_ERR(PM_ERR_UNSATISFIED_DEPS, -1);
			}
		}

		if(trans->flags & PM_TRANS_FLAG_RECURSE) {
			_pacman_log(PM_LOG_FLOW1, _("finding removable dependencies"));
			trans->_packages = _pacman_removedeps(db_local, trans->_packages);
		}

		/* re-order w.r.t. dependencies */
		_pacman_log(PM_LOG_FLOW1, _("sorting by dependencies"));
		lp = _pacman_sortbydeps(trans->_packages, PM_TRANS_TYPE_REMOVE);
		/* free the old alltargs */
		FREELISTPTR(trans->_packages);
		trans->_packages = lp;

		EVENT(trans, PM_TRANS_EVT_CHECKDEPS_DONE, NULL, NULL);
	}

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
