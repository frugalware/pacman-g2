/*
 *  trans.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2005, 2006 by Miklos Vajna <vmiklos@frugalware.org>
 *  Copyright (c) 2013 by Michel Hermier <hermier@frugalware.org>
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

#include "conflict.h"
#include "deps.h"
#include "error.h"
#include "package.h"
#include "util.h"
#include "log.h"
#include "handle.h"
#include "sync.h"
#include "cache.h"
#include "pacman.h"

#include "trans_sysupgrade.h"

#include <fstring.h>
#include <fstringlist.h>

pmsyncpkg_t *__pacman_trans_pkg_new (pmtrans_t *trans) {
	pmsyncpkg_t *transpkg = _pacman_zalloc (sizeof (*transpkg));

	if (transpkg == NULL) {
		return(NULL);
	}

	f_graphvertex_init (&transpkg->as_graphvertex);
	return transpkg;
}

void __pacman_trans_pkg_delete(pmsyncpkg_t *trans_pkg)
{
	if (trans_pkg == NULL) {
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

const char *__pacman_transpkg_name (pmsyncpkg_t *transpkg) {
	if (transpkg != NULL) {
		if (transpkg->pkg_new != NULL) {
			return transpkg->pkg_new->name;
		}
		if (transpkg->pkg_local != NULL) {
			return transpkg->pkg_local->name;
		}
	}
	return NULL;
}

static
int __pacman_transpkg_cmp (pmtranspkg_t *transpkg1, pmtranspkg_t *transpkg2) {
	return strcmp (__pacman_transpkg_name (transpkg1), __pacman_transpkg_name (transpkg2)); 
}

static
int __pacman_transpkg_detect_name (pmtranspkg_t *transpkg, const char *package) {
	return strcmp (__pacman_transpkg_name (transpkg), package);
}

int _pacman_transpkg_has_flags (pmtranspkg_t *transpkg, int flagsmask) {
	return (transpkg->flags & flagsmask) == flagsmask ? 0 : 1;
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

pmsyncpkg_t *_pacman_trans_get_transpkg (pmtrans_t *trans, const char *package_name, int flags) {
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, NULL));
	ASSERT(f_strlen (package_name) != 0, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return f_ptrlistitem_get (f_ptrlist_detect (trans->packages, (FDetectFunc)__pacman_transpkg_detect_name, (void *)package_name));
}

pmsyncpkg_t *__pacman_trans_get_trans_pkg(pmtrans_t *trans, const char *package) {
	return _pacman_trans_get_transpkg(trans, package, 0);
}

static
void __pacman_trans_fini(FObject *obj) {
	pmtrans_t *trans = (pmtrans_t *)obj;

	f_ptrlist_delete (trans->packages, (FVisitorFunc)__pacman_trans_pkg_delete, NULL);
	FREELIST(trans->skiplist);
}

static const
FObjectOperations _pacman_trans_ops = {
	.fini = __pacman_trans_fini,
};

pmtrans_t *_pacman_trans_new()
{
	pmtrans_t *trans = _pacman_zalloc(sizeof(pmtrans_t));

	if (trans != NULL) {
		f_object_init (&trans->base, &_pacman_trans_ops);
		f_graph_init (&trans->transpkg_graph);
		trans->state = STATE_IDLE;
	}

	return(trans);
}

void _pacman_trans_free(pmtrans_t *trans) {
	f_object_delete (&trans->base);
}

int _pacman_trans_init(pmtrans_t *trans, pmtranstype_t type, unsigned int flags, pmtrans_cbs_t cbs)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	switch(type) {
		case PM_TRANS_TYPE_ADD:
		case PM_TRANS_TYPE_REMOVE:
		case PM_TRANS_TYPE_UPGRADE:
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
pmpkg_t *_pacman_db_search_provider(pmdb_t *db, const char *name) {
	pmlist_t *p;
	pmpkg_t *pkg;

	_pacman_log(PM_LOG_FLOW2, _("looking for provisions of '%s' in database '%s'"), name, db->treename);
	p = _pacman_db_whatprovides(db, name);
	if (p == NULL) {
		RET_ERR(PM_ERR_PKG_NOT_FOUND, NULL);
	}
	pkg = p->data;
	FREELISTPTR(p);
	_pacman_log(PM_LOG_DEBUG, _("found '%s' as a provision for '%s' in database '%S'"), pkg->name, name, db->treename);
	return pkg;
}

static
pmpkg_t *_pacman_dblist_get_pkg(pmlist_t *dblist, const char *pkg_name, int flags) {
	pmlist_t *i;
	pmpkg_t *pkg = NULL;

	f_foreach (i, dblist) {
		pmdb_t *db = i->data;

		if ((pkg = _pacman_db_get_pkgfromcache(db, pkg_name)) != NULL) {
			break;
		}
	}
	if (pkg == NULL &&
			flags & PM_TRANS_FLAG_ALLOWPROVIDEREPLACEMENT) {
		_pacman_log(PM_LOG_FLOW2, _("target '%s' not found -- looking for provisions"), pkg_name);
		f_foreach (i, dblist) {
			pmdb_t *db = i->data;

			if ((pkg = _pacman_db_search_provider (db, pkg_name)) != NULL) {
				break;
			}
		}
	}
	if (pkg == NULL) {
		RET_ERR(PM_ERR_PKG_NOT_FOUND, NULL);
	}
	return pkg;
}

static
pmtranspkg_t *_pacman_trans_add (pmtrans_t *trans, pmtranspkg_t *transpkg) {
	const char *transpkg_name = __pacman_transpkg_name (transpkg);
	pmtranspkg_t *transpkg_in;

	/* Check if an transaction packages already exist for the same package,
	 * and if so try to compress it */
	if ((transpkg_in = __pacman_trans_get_trans_pkg (trans, transpkg_name)) != NULL) {
		/* If both transpkg tries to install (or upgrade), let's compress the transpkg. */
		/* FIXME: do something with transpkg->flags */
		if (transpkg_in->type & PM_TRANS_TYPE_ADD &&
				transpkg->type & PM_TRANS_TYPE_ADD) {
			if (_pacman_versioncmp(transpkg_in->pkg_new->version, transpkg->pkg_new->version) < 0) {
				_pacman_log (PM_LOG_WARNING, _("replacing older version %s-%s by %s in target list"),
						transpkg_in->pkg_new->name, transpkg_in->pkg_new->version, transpkg->pkg_new->version);
				f_ptrswap (&transpkg_in->pkg_new, &transpkg->pkg_new);
			} else {
				_pacman_log(PM_LOG_WARNING, _("newer version %s-%s is in the target list -- skipping"),
						transpkg_in->pkg_new->name, transpkg_in->pkg_new->version, transpkg->pkg_new->version);
			}
			__pacman_trans_pkg_delete (transpkg);
			return transpkg_in;
		}

		/* If both transpkg tries to remove (and not upgrade), let's compress the transpkg. */
		if (transpkg_in->type == PM_TRANS_TYPE_REMOVE &&
				transpkg->type == PM_TRANS_TYPE_REMOVE) {
			_pacman_log(PM_LOG_DEBUG, _("%s transpkg removal compressed"), transpkg_name);
			__pacman_trans_pkg_delete (transpkg);
			return transpkg_in;
		}
		RET_ERR(PM_ERR_TRANS_DUP_TARGET, NULL);
	}

	if (transpkg->type & PM_TRANS_TYPE_ADD) {
		/* check pmo_ignorepkg and pmo_s_ignore to make sure we haven't pulled in
		 * something we're not supposed to.
		 */
		if(f_stringlist_find (handle->ignorepkg, transpkg_name)) {
			int usedep = 1;

			QUESTION(trans, PM_TRANS_CONV_INSTALL_IGNOREPKG, NULL, transpkg->pkg_new, NULL, &usedep);
			if (usedep == 0) {
				return NULL;
			}
		}

		/* Ensure pkg_local is filled */
		if (transpkg->pkg_local == NULL) {
			transpkg->pkg_local = _pacman_db_scan(trans->handle->db_local, transpkg_name, INFRQ_ALL);
//			trans_pkg->pkg_local = _pacman_db_get_pkgfromcache(db_local, _pacman_pkg_getinfo(trans_pkg->pkg_new, PM_PKG_NAME));
		}

		if (transpkg->pkg_local != NULL) {
			/* Only install this package if it is not already installed */
			if (transpkg->type == PM_TRANS_TYPE_ADD) {
				RET_ERR(PM_ERR_PKG_INSTALLED, NULL);
			}

			/* Copy over the install reason */
			transpkg->pkg_new->reason = (long)_pacman_pkg_getinfo(transpkg->pkg_local, PM_PKG_REASON);
		} else {
			/* no previous package version is installed, so this is actually
			 * just an install. */
			transpkg->type &= ~PM_TRANS_TYPE_REMOVE;
		}

		if (trans->flags & PM_TRANS_FLAG_ALLDEPS) {
			transpkg->pkg_new->reason = PM_PKG_REASON_DEPEND;
		}
		if (transpkg->flags & PM_TRANS_FLAG_EXPLICIT) {
			transpkg->pkg_new->reason = PM_PKG_REASON_EXPLICIT;
		}
	}

	/* Check if it is really a upgrade and not an install */
	if (transpkg->type == PM_TRANS_TYPE_UPGRADE &&
			transpkg->pkg_local == NULL) {
		if (trans->flags & PM_TRANS_FLAG_FRESHEN) {
			RET_ERR(PM_ERR_PKG_CANT_FRESH, -1);
		}
		_pacman_log(PM_LOG_DEBUG, _("promote target '%s' from upgrade to install"), transpkg_name);
		transpkg->type = PM_TRANS_TYPE_ADD;
	}

	if (transpkg->type == PM_TRANS_TYPE_UPGRADE) {
		int cmp = _pacman_versioncmp(transpkg->pkg_local->version, transpkg->pkg_new->version);

		if (trans->flags & PM_TRANS_FLAG_FRESHEN &&
				cmp >= 0) {
			/* only upgrade/install this package if it is at a lesser version */
			RET_ERR(PM_ERR_PKG_CANT_FRESH, NULL);
		}
		if(cmp > 0) {
			/* local version is newer -- get confirmation before adding */
			int resp = 0;
			QUESTION(trans, PM_TRANS_CONV_LOCAL_NEWER, transpkg->pkg_local, NULL, NULL, &resp);
			if(!resp) {
				_pacman_log(PM_LOG_WARNING, _("%s-%s: local version is newer -- skipping"), transpkg_name, transpkg->pkg_local->version);
				return NULL;
			}
		} else if(cmp == 0) {
			/* versions are identical -- get confirmation before adding */
			int resp = 0;
			QUESTION(trans, PM_TRANS_CONV_LOCAL_UPTODATE, transpkg->pkg_local, NULL, NULL, &resp);
			if(!resp) {
				_pacman_log(PM_LOG_WARNING, _("%s-%s is up to date -- skipping"), transpkg_name, transpkg->pkg_local->version);
				return NULL;
			}
		}
	}

	/* ignore holdpkgs on upgrade */
	if(transpkg->type == PM_TRANS_TYPE_REMOVE &&
			f_stringlist_find (handle->holdpkg, transpkg_name) != NULL) {
		int resp = 0;
		QUESTION(trans, PM_TRANS_CONV_REMOVE_HOLDPKG, transpkg->pkg_local, NULL, NULL, &resp);
		if(!resp) {
			RET_ERR(PM_ERR_PKG_HOLD, NULL);
		}
	}

	_pacman_log(PM_LOG_FLOW2, _("adding target '%s' to the transaction set"), transpkg_name);
	f_graph_add_vertex (&trans->transpkg_graph, &transpkg->as_graphvertex);
	trans->packages = _pacman_list_add(trans->packages, transpkg);

	return transpkg;

