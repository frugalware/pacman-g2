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
#include "db/localdb_files.h"
#include "package/fpmpackage.h"
#include "conflict.h"
#include "deps.h"
#include "error.h"
#include "package.h"
#include "util.h"
#include "handle.h"
#include "server.h"
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

__pmtrans_t::__pmtrans_t(pmtranstype_t type, unsigned int flags, pmtrans_cbs_t cbs)
	: state(STATE_IDLE)
{
	switch(type) {
		case PM_TRANS_TYPE_ADD:
		case PM_TRANS_TYPE_UPGRADE:
		case PM_TRANS_TYPE_REMOVE:
		case PM_TRANS_TYPE_SYNC:
			break;
		default:
			// Be more verbose about the trans type
			_pacman_log(PM_LOG_ERROR,
					_("could not initialize transaction: Unknown Transaction Type %d"), type);
	}

	this->handle = ::handle;
	this->type = type;
	this->flags = flags;
	this->cbs = cbs;
//	packages = f_ptrlist_new();
	targets = f_stringlist_new();
	skiplist = f_stringlist_new();
	triggers = f_stringlist_new();

	state = STATE_INITIALIZED;

	check_oldcache();
}

__pmtrans_t::~__pmtrans_t()
{
	/* Sanity checks */
//	ASSERT(state != STATE_COMMITING, RET_ERR(PM_ERR_TRANS_COMMITING, -1));

	FREELISTPKGS(packages);
#if 0
	{
		FVisitor visitor = {
			.fn = _pacman_syncpkg_delete,
			.data = NULL,
		};
		
		f_ptrlist_delete(syncpkgs, &visitor);
	}
#endif
	f_stringlist_delete(targets);
	f_stringlist_delete(skiplist);
	f_stringlist_delete(triggers);

	state = STATE_IDLE;
}

