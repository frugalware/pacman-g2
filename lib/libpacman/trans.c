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

#include <errno.h>
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
			trans->ops = NULL;
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
	ASSERT(target != NULL && strlen(target) != 0, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(trans_pkg != NULL, return -1); /* pm_errno is allready set by __pacman_trans_pkg_new */

	db_local = trans->handle->db_local;
	pkg_name = target;
	trans_pkg->flags = flags;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	if(_pacman_strlist_find(trans->targets, target)) {
		RET_ERR(PM_ERR_TRANS_DUP_TARGET, -1);
	}

	if(trans->ops != NULL && trans->ops->addtarget != NULL) {
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
	ASSERT(data != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	*data = NULL;
	db_local = trans->handle->db_local;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	/* If there's nothing to do, return without complaining */
	if(trans->_packages == NULL && trans->packages == NULL) {
		return(0);
	}

	if(trans->ops != NULL && trans->ops->prepare != NULL) {
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
	pmdb_t *db_local;
	int ret = 0;

	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	if(data!=NULL)
		*data = NULL;

	db_local = trans->handle->db_local;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	/* If there's nothing to do, return without complaining */
	if(trans->_packages == NULL && trans->packages == NULL) {
		return(0);
	}

	_pacman_trans_set_state(trans, STATE_COMMITING);

	if (trans->ops != NULL && trans->ops->commit != NULL) {
		if (trans->ops->commit(trans, data) == -1) {
			/* pm_errno is set by trans->ops->commit() */
			_pacman_trans_set_state(trans, STATE_PREPARED);
			return(-1);
		}
	} else {
	int i, errors = 0, needdisp = 0;
	int remain, howmany, archive_ret;
	double percent;
	register struct archive *archive;
	struct archive_entry *entry;
	char expath[PATH_MAX], cwd[PATH_MAX] = "", *what;
	unsigned char cb_state;
	time_t t;
	pmlist_t *targ, *lp;

	for(targ = trans->_packages; targ; targ = targ->next) {
		unsigned short pmo_upgrade;
		char pm_install[PATH_MAX];
		pmpkg_t *info = (pmpkg_t *)targ->data;
		pmpkg_t *oldpkg = NULL;
		errors = 0;
		remain = _pacman_list_count(targ);
		howmany = _pacman_list_count(trans->_packages);

		if(handle->trans->state == STATE_INTERRUPTED) {
			break;
		}

		pmo_upgrade = (trans->type == PM_TRANS_TYPE_UPGRADE) ? 1 : 0;

		/* see if this is an upgrade.  if so, remove the old package first */
		if(pmo_upgrade) {
			pmpkg_t *local = _pacman_db_get_pkgfromcache(db_local, info->name);
			if(local) {
				EVENT(trans, PM_TRANS_EVT_UPGRADE_START, info, NULL);
				cb_state = PM_TRANS_PROGRESS_UPGRADE_START;
				_pacman_log(PM_LOG_FLOW1, _("upgrading package %s-%s"), info->name, info->version);
				if((what = (char *)malloc(strlen(info->name)+1)) == NULL) {
					RET_ERR(PM_ERR_MEMORY, -1);
				}
				STRNCPY(what, info->name, strlen(info->name)+1);

				/* we'll need to save some record for backup checks later */
				oldpkg = _pacman_pkg_new(local->name, local->version);
				if(oldpkg) {
					if(!(local->infolevel & INFRQ_FILES)) {
						_pacman_log(PM_LOG_DEBUG, _("loading FILES info for '%s'"), local->name);
						_pacman_db_read(db_local, INFRQ_FILES, local);
					}
					oldpkg->backup = _pacman_strlist_dup(local->backup);
					strncpy(oldpkg->name, local->name, PKG_NAME_LEN);
					strncpy(oldpkg->version, local->version, PKG_VERSION_LEN);
				}

				/* pre_upgrade scriptlet */
				if(info->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
					_pacman_runscriptlet(handle->root, info->data, "pre_upgrade", info->version, oldpkg ? oldpkg->version : NULL,
						trans);
				}

				if(oldpkg) {
					pmtrans_t *tr;
					_pacman_log(PM_LOG_FLOW1, _("removing old package first (%s-%s)"), oldpkg->name, oldpkg->version);
					tr = _pacman_trans_new();
					if(tr == NULL) {
						RET_ERR(PM_ERR_TRANS_ABORT, -1);
					}
					pmtrans_cbs_t null_cbs = { NULL };
					if(_pacman_trans_init(tr, PM_TRANS_TYPE_UPGRADE, trans->flags, null_cbs) == -1) {
						FREETRANS(tr);
						RET_ERR(PM_ERR_TRANS_ABORT, -1);
					}
					if(_pacman_trans_addtarget(tr, info->name, PM_TRANS_TYPE_REMOVE, 0) == -1) {
						FREETRANS(tr);
						RET_ERR(PM_ERR_TRANS_ABORT, -1);
					}
					/* copy the skiplist over */
					tr->skiplist = _pacman_strlist_dup(trans->skiplist);
					if(_pacman_remove_commit(tr, NULL) == -1) {
						FREETRANS(tr);
						RET_ERR(PM_ERR_TRANS_ABORT, -1);
					}
					FREETRANS(tr);
				}
			} else {
				/* no previous package version is installed, so this is actually
				 * just an install.  */
				pmo_upgrade = 0;
			}
		}
		if(!pmo_upgrade) {
			EVENT(trans, PM_TRANS_EVT_ADD_START, info, NULL);
			cb_state = PM_TRANS_PROGRESS_ADD_START;
			_pacman_log(PM_LOG_FLOW1, _("adding package %s-%s"), info->name, info->version);
			if((what = (char *)malloc(strlen(info->name)+1)) == NULL) {
				RET_ERR(PM_ERR_MEMORY, -1);
			}
			STRNCPY(what, info->name, strlen(info->name)+1);

			/* pre_install scriptlet */
			if(info->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
				_pacman_runscriptlet(handle->root, info->data, "pre_install", info->version, NULL, trans);
			}
		} else {
			_pacman_log(PM_LOG_FLOW1, _("adding new package %s-%s"), info->name, info->version);
		}

		if(!(trans->flags & PM_TRANS_FLAG_DBONLY)) {
			_pacman_log(PM_LOG_FLOW1, _("extracting files"));

			/* Extract the package */
			if ((archive = archive_read_new ()) == NULL)
				RET_ERR(PM_ERR_LIBARCHIVE_ERROR, -1);

			archive_read_support_compression_all (archive);
			archive_read_support_format_all (archive);

			if (archive_read_open_file (archive, info->data, PM_DEFAULT_BYTES_PER_BLOCK) != ARCHIVE_OK) {
				RET_ERR(PM_ERR_PKG_OPEN, -1);
			}

			/* save the cwd so we can restore it later */
			if(getcwd(cwd, PATH_MAX) == NULL) {
				_pacman_log(PM_LOG_ERROR, _("could not get current working directory"));
				/* in case of error, cwd content is undefined: so we set it to something */
				cwd[0] = 0;
			}

			/* libarchive requires this for extracting hard links */
			chdir(handle->root);

			for(i = 0; (archive_ret = archive_read_next_header (archive, &entry)) == ARCHIVE_OK; i++) {
				int nb = 0;
				int notouch = 0;
				char *md5_orig = NULL;
				char *sha1_orig = NULL;
				char pathname[PATH_MAX];
				struct stat buf;

				STRNCPY(pathname, archive_entry_pathname (entry), PATH_MAX);

				if (info->size != 0)
		    			percent = (double)archive_position_uncompressed(archive) / info->size;
				if (needdisp == 0) {
					PROGRESS(trans, cb_state, what, (int)(percent * 100), howmany, (howmany - remain +1));
				}

				if(!strcmp(pathname, ".PKGINFO") || !strcmp(pathname, ".FILELIST")) {
					archive_read_data_skip (archive);
					continue;
				}

				/*if(!strcmp(pathname, "._install") || !strcmp(pathname, ".INSTALL")) {
				*	 the install script goes inside the db
				*	snprintf(expath, PATH_MAX, "%s/%s-%s/install", db->path, info->name, info->version); */
				if(!strcmp(pathname, "._install") || !strcmp(pathname, ".INSTALL") ||
					!strcmp(pathname, ".CHANGELOG")) {
					if(!strcmp(pathname, ".CHANGELOG")) {
						/* the changelog goes inside the db */
						snprintf(expath, PATH_MAX, "%s/%s-%s/changelog", db_local->path,
							info->name, info->version);
					} else {
						/* the install script goes inside the db */
						snprintf(expath, PATH_MAX, "%s/%s-%s/install", db_local->path,
							info->name, info->version);
					}
				} else {
					/* build the new pathname relative to handle->root */
					snprintf(expath, PATH_MAX, "%s%s", handle->root, pathname);
					if(expath[strlen(expath)-1] == '/') {
						expath[strlen(expath)-1] = '\0';
					}
				}

				/* if a file is in NoExtract then we never extract it.
				 *
				 * eg, /home/httpd/html/index.html may be removed so index.php
				 * could be used.
				 */
				if(_pacman_strlist_find(handle->noextract, pathname)) {
					pacman_logaction(_("notice: %s is in NoExtract -- skipping extraction"), pathname);
					archive_read_data_skip (archive);
					continue;
				}

				if(!lstat(expath, &buf)) {
					/* file already exists */
					if(S_ISLNK(buf.st_mode)) {
						continue;
					} else if(!S_ISDIR(buf.st_mode)) {
						if(_pacman_strlist_find(handle->noupgrade, pathname)) {
							notouch = 1;
						} else {
							if(!pmo_upgrade || oldpkg == NULL) {
								nb = _pacman_strlist_find(info->backup, pathname);
							} else {
								/* op == PM_TRANS_TYPE_UPGRADE */
								md5_orig = _pacman_needbackup(pathname, oldpkg->backup);
								sha1_orig = _pacman_needbackup(pathname, oldpkg->backup);
								if(md5_orig || sha1_orig) {
									nb = 1;
								}
							}
						}
					}
				}

				if(nb) {
					char *temp;
					char *md5_local, *md5_pkg;
					char *sha1_local, *sha1_pkg;
					int fd;

					/* extract the package's version to a temporary file and md5 it */
					temp = strdup("/tmp/pacman_XXXXXX");
					fd = mkstemp(temp);

					archive_entry_set_pathname (entry, temp);

					if(archive_read_extract (archive, entry, ARCHIVE_EXTRACT_FLAGS) != ARCHIVE_OK) {
						_pacman_log(PM_LOG_ERROR, _("could not extract %s (%s)"), pathname, strerror(errno));
						pacman_logaction(_("could not extract %s (%s)"), pathname, strerror(errno));
						errors++;
						unlink(temp);
						FREE(temp);
						FREE(md5_orig);
						FREE(sha1_orig);
						close(fd);
						continue;
					}
					md5_local = _pacman_MDFile(expath);
					md5_pkg = _pacman_MDFile(temp);
					sha1_local = _pacman_SHAFile(expath);
					sha1_pkg = _pacman_SHAFile(temp);
					/* append the new md5 or sha1 hash to it's respective entry in info->backup
					 * (it will be the new orginal)
					 */
					for(lp = info->backup; lp; lp = lp->next) {
						char *fn;
						char *file = lp->data;

						if(!file) continue;
						if(!strcmp(file, pathname)) {
						    if(info->sha1sum != NULL && info->sha1sum != '\0') {
							/* 32 for the hash, 1 for the terminating NULL, and 1 for the tab delimiter */
							if((fn = (char *)malloc(strlen(file)+34)) == NULL) {
								RET_ERR(PM_ERR_MEMORY, -1);
							}
							sprintf(fn, "%s\t%s", file, md5_pkg);
							FREE(file);
							lp->data = fn;
						    } else {
							/* 41 for the hash, 1 for the terminating NULL, and 1 for the tab delimiter */
							if((fn = (char *)malloc(strlen(file)+43)) == NULL) {
								RET_ERR(PM_ERR_MEMORY, -1);
							}
							sprintf(fn, "%s\t%s", file, sha1_pkg);
							FREE(file);
							lp->data = fn;
						    }
						}
					}

					if (info->sha1sum != NULL && info->sha1sum != '\0') {
					_pacman_log(PM_LOG_DEBUG, _("checking md5 hashes for %s"), pathname);
					_pacman_log(PM_LOG_DEBUG, _("current:  %s"), md5_local);
					_pacman_log(PM_LOG_DEBUG, _("new:      %s"), md5_pkg);
					if(md5_orig) {
						_pacman_log(PM_LOG_DEBUG, _("original: %s"), md5_orig);
					}
					} else {
                                        _pacman_log(PM_LOG_DEBUG, _("checking sha1 hashes for %s"), pathname);
					_pacman_log(PM_LOG_DEBUG, _("current:  %s"), sha1_local);
                                        _pacman_log(PM_LOG_DEBUG, _("new:      %s"), sha1_pkg);
                                        if(sha1_orig) {
                                        _pacman_log(PM_LOG_DEBUG, _("original: %s"), sha1_orig);
					}
					}

					if(!pmo_upgrade) {
						/* PM_ADD */

						/* if a file already exists with a different md5 or sha1 hash,
						 * then we rename it to a .pacorig extension and continue */
						if(strcmp(md5_local, md5_pkg) || strcmp(sha1_local, sha1_pkg)) {
							char newpath[PATH_MAX];
							snprintf(newpath, PATH_MAX, "%s.pacorig", expath);
							if(rename(expath, newpath)) {
								archive_entry_set_pathname (entry, expath);
								_pacman_log(PM_LOG_ERROR, _("could not rename %s (%s)"), pathname, strerror(errno));
								pacman_logaction(_("error: could not rename %s (%s)"), expath, strerror(errno));
							}
							if(_pacman_copyfile(temp, expath)) {
								archive_entry_set_pathname (entry, expath);
								_pacman_log(PM_LOG_ERROR, _("could not copy %s to %s (%s)"), temp, pathname, strerror(errno));
								pacman_logaction(_("error: could not copy %s to %s (%s)"), temp, expath, strerror(errno));
								errors++;
							} else {
								archive_entry_set_pathname (entry, expath);
								_pacman_log(PM_LOG_WARNING, _("%s saved as %s.pacorig"), pathname, pathname);
								pacman_logaction(_("warning: %s saved as %s"), expath, newpath);
							}
						}
					} else if(md5_orig || sha1_orig) {
						/* PM_UPGRADE */
						int installnew = 0;

						/* the fun part */
						if(!strcmp(md5_orig, md5_local)|| !strcmp(sha1_orig, sha1_local)) {
							if(!strcmp(md5_local, md5_pkg) || !strcmp(sha1_local, sha1_pkg)) {
								_pacman_log(PM_LOG_DEBUG, _("action: installing new file"));
								installnew = 1;
							} else {
								_pacman_log(PM_LOG_DEBUG, _("action: installing new file"));
								installnew = 1;
							}
						} else if(!strcmp(md5_orig, md5_pkg) || !strcmp(sha1_orig, sha1_pkg)) {
							_pacman_log(PM_LOG_DEBUG, _("action: leaving existing file in place"));
						} else if(!strcmp(md5_local, md5_pkg) || !strcmp(sha1_local, sha1_pkg)) {
							_pacman_log(PM_LOG_DEBUG, _("action: installing new file"));
							installnew = 1;
						} else {
							char newpath[PATH_MAX];
							_pacman_log(PM_LOG_DEBUG, _("action: keeping current file and installing new one with .pacnew ending"));
							installnew = 0;
							snprintf(newpath, PATH_MAX, "%s.pacnew", expath);
							if(_pacman_copyfile(temp, newpath)) {
								_pacman_log(PM_LOG_ERROR, _("could not install %s as %s: %s"), expath, newpath, strerror(errno));
								pacman_logaction(_("error: could not install %s as %s: %s"), expath, newpath, strerror(errno));
							} else {
								_pacman_log(PM_LOG_WARNING, _("%s installed as %s"), expath, newpath);
								pacman_logaction(_("warning: %s installed as %s"), expath, newpath);
							}
						}

						if(installnew) {
							_pacman_log(PM_LOG_FLOW2, _("extracting %s"), pathname);
							if(_pacman_copyfile(temp, expath)) {
								_pacman_log(PM_LOG_ERROR, _("could not copy %s to %s (%s)"), temp, pathname, strerror(errno));
								errors++;
							}
						    archive_entry_set_pathname (entry, expath);
						}
					}

					FREE(md5_local);
					FREE(md5_pkg);
					FREE(md5_orig);
					FREE(sha1_local);
					FREE(sha1_pkg);
					FREE(sha1_orig);
					unlink(temp);
					FREE(temp);
					close(fd);
				} else {
					if(!notouch) {
						_pacman_log(PM_LOG_FLOW2, _("extracting %s"), pathname);
					} else {
						_pacman_log(PM_LOG_FLOW2, _("%s is in NoUpgrade -- skipping"), pathname);
						strncat(expath, ".pacnew", PATH_MAX);
						_pacman_log(PM_LOG_WARNING, _("extracting %s as %s.pacnew"), pathname, pathname);
						pacman_logaction(_("warning: extracting %s%s as %s"), handle->root, pathname, expath);
						/*tar_skip_regfile(tar);*/
					}
					if(trans->flags & PM_TRANS_FLAG_FORCE) {
						/* if FORCE was used, then unlink() each file (whether it's there
						 * or not) before extracting.  this prevents the old "Text file busy"
						 * error that crops up if one tries to --force a glibc or pacman-g2
						 * upgrade.
						 */
						unlink(expath);
					}
					archive_entry_set_pathname (entry, expath);
					if(archive_read_extract (archive, entry, ARCHIVE_EXTRACT_FLAGS) != ARCHIVE_OK) {
						_pacman_log(PM_LOG_ERROR, _("could not extract %s (%s)"), expath, strerror(errno));
						pacman_logaction(_("error: could not extract %s (%s)"), expath, strerror(errno));
						errors++;
					}
					/* calculate an md5 or sha1 hash if this is in info->backup */
					for(lp = info->backup; lp; lp = lp->next) {
						char *fn, *md5, *sha1;
						char path[PATH_MAX];
						char *file = lp->data;

						if(!file) continue;
						if(!strcmp(file, pathname)) {
							_pacman_log(PM_LOG_DEBUG, _("appending backup entry"));
							snprintf(path, PATH_MAX, "%s%s", handle->root, file);
							if (info->sha1sum != NULL && info->sha1sum != '\0') {
							    md5 = _pacman_MDFile(path);
							    /* 32 for the hash, 1 for the terminating NULL, and 1 for the tab delimiter */
							    if((fn = (char *)malloc(strlen(file)+34)) == NULL) {
										RET_ERR(PM_ERR_MEMORY, -1);
									}
							    sprintf(fn, "%s\t%s", file, md5);
							    FREE(md5);
							} else {
							    /* 41 for the hash, 1 for the terminating NULL, and 1 for the tab delimiter */
							    sha1 = _pacman_SHAFile(path);
							    if((fn = (char *)malloc(strlen(file)+43)) == NULL) {
										RET_ERR(PM_ERR_MEMORY, -1);
									}
							    sprintf(fn, "%s\t%s", file, sha1);
							    FREE(sha1);
							}
							FREE(file);
							lp->data = fn;
						}
					}
				}
			}
			if(archive_ret == ARCHIVE_FATAL) {
				errors++;
			}
			if(strlen(cwd)) {
				chdir(cwd);
			}
			archive_read_finish (archive);

			if(errors) {
				ret = 1;
				_pacman_log(PM_LOG_WARNING, _("errors occurred while %s %s"),
					(pmo_upgrade ? _("upgrading") : _("installing")), info->name);
				pacman_logaction(_("errors occurred while %s %s"),
					(pmo_upgrade ? _("upgrading") : _("installing")), info->name);
			} else {
			PROGRESS(trans, cb_state, what, 100, howmany, howmany - remain + 1);
			}
		}

		/* Add the package to the database */
		t = time(NULL);

		/* Update the requiredby field by scanning the whole database
		 * looking for packages depending on the package to add */
		for(lp = _pacman_db_get_pkgcache(db_local); lp; lp = lp->next) {
			pmpkg_t *tmpp = lp->data;
			pmlist_t *tmppm = NULL;
			if(tmpp == NULL) {
				continue;
			}
			for(tmppm = tmpp->depends; tmppm; tmppm = tmppm->next) {
				pmdepend_t depend;
				if(_pacman_splitdep(tmppm->data, &depend)) {
					continue;
				}
				if(tmppm->data && (!strcmp(depend.name, info->name) || _pacman_strlist_find(info->provides, depend.name))) {
					_pacman_log(PM_LOG_DEBUG, _("adding '%s' in requiredby field for '%s'"), tmpp->name, info->name);
					info->requiredby = _pacman_list_add(info->requiredby, strdup(tmpp->name));
				}
			}
		}

		/* make an install date (in UTC) */
		STRNCPY(info->installdate, asctime(gmtime(&t)), sizeof(info->installdate));
		/* remove the extra line feed appended by asctime() */
		info->installdate[strlen(info->installdate)-1] = 0;

		_pacman_log(PM_LOG_FLOW1, _("updating database"));
		_pacman_log(PM_LOG_FLOW2, _("adding database entry '%s'"), info->name);
		if(_pacman_db_write(db_local, info, INFRQ_ALL)) {
			_pacman_log(PM_LOG_ERROR, _("could not update database entry %s-%s"),
			          info->name, info->version);
			pacman_logaction(NULL, _("error updating database for %s-%s!"), info->name, info->version);
			RET_ERR(PM_ERR_DB_WRITE, -1);
		}
		if(_pacman_db_add_pkgincache(db_local, info) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not add entry '%s' in cache"), info->name);
		}

		/* update dependency packages' REQUIREDBY fields */
		if(info->depends) {
			_pacman_log(PM_LOG_FLOW2, _("updating dependency packages 'requiredby' fields"));
		}
		for(lp = info->depends; lp; lp = lp->next) {
			pmpkg_t *depinfo;
			pmdepend_t depend;
			if(_pacman_splitdep(lp->data, &depend)) {
				continue;
			}
			depinfo = _pacman_db_get_pkgfromcache(db_local, depend.name);
			if(depinfo == NULL) {
				/* look for a provides package */
				pmlist_t *provides = _pacman_db_whatprovides(db_local, depend.name);
				if(provides) {
					/* TODO: should check _all_ packages listed in provides, not just
					 *       the first one.
					 */
					/* use the first one */
					depinfo = _pacman_db_get_pkgfromcache(db_local, ((pmpkg_t *)provides->data)->name);
					FREELISTPTR(provides);
				}
				if(depinfo == NULL) {
					_pacman_log(PM_LOG_ERROR, _("could not find dependency '%s'"), depend.name);
					/* wtf */
					continue;
				}
			}
			_pacman_log(PM_LOG_DEBUG, _("adding '%s' in requiredby field for '%s'"), info->name, depinfo->name);
			depinfo->requiredby = _pacman_list_add(_pacman_pkg_getinfo(depinfo, PM_PKG_REQUIREDBY), strdup(info->name));
			if(_pacman_db_write(db_local, depinfo, INFRQ_DEPENDS)) {
				_pacman_log(PM_LOG_ERROR, _("could not update 'requiredby' database entry %s-%s"),
				          depinfo->name, depinfo->version);
			}
		}

		needdisp = 0;
		EVENT(trans, PM_TRANS_EVT_EXTRACT_DONE, NULL, NULL);
		FREE(what);

		/* run the post-install script if it exists  */
		if(info->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
			/* must run ldconfig here because some scriptlets fail due to missing libs otherwise */
			_pacman_ldconfig(handle->root);
			snprintf(pm_install, PATH_MAX, "%s%s/%s/%s-%s/install", handle->root, handle->dbpath, db_local->treename, info->name, info->version);
			if(pmo_upgrade) {
				_pacman_runscriptlet(handle->root, pm_install, "post_upgrade", info->version, oldpkg ? oldpkg->version : NULL, trans);
			} else {
				_pacman_runscriptlet(handle->root, pm_install, "post_install", info->version, NULL, trans);
			}
		}

		EVENT(trans, (pmo_upgrade) ? PM_TRANS_EVT_UPGRADE_DONE : PM_TRANS_EVT_ADD_DONE, info, oldpkg);

		FREEPKG(oldpkg);
	}

	/* run ldconfig if it exists */
	if(handle->trans->state != STATE_INTERRUPTED) {
		_pacman_ldconfig(handle->root);
	}

	}

	_pacman_trans_set_state(trans, STATE_COMMITED);

	return(ret);
}

/* vim: set ts=2 sw=2 noet: */