error:
	__pacman_trans_pkg_delete (transpkg);
	return NULL;
}

pmtranspkg_t *_pacman_trans_add_pkg (pmtrans_t *trans, pmpkg_t *pkg, pmtranstype_t type, unsigned int flags) {
	pmtranspkg_t *transpkg = __pacman_trans_pkg_new (trans);
	
	/* Sanity checks */
	ASSERT(transpkg != NULL, return NULL); /* pm_errno is allready set by __pacman_trans_pkg_new */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, NULL));
	ASSERT(pkg != NULL, RET_ERR(PM_ERR_TRANS_NULL, NULL));

	transpkg->type = type;
	if (type & PM_TRANS_TYPE_ADD) {
		/* FIXME: Ensure pkg is not from local */
		transpkg->pkg_new = pkg;
	} else {
		/* FIXME: Ensure pkg is from local */
		transpkg->pkg_local = pkg;
	}
	transpkg->flags = flags;

	return _pacman_trans_add (trans, transpkg);
}

pmtranspkg_t *_pacman_trans_add_target(pmtrans_t *trans, const char *target, pmtranstype_t type, unsigned int flags)
{
	pmtranspkg_t *transpkg;
	char *pkg_name;
	pmpkg_t *pkg = NULL;
	pmdb_t *db_local;

	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, NULL));
	ASSERT(target != NULL && strlen(target) != 0, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	db_local = trans->handle->db_local;
	pkg_name = target;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));

	if(f_stringlist_find (trans->targets, target)) {
		RET_ERR(PM_ERR_TRANS_DUP_TARGET, NULL);
	}

	if (type & PM_TRANS_TYPE_ADD) {
		struct stat buf;

		/* Check if we need to add a fake target to the transaction. */
		if(strchr(pkg_name, '|')) {
			pkg = fakepkg_create(pkg_name);
			if (pkg == NULL) {
				return NULL;
			}
		}

		if (stat(pkg_name, &buf) == 0) {
			_pacman_log(PM_LOG_FLOW2, _("loading target '%s'"), pkg_name);
			pkg = _pacman_pkg_load(pkg_name);
			if(pkg == NULL) {
				/* pm_errno is already set by pkg_load() */
				return NULL;
			}
		}

		if (pkg == NULL) {
			char targline[PKG_FULLNAME_LEN];
			char *targ;
			pmlist_t *dbs_search = NULL;

			STRNCPY(targline, target, PKG_FULLNAME_LEN);
			targ = strchr(targline, '/');
			if(targ) {
				pmdb_t *dbs;
				*targ = '\0';
				targ++;
				dbs = _pacman_handle_get_db_sync (trans->handle, targline);
				if (dbs != NULL) {
					/* FIXME: the list is leaked in this case */
					dbs_search = _pacman_list_add (dbs_search, dbs);
					if(dbs_search == NULL) {
						return NULL;
					}
				}
			} else {
				targ = targline;
				dbs_search = trans->handle->dbs_sync;
			}
			pkg = _pacman_dblist_get_pkg (dbs_search, targ, flags);
		}

		if (pkg == NULL) {
			RET_ERR(PM_ERR_PKG_NOT_FOUND, NULL);
		}
		goto out;
	}

	if (type & PM_TRANS_TYPE_REMOVE) {
		if (__pacman_trans_get_trans_pkg (trans, pkg_name) != NULL) {
			RET_ERR(PM_ERR_TRANS_DUP_TARGET, NULL);
		}

		if((pkg = _pacman_db_scan(db_local, pkg_name, INFRQ_ALL)) == NULL) {
			_pacman_log(PM_LOG_ERROR, _("could not find %s in database"), pkg_name);
			RET_ERR(PM_ERR_PKG_NOT_FOUND, NULL);
		}
	}

