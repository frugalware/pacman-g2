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
#include "conflict.h"
#include "deps.h"
#include "error.h"
#include "package.h"
#include "util.h"
#include "handle.h"
#include "sync.h"
#include "cache.h"
#include "pacman.h"
#include "versioncmp.h"

#include "hash/md5.h"
#include "hash/sha1.h"
#include "io/archive.h"
#include "util/list.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "fstdlib.h"

#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace libpacman;

static
int _pacman_remove_commit(pmtrans_t *trans, pmlist_t **data);

static int check_oldcache(void)
{
	Database *db = handle->db_local;
	time_t timestamp;

	if(db->gettimestamp(&timestamp) == -1) {
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
		case PM_TRANS_TYPE_REMOVE:
			trans->ops = NULL;
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
		Package *pkg = lp->data;

		trans->triggers = f_stringlist_append_stringlist(trans->triggers, pkg->triggers);
	}
	for(lp = trans->syncpkgs; lp; lp = lp->next) {
		Package *pkg = ((pmsyncpkg_t *)lp->data)->pkg;

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

		if(ps && !strcmp(ps->pkg->name(), pkgname)) {
			return ps;
		}
	}
	return NULL;
}

static
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
	pmsyncpkg_t *syncpkg_queued;

	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, NULL));
	ASSERT(syncpkg != NULL, RET_ERR(PM_ERR_TRANS_NULL, NULL));
	ASSERT(syncpkg->pkg != NULL || syncpkg->pkg_local != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	if(syncpkg->pkg != NULL && syncpkg->pkg_local == NULL) {
		Database *db_local = trans->handle->db_local;

		syncpkg->pkg_local = _pacman_db_get_pkgfromcache(db_local, syncpkg->pkg->name());
	}

	if((syncpkg_queued = _pacman_trans_find(trans, syncpkg->pkg_name)) != NULL) {
		/* FIXME: Try to compress syncpkg in syncpkg_queued more */
		return NULL;
	}
	_pacman_log(PM_LOG_FLOW2, _("adding target '%s' to the transaction set"), syncpkg->pkg_name);
	trans->syncpkgs = _pacman_list_add(trans->syncpkgs, syncpkg);
	return syncpkg;
}

int _pacman_trans_add_package(pmtrans_t *trans, Package *pkg, pmtranstype_t type, int flags)
{
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(pkg != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	_pacman_log(PM_LOG_FLOW2, _("adding %s in the targets list"), pkg->name());
	trans->packages = _pacman_list_add(trans->packages, pkg);
	return 0;
}

pmsyncpkg_t *_pacman_trans_add_target(pmtrans_t *trans, const char *target, pmtranstype_t type, int flags)
{
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, NULL));
	ASSERT(!f_strempty(target), RET_ERR(PM_ERR_TRANS_NULL, NULL));

	return NULL;
}