static
int _pacman_trans_compute_triggers(pmtrans_t *trans)
{
	pmlist_t *lp;

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
	case PM_TRANS_EVT_ADD_DONE: {
			Package *pkg_new = (Package *)data1;

			snprintf(str, LOG_STR_LEN, "installed %s (%s)",
					pkg_new->name(), pkg_new->version());
			pacman_logaction(str);
		}
		break;
	case PM_TRANS_EVT_REMOVE_DONE: {
			Package *pkg_old = (Package *)data1;

			snprintf(str, LOG_STR_LEN, "removed %s (%s)",
					pkg_old->name(), pkg_old->version());
			pacman_logaction(str);
		}
		break;
	case PM_TRANS_EVT_UPGRADE_DONE: {
			Package *pkg_new = (Package *)data1;
			Package *pkg_old = (Package *)data2;

			snprintf(str, LOG_STR_LEN, "upgraded %s (%s -> %s)",
					pkg_new->name(), pkg_old->version(), pkg_new->version());
			pacman_logaction(str);
		}
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
pmsyncpkg_t *__pmtrans_t::find(const char *pkgname) const
{
	pmlist_t *i;

	for(i = syncpkgs; i != NULL ; i = i->next) {
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

/* Helper functions for _pacman_list_remove
 */
static
int _pacman_syncpkg_cmp(const void *s1, const void *s2)
{
	return(strcmp(((pmsyncpkg_t *)s1)->pkg->name(), ((pmsyncpkg_t *)s2)->pkg->name()));
}

static int pkg_cmp(const void *p1, const void *p2)
{
	return(strcmp(((Package *)p1)->name(), ((pmsyncpkg_t *)p2)->pkg->name()));
}

static int check_olddelay(void)
{
	pmlist_t *i;
	time_t tm;

	if(!handle->olddelay) {
		return(0);
	}

	for(i = handle->dbs_sync; i; i= i->next) {
		Database *db = i->data;
		if(db->gettimestamp(&tm) == -1) {
			continue;
		}
		if(difftime(time(NULL), tm) > handle->olddelay) {
			_pacman_log(PM_LOG_WARNING, _("local copy of '%s' repo is too old"), db->treename);
		}
	}
	return(0);
}

static
int _pacman_sync_commit(pmtrans_t *trans, pmlist_t **data)
{
	pmlist_t *i, *j;
	pmtrans_t *tr = NULL;
	int replaces = 0;
	Database *db_local = trans->handle->db_local;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	if(handle->sysupgrade) {
		_pacman_runhook("pre_sysupgrade", trans);
	}
	/* remove conflicting and to-be-replaced packages */
	tr = new __pmtrans_t(PM_TRANS_TYPE_REMOVE, PM_TRANS_FLAG_NODEPS, trans->cbs);
	if(tr == NULL) {
		_pacman_log(PM_LOG_ERROR, _("could not create removal transaction"));
		pm_errno = PM_ERR_MEMORY;
		goto error;
	}
	for(i = trans->syncpkgs; i; i = i->next) {
		pmsyncpkg_t *ps = i->data;
		if(ps->type == PM_SYNC_TYPE_REPLACE) {
			for(j = ps->data; j; j = j->next) {
				Package *pkg = j->data;
				if(!_pacman_pkg_isin(pkg->name(), tr->packages)) {
					if(tr->add(pkg->name()) == -1) {
						goto error;
					}
					replaces++;
				}
			}
		}
	}
	if(replaces) {
		_pacman_log(PM_LOG_FLOW1, _("removing conflicting and to-be-replaced packages"));
		if(tr->prepare(data) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not prepare removal transaction"));
			goto error;
		}
		/* we want the frontend to be aware of commit details */
		tr->cbs.event = trans->cbs.event;
		if(tr->commit(NULL) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not commit removal transaction"));
			goto error;
		}
	}
	delete tr;
	tr = NULL;

	/* install targets */
	_pacman_log(PM_LOG_FLOW1, _("installing packages"));
	tr = new __pmtrans_t(PM_TRANS_TYPE_UPGRADE, trans->flags | PM_TRANS_FLAG_NODEPS, trans->cbs);
	if(tr == NULL) {
		_pacman_log(PM_LOG_ERROR, _("could not create transaction"));
		pm_errno = PM_ERR_MEMORY;
		goto error;
	}
	for(i = trans->syncpkgs; i; i = i->next) {
		pmsyncpkg_t *ps = i->data;
		Package *spkg = ps->pkg;
		char str[PATH_MAX];
		snprintf(str, PATH_MAX, "%s%s/%s-%s-%s" PM_EXT_PKG, handle->root, handle->cachedir, spkg->name(), spkg->version(), spkg->arch);
		if(tr->add(str) == -1) {
			goto error;
		}
		/* using _pacman_list_last() is ok because addtarget() adds the new target at the
		 * end of the tr->packages list */
		spkg = _pacman_list_last(tr->packages)->data;
		if(ps->type == PM_SYNC_TYPE_DEPEND || trans->flags & PM_TRANS_FLAG_ALLDEPS) {
			spkg->m_reason = PM_PKG_REASON_DEPEND;
		} else if(ps->type == PM_SYNC_TYPE_UPGRADE && !handle->sysupgrade) {
			spkg->m_reason = PM_PKG_REASON_EXPLICIT;
		}
	}
	if(tr->prepare(data) == -1) {
		_pacman_log(PM_LOG_ERROR, _("could not prepare transaction"));
		/* pm_errno is set by trans_prepare */
		goto error;
	}
	if(tr->commit(NULL) == -1) {
		_pacman_log(PM_LOG_ERROR, _("could not commit transaction"));
		goto error;
	}
	delete tr;
	tr = NULL;

	/* propagate replaced packages' requiredby fields to their new owners */
	if(replaces) {
		_pacman_log(PM_LOG_FLOW1, _("updating database for replaced packages' dependencies"));
		for(i = trans->syncpkgs; i; i = i->next) {
			pmsyncpkg_t *ps = i->data;
			if(ps->type == PM_SYNC_TYPE_REPLACE) {
				Package *pkg_new = _pacman_db_get_pkgfromcache(db_local, ps->pkg_name);
				for(j = ps->data; j; j = j->next) {
					pmlist_t *k;
					Package *old = j->data;
					/* merge lists */
					for(k = old->requiredby(); k; k = k->next) {
						if(!_pacman_list_is_strin(k->data, pkg_new->requiredby())) {
							/* replace old's name with new's name in the requiredby's dependency list */
							pmlist_t *m;
							Package *depender = _pacman_db_get_pkgfromcache(db_local, k->data);
							if(depender == NULL) {
								/* If the depending package no longer exists in the local db,
								 * then it must have ALSO conflicted with ps->pkg.  If
								 * that's the case, then we don't have anything to propagate
								 * here. */
								continue;
							}
							for(m = depender->depends(); m; m = m->next) {
								if(!strcmp(m->data, old->name())) {
									FREE(m->data);
									m->data = strdup(pkg_new->name());
								}
							}
							if(db_local->write(depender, INFRQ_DEPENDS) == -1) {
								_pacman_log(PM_LOG_ERROR, _("could not update requiredby for database entry %s-%s"),
										  pkg_new->name(), pkg_new->version());
							}
							/* add the new requiredby */
							pkg_new->m_requiredby = _pacman_stringlist_append(pkg_new->m_requiredby, k->data);
						}
					}
				}
				if(db_local->write(pkg_new, INFRQ_DEPENDS) == -1) {
					_pacman_log(PM_LOG_ERROR, _("could not update new database entry %s-%s"),
							  pkg_new->name(), pkg_new->version());
				}
			}
		}
	}

	if(handle->sysupgrade) {
		_pacman_runhook("post_sysupgrade", trans);
	}
	return(0);

error:
	delete tr;
	/* commiting failed, so this is still just a prepared transaction */
	return(-1);
}

static
int _pacman_trans_download_commit(pmtrans_t *trans, pmlist_t **data)
{
	pmlist_t *i, *j, *files = NULL;
	char ldir[PATH_MAX];
	int doremove, retval = 0, tries = 0;
	int varcache = 1;

	trans->state = STATE_DOWNLOADING;
	/* group sync records by repository and download */
	snprintf(ldir, PATH_MAX, "%s%s", handle->root, handle->cachedir);

	for(tries = 0; tries < handle->maxtries; tries++) {
		retval = 0;
		FREELIST(*data);
		int done = 1;
		for(i = handle->dbs_sync; i; i = i->next) {
			struct stat buf;
			Database *current = i->data;

			for(j = trans->syncpkgs; j; j = j->next) {
				pmsyncpkg_t *ps = j->data;
				Package *spkg = ps->pkg;
				Database *dbs = spkg->data;

				if(current == dbs) {
					char filename[PATH_MAX];
					char lcpath[PATH_MAX];
					spkg->filename(filename, sizeof(filename));
					snprintf(lcpath, sizeof(lcpath), "%s/%s", ldir, filename);

					if(trans->flags & PM_TRANS_FLAG_PRINTURIS) {
						if (!(trans->flags & PM_TRANS_FLAG_PRINTURIS_CACHED)) {
							if (stat(lcpath, &buf) == 0) {
								continue;
							}
						}

						EVENT(trans, PM_TRANS_EVT_PRINTURI, pacman_db_getinfo(c_cast(current), PM_DB_FIRSTSERVER), filename);
					} else {
						if(stat(lcpath, &buf)) {
							/* file is not in the cache dir, so add it to the list */
							files = _pacman_stringlist_append(files, filename);
						} else {
							_pacman_log(PM_LOG_DEBUG, _("%s is already in the cache\n"), filename);
						}
					}
				}
			}

			if(files) {
				EVENT(trans, PM_TRANS_EVT_RETRIEVE_START, current->treename, NULL);
				if(stat(ldir, &buf)) {
					/* no cache directory.... try creating it */
					_pacman_log(PM_LOG_WARNING, _("no %s cache exists.  creating..."), ldir);
					if(_pacman_makepath(ldir)) {
						/* couldn't mkdir the cache directory, so fall back to /tmp and unlink
						 * the package afterwards.
						 */
						_pacman_log(PM_LOG_WARNING, _("couldn't create package cache, using /tmp instead"));
						snprintf(ldir, PATH_MAX, "%s/tmp", handle->root);
						if(pacman_set_option(PM_OPT_CACHEDIR, (long)"/tmp") == -1) {
							_pacman_log(PM_LOG_WARNING, _("failed to set option CACHEDIR (%s)\n"), pacman_strerror(pm_errno));
							RET_ERR(PM_ERR_RETRIEVE, -1);
						}
						varcache = 0;
					}
				}
				if(_pacman_downloadfiles(current->servers, ldir, files, tries) == -1) {
					_pacman_log(PM_LOG_WARNING, _("failed to retrieve some files from %s\n"), current->treename);
					retval=1;
					done = 0;
				}
				FREELIST(files);
			}
		}
		if (!done)
			continue;
		if(trans->flags & PM_TRANS_FLAG_PRINTURIS) {
			return(0);
		}

		/* Check integrity of files */
		if(!(trans->flags & PM_TRANS_FLAG_NOINTEGRITY)) {
			EVENT(trans, PM_TRANS_EVT_INTEGRITY_START, NULL, NULL);

			for(i = trans->syncpkgs; i; i = i->next) {
				pmsyncpkg_t *ps = i->data;
				Package *spkg = ps->pkg;
				char str[PATH_MAX], pkgname[PATH_MAX];
				char *md5sum1, *md5sum2, *sha1sum1, *sha1sum2;
				char *ptr=NULL;

				spkg->filename(pkgname, sizeof(pkgname));
				md5sum1 = spkg->md5sum;
				sha1sum1 = spkg->sha1sum;

				if((md5sum1 == NULL) && (sha1sum1 == NULL)) {
					if((ptr = (char *)malloc(512)) == NULL) {
						RET_ERR(PM_ERR_MEMORY, -1);
					}
					snprintf(ptr, 512, _("can't get md5 or sha1 checksum for package %s\n"), pkgname);
					*data = _pacman_list_add(*data, ptr);
					retval = 1;
					continue;
				}
				snprintf(str, PATH_MAX, "%s/%s/%s", handle->root, handle->cachedir, pkgname);
				md5sum2 = _pacman_MDFile(str);
				sha1sum2 = _pacman_SHAFile(str);
				if(md5sum2 == NULL && sha1sum2 == NULL) {
					if((ptr = (char *)malloc(512)) == NULL) {
						RET_ERR(PM_ERR_MEMORY, -1);
					}
					snprintf(ptr, 512, _("can't get md5 or sha1 checksum for package %s\n"), pkgname);
					*data = _pacman_list_add(*data, ptr);
					retval = 1;
					continue;
				}
				if((strcmp(md5sum1, md5sum2) != 0) && (strcmp(sha1sum1, sha1sum2) != 0)) {
					_pacman_log(PM_LOG_DEBUG, _("expected md5:  '%s'"), md5sum1);
					_pacman_log(PM_LOG_DEBUG, _("actual md5:    '%s'"), md5sum2);
					_pacman_log(PM_LOG_DEBUG, _("expected sha1: '%s'"), sha1sum1);
					_pacman_log(PM_LOG_DEBUG, _("actual sha1:   '%s'"), sha1sum2);

					doremove = 0;
					if((ptr = (char *)malloc(512)) == NULL) {
						RET_ERR(PM_ERR_MEMORY, -1);
					}
					if(trans->flags & PM_TRANS_FLAG_ALLDEPS) {
						doremove=1;
					} else {
						QUESTION(trans, PM_TRANS_CONV_CORRUPTED_PKG, pkgname, NULL, NULL, &doremove);
					}
					if(doremove) {
						snprintf(str, PATH_MAX, "%s%s/%s-%s-%s" PM_EXT_PKG, handle->root, handle->cachedir, spkg->name(), spkg->version(), spkg->arch);
						unlink(str);
						snprintf(ptr, 512, _("archive %s was corrupted (bad MD5 or SHA1 checksum)\n"), pkgname);
					} else {
						snprintf(ptr, 512, _("archive %s is corrupted (bad MD5 or SHA1 checksum)\n"), pkgname);
					}
					*data = _pacman_list_add(*data, ptr);
					retval = 1;
				}
				FREE(md5sum2);
				FREE(sha1sum2);
			}
			if(!retval) {
				break;
			}
		}
	}

	if(retval) {
		pm_errno = PM_ERR_PKG_CORRUPTED;
		goto error;
	}
	if(!(trans->flags & PM_TRANS_FLAG_NOINTEGRITY)) {
		EVENT(trans, PM_TRANS_EVT_INTEGRITY_DONE, NULL, NULL);
	}
	if(trans->flags & PM_TRANS_FLAG_DOWNLOADONLY) {
		return(0);
	}
	if(!retval) {
		trans->state = STATE_COMMITING;
		retval = _pacman_sync_commit(trans, data);
		if(retval) {
			goto error;
		}
	}

	if(!varcache && !(trans->flags & PM_TRANS_FLAG_DOWNLOADONLY)) {
		/* delete packages */
		for(i = files; i; i = i->next) {
			unlink(i->data);
		}
	}
	return(retval);

error:
	/* commiting failed, so this is still just a prepared transaction */
	return(-1);
}

pmsyncpkg_t *__pmtrans_t::add(pmsyncpkg_t *syncpkg, int flags)
{
	pmsyncpkg_t *syncpkg_queued;

	ASSERT(syncpkg != NULL, RET_ERR(PM_ERR_TRANS_NULL, NULL));
	ASSERT(syncpkg->pkg != NULL || syncpkg->pkg_local != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	if(syncpkg->pkg != NULL && syncpkg->pkg_local == NULL) {
		Database *db_local = handle->db_local;

		syncpkg->pkg_local = _pacman_db_get_pkgfromcache(db_local, syncpkg->pkg->name());
	}

	if((syncpkg_queued = find(syncpkg->pkg_name)) != NULL) {
		/* FIXME: Try to compress syncpkg in syncpkg_queued more */
		return NULL;
	}
	_pacman_log(PM_LOG_FLOW2, _("adding target '%s' to the transaction set"), syncpkg->pkg_name);
	syncpkgs = _pacman_list_add(syncpkgs, syncpkg);
	return syncpkg;
}

int __pmtrans_t::add(Package *pkg, pmtranstype_t type, int flags)
{
	ASSERT(pkg != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	_pacman_log(PM_LOG_FLOW2, _("adding %s in the targets list"), pkg->name());
	packages = _pacman_list_add(packages, pkg);
	return 0;
}

pmsyncpkg_t *__pmtrans_t::add(const char *target, pmtranstype_t type, int flags)
{
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

	return trans->add(pkg_local, trans->type, 0);
}

int __pmtrans_t::add(const char *target)
{
	pmlist_t *i;
	Package *pkg_new, *pkg_local, *pkg_queued = NULL;
	Database *db_local;
	pmlist_t *dbs_sync = handle->dbs_sync;

	/* Sanity checks */
	ASSERT((db_local = handle->db_local) != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(!_pacman_strempty(target), RET_ERR(PM_ERR_WRONG_ARGS, -1));

	if(_pacman_list_is_strin(target, targets)) {
		RET_ERR(PM_ERR_TRANS_DUP_TARGET, -1);
	}
	targets = _pacman_stringlist_append(targets, target);

	if(type == PM_TRANS_TYPE_SYNC) {
	char targline[PKG_FULLNAME_LEN];
	char *targ;
	Package *spkg = NULL;
	int cmp;

	STRNCPY(targline, target, PKG_FULLNAME_LEN);
	targ = strchr(targline, '/');
	if(targ) {
		*targ = '\0';
		targ++;
		for(i = dbs_sync; i && !spkg; i = i->next) {
			Database *dbs = i->data;
			if(strcmp(dbs->treename, targline) == 0) {
				spkg = _pacman_db_get_pkgfromcache(dbs, targ);
				if(spkg == NULL) {
					/* Search provides */
					pmlist_t *p;
					_pacman_log(PM_LOG_FLOW2, _("target '%s' not found -- looking for provisions"), targ);
					p = _pacman_db_whatprovides(dbs, targ);
					if(p == NULL) {
						RET_ERR(PM_ERR_PKG_NOT_FOUND, -1);
					}
					_pacman_log(PM_LOG_DEBUG, _("found '%s' as a provision for '%s'"), p->data, targ);
					spkg = _pacman_db_get_pkgfromcache(dbs, p->data);
					FREELISTPTR(p);
				}
			}
		}
	} else {
		targ = targline;
		for(i = dbs_sync; i && !spkg; i = i->next) {
			Database *dbs = i->data;
			spkg = _pacman_db_get_pkgfromcache(dbs, targ);
		}
		if(spkg == NULL) {
			/* Search provides */
			_pacman_log(PM_LOG_FLOW2, _("target '%s' not found -- looking for provisions"), targ);
			for(i = dbs_sync; i && !spkg; i = i->next) {
				Database *dbs = i->data;
				pmlist_t *p = _pacman_db_whatprovides(dbs, targ);
				if(p) {
					_pacman_log(PM_LOG_DEBUG, _("found '%s' as a provision for '%s'"), p->data, targ);
					spkg = _pacman_db_get_pkgfromcache(dbs, p->data);
					FREELISTPTR(p);
				}
			}
		}
	}
	if(spkg == NULL) {
		RET_ERR(PM_ERR_PKG_NOT_FOUND, -1);
	}

	pkg_local = _pacman_db_get_pkgfromcache(db_local, spkg->name());
	if(pkg_local) {
		cmp = _pacman_versioncmp(pkg_local->version(), spkg->version());
		if(cmp > 0) {
			/* pkg_local version is newer -- get confirmation before adding */
			int resp = 0;
			QUESTION(this, PM_TRANS_CONV_LOCAL_NEWER, pkg_local, NULL, NULL, &resp);
			if(!resp) {
				_pacman_log(PM_LOG_WARNING, _("%s-%s: local version is newer -- skipping"), pkg_local->name(), pkg_local->version());
				return(0);
			}
		} else if(cmp == 0) {
			/* versions are identical -- get confirmation before adding */
			int resp = 0;
			QUESTION(this, PM_TRANS_CONV_LOCAL_UPTODATE, pkg_local, NULL, NULL, &resp);
			if(!resp) {
				_pacman_log(PM_LOG_WARNING, _("%s-%s is up to date -- skipping"), pkg_local->name(), pkg_local->version());
				return(0);
			}
		}
	}

	/* add the package to the transaction */
	if(!find(spkg->name())) {
		Package *dummy = NULL;
		pmsyncpkg_t *ps;

		if(pkg_local) {
			dummy = new Package(pkg_local->name(), pkg_local->version());
			if(dummy == NULL) {
				RET_ERR(PM_ERR_MEMORY, -1);
			}
		}
		ps = new __pmsyncpkg_t(PM_SYNC_TYPE_UPGRADE, spkg, dummy);
		if(ps == NULL) {
			delete dummy;
			RET_ERR(PM_ERR_MEMORY, -1);
		}
		add(ps, 0);
	}
	} else {
	if(type & PM_TRANS_TYPE_ADD) {
	/* Check if we need to add a fake target to the transaction. */
	if(strchr(target, '|')) {
		return(_pacman_fakedb_addtarget(this, target));
	}

	pkg_new = _pacman_filedb_load(NULL, target);
	if(pkg_new == NULL || _pacman_pkg_is_valid(pkg_new, this, target) != 0) {
		/* pm_errno is already set by _pacman_filedb_load() */
		goto error;
	}

	pkg_local = _pacman_db_get_pkgfromcache(db_local, pkg_new->name());
	if(type != PM_TRANS_TYPE_UPGRADE) {
		/* only install this package if it is not already installed */
		if(pkg_local != NULL) {
			pm_errno = PM_ERR_PKG_INSTALLED;
			goto error;
		}
	} else {
		if(flags & PM_TRANS_FLAG_FRESHEN) {
			/* only upgrade/install this package if it is already installed and at a lesser version */
			if(pkg_local == NULL || _pacman_versioncmp(pkg_local->version(), pkg_new->version()) >= 0) {
				pm_errno = PM_ERR_PKG_CANT_FRESH;
				goto error;
			}
		}
	}

	if(flags & PM_TRANS_FLAG_ALLDEPS) {
		pkg_new->m_reason = PM_PKG_REASON_DEPEND;
	}

	/* copy over the install reason */
	if(pkg_local) {
		pkg_new->m_reason = pkg_local->reason();
	}

	/* check if an older version of said package is already in transaction packages.
	 * if so, replace it in the list */
	for(i = packages; i; i = i->next) {
		Package *pkg = i->data;
		if(strcmp(pkg->name(), pkg_new->name()) == 0) {
			pkg_queued = pkg;
			break;
		}
	}

	if(pkg_queued != NULL) {
		if(_pacman_versioncmp(pkg_queued->version(), pkg_new->version()) < 0) {
			_pacman_log(PM_LOG_WARNING, _("replacing older version %s-%s by %s in target list"),
			          pkg_queued->name(), pkg_queued->version(), pkg_new->version());
			f_ptrswap(&i->data, (void **)&pkg_new);
		} else {
			_pacman_log(PM_LOG_WARNING, _("newer version %s-%s is in the target list -- skipping"),
			          pkg_queued->name(), pkg_queued->version(), pkg_new->version());
		}
		delete pkg_new;
	} else {
		add(pkg_new, type, 0);
	}
	}
	if(type == PM_TRANS_TYPE_REMOVE) {
		if(_pacman_remove_addtarget(this, target) == -1) {
			return -1;
		}
	}
	}

	return(0);

error:
	delete pkg_new;
	return(-1);
}

int __pmtrans_t::prepare(pmlist_t **data)
{
	Database *db_local;
	pmlist_t *lp;
	pmlist_t *deps = NULL;
	pmlist_t *list = NULL; /* list allowing checkdeps usage with data from packages */
	pmlist_t *trail = NULL; /* breadcrum list to avoid running into circles */
	pmlist_t *asked = NULL;
	pmlist_t *i, *j, *k, *l, *m;
	int ret = 0;

	/* Sanity checks */
	ASSERT((db_local = handle->db_local) != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	if(data) {
		*data = NULL;
	}

	/* If there's nothing to do, return without complaining */
	if(_pacman_list_empty(packages) &&
		_pacman_list_empty(syncpkgs)) {
		return(0);
	}

	_pacman_trans_compute_triggers(this);

	if(type == PM_TRANS_TYPE_SYNC) {
	for(i = syncpkgs; i; i = i->next) {
		pmsyncpkg_t *ps = i->data;
		list = _pacman_list_add(list, ps->pkg);
	}

	if(!(flags & PM_TRANS_FLAG_NODEPS)) {
		trail = _pacman_list_new();

		/* Resolve targets dependencies */
		EVENT(this, PM_TRANS_EVT_RESOLVEDEPS_START, NULL, NULL);
		_pacman_log(PM_LOG_FLOW1, _("resolving targets dependencies"));
		for(i = syncpkgs; i; i = i->next) {
			Package *spkg = ((pmsyncpkg_t *)i->data)->pkg;
			if(_pacman_resolvedeps(this, spkg, list, trail, data) == -1) {
				/* pm_errno is set by resolvedeps */
				ret = -1;
				goto cleanup;
			}
		}

		for(i = list; i; i = i->next) {
			/* add the dependencies found by resolvedeps to the transaction set */
			Package *spkg = i->data;
			if(!find(spkg->name())) {
				pmsyncpkg_t *ps = new __pmsyncpkg_t(PM_SYNC_TYPE_DEPEND, spkg, NULL);
				if(ps == NULL) {
					ret = -1;
					goto cleanup;
				}
				syncpkgs = _pacman_list_add(syncpkgs, ps);
				_pacman_log(PM_LOG_FLOW2, _("adding package %s-%s to the transaction targets"),
						spkg->name(), spkg->version());
			} else {
				/* remove the original targets from the list if requested */
				if((flags & PM_TRANS_FLAG_DEPENDSONLY)) {
					/* they are just pointers so we don't have to free them */
					syncpkgs = _pacman_list_remove(syncpkgs, spkg, pkg_cmp, NULL);
				}
			}
		}

		/* re-order w.r.t. dependencies */
		k = l = NULL;
		for(i=syncpkgs; i; i=i->next) {
			pmsyncpkg_t *s = (pmsyncpkg_t*)i->data;
			k = _pacman_list_add(k, s->pkg);
		}
		m = _pacman_sortbydeps(k, PM_TRANS_TYPE_ADD);
		for(i=m; i; i=i->next) {
			for(j=syncpkgs; j; j=j->next) {
				pmsyncpkg_t *s = (pmsyncpkg_t*)j->data;
				if(s->pkg==i->data) {
					l = _pacman_list_add(l, s);
				}
			}
		}
		FREELISTPTR(k);
		FREELISTPTR(m);
		FREELISTPTR(syncpkgs);
		syncpkgs = l;

		EVENT(this, PM_TRANS_EVT_RESOLVEDEPS_DONE, NULL, NULL);

		_pacman_log(PM_LOG_FLOW1, _("looking for unresolvable dependencies"));
		deps = _pacman_checkdeps(this, PM_TRANS_TYPE_UPGRADE, list);
		if(!f_ptrlist_empty(deps)) {
			if(data) {
				*data = deps;
			}
			pm_errno = PM_ERR_UNSATISFIED_DEPS;
			ret = -1;
			goto cleanup;
		}

		FREELISTPTR(trail);
	}

	if(!(flags & PM_TRANS_FLAG_NOCONFLICTS)) {
		/* check for inter-conflicts and whatnot */
		EVENT(this, PM_TRANS_EVT_INTERCONFLICTS_START, NULL, NULL);

		_pacman_log(PM_LOG_FLOW1, _("looking for conflicts"));
		deps = _pacman_checkconflicts(this, list);
		if(!f_ptrlist_empty(deps)) {
			int errorout = 0;

			for(i = deps; i && !errorout; i = i->next) {
				pmdepmissing_t *miss = i->data;
				int found = 0;
				pmsyncpkg_t *ps;
				Package *local;

				_pacman_log(PM_LOG_FLOW2, _("package '%s' is conflicting with '%s'"),
						  miss->target, miss->depend.name);

				/* check if the conflicting package is one that's about to be removed/replaced.
				 * if so, then just ignore it
				 */
				for(j = syncpkgs; j && !found; j = j->next) {
					ps = j->data;
					if(ps->type == PM_SYNC_TYPE_REPLACE) {
						if(_pacman_pkg_isin(miss->depend.name, ps->data)) {
							found = 1;
						}
					}
				}
				if(found) {
					_pacman_log(PM_LOG_DEBUG, _("'%s' is already elected for removal -- skipping"),
							miss->depend.name);
					continue;
				}

				ps = find(miss->target);
				if(ps == NULL) {
					_pacman_log(PM_LOG_DEBUG, _("'%s' not found in transaction set -- skipping"),
							  miss->target);
					continue;
				}
				local = _pacman_db_get_pkgfromcache(db_local, miss->depend.name);
				/* check if this package also "provides" the package it's conflicting with
				 */
				if(ps->pkg->provides(miss->depend.name)) {
					/* so just treat it like a "replaces" item so the REQUIREDBY
					 * fields are inherited properly.
					 */
					_pacman_log(PM_LOG_DEBUG, _("package '%s' provides its own conflict"), miss->target);
					if(local) {
						/* nothing to do for now: it will be handled later
						 * (not the same behavior as in pacman) */
					} else {
						char *rmpkg = NULL;
						int target, depend;
						/* hmmm, depend.name isn't installed, so it must be conflicting
						 * with another package in our final list.  For example:
						 *
						 *     pacman-g2 -S blackbox xfree86
						 *
						 * If no x-servers are installed and blackbox pulls in xorg, then
						 * xorg and xfree86 will conflict with each other.  In this case,
						 * we should follow the user's preference and rip xorg out of final,
						 * opting for xfree86 instead.
						 */

						/* figure out which one was requested in targets.  If they both were,
						 * then it's still an unresolvable conflict. */
						target = _pacman_list_is_strin(miss->target, targets);
						depend = _pacman_list_is_strin(miss->depend.name, targets);
						if(depend && !target) {
							_pacman_log(PM_LOG_DEBUG, _("'%s' is in the target list -- keeping it"),
								miss->depend.name);
							/* remove miss->target */
							rmpkg = miss->target;
						} else if(target && !depend) {
							_pacman_log(PM_LOG_DEBUG, _("'%s' is in the target list -- keeping it"),
								miss->target);
							/* remove miss->depend.name */
							rmpkg = miss->depend.name;
						} else {
							/* miss->depend.name is not needed, miss->target already provides
							 * it, let's resolve the conflict */
							rmpkg = miss->depend.name;
						}
						if(rmpkg) {
							pmsyncpkg_t *rsync = find(rmpkg);
							pmsyncpkg_t *spkg = NULL;

							_pacman_log(PM_LOG_FLOW2, _("removing '%s' from target list"), rmpkg);
							syncpkgs = _pacman_list_remove(syncpkgs, rsync, _pacman_syncpkg_cmp, (void **)&spkg);
							delete spkg;
							continue;
						}
					}
				}
				/* It's a conflict -- see if they want to remove it
				*/
				_pacman_log(PM_LOG_DEBUG, _("resolving package '%s' conflict"), miss->target);
				if(local) {
					int doremove = 0;
					if(!_pacman_list_is_strin(miss->depend.name, asked)) {
						QUESTION(this, PM_TRANS_CONV_CONFLICT_PKG, miss->target, miss->depend.name, NULL, &doremove);
						asked = _pacman_stringlist_append(asked, miss->depend.name);
						if(doremove) {
							pmsyncpkg_t *rsync = find(miss->depend.name);
							Package *q = new Package(miss->depend.name, NULL);
							if(q == NULL) {
								if(data) {
									FREELIST(*data);
								}
								ret = -1;
								goto cleanup;
							}
							q->m_requiredby = _pacman_list_strdup(local->requiredby());
							if(ps->type != PM_SYNC_TYPE_REPLACE) {
								/* switch this sync type to REPLACE */
								ps->type = PM_SYNC_TYPE_REPLACE;
								delete ps->data;
								ps->data = NULL;
							}
							/* append to the replaces list */
							_pacman_log(PM_LOG_FLOW2, _("electing '%s' for removal"), miss->depend.name);
							ps->data = _pacman_list_add(ps->data, q);
							if(rsync) {
								/* remove it from the target list */
								pmsyncpkg_t *spkg = NULL;

								_pacman_log(PM_LOG_FLOW2, _("removing '%s' from target list"), miss->depend.name);
								syncpkgs = _pacman_list_remove(syncpkgs, rsync, _pacman_syncpkg_cmp, (void **)&spkg);
								delete spkg;
							}
						} else {
							/* abort */
							_pacman_log(PM_LOG_ERROR, _("unresolvable package conflicts detected"));
							errorout = 1;
							if(data) {
								if((miss = _pacman_malloc(sizeof(pmdepmissing_t))) == NULL) {
									FREELIST(*data);
									ret = -1;
									goto cleanup;
								}
								*miss = *(pmdepmissing_t *)i->data;
								*data = _pacman_list_add(*data, miss);
							}
						}
					}
				} else {
					_pacman_log(PM_LOG_ERROR, _("unresolvable package conflicts detected"));
					errorout = 1;
					if(data) {
						if((miss = _pacman_malloc(sizeof(pmdepmissing_t))) == NULL) {
							FREELIST(*data);
							ret = -1;
							goto cleanup;
						}
						*miss = *(pmdepmissing_t *)i->data;
						*data = _pacman_list_add(*data, miss);
					}
				}
			}
			if(errorout) {
				pm_errno = PM_ERR_CONFLICTING_DEPS;
				ret = -1;
				goto cleanup;
			}
			FREELIST(deps);
			FREELIST(asked);
		}
		EVENT(this, PM_TRANS_EVT_INTERCONFLICTS_DONE, NULL, NULL);
	}

	FREELISTPTR(list);

	/* XXX: this fails for cases where a requested package wants
	 *      a dependency that conflicts with an older version of
	 *      the package.  It will be removed from final, and the user
	 *      has to re-request it to get it installed properly.
	 *
	 *      Not gonna happen very often, but should be dealt with...
	 */

	if(!(flags & PM_TRANS_FLAG_NODEPS)) {
		/* Check dependencies of packages in rmtargs and make sure
		 * we won't be breaking anything by removing them.
		 * If a broken dep is detected, make sure it's not from a
		 * package that's in our final (upgrade) list.
		 */
		/*EVENT(this, PM_TRANS_EVT_CHECKDEPS_DONE, NULL, NULL);*/
		for(i = syncpkgs; i; i = i->next) {
			pmsyncpkg_t *ps = i->data;
			if(ps->type == PM_SYNC_TYPE_REPLACE) {
				for(j = ps->data; j; j = j->next) {
					list = _pacman_list_add(list, j->data);
				}
			}
		}
		if(list) {
			_pacman_log(PM_LOG_FLOW1, _("checking dependencies of packages designated for removal"));
			deps = _pacman_checkdeps(this, PM_TRANS_TYPE_REMOVE, list);
			if(deps) {
				int errorout = 0;
				for(i = deps; i; i = i->next) {
					pmdepmissing_t *miss = i->data;
					if(!find(miss->depend.name)) {
						int pfound = 0;
						/* If miss->depend.name depends on something that miss->target and a
						 * package in final both provide, then it's okay...  */
						Package *leavingp  = _pacman_db_get_pkgfromcache(db_local, miss->target);
						Package *conflictp = _pacman_db_get_pkgfromcache(db_local, miss->depend.name);
						if(!leavingp || !conflictp) {
							_pacman_log(PM_LOG_ERROR, _("something has gone horribly wrong"));
							ret = -1;
							goto cleanup;
						}
						/* Look through the upset package's dependencies and try to match one up
						 * to a provisio from the package we want to remove */
						for(k = conflictp->depends(); k && !pfound; k = k->next) {
							pmlist_t *m;
							for(m = leavingp->provides(); m && !pfound; m = m->next) {
								if(!strcmp(k->data, m->data)) {
									/* Found a match -- now look through final for a package that
									 * provides the same thing.  If none are found, then it truly
									 * is an unresolvable conflict. */
									pmlist_t *n, *o;
									for(n = syncpkgs; n && !pfound; n = n->next) {
										pmsyncpkg_t *sp = n->data;
										for(o = sp->pkg->provides(); o && !pfound; o = o->next) {
											if(!strcmp(m->data, o->data)) {
												/* found matching provisio -- we're good to go */
												_pacman_log(PM_LOG_FLOW2, _("found '%s' as a provision for '%s' -- conflict aborted"),
														sp->pkg->name(), (char *)o->data);
												pfound = 1;
											}
										}
									}
								}
							}
						}
						if(!pfound) {
							if(!errorout) {
								errorout = 1;
							}
							if(data) {
								if((miss = _pacman_malloc(sizeof(pmdepmissing_t))) == NULL) {
									FREELIST(*data);
									ret = -1;
									goto cleanup;
								}
								*miss = *(pmdepmissing_t *)i->data;
								*data = _pacman_list_add(*data, miss);
							}
						}
					}
				}
				if(errorout) {
					pm_errno = PM_ERR_UNSATISFIED_DEPS;
					ret = -1;
					goto cleanup;
				}
				FREELIST(deps);
			}
		}
		/*EVENT(this, PM_TRANS_EVT_CHECKDEPS_DONE, NULL, NULL);*/
	}

#ifndef __sun__
	/* check for free space only in case the packages will be extracted */
	if(!(flags & PM_TRANS_FLAG_NOCONFLICTS)) {
		if(_pacman_check_freespace(this, data) == -1) {
				/* pm_errno is set by check_freespace */
				ret = -1;
				goto cleanup;
		}
	}
#endif

	/* issue warning if the local db is too old */
	check_olddelay();

cleanup:
	FREELISTPTR(list);
	FREELISTPTR(trail);
	FREELIST(asked);

	if(ret != 0) {
		return ret;
	}
	} else {
	/* Check dependencies
	 */
	if(!(flags & PM_TRANS_FLAG_NODEPS)) {
		EVENT(this, PM_TRANS_EVT_CHECKDEPS_START, NULL, NULL);
		_pacman_log(PM_LOG_FLOW1, _("looking for unsatisfied dependencies"));

		lp = _pacman_checkdeps(this, type, packages);

		/* look for unsatisfied dependencies */
		if(lp != NULL) {
			if((type == PM_TRANS_TYPE_REMOVE) && (flags & PM_TRANS_FLAG_CASCADE)) {
				while(lp) {
					pmlist_t *i;
					for(i = lp; i; i = i->next) {
						pmdepmissing_t *miss = (pmdepmissing_t *)i->data;
						Package *pkg_local = db_local->scan(miss->depend.name, INFRQ_ALL);
						if(pkg_local) {
							_pacman_log(PM_LOG_FLOW2, _("pulling %s in the targets list"), pkg_local->name());
							packages = _pacman_list_add(packages, pkg_local);
						} else {
							_pacman_log(PM_LOG_ERROR, _("could not find %s in database -- skipping"),
								miss->depend.name);
						}
					}
					FREELIST(lp);
					lp = _pacman_checkdeps(this, type, packages);
				}
			}
		}
		if(lp != NULL){
			if(data) {
				*data = lp;
			} else {
				FREELIST(lp);
			}
			RET_ERR(PM_ERR_UNSATISFIED_DEPS, -1);
		}

		if(type & PM_TRANS_TYPE_ADD) {
		/* no unsatisfied deps, so look for conflicts */
		_pacman_log(PM_LOG_FLOW1, _("looking for conflicts"));
		lp = _pacman_checkconflicts(this, packages);
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
		lp = _pacman_sortbydeps(packages, PM_TRANS_TYPE_ADD);
		/* free the old alltargs */
		FREELISTPTR(packages);
		packages = lp;
		}

		if(type == PM_TRANS_TYPE_REMOVE && type != PM_TRANS_TYPE_UPGRADE) {
		if(flags & PM_TRANS_FLAG_RECURSE) {
			_pacman_log(PM_LOG_FLOW1, _("finding removable dependencies"));
			packages = _pacman_removedeps(db_local, packages);
		}

		/* re-order w.r.t. dependencies */
		_pacman_log(PM_LOG_FLOW1, _("sorting by dependencies"));
		lp = _pacman_sortbydeps(packages, PM_TRANS_TYPE_REMOVE);
		/* free the old alltargs */
		FREELISTPTR(packages);
		packages = lp;
		}
		EVENT(this, PM_TRANS_EVT_CHECKDEPS_DONE, NULL, NULL);
	}

	/* Cleaning up
	 */
	if(type & PM_TRANS_TYPE_ADD) {
	EVENT(this, PM_TRANS_EVT_CLEANUP_START, NULL, NULL);
	_pacman_log(PM_LOG_FLOW1, _("cleaning up"));
	for (lp = packages; lp!=NULL; lp=lp->next) {
		Package *pkg_new=(Package *)lp->data;
		pmlist_t *rmlist;

		for (rmlist=pkg_new->removes(); rmlist!=NULL; rmlist=rmlist->next) {
			char rm_fname[PATH_MAX];

			snprintf(rm_fname, PATH_MAX, "%s%s", handle->root, (char *)rmlist->data);
			remove(rm_fname);
		}
	}
	EVENT(this, PM_TRANS_EVT_CLEANUP_DONE, NULL, NULL);

	/* Check for file conflicts
	 */
	if(!(flags & PM_TRANS_FLAG_FORCE)) {
		EVENT(this, PM_TRANS_EVT_FILECONFLICTS_START, NULL, NULL);

		_pacman_log(PM_LOG_FLOW1, _("looking for file conflicts"));
		lp = _pacman_db_find_conflicts(this);
		if(lp != NULL) {
			if(data) {
				*data = lp;
			} else {
				FREELIST(lp);
			}
			RET_ERR(PM_ERR_FILE_CONFLICTS, -1);
		}
		EVENT(this, PM_TRANS_EVT_FILECONFLICTS_DONE, NULL, NULL);
	}

#ifndef __sun__
	if(_pacman_check_freespace(this, data) == -1) {
			/* pm_errno is set by check_freespace */
			return(-1);
	}
#endif
	}
	}

	_pacman_trans_set_state(this, STATE_PREPARED);

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
							pkg->name(), pkg->version());
					} else {
						/* the install script goes inside the db */
						snprintf(expath, PATH_MAX, "%s/%s-%s/install", db_local->path,
							pkg->name(), pkg->version());
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
								nb = _pacman_list_is_strin(pathname, pkg->backup());
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
					for(lp = pkg->backup(); lp; lp = lp->next) {
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

					free(md5_local);
					free(md5_pkg);
					free(sha1_local);
					free(sha1_pkg);
					free(hash_orig);
					unlink(temp);
					free(temp);
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
					for(lp = pkg->backup(); lp; lp = lp->next) {
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

			int filenum = _pacman_list_count(pkg->files());
			_pacman_log(PM_LOG_FLOW1, _("removing files"));

			/* iterate through the list backwards, unlinking files */
			for(lp = _pacman_list_last(pkg->files()); lp; lp = lp->prev) {
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
				_pacman_log(PM_LOG_FLOW1, _("upgrading package %s-%s"), pkg_new->name(), pkg_new->version());

				/* we'll need to save some record for backup checks later */
				oldpkg = new Package(pkg_local->name(), pkg_local->version());
				if(oldpkg) {
					if(!(pkg_local->flags & INFRQ_FILES)) {
						_pacman_log(PM_LOG_DEBUG, _("loading FILES info for '%s'"), pkg_local->name());
						pkg_local->read(INFRQ_FILES);
					}
					oldpkg->m_backup = _pacman_list_strdup(pkg_local->m_backup);
					strncpy(oldpkg->m_name, pkg_local->name(), PKG_NAME_LEN);
					strncpy(oldpkg->m_version, pkg_local->version(), PKG_VERSION_LEN);
				}

				/* pre_upgrade scriptlet */
				if(pkg_new->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
					_pacman_runscriptlet(handle->root, pkg_new->data, "pre_upgrade", pkg_new->version(), oldpkg ? oldpkg->version() : NULL,
						trans);
				}

				if(oldpkg) {
					pmtrans_t *tr;
					pmtrans_cbs_t null_cbs = { NULL };

					_pacman_log(PM_LOG_FLOW1, _("removing old package first (%s-%s)"), oldpkg->name(), oldpkg->version());
					tr = new __pmtrans_t(PM_TRANS_TYPE_UPGRADE, trans->flags, null_cbs);
					if(tr == NULL) {
						RET_ERR(PM_ERR_TRANS_ABORT, -1);
					}
					if(_pacman_remove_addtarget(tr, pkg_new->name()) == -1) {
						delete tr;
						RET_ERR(PM_ERR_TRANS_ABORT, -1);
					}
					/* copy the skiplist over */
					tr->skiplist = _pacman_list_strdup(trans->skiplist);
					if(_pacman_remove_commit(tr, NULL) == -1) {
						delete tr;
						RET_ERR(PM_ERR_TRANS_ABORT, -1);
					}
					delete tr;
				}
			_pacman_log(PM_LOG_FLOW1, _("adding new package %s-%s"), pkg_new->name(), pkg_new->version());
		} else {
			EVENT(trans, PM_TRANS_EVT_ADD_START, pkg_new, NULL);
			cb_state = PM_TRANS_PROGRESS_ADD_START;
			_pacman_log(PM_LOG_FLOW1, _("adding package %s-%s"), pkg_new->name(), pkg_new->version());

			/* pre_install scriptlet */
			if(pkg_new->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
				_pacman_runscriptlet(handle->root, pkg_new->data, "pre_install", pkg_new->version(), NULL, trans);
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
			for(tmppm = tmpp->depends(); tmppm; tmppm = tmppm->next) {
				pmdepend_t depend;
				if(_pacman_splitdep(tmppm->data, &depend)) {
					continue;
				}
				if(tmppm->data && (!strcmp(depend.name, pkg_new->name()) || pkg_new->provides(depend.name))) {
					_pacman_log(PM_LOG_DEBUG, _("adding '%s' in requiredby field for '%s'"), tmpp->name(), pkg_new->name());
					pkg_new->m_requiredby = _pacman_stringlist_append(pkg_new->m_requiredby, tmpp->name());
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
			          pkg_new->name(), pkg_new->version());
			RET_ERR(PM_ERR_DB_WRITE, -1);
		}
		if(_pacman_db_add_pkgincache(db_local, pkg_new) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not add entry '%s' in cache"), pkg_new->name());
		}

		/* update dependency packages' REQUIREDBY fields */
		if(pkg_new->depends()) {
			_pacman_log(PM_LOG_FLOW2, _("updating dependency packages 'requiredby' fields"));
		}
		for(lp = pkg_new->depends(); lp; lp = lp->next) {
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
			depinfo->m_requiredby = _pacman_stringlist_append(depinfo->requiredby(), pkg_new->name());
			if(db_local->write(depinfo, INFRQ_DEPENDS)) {
				_pacman_log(PM_LOG_ERROR, _("could not update 'requiredby' database entry %s-%s"),
				          depinfo->name(), depinfo->version());
			}
		}

		EVENT(trans, PM_TRANS_EVT_EXTRACT_DONE, NULL, NULL);

		/* run the post-install script if it exists  */
		if(pkg_new->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
			/* must run ldconfig here because some scriptlets fail due to missing libs otherwise */
			_pacman_ldconfig(handle->root);
			snprintf(pm_install, PATH_MAX, "%s%s/%s/%s-%s/install", handle->root, handle->dbpath, db_local->treename, pkg_new->name(), pkg_new->version());
			if(pmo_upgrade) {
				_pacman_runscriptlet(handle->root, pm_install, "post_upgrade", pkg_new->version(), oldpkg ? oldpkg->version() : NULL, trans);
			} else {
				_pacman_runscriptlet(handle->root, pm_install, "post_install", pkg_new->version(), NULL, trans);
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
			_pacman_log(PM_LOG_FLOW1, _("removing package %s-%s"), pkg_local->name(), pkg_local->version());

			/* run the pre-remove scriptlet if it exists */
			if(pkg_local->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
				snprintf(pm_install, PATH_MAX, "%s/%s-%s/install", db_local->path, pkg_local->name(), pkg_local->version());
				_pacman_runscriptlet(handle->root, pm_install, "pre_remove", pkg_local->version(), NULL, trans);
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
				snprintf(pm_install, PATH_MAX, "%s/%s-%s/install", db_local->path, pkg_local->name(), pkg_local->version());
				_pacman_runscriptlet(handle->root, pm_install, "post_remove", pkg_local->version(), NULL, trans);
			}
		}

		/* remove the package from the database */
		_pacman_log(PM_LOG_FLOW1, _("updating database"));
		_pacman_log(PM_LOG_FLOW2, _("removing database entry '%s'"), pkg_local->name());
		if(pkg_local->remove() == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not remove database entry %s-%s"), pkg_local->name(), pkg_local->version());
		}
		if(_pacman_db_remove_pkgfromcache(db_local, pkg_local) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not remove entry '%s' from cache"), pkg_local->name());
		}

		/* update dependency packages' REQUIREDBY fields */
		_pacman_log(PM_LOG_FLOW2, _("updating dependency packages 'requiredby' fields"));
		for(lp = pkg_local->depends(); lp; lp = lp->next) {
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
			depinfo->m_requiredby = _pacman_list_remove(depinfo->requiredby(), pkg_local->name(), str_cmp, (void **)&data);
			FREE(data);
			_pacman_log(PM_LOG_DEBUG, _("updating 'requiredby' field for package '%s'"), depinfo->name());
			if(db_local->write(depinfo, INFRQ_DEPENDS)) {
				_pacman_log(PM_LOG_ERROR, _("could not update 'requiredby' database entry %s-%s"),
					depinfo->name(), depinfo->version());
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

int __pmtrans_t::commit(pmlist_t **data)
{
	if(data!=NULL)
		*data = NULL;

	/* If there's nothing to do, return without complaining */
	if(_pacman_list_empty(packages) &&
		_pacman_list_empty(syncpkgs)) {
		return(0);
	}

	_pacman_trans_set_state(this, STATE_COMMITING);

	if(type == PM_TRANS_TYPE_SYNC) {
		if(_pacman_trans_download_commit(this, data) == -1) {
			_pacman_trans_set_state(this, STATE_PREPARED);
			return(-1);
		}
	} else {
	if(type & PM_TRANS_TYPE_ADD) {
		if(_pacman_add_commit(this, data) == -1) {
			return -1;
		}
	}
	if(type == PM_TRANS_TYPE_REMOVE) {
		if(_pacman_remove_commit(this, data) == -1) {
			return -1;
		}
	}
	}

	_pacman_trans_set_state(this, STATE_COMMITED);

	return(0);
}

/* vim: set ts=2 sw=2 noet: */