out:
	transpkg = _pacman_trans_add_pkg (trans, pkg, type, flags);
	if (transpkg != NULL) {
		trans->targets = f_stringlist_add(trans->targets, target);
	}
	return transpkg;
}

int _pacman_trans_addtarget (pmtrans_t *trans, const char *target, pmtranstype_t type, unsigned int flags) {
	return _pacman_trans_add_target (trans, target, type, flags) != NULL ? 0 : -1;
}

static
void _pacman_trans_remove_target (pmtrans_t *trans, const char *target) {
	FPtrListItem *item = f_ptrlist_detect (trans->packages, (FDetectFunc)__pacman_transpkg_detect_name, (void *)target);

	if (item != f_ptrlist_end (trans->packages)) {
		_pacman_log(PM_LOG_FLOW2, _("removing '%s' from target list"), target);
		f_ptrlistitem_delete (item, (FVisitorFunc)__pacman_trans_pkg_delete, NULL, &trans->packages);
	}
}

int _pacman_trans_set_state(pmtrans_t *trans, int new_state)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	/* Ignore unchanged state */
	if (trans->state == new_state) {
		return(0);
	}

	/* FIXME: Call triggers from here */
	trans->state = new_state;

	return 0;
}

static
int pkg_cmp(const void *p1, const void *p2)
{
	return(strcmp(((pmpkg_t *)p1)->name, ((pmsyncpkg_t *)p2)->pkg_new->name));
}