static
Package *_pacman_filedb_load(Database *db, const char *name)
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
int _pacman_remove_addtarget(pmtrans_t *trans, const char *name)
{
	Package *pkg_local;
	Database *db_local = trans->handle->db_local;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(name != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	if(_pacman_pkg_isin(name, trans->packages)) {
		RET_ERR(PM_ERR_TRANS_DUP_TARGET, -1);
	}

	if((pkg_local = db_local->scan(name, INFRQ_ALL)) == NULL) {
		_pacman_log(PM_LOG_ERROR, _("could not find %s in database"), name);
		RET_ERR(PM_ERR_PKG_NOT_FOUND, -1);
	}

	/* ignore holdpkgs on upgrade */
	if((trans == handle->trans) && _pacman_list_is_strin(pkg_local->name(), handle->holdpkg)) {
		int resp = 0;
		QUESTION(trans, PM_TRANS_CONV_REMOVE_HOLDPKG, pkg_local, NULL, NULL, &resp);
		if(!resp) {
			RET_ERR(PM_ERR_PKG_HOLD, -1);
		}
	}

	return _pacman_trans_add_package(trans, pkg_local, trans->type, 0);
}

int _pacman_trans_addtarget(pmtrans_t *trans, const char *target)
{
	pmlist_t *i;
	Package *pkg_new, *pkg_local, *pkg_queued = NULL;
	Database *db_local = trans->handle->db_local;

	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(!_pacman_strempty(target), RET_ERR(PM_ERR_WRONG_ARGS, -1));

	if(_pacman_list_is_strin(target, trans->targets)) {
		RET_ERR(PM_ERR_TRANS_DUP_TARGET, -1);
	}

	if(trans->ops != NULL && trans->ops->addtarget != NULL) {
		if(trans->ops->addtarget(trans, target) == -1) {
			/* pm_errno is set by trans->ops->addtarget() */
			return(-1);
		}
	} else {
	if(trans->type & PM_TRANS_TYPE_ADD) {
	/* Check if we need to add a fake target to the transaction. */
	if(strchr(target, '|')) {
		return(_pacman_fakedb_addtarget(trans, target));
	}

	pkg_new = _pacman_filedb_load(NULL, target);
	if(pkg_new == NULL || _pacman_pkg_is_valid(pkg_new, trans, target) != 0) {
		/* pm_errno is already set by _pacman_filedb_load() */
		goto error;
	}

	pkg_local = _pacman_db_get_pkgfromcache(db_local, pkg_new->name());
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
		pkg_new->reason = (long)pkg_local->getinfo(PM_PKG_REASON);
	}

	/* check if an older version of said package is already in transaction packages.
	 * if so, replace it in the list */
	for(i = trans->packages; i; i = i->next) {
		Package *pkg = i->data;
		if(strcmp(pkg->name(), pkg_new->name()) == 0) {
			pkg_queued = pkg;
			break;
		}
	}

	if(pkg_queued != NULL) {
		if(_pacman_versioncmp(pkg_queued->version, pkg_new->version) < 0) {
			_pacman_log(PM_LOG_WARNING, _("replacing older version %s-%s by %s in target list"),
			          pkg_queued->name(), pkg_queued->version, pkg_new->version);
			f_ptrswap(&i->data, (void **)&pkg_new);
		} else {
			_pacman_log(PM_LOG_WARNING, _("newer version %s-%s is in the target list -- skipping"),
			          pkg_queued->name(), pkg_queued->version, pkg_new->version);
		}
		delete pkg_new;
	} else {
		_pacman_trans_add_package(trans, pkg_new, trans->type, 0);
	}
	}
	if(trans->type == PM_TRANS_TYPE_REMOVE) {
		if(_pacman_remove_addtarget(trans, target) == -1) {
			return -1;
		}
	}
	}

	trans->targets = _pacman_stringlist_append(trans->targets, target);

	return(0);

error:
	delete pkg_new;
	return(-1);
}