static
int check_olddelay(void)
{
	pmlist_t *i;
	char lastupdate[16] = "";
	struct tm tm;

	if(!handle->olddelay) {
		return(0);
	}

	memset(&tm,0,sizeof(struct tm));

	f_foreach (i, handle->dbs_sync) {
		pmdb_t *db = i->data;
		if(_pacman_db_getlastupdate(db, lastupdate) == -1) {
			continue;
		}
		if(strptime(lastupdate, "%Y%m%d%H%M%S", &tm) == NULL) {
			continue;
		}
		if((time(NULL)-mktime(&tm)) > handle->olddelay) {
			_pacman_log(PM_LOG_WARNING, _("local copy of '%s' repo is too old"), db->treename);
		}
	}
	return(0);
}

static
int _pacman_sync_prepare (pmtrans_t *trans, pmlist_t **data)
{
	pmlist_t *deps = NULL;
	pmlist_t *asked = NULL;
	pmlist_t *i, *j, *k;
	int ret = 0;
	pmdb_t *db_local = trans->handle->db_local;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	if(data) {
		*data = NULL;
	}

	if(!(trans->flags & PM_TRANS_FLAG_NODEPS)) {
		/* Resolve targets dependencies */
		EVENT(trans, PM_TRANS_EVT_RESOLVEDEPS_START, NULL, NULL);
		if (_pacman_trans_resolvedeps (trans, data) == -1) {
			/* pm_errno is set by resolvedeps */
			ret = -1;
			goto cleanup;
		}

		_pacman_sortbydeps (trans);

		EVENT(trans, PM_TRANS_EVT_RESOLVEDEPS_DONE, NULL, NULL);

		_pacman_log(PM_LOG_FLOW1, _("looking for unresolvable dependencies"));
		_pacman_trans_checkdeps (trans, &deps);
		if(deps) {
			if(data) {
				*data = deps;
				deps = NULL;
			}
			pm_errno = PM_ERR_UNSATISFIED_DEPS;
			ret = -1;
			goto cleanup;
		}
	}

	if(!(trans->flags & PM_TRANS_FLAG_NOCONFLICTS)) {
		/* check for inter-conflicts and whatnot */
		EVENT(trans, PM_TRANS_EVT_INTERCONFLICTS_START, NULL, NULL);

		_pacman_log(PM_LOG_FLOW1, _("looking for conflicts"));
		deps = _pacman_checkconflicts(trans);
		if(deps) {
			int errorout = 0;

			for(i = deps; i && !errorout; i = i->next) {
				pmdepmissing_t *miss = i->data;
				int found = 0;
				pmsyncpkg_t *ps;
				pmpkg_t *local;

				_pacman_log(PM_LOG_FLOW2, _("package '%s' is conflicting with '%s'"),
				          miss->target, miss->depend.name);

				/* check if the conflicting package is one that's about to be removed/replaced.
				 * if so, then just ignore it
				 */
				f_foreach (j, trans->packages) {
					ps = j->data;
					if(_pacman_pkg_isin(miss->depend.name, ps->replaces)) {
						found = 1;
						break;
					}
				}
				if(found) {
					_pacman_log(PM_LOG_DEBUG, _("'%s' is already elected for removal -- skipping"),
							miss->depend.name);
					continue;
				}

				ps = __pacman_trans_get_trans_pkg(trans, miss->target);
				if(ps == NULL) {
					_pacman_log(PM_LOG_DEBUG, _("'%s' not found in transaction set -- skipping"),
					          miss->target);
					continue;
				}
				local = _pacman_db_get_pkgfromcache(db_local, miss->depend.name);
				/* check if this package also "provides" the package it's conflicting with
				 */
				if(f_stringlist_find (ps->pkg_new->provides, miss->depend.name)) {
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
						target = f_stringlist_find (trans->targets, miss->target);
						depend = f_stringlist_find (trans->targets, miss->depend.name);
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
							_pacman_trans_remove_target (trans, rmpkg);
							continue;
						}
					}
				}
				/* It's a conflict -- see if they want to remove it
				*/
				_pacman_log(PM_LOG_DEBUG, _("resolving package '%s' conflict"), miss->target);
				if(local) {
					int doremove = 0;
					if(!f_stringlist_find (asked, miss->depend.name)) {
						QUESTION(trans, PM_TRANS_CONV_CONFLICT_PKG, miss->target, miss->depend.name, NULL, &doremove);
						asked = f_stringlist_append (asked, miss->depend.name);
						if(doremove) {
							pmpkg_t *q = _pacman_pkg_new(miss->depend.name, NULL);
							if(q == NULL) {
								if(data) {
									FREELIST(*data);
								}
								ret = -1;
								goto cleanup;
							}
							q->requiredby = f_stringlist_deep_copy(local->requiredby);
							if(ps->type != PM_TRANS_TYPE_ADD) {
								/* switch this sync type to REPLACE */
								ps->type = PM_TRANS_TYPE_ADD;
								//FREEPKG(ps->data);
							}
							/* append to the replaces list */
							_pacman_log(PM_LOG_FLOW2, _("electing '%s' for removal"), miss->depend.name);
							ps->replaces = _pacman_list_add(ps->replaces, q);
							_pacman_trans_remove_target (trans, miss->depend.name);
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
		EVENT(trans, PM_TRANS_EVT_INTERCONFLICTS_DONE, NULL, NULL);
	}

	/* XXX: this fails for cases where a requested package wants
	 *      a dependency that conflicts with an older version of
	 *      the package.  It will be removed from final, and the user
	 *      has to re-request it to get it installed properly.
	 *
	 *      Not gonna happen very often, but should be dealt with...
	 */

	if(!(trans->flags & PM_TRANS_FLAG_NODEPS)) {
		/* Check dependencies of packages in rmtargs and make sure
		 * we won't be breaking anything by removing them.
		 * If a broken dep is detected, make sure it's not from a
		 * package that's in our final (upgrade) list.
		 */
		/*EVENT(trans, PM_TRANS_EVT_CHECKDEPS_DONE, NULL, NULL);*/
		if (trans->packages) {
			_pacman_log(PM_LOG_FLOW1, _("checking dependencies of packages designated for removal"));
			_pacman_trans_checkdeps (trans, &deps);
			if(deps) {
				int errorout = 0;
				f_foreach (i, deps) {
					pmdepmissing_t *miss = i->data;
					if(!__pacman_trans_get_trans_pkg(trans, miss->depend.name)) {
						int pfound = 0;
						/* If miss->depend.name depends on something that miss->target and a
						 * package in final both provide, then it's okay...  */
						pmpkg_t *leavingp  = _pacman_db_get_pkgfromcache(db_local, miss->target);
						pmpkg_t *conflictp = _pacman_db_get_pkgfromcache(db_local, miss->depend.name);
						if(!leavingp || !conflictp) {
							_pacman_log(PM_LOG_ERROR, _("something has gone horribly wrong"));
							ret = -1;
							goto cleanup;
						}
						/* Look through the upset package's dependencies and try to match one up
						 * to a provisio from the package we want to remove */
						for(k = conflictp->depends; k && !pfound; k = k->next) {
							pmlist_t *m;
							for(m = leavingp->provides; m && !pfound; m = m->next) {
								if(!strcmp(k->data, m->data)) {
									/* Found a match -- now look through final for a package that
									 * provides the same thing.  If none are found, then it truly
									 * is an unresolvable conflict. */
									pmlist_t *n, *o;
									for(n = trans->packages; n && !pfound; n = n->next) {
										pmsyncpkg_t *sp = n->data;
										f_foreach (o, sp->pkg_new->provides) {
											if(!strcmp(m->data, o->data)) {
												/* found matching provisio -- we're good to go */
												_pacman_log(PM_LOG_FLOW2, _("found '%s' as a provision for '%s' -- conflict aborted"),
														sp->pkg_new->name, (char *)o->data);
												pfound = 1;
												break;
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
		/*EVENT(trans, PM_TRANS_EVT_CHECKDEPS_DONE, NULL, NULL);*/
	}

#ifndef __sun__
	/* check for free space only in case the packages will be extracted */
	if(!(trans->flags & PM_TRANS_FLAG_NOCONFLICTS)) {
		if(_pacman_check_freespace(trans, data) == -1) {
				/* pm_errno is set by check_freespace */
				ret = -1;
				goto cleanup;
		}
	}
#endif

	/* issue warning if the local db is too old */
	check_olddelay();

cleanup:
	FREELIST(asked);

	return(ret);
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
	if (trans->packages == NULL) {
		return(0);
	}

	if(trans->type == PM_TRANS_TYPE_SYNC) {
		if(_pacman_sync_prepare (trans, data) == -1) {
			/* pm_errno is set by trans->ops->prepare() */
			return(-1);
		}
	} else {

	pmlist_t *lp = NULL;
	pmlist_t *rmlist = NULL;
	char rm_fname[PATH_MAX];
	pmpkg_t *info = NULL;

	if (trans->type & PM_TRANS_TYPE_ADD) {
		/* Check dependencies */
		if(!(trans->flags & PM_TRANS_FLAG_NODEPS)) {
			EVENT(trans, PM_TRANS_EVT_CHECKDEPS_START, NULL, NULL);
			/* look for unsatisfied dependencies */
			_pacman_log(PM_LOG_FLOW1, _("looking for unsatisfied dependencies"));
			_pacman_trans_checkdeps (trans, &lp);
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
			lp = _pacman_checkconflicts(trans);
			if(lp != NULL) {
				if(data) {
					*data = lp;
				} else {
					FREELIST(lp);
				}
				RET_ERR(PM_ERR_CONFLICTING_DEPS, -1);
			}

			_pacman_sortbydeps (trans);
			EVENT(trans, PM_TRANS_EVT_CHECKDEPS_DONE, NULL, NULL);
		}

		/* Cleaning up */
		EVENT(trans, PM_TRANS_EVT_CLEANUP_START, NULL, NULL);
		_pacman_log(PM_LOG_FLOW1, _("cleaning up"));
		f_foreach (lp, trans->packages) {
			pmtranspkg_t *transpkg = lp->data;
			info = transpkg->pkg_new;
			if (info == NULL) {
				continue;
			}
			f_foreach (rmlist, info->removes) {
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
			lp = _pacman_db_find_conflicts(trans, &skiplist);
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
		_pacman_trans_checkdeps (trans, &lp);
		if(lp != NULL) {
			if(trans->flags & PM_TRANS_FLAG_CASCADE) {
				while(lp) {
					pmlist_t *i;
					f_foreach (i, lp) {
						pmdepmissing_t *miss = (pmdepmissing_t *)i->data;
						pmpkg_t *info = _pacman_db_scan(db_local, miss->depend.name, INFRQ_ALL);
						if(info) {
							_pacman_trans_add_pkg (trans, info, PM_TRANS_TYPE_UPGRADE, 0);
						} else {
							_pacman_log(PM_LOG_ERROR, _("could not find %s in database -- skipping"),
								miss->depend.name);
						}
					}
					FREELIST(lp);
					_pacman_trans_checkdeps (trans, &lp);
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
			_pacman_removedeps(trans);
		}

		_pacman_sortbydeps (trans);

		EVENT(trans, PM_TRANS_EVT_CHECKDEPS_DONE, NULL, NULL);
	}

	}

	_pacman_trans_set_state(trans, STATE_PREPARED);

	return(0);
}

static
struct trans_event {
	int event;
	const char *hook;
};

static const
struct trans_event_table_item {
		struct trans_event pre, post;	
} trans_event_table[4] = {
	[PM_TRANS_TYPE_ADD] = {
		.pre =	{ PM_TRANS_EVT_ADD_START, "pre_install" },
		.post =	{ PM_TRANS_EVT_ADD_DONE,  "post_install" }
	},
	[PM_TRANS_TYPE_REMOVE] = {
		.pre =  { PM_TRANS_EVT_REMOVE_START, "pre_remove" },
		.post = { PM_TRANS_EVT_REMOVE_DONE,  "post_remove" }
	},
	[PM_TRANS_TYPE_UPGRADE] = {
		.pre =  { PM_TRANS_EVT_UPGRADE_START, "pre_upgrade" },
		.post = { PM_TRANS_EVT_UPGRADE_DONE,  "post_upgrade" }
	}
};

int _pacman_localdb_remove_files (pmdb_t *localdb, pmpkg_t *info, pmtrans_t *trans) {
	pmlist_t *lp;
	int position = 0;
	struct stat buf;
	char line[PATH_MAX+1];

		if(!(trans->flags & PM_TRANS_FLAG_DBONLY)) {
			int filenum = f_ptrlist_count (info->files);
			_pacman_log(PM_LOG_FLOW1, _("removing files"));

			/* iterate through the list backwards, unlinking files */
			for(lp = f_ptrlist_last(info->files); lp; lp = lp->prev) {
				int nb = 0;
				double percent;
				const char *file = lp->data;
				char *hash_orig = _pacman_pkg_needbackup (info, file);

				if (position != 0) {
				percent = (double)position / filenum;
				}
				if (hash_orig != NULL) {
					nb = 1;
					FREE(hash_orig);
				}
				if(!nb && trans->type == PM_TRANS_TYPE_UPGRADE) {
					/* check noupgrade */
					if(f_stringlist_find (handle->noupgrade, file)) {
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
					if (f_stringlist_find (trans->skiplist, file) != NULL) {
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
									pacman_logaction(_("%s saved as %s"), line, newpath);
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
//							PROGRESS(trans, PM_TRANS_PROGRESS_REMOVE_START, info->name, (int)(percent * 100), howmany, howmany - remain + 1);
							position++;
							if(unlink(line)) {
								_pacman_log(PM_LOG_ERROR, _("cannot remove file %s"), file);
							}
						}
					}
				}
			}
//			PROGRESS(trans, PM_TRANS_PROGRESS_REMOVE_START, info->name, 100, howmany, howmany - remain + 1);
		}
}

int _pacman_remove_commit(pmtrans_t *trans, pmlist_t **data)
{
	pmpkg_t *info;
	pmlist_t *targ, *lp;
	int howmany, remain;
	pmdb_t *db = trans->handle->db_local;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	howmany = f_ptrlist_count (trans->packages);

	f_foreach (targ, trans->packages) {
		char pm_install[PATH_MAX];
		info = ((pmtranspkg_t*)targ->data)->pkg_local;

		if(handle->trans->state == STATE_INTERRUPTED) {
			break;
		}

		remain = f_ptrlist_count (targ);

		if(trans->type != PM_TRANS_TYPE_UPGRADE) {
			EVENT(trans, PM_TRANS_EVT_REMOVE_START, info, NULL);
			_pacman_log(PM_LOG_FLOW1, _("removing package %s-%s"), info->name, info->version);

			/* run the pre-remove scriptlet if it exists */
			if(info->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
				snprintf(pm_install, PATH_MAX, "%s/%s-%s/install", db->path, info->name, info->version);
				_pacman_runscriptlet(handle->root, pm_install, "pre_remove", info->version, NULL, trans);
			}
		}

		_pacman_localdb_remove_files (db, info, trans);

		if(trans->type != PM_TRANS_TYPE_UPGRADE) {
			/* run the post-remove script if it exists */
			if(info->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
				/* must run ldconfig here because some scriptlets fail due to missing libs otherwise */
				_pacman_ldconfig(handle->root);
				snprintf(pm_install, PATH_MAX, "%s/%s-%s/install", db->path, info->name, info->version);
				_pacman_runscriptlet(handle->root, pm_install, "post_remove", info->version, NULL, trans);
			}
		}

		/* remove the package from the database */
		_pacman_log(PM_LOG_FLOW1, _("updating database"));
		_pacman_log(PM_LOG_FLOW2, _("removing database entry '%s'"), info->name);
		if(_pacman_db_remove(db, info) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not remove database entry %s-%s"), info->name, info->version);
		}
		if(_pacman_db_remove_pkgfromcache(db, info) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not remove entry '%s' from cache"), info->name);
		}

		/* update dependency packages' REQUIREDBY fields */
		_pacman_log(PM_LOG_FLOW2, _("updating dependency packages 'requiredby' fields"));
		f_foreach (lp, info->depends) {
			pmpkg_t *depinfo = NULL;
			pmdepend_t depend;
			char *data;
			if(_pacman_splitdep((char*)lp->data, &depend)) {
				continue;
			}
			/* if this dependency is in the transaction targets, no need to update
			 * its requiredby info: it is in the process of being removed (if not
			 * already done!)
			 */
			if(__pacman_trans_get_trans_pkg(trans, depend.name) != NULL) {
				continue;
			}
			depinfo = _pacman_db_get_pkgfromcache(db, depend.name);
			if(depinfo == NULL) {
				/* look for a provides package */
				pmlist_t *provides = _pacman_db_whatprovides(db, depend.name);
				if(provides) {
					/* TODO: should check _all_ packages listed in provides, not just
					 *			 the first one.
					 */
					/* use the first one */
					depinfo = _pacman_db_get_pkgfromcache(db, ((pmpkg_t *)provides->data)->name);
					FREELISTPTR(provides);
				}
				if(depinfo == NULL) {
					_pacman_log(PM_LOG_ERROR, _("could not find dependency '%s'"), depend.name);
					/* wtf */
					continue;
				}
			}
			/* splice out this entry from requiredby */
			depinfo->requiredby = f_stringlist_remove_all (_pacman_pkg_getinfo(depinfo, PM_PKG_REQUIREDBY), info->name);
			_pacman_log(PM_LOG_DEBUG, _("updating 'requiredby' field for package '%s'"), depinfo->name);
			if(_pacman_db_write(db, depinfo, INFRQ_DEPENDS)) {
				_pacman_log(PM_LOG_ERROR, _("could not update 'requiredby' database entry %s-%s"),
					depinfo->name, depinfo->version);
			}
		}

		if(trans->type != PM_TRANS_TYPE_UPGRADE) {
			EVENT(trans, PM_TRANS_EVT_REMOVE_DONE, info, NULL);
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
	pmdb_t *db_local;
	int ret = 0;

	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	if(data!=NULL)
		*data = NULL;

	db_local = trans->handle->db_local;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	/* If there's nothing to do, return without complaining */
	if (trans->packages == NULL) {
		return(0);
	}

	_pacman_trans_set_state(trans, STATE_COMMITING);

	if (trans->type == PM_TRANS_TYPE_REMOVE) {
		if (_pacman_remove_commit (trans, data) == -1) {
			_pacman_trans_set_state(trans, STATE_PREPARED);
			return(-1);
		}
	} else if (trans->ops != NULL && trans->ops->commit != NULL) {
		if (trans->ops->commit(trans, data) == -1) {
			/* pm_errno is set by trans->ops->commit() */
			_pacman_trans_set_state(trans, STATE_PREPARED);
			return(-1);
		}
	} else {
	int i, errors = 0;
	int remain, archive_ret;
	double percent;
	register struct archive *archive;
	struct archive_entry *entry;
	char expath[PATH_MAX], cwd[PATH_MAX] = "";
	pmlist_t *targ, *lp;
	const int howmany = f_ptrlist_count (trans->packages);

	f_foreach (targ, trans->packages) {
		char pm_install[PATH_MAX];
		pmtranspkg_t *transpkg = targ->data;
		pmpkg_t *info = transpkg->pkg_new;
		pmpkg_t *local = transpkg->pkg_local;
		pmpkg_t *oldpkg = NULL;
		pmtranstype_t transtype = transpkg->type;
		errors = 0;
		remain = f_ptrlist_count (targ);
		struct trans_event_table_item *event;

		if(handle->trans->state == STATE_INTERRUPTED) {
			break;
		}

		/* see if this is an upgrade.  if so, remove the old package first */
		event = &trans_event_table[transtype];
		EVENT(trans, event->pre.event, info, NULL);
		if(info->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
			/* FIXME: check that it is really info->archive_path that whas expected */
			_pacman_runscriptlet (handle->root, info->archive_path, event->pre.hook, info->version,
					local ? local->version : NULL, trans);
		}

		if (transtype & PM_TRANS_TYPE_REMOVE) {
				_pacman_log(PM_LOG_FLOW1, _("removing package %s-%s"), info->name, info->version);

				/* we'll need to save some record for backup checks later */
				oldpkg = _pacman_pkg_new(local->name, local->version);
				if(oldpkg) {
					if(!(local->infolevel & INFRQ_FILES)) {
						_pacman_log(PM_LOG_DEBUG, _("loading FILES info for '%s'"), local->name);
						_pacman_db_read(db_local, INFRQ_FILES, local);
					}
					oldpkg->backup = f_stringlist_deep_copy(local->backup);
					strncpy(oldpkg->name, local->name, PKG_NAME_LEN);
					strncpy(oldpkg->version, local->version, PKG_VERSION_LEN);
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
					tr->skiplist = f_stringlist_deep_copy(trans->skiplist);
					if(_pacman_remove_commit(tr, NULL) == -1) {
						FREETRANS(tr);
						RET_ERR(PM_ERR_TRANS_ABORT, -1);
					}
					FREETRANS(tr);
				}
		}
		if(!(trans->flags & PM_TRANS_FLAG_DBONLY)) {
			_pacman_log(PM_LOG_FLOW1, _("extracting files"));

			if(transtype & PM_TRANS_TYPE_ADD) {
				_pacman_log(PM_LOG_FLOW1, _("adding package %s-%s"), info->name, info->version);

			/* Extract the package */
			if ((archive = archive_read_new ()) == NULL)
				RET_ERR(PM_ERR_LIBARCHIVE_ERROR, -1);

			archive_read_support_compression_all (archive);
			archive_read_support_format_all (archive);

			if (archive_read_open_file (archive, info->archive_path, PM_DEFAULT_BYTES_PER_BLOCK) != ARCHIVE_OK) {
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
				char *hash_orig = NULL;
				char pathname[PATH_MAX];
				struct stat buf;

				STRNCPY(pathname, archive_entry_pathname (entry), PATH_MAX);

				if (info->size != 0)
					percent = (double)archive_position_uncompressed(archive) / info->size;
				PROGRESS(trans, event->pre.event, info->name, (int)(percent * 100), howmany, (howmany - remain +1));

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
				if(f_stringlist_find (handle->noextract, pathname)) {
					pacman_logaction(_("notice: %s is in NoExtract -- skipping extraction"), pathname);
					archive_read_data_skip (archive);
					continue;
				}

				if(!lstat(expath, &buf)) {
					/* file already exists */
					if(S_ISLNK(buf.st_mode)) {
						continue;
					} else if(!S_ISDIR(buf.st_mode)) {
						if(f_stringlist_find (handle->noupgrade, pathname)) {
							notouch = 1;
						} else {
							if((transtype == PM_TRANS_TYPE_ADD)|| oldpkg == NULL) {
								nb = f_stringlist_find (info->backup, pathname);
							} else {
								/* op == PM_TRANS_TYPE_UPGRADE */
								hash_orig = _pacman_pkg_needbackup (oldpkg, pathname);
								if (hash_orig != NULL) {
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
						FREE(hash_orig);
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
					f_foreach (lp, info->backup) {
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
						if (hash_orig) {
							_pacman_log(PM_LOG_DEBUG, _("original: %s"), hash_orig);
						}
					} else {
						_pacman_log(PM_LOG_DEBUG, _("checking sha1 hashes for %s"), pathname);
						_pacman_log(PM_LOG_DEBUG, _("current:  %s"), sha1_local);
						_pacman_log(PM_LOG_DEBUG, _("new:      %s"), sha1_pkg);
						if (hash_orig) {
							_pacman_log(PM_LOG_DEBUG, _("original: %s"), hash_orig);
						}
					}

					if(transtype == PM_TRANS_TYPE_ADD) {
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
					} else if (hash_orig != NULL) {
						/* PM_UPGRADE */
						int installnew = 0;

						/* the fun part */
						if (!strcmp(hash_orig, md5_local)|| !strcmp(hash_orig, sha1_local)) {
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

					FREE(hash_orig);
					FREE(md5_local);
					FREE(md5_pkg);
					FREE(sha1_local);
					FREE(sha1_pkg);
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
					f_foreach (lp, info->backup) {
						char *fn;
						char path[PATH_MAX];
						char *file = lp->data;
						char *file_hash = NULL;
						size_t file_hash_length;

						if(!file) continue;
						if(!strcmp(file, pathname)) {
							_pacman_log(PM_LOG_DEBUG, _("appending backup entry"));
							snprintf(path, PATH_MAX, "%s%s", handle->root, file);
							if (info->sha1sum != NULL && info->sha1sum != '\0') {
								file_hash = _pacman_MDFile(path);
							} else {
								file_hash = _pacman_SHAFile(path);
							}
							file_hash_length = f_strlen (file_hash);
							/* + 2 : 1 for the terminating NULL, and 1 for the tab delimiter */
							if (file_hash_length <= 0 || 
									(fn = (char *)malloc(strlen(file) + file_hash_length + 2)) == NULL) {
								RET_ERR(PM_ERR_MEMORY, -1);
							}
							sprintf(fn, "%s\t%s", file, file_hash);
							FREE(file_hash);
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
					((transtype == PM_TRANS_TYPE_UPGRADE) ? _("upgrading") : _("installing")), info->name);
				pacman_logaction(_("errors occurred while %s %s"),
					((transtype == PM_TRANS_TYPE_UPGRADE) ? _("upgrading") : _("installing")), info->name);
			} else {
			PROGRESS(trans, event->pre.event, info->name, 100, howmany, howmany - remain + 1);
			}
			}
		}

		if(transtype & PM_TRANS_TYPE_ADD) {
		/* Add the package to the database */
			time_t t = time(NULL);

			_pacman_log(PM_LOG_FLOW1, _("adding package %s-%s to the database"), info->name, info->version);

		/* Update the requiredby field by scanning the whole database
		 * looking for packages depending on the package to add */
		for(lp = _pacman_db_get_pkgcache(db_local); lp; lp = lp->next) {
			pmpkg_t *tmpp = lp->data;
			pmlist_t *tmppm = NULL;
			if(tmpp == NULL) {
				continue;
			}
			f_foreach (tmppm, tmpp->depends) {
				pmdepend_t depend;
				if(_pacman_splitdep(tmppm->data, &depend)) {
					continue;
				}
				if(tmppm->data && (!strcmp(depend.name, info->name) || f_stringlist_find (info->provides, depend.name))) {
					_pacman_log(PM_LOG_DEBUG, _("adding '%s' in requiredby field for '%s'"), tmpp->name, info->name);
					info->requiredby = f_stringlist_add(info->requiredby, tmpp->name);
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
		f_foreach (lp, info->depends) {
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
			depinfo->requiredby = f_stringlist_add (_pacman_pkg_getinfo(depinfo, PM_PKG_REQUIREDBY), info->name);
			if(_pacman_db_write(db_local, depinfo, INFRQ_DEPENDS)) {
				_pacman_log(PM_LOG_ERROR, _("could not update 'requiredby' database entry %s-%s"),
				          depinfo->name, depinfo->version);
			}
		}
		}
		EVENT(trans, PM_TRANS_EVT_EXTRACT_DONE, NULL, NULL);

		/* run the post-install script if it exists  */
		if(info->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
			/* must run ldconfig here because some scriptlets fail due to missing libs otherwise */
			_pacman_ldconfig(handle->root);
			snprintf(pm_install, PATH_MAX, "%s/%s-%s/install", db_local->path, info->name, info->version);
			_pacman_runscriptlet(handle->root, pm_install, event->post.hook, info->version, oldpkg ? oldpkg->version : NULL, trans);
		}

		EVENT(trans, event->post.event, info, oldpkg);

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