static
int _pacman_add_prepare(pmtrans_t *trans, pmlist_t **data)
{
	pmlist_t *lp;
	Database *db_local = trans->handle->db_local;

	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	/* Check dependencies
	 */
	if(!(trans->flags & PM_TRANS_FLAG_NODEPS)) {
		EVENT(trans, PM_TRANS_EVT_CHECKDEPS_START, NULL, NULL);

		/* look for unsatisfied dependencies */
		_pacman_log(PM_LOG_FLOW1, _("looking for unsatisfied dependencies"));
		lp = _pacman_checkdeps(trans, trans->type, trans->packages);
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
		lp = _pacman_checkconflicts(trans, trans->packages);
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
		lp = _pacman_sortbydeps(trans->packages, PM_TRANS_TYPE_ADD);
		/* free the old alltargs */
		FREELISTPTR(trans->packages);
		trans->packages = lp;

		EVENT(trans, PM_TRANS_EVT_CHECKDEPS_DONE, NULL, NULL);
	}

	/* Cleaning up
	 */
	EVENT(trans, PM_TRANS_EVT_CLEANUP_START, NULL, NULL);
	_pacman_log(PM_LOG_FLOW1, _("cleaning up"));
	for (lp=trans->packages; lp!=NULL; lp=lp->next) {
		Package *pkg_new=(Package *)lp->data;
		pmlist_t *rmlist;

		for (rmlist=pkg_new->removes; rmlist!=NULL; rmlist=rmlist->next) {
			char rm_fname[PATH_MAX];

			snprintf(rm_fname, PATH_MAX, "%s%s", handle->root, (char *)rmlist->data);
			remove(rm_fname);
		}
	}
	EVENT(trans, PM_TRANS_EVT_CLEANUP_DONE, NULL, NULL);

	/* Check for file conflicts
	 */
	if(!(trans->flags & PM_TRANS_FLAG_FORCE)) {
		pmlist_t *skiplist = NULL;

		EVENT(trans, PM_TRANS_EVT_FILECONFLICTS_START, NULL, NULL);

		_pacman_log(PM_LOG_FLOW1, _("looking for file conflicts"));
		lp = _pacman_db_find_conflicts(trans, handle->root, &skiplist);
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

	return(0);
}

static
int _pacman_remove_prepare(pmtrans_t *trans, pmlist_t **data)
{
	pmlist_t *lp;
	Database *db_local = trans->handle->db_local;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	if(!(trans->flags & (PM_TRANS_FLAG_NODEPS)) && (trans->type != PM_TRANS_TYPE_UPGRADE)) {
		EVENT(trans, PM_TRANS_EVT_CHECKDEPS_START, NULL, NULL);

		_pacman_log(PM_LOG_FLOW1, _("looking for unsatisfied dependencies"));
		lp = _pacman_checkdeps(trans, trans->type, trans->packages);
		if(lp != NULL) {
			if(trans->flags & PM_TRANS_FLAG_CASCADE) {
				while(lp) {
					pmlist_t *i;
					for(i = lp; i; i = i->next) {
						pmdepmissing_t *miss = (pmdepmissing_t *)i->data;
						Package *pkg_local = db_local->scan(miss->depend.name, INFRQ_ALL);
						if(pkg_local) {
							_pacman_log(PM_LOG_FLOW2, _("pulling %s in the targets list"), pkg_local->name());
							trans->packages = _pacman_list_add(trans->packages, pkg_local);
						} else {
							_pacman_log(PM_LOG_ERROR, _("could not find %s in database -- skipping"),
								miss->depend.name);
						}
					}
					FREELIST(lp);
					lp = _pacman_checkdeps(trans, trans->type, trans->packages);
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
			trans->packages = _pacman_removedeps(db_local, trans->packages);
		}

		/* re-order w.r.t. dependencies */
		_pacman_log(PM_LOG_FLOW1, _("sorting by dependencies"));
		lp = _pacman_sortbydeps(trans->packages, PM_TRANS_TYPE_REMOVE);
		/* free the old alltargs */
		FREELISTPTR(trans->packages);
		trans->packages = lp;

		EVENT(trans, PM_TRANS_EVT_CHECKDEPS_DONE, NULL, NULL);
	}

	return(0);
}

int _pacman_trans_prepare(pmtrans_t *trans, pmlist_t **data)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(data != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	*data = NULL;

	/* If there's nothing to do, return without complaining */
	if(_pacman_list_empty(trans->packages) &&
		_pacman_list_empty(trans->syncpkgs)) {
		return(0);
	}

	_pacman_trans_compute_triggers(trans);

	if(trans->ops != NULL && trans->ops->prepare != NULL) {
		if(trans->ops->prepare(trans, data) == -1) {
			/* pm_errno is set by trans->ops->prepare() */
			return(-1);
		}
	} else {
	if(trans->type & PM_TRANS_TYPE_ADD) {
		if(_pacman_add_prepare(trans, data) == -1) {
			return -1;
		}
	}
	if(trans->type == PM_TRANS_TYPE_REMOVE) {
		if(_pacman_remove_prepare(trans, data) == -1) {
			return -1;
		}
	}
	}

	_pacman_trans_set_state(trans, STATE_PREPARED);

	return(0);
}

static
int _pacman_fpmpackage_install(Package *pkg, pmtranstype_t type, pmtrans_t *trans, unsigned char cb_state, int howmany, int remain, Package *oldpkg)
{
	int archive_ret, errors = 0, i;
	double percent = 0;
	struct archive *archive;
	struct archive_entry *entry;
	Database *db_local = trans->handle->db_local;
	pmlist_t *lp;
	char expath[PATH_MAX], cwd[PATH_MAX] = "";

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

			_pacman_log(PM_LOG_FLOW1, _("extracting files"));

			/* Extract the package */
			if((archive = _pacman_archive_read_open_all_file(pkg->data)) == NULL) {
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
				int nb = 0, notouch = 0;
				char *hash_orig = NULL;
				struct stat buf;
				char pathname[PATH_MAX];

				STRNCPY(pathname, archive_entry_pathname (entry), PATH_MAX);

				if (pkg->size != 0)
					percent = (double)archive_position_uncompressed(archive) / pkg->size;
				PROGRESS(trans, cb_state, pkg->name(), (int)(percent * 100), howmany, (howmany - remain +1));

				if(!strcmp(pathname, ".PKGINFO") || !strcmp(pathname, ".FILELIST")) {
					archive_read_data_skip (archive);
					continue;
				}

				/*if(!strcmp(pathname, "._install") || !strcmp(pathname, ".INSTALL")) {
				*	 the install script goes inside the db
				*	snprintf(expath, PATH_MAX, "%s/%s-%s/install", db_local->path, pkg->name, pkg->version); */
				if(!strcmp(pathname, "._install") || !strcmp(pathname, ".INSTALL") ||
					!strcmp(pathname, ".CHANGELOG")) {
					if(!strcmp(pathname, ".CHANGELOG")) {
						/* the changelog goes inside the db */
						snprintf(expath, PATH_MAX, "%s/%s-%s/changelog", db_local->path,
							pkg->name(), pkg->version);
					} else {
						/* the install script goes inside the db */
						snprintf(expath, PATH_MAX, "%s/%s-%s/install", db_local->path,
							pkg->name(), pkg->version);
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
				if(_pacman_list_is_strin(pathname, handle->noextract)) {
					pacman_logaction(_("notice: %s is in NoExtract -- skipping extraction"), pathname);
					archive_read_data_skip (archive);
					continue;
				}

				if(!lstat(expath, &buf)) {
					/* file already exists */
					if(S_ISLNK(buf.st_mode)) {
						continue;
					} else if(!S_ISDIR(buf.st_mode)) {
						if(_pacman_list_is_strin(pathname, handle->noupgrade)) {
							notouch = 1;
						} else {
							if(type == PM_TRANS_TYPE_ADD || oldpkg == NULL) {
								nb = _pacman_list_is_strin(pathname, pkg->backup);
							} else {
								/* op == PM_TRANS_TYPE_UPGRADE */
								hash_orig = oldpkg->fileneedbackup(pathname);
								if(!_pacman_strempty(hash_orig)) {
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
						errors++;
						unlink(temp);
						FREE(temp);
						FREE(hash_orig);
						close(fd);
						continue;
					}
					md5_local = _pacman_MDFile(expath);
					md5_pkg = _pacman_MDFile(temp);
					sha1_local = _pacman_SHAFile(expath);
					sha1_pkg = _pacman_SHAFile(temp);
					/* append the new md5 or sha1 hash to it's respective entry in pkg->backup
					 * (it will be the new orginal)
					 */
					for(lp = pkg->backup; lp; lp = lp->next) {
						char *fn;
						char *file = lp->data;

						if(!file) continue;
						if(!strcmp(file, pathname)) {
							if(!_pacman_strempty(pkg->sha1sum)) {
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

					if (!_pacman_strempty(pkg->sha1sum)) {
						_pacman_log(PM_LOG_DEBUG, _("checking md5 hashes for %s"), pathname);
						_pacman_log(PM_LOG_DEBUG, _("current:  %s"), md5_local);
						_pacman_log(PM_LOG_DEBUG, _("new:      %s"), md5_pkg);
					} else {	
						_pacman_log(PM_LOG_DEBUG, _("checking sha1 hashes for %s"), pathname);
						_pacman_log(PM_LOG_DEBUG, _("current:  %s"), sha1_local);
						_pacman_log(PM_LOG_DEBUG, _("new:      %s"), sha1_pkg);
					}
					if(!_pacman_strempty(hash_orig)) {
						_pacman_log(PM_LOG_DEBUG, _("original: %s"), hash_orig);
					}

					if(type == PM_TRANS_TYPE_ADD) {
						/* if a file already exists with a different md5 or sha1 hash,
						 * then we rename it to a .pacorig extension and continue */
						if(strcmp(md5_local, md5_pkg) || strcmp(sha1_local, sha1_pkg)) {
							char newpath[PATH_MAX];
							snprintf(newpath, PATH_MAX, "%s.pacorig", expath);
							if(rename(expath, newpath)) {
								archive_entry_set_pathname (entry, expath);
								_pacman_log(PM_LOG_ERROR, _("could not rename %s (%s)"), pathname, strerror(errno));
							}
							if(_pacman_copyfile(temp, expath)) {
								archive_entry_set_pathname (entry, expath);
								_pacman_log(PM_LOG_ERROR, _("could not copy %s to %s (%s)"), temp, pathname, strerror(errno));
								errors++;
							} else {
								archive_entry_set_pathname (entry, expath);
								_pacman_log(PM_LOG_WARNING, _("%s saved as %s.pacorig"), pathname, pathname);
							}
						}
					} else if(!_pacman_strempty(hash_orig)) {
						/* PM_UPGRADE */
						int installnew = 0;

						/* the fun part */
						if(!strcmp(hash_orig, md5_local) || !strcmp(hash_orig, sha1_local)) {
							if(!strcmp(md5_local, md5_pkg) || !strcmp(sha1_local, sha1_pkg)) {
								_pacman_log(PM_LOG_DEBUG, _("action: installing new file"));
								installnew = 1;
							} else {
								_pacman_log(PM_LOG_DEBUG, _("action: installing new file"));
								installnew = 1;
							}
						} else if(!strcmp(hash_orig, md5_pkg) || !strcmp(hash_orig, sha1_pkg)) {
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
							} else {
								_pacman_log(PM_LOG_WARNING, _("%s installed as %s"), expath, newpath);
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
					FREE(sha1_local);
					FREE(sha1_pkg);
					FREE(hash_orig);
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
						errors++;
					}
					/* calculate an md5 or sha1 hash if this is in pkg->backup */
					for(lp = pkg->backup; lp; lp = lp->next) {
						char *fn, *md5, *sha1;
						char path[PATH_MAX];
						char *file = lp->data;

						if(!file) continue;
						if(!strcmp(file, pathname)) {
							_pacman_log(PM_LOG_DEBUG, _("appending backup entry"));
							snprintf(path, PATH_MAX, "%s%s", handle->root, file);
							if (!_pacman_strempty(pkg->sha1sum)) {
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
			if(!_pacman_strempty(cwd)) {
				chdir(cwd);
			}
			archive_read_finish (archive);
	return errors;
}

static
int _pacman_localpackage_remove(Package *pkg, pmtrans_t *trans, int howmany, int remain)
{
	pmlist_t *lp;
	struct stat buf;
	int position = 0;
	char line[PATH_MAX+1];

			int filenum = _pacman_list_count(pkg->files);
			_pacman_log(PM_LOG_FLOW1, _("removing files"));

			/* iterate through the list backwards, unlinking files */
			for(lp = _pacman_list_last(pkg->files); lp; lp = lp->prev) {
				int nb = 0;
				double percent = 0;
				char *file = lp->data;
				char *hash_orig = pkg->fileneedbackup(file);

				if (position != 0) {
				percent = (double)position / filenum;
				}
				if(!_pacman_strempty(hash_orig)) {
					nb = 1;
				}
				FREE(hash_orig);
				if(!nb && trans->type == PM_TRANS_TYPE_UPGRADE) {
					/* check noupgrade */
					if(_pacman_list_is_strin(file, handle->noupgrade)) {
						nb = 1;
					}
				}
				snprintf(line, PATH_MAX, "%s%s", handle->root, file);
				if(lstat(line, &buf)) {
					_pacman_log(PM_LOG_DEBUG, _("file %s does not exist"), file);
					continue;
				}
				if(S_ISDIR(buf.st_mode)) {
					if(rmdir(line)) {
						/* this is okay, other packages are probably using it. */
						_pacman_log(PM_LOG_DEBUG, _("keeping directory %s"), file);
					} else {
						_pacman_log(PM_LOG_FLOW2, _("removing directory %s"), file);
					}
				} else {
					/* check the "skip list" before removing the file.
					 * see the big comment block in db_find_conflicts() for an
					 * explanation. */
					int skipit = 0;
					pmlist_t *j;
					for(j = trans->skiplist; j; j = j->next) {
						if(!strcmp(file, (char*)j->data)) {
							skipit = 1;
						}
					}
					if(skipit) {
						_pacman_log(PM_LOG_FLOW2, _("skipping removal of %s as it has moved to another package"),
							file);
					} else {
						/* if the file is flagged, back it up to .pacsave */
						if(nb) {
							if(trans->type == PM_TRANS_TYPE_UPGRADE) {
								/* we're upgrading so just leave the file as is. pacman_add() will handle it */
							} else {
								if(!(trans->flags & PM_TRANS_FLAG_NOSAVE)) {
									char newpath[PATH_MAX];
									snprintf(newpath, PATH_MAX, "%s.pacsave", line);
									rename(line, newpath);
									_pacman_log(PM_LOG_WARNING, _("%s saved as %s"), line, newpath);
								} else {
									_pacman_log(PM_LOG_FLOW2, _("unlinking %s"), file);
									if(unlink(line)) {
										_pacman_log(PM_LOG_ERROR, _("cannot remove file %s"), file);
									}
								}
							}
						} else {
							_pacman_log(PM_LOG_FLOW2, _("unlinking %s"), file);
							/* Need at here because we count only real unlinked files ? */
							PROGRESS(trans, PM_TRANS_PROGRESS_REMOVE_START, pkg->name(), (int)(percent * 100), howmany, howmany - remain + 1);
							position++;
							if(unlink(line)) {
								_pacman_log(PM_LOG_ERROR, _("cannot remove file %s"), file);
							}
						}
					}
				}
			}
	return 0;
}

static
int _pacman_add_commit(pmtrans_t *trans, pmlist_t **data)
{
	int ret = 0;
	int remain, howmany;
	unsigned char cb_state;
	time_t t;
	pmlist_t *targ, *lp;
	Database *db_local = trans->handle->db_local;

	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	howmany = _pacman_list_count(trans->packages);
	for(targ = trans->packages; targ; targ = targ->next) {
		unsigned short pmo_upgrade;
		pmtranstype_t type;
		char pm_install[PATH_MAX];
		Package *pkg_new = (Package *)targ->data;
		Package *oldpkg = NULL, *pkg_local = NULL;
		remain = _pacman_list_count(targ);

		if(handle->trans->state == STATE_INTERRUPTED) {
			break;
		}

		pmo_upgrade = (trans->type == PM_TRANS_TYPE_UPGRADE) ? 1 : 0;
		type = trans->type;

		/* see if this is an upgrade.  if so, remove the old package first */
		if(type == PM_TRANS_TYPE_UPGRADE) {
			pkg_local = _pacman_db_get_pkgfromcache(db_local, pkg_new->name());
			if(pkg_local == NULL) {
				/* no previous package version is installed, so this is actually
				 * just an install.  */
				pmo_upgrade = 0;
				type &= ~PM_TRANS_TYPE_REMOVE;
			}
		}
		if(type == PM_TRANS_TYPE_UPGRADE)
		{
				EVENT(trans, PM_TRANS_EVT_UPGRADE_START, pkg_new, NULL);
				cb_state = PM_TRANS_PROGRESS_UPGRADE_START;
				_pacman_log(PM_LOG_FLOW1, _("upgrading package %s-%s"), pkg_new->name(), pkg_new->version);

				/* we'll need to save some record for backup checks later */
				oldpkg = new Package(pkg_local->name(), pkg_local->version);
				if(oldpkg) {
					if(!(pkg_local->infolevel & INFRQ_FILES)) {
						_pacman_log(PM_LOG_DEBUG, _("loading FILES info for '%s'"), pkg_local->name());
						db_local->read(pkg_local, INFRQ_FILES);
					}
					oldpkg->backup = _pacman_list_strdup(pkg_local->backup);
					strncpy(oldpkg->m_name, pkg_local->name(), PKG_NAME_LEN);
					strncpy(oldpkg->version, pkg_local->version, PKG_VERSION_LEN);
				}

				/* pre_upgrade scriptlet */
				if(pkg_new->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
					_pacman_runscriptlet(handle->root, pkg_new->data, "pre_upgrade", pkg_new->version, oldpkg ? oldpkg->version : NULL,
						trans);
				}

				if(oldpkg) {
					pmtrans_t *tr;
					_pacman_log(PM_LOG_FLOW1, _("removing old package first (%s-%s)"), oldpkg->name(), oldpkg->version);
					tr = _pacman_trans_new();
					if(tr == NULL) {
						RET_ERR(PM_ERR_TRANS_ABORT, -1);
					}
					pmtrans_cbs_t null_cbs = { NULL };
					if(_pacman_trans_init(tr, PM_TRANS_TYPE_UPGRADE, trans->flags, null_cbs) == -1) {
						FREETRANS(tr);
						RET_ERR(PM_ERR_TRANS_ABORT, -1);
					}
					if(_pacman_remove_addtarget(tr, pkg_new->name()) == -1) {
						FREETRANS(tr);
						RET_ERR(PM_ERR_TRANS_ABORT, -1);
					}
					/* copy the skiplist over */
					tr->skiplist = _pacman_list_strdup(trans->skiplist);
					if(_pacman_remove_commit(tr, NULL) == -1) {
						FREETRANS(tr);
						RET_ERR(PM_ERR_TRANS_ABORT, -1);
					}
					FREETRANS(tr);
				}
			_pacman_log(PM_LOG_FLOW1, _("adding new package %s-%s"), pkg_new->name(), pkg_new->version);
		} else {
			EVENT(trans, PM_TRANS_EVT_ADD_START, pkg_new, NULL);
			cb_state = PM_TRANS_PROGRESS_ADD_START;
			_pacman_log(PM_LOG_FLOW1, _("adding package %s-%s"), pkg_new->name(), pkg_new->version);

			/* pre_install scriptlet */
			if(pkg_new->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
				_pacman_runscriptlet(handle->root, pkg_new->data, "pre_install", pkg_new->version, NULL, trans);
			}
		}

		if(!(trans->flags & PM_TRANS_FLAG_DBONLY)) {
			int errors = _pacman_fpmpackage_install(pkg_new, type, trans, cb_state, howmany, remain, oldpkg);

			if(errors) {
				ret = 1;
				_pacman_log(PM_LOG_WARNING, _("errors occurred while %s %s"),
					(pmo_upgrade ? _("upgrading") : _("installing")), pkg_new->name());
			} else {
			PROGRESS(trans, cb_state, pkg_new->name(), 100, howmany, howmany - remain + 1);
			}
		}

		/* Add the package to the database */
		t = time(NULL);

		/* Update the requiredby field by scanning the whole database
		 * looking for packages depending on the package to add */
		for(lp = _pacman_db_get_pkgcache(db_local); lp; lp = lp->next) {
			Package *tmpp = lp->data;
			pmlist_t *tmppm = NULL;
			if(tmpp == NULL) {
				continue;
			}
			for(tmppm = tmpp->depends; tmppm; tmppm = tmppm->next) {
				pmdepend_t depend;
				if(_pacman_splitdep(tmppm->data, &depend)) {
					continue;
				}
				if(tmppm->data && (!strcmp(depend.name, pkg_new->name()) || pkg_new->provides(depend.name))) {
					_pacman_log(PM_LOG_DEBUG, _("adding '%s' in requiredby field for '%s'"), tmpp->name(), pkg_new->name());
					pkg_new->requiredby = _pacman_stringlist_append(pkg_new->requiredby, tmpp->name());
				}
			}
		}

		/* make an install date (in UTC) */
		STRNCPY(pkg_new->installdate, asctime(gmtime(&t)), sizeof(pkg_new->installdate));
		/* remove the extra line feed appended by asctime() */
		pkg_new->installdate[strlen(pkg_new->installdate)-1] = 0;

		_pacman_log(PM_LOG_FLOW1, _("updating database"));
		_pacman_log(PM_LOG_FLOW2, _("adding database entry '%s'"), pkg_new->name());
		if(db_local->write(pkg_new, INFRQ_ALL)) {
			_pacman_log(PM_LOG_ERROR, _("error updating database for %s-%s!"),
			          pkg_new->name(), pkg_new->version);
			RET_ERR(PM_ERR_DB_WRITE, -1);
		}
		if(_pacman_db_add_pkgincache(db_local, pkg_new) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not add entry '%s' in cache"), pkg_new->name());
		}

		/* update dependency packages' REQUIREDBY fields */
		if(pkg_new->depends) {
			_pacman_log(PM_LOG_FLOW2, _("updating dependency packages 'requiredby' fields"));
		}
		for(lp = pkg_new->depends; lp; lp = lp->next) {
			Package *depinfo;
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
					depinfo = _pacman_db_get_pkgfromcache(db_local, ((Package *)provides->data)->name());
					FREELISTPTR(provides);
				}
				if(depinfo == NULL) {
					_pacman_log(PM_LOG_ERROR, _("could not find dependency '%s'"), depend.name);
					/* wtf */
					continue;
				}
			}
			_pacman_log(PM_LOG_DEBUG, _("adding '%s' in requiredby field for '%s'"), pkg_new->name(), depinfo->name());
			depinfo->requiredby = _pacman_stringlist_append(depinfo->getinfo(PM_PKG_REQUIREDBY), pkg_new->name());
			if(db_local->write(depinfo, INFRQ_DEPENDS)) {
				_pacman_log(PM_LOG_ERROR, _("could not update 'requiredby' database entry %s-%s"),
				          depinfo->name(), depinfo->version);
			}
		}

		EVENT(trans, PM_TRANS_EVT_EXTRACT_DONE, NULL, NULL);

		/* run the post-install script if it exists  */
		if(pkg_new->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
			/* must run ldconfig here because some scriptlets fail due to missing libs otherwise */
			_pacman_ldconfig(handle->root);
			snprintf(pm_install, PATH_MAX, "%s%s/%s/%s-%s/install", handle->root, handle->dbpath, db_local->treename, pkg_new->name(), pkg_new->version);
			if(pmo_upgrade) {
				_pacman_runscriptlet(handle->root, pm_install, "post_upgrade", pkg_new->version, oldpkg ? oldpkg->version : NULL, trans);
			} else {
				_pacman_runscriptlet(handle->root, pm_install, "post_install", pkg_new->version, NULL, trans);
			}
		}

		EVENT(trans, (pmo_upgrade) ? PM_TRANS_EVT_UPGRADE_DONE : PM_TRANS_EVT_ADD_DONE, pkg_new, oldpkg);

		delete oldpkg;
	}

	/* run ldconfig if it exists */
	if(handle->trans->state != STATE_INTERRUPTED) {
		_pacman_ldconfig(handle->root);
	}

	return(ret);
}

/* Helper function for comparing strings
 */
static int str_cmp(const void *s1, const void *s2)
{
	return(strcmp(s1, s2));
}

static
int _pacman_remove_commit(pmtrans_t *trans, pmlist_t **data)
{
	Package *pkg_local;
	pmlist_t *targ, *lp;
	int howmany, remain;
	Database *db_local = trans->handle->db_local;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	howmany = _pacman_list_count(trans->packages);

	for(targ = trans->packages; targ; targ = targ->next) {
		char pm_install[PATH_MAX];
		pkg_local = (Package*)targ->data;

		if(handle->trans->state == STATE_INTERRUPTED) {
			break;
		}

		remain = _pacman_list_count(targ);

		if(trans->type != PM_TRANS_TYPE_UPGRADE) {
			EVENT(trans, PM_TRANS_EVT_REMOVE_START, pkg_local, NULL);
			_pacman_log(PM_LOG_FLOW1, _("removing package %s-%s"), pkg_local->name(), pkg_local->version);

			/* run the pre-remove scriptlet if it exists */
			if(pkg_local->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
				snprintf(pm_install, PATH_MAX, "%s/%s-%s/install", db_local->path, pkg_local->name(), pkg_local->version);
				_pacman_runscriptlet(handle->root, pm_install, "pre_remove", pkg_local->version, NULL, trans);
			}
		}

		if(!(trans->flags & PM_TRANS_FLAG_DBONLY)) {
			_pacman_localpackage_remove(pkg_local, trans, howmany, remain);
		}

		PROGRESS(trans, PM_TRANS_PROGRESS_REMOVE_START, pkg_local->name(), 100, howmany, howmany - remain + 1);
		if(trans->type != PM_TRANS_TYPE_UPGRADE) {
			/* run the post-remove script if it exists */
			if(pkg_local->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
				/* must run ldconfig here because some scriptlets fail due to missing libs otherwise */
				_pacman_ldconfig(handle->root);
				snprintf(pm_install, PATH_MAX, "%s/%s-%s/install", db_local->path, pkg_local->name(), pkg_local->version);
				_pacman_runscriptlet(handle->root, pm_install, "post_remove", pkg_local->version, NULL, trans);
			}
		}

		/* remove the package from the database */
		_pacman_log(PM_LOG_FLOW1, _("updating database"));
		_pacman_log(PM_LOG_FLOW2, _("removing database entry '%s'"), pkg_local->name());
		if(db_local->remove(pkg_local) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not remove database entry %s-%s"), pkg_local->name(), pkg_local->version);
		}
		if(_pacman_db_remove_pkgfromcache(db_local, pkg_local) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not remove entry '%s' from cache"), pkg_local->name());
		}

		/* update dependency packages' REQUIREDBY fields */
		_pacman_log(PM_LOG_FLOW2, _("updating dependency packages 'requiredby' fields"));
		for(lp = pkg_local->depends; lp; lp = lp->next) {
			Package *depinfo = NULL;
			pmdepend_t depend;
			char *data;
			if(_pacman_splitdep((char*)lp->data, &depend)) {
				continue;
			}
			/* if this dependency is in the transaction targets, no need to update
			 * its requiredby info: it is in the process of being removed (if not
			 * already done!)
			 */
			if(_pacman_pkg_isin(depend.name, trans->packages)) {
				continue;
			}
			depinfo = _pacman_db_get_pkgfromcache(db_local, depend.name);
			if(depinfo == NULL) {
				/* look for a provides package */
				pmlist_t *provides = _pacman_db_whatprovides(db_local, depend.name);
				if(provides) {
					/* TODO: should check _all_ packages listed in provides, not just
					 *			 the first one.
					 */
					/* use the first one */
					depinfo = _pacman_db_get_pkgfromcache(db_local, ((Package *)provides->data)->name());
					FREELISTPTR(provides);
				}
				if(depinfo == NULL) {
					_pacman_log(PM_LOG_ERROR, _("could not find dependency '%s'"), depend.name);
					/* wtf */
					continue;
				}
			}
			/* splice out this entry from requiredby */
			depinfo->requiredby = _pacman_list_remove(depinfo->getinfo(PM_PKG_REQUIREDBY), pkg_local->name(), str_cmp, (void **)&data);
			FREE(data);
			_pacman_log(PM_LOG_DEBUG, _("updating 'requiredby' field for package '%s'"), depinfo->name());
			if(db_local->write(depinfo, INFRQ_DEPENDS)) {
				_pacman_log(PM_LOG_ERROR, _("could not update 'requiredby' database entry %s-%s"),
					depinfo->name(), depinfo->version);
			}
		}

		if(trans->type != PM_TRANS_TYPE_UPGRADE) {
			EVENT(trans, PM_TRANS_EVT_REMOVE_DONE, pkg_local, NULL);
		}
	}

	/* run ldconfig if it exists */
	if((trans->type != PM_TRANS_TYPE_UPGRADE) && (handle->trans->state != STATE_INTERRUPTED)) {
		_pacman_ldconfig(handle->root);
	}

	return(0);
}

int _pacman_trans_commit(pmtrans_t *trans, pmlist_t **data)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	if(data!=NULL)
		*data = NULL;

	/* If there's nothing to do, return without complaining */
	if(_pacman_list_empty(trans->packages) &&
		_pacman_list_empty(trans->syncpkgs)) {
		return(0);
	}

	_pacman_trans_set_state(trans, STATE_COMMITING);

	if(trans->ops != NULL && trans->ops->commit != NULL) {
		if(trans->ops->commit(trans, data) == -1) {
			/* pm_errno is set by trans->ops->commit() */
			_pacman_trans_set_state(trans, STATE_PREPARED);
			return(-1);
		}
	} else {
	if(trans->type & PM_TRANS_TYPE_ADD) {
		if(_pacman_add_commit(trans, data) == -1) {
			return -1;
		}
	}
	if(trans->type == PM_TRANS_TYPE_REMOVE) {
		if(_pacman_remove_commit(trans, data) == -1) {
			return -1;
		}
	}
	}

	_pacman_trans_set_state(trans, STATE_COMMITED);

	return(0);
}

/* vim: set ts=2 sw=2 noet: */
