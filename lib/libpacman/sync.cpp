/*
 *  sync.c
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

/* pacman-g2 */
#include "sync.h"

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
#include "pacman_p.h"
#include "handle.h"
#include "server.h"
#include "packages_transaction.h"

#include "hash/md5.h"
#include "hash/sha1.h"
#include "util/list.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "fstdlib.h"
#include "fstring.h"

#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h> /* PATH_MAX */
#include <stdio.h>
#include <time.h>

using namespace libpacman;

pmsyncpkg_t *_pacman_syncpkg_new(int type, Package *spkg, void *data)
{
	pmsyncpkg_t *ps;

	ASSERT(spkg != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
	ASSERT((ps = f_zalloc(sizeof(*ps))) != NULL, return NULL);

	ps->type = type;
	ps->pkg_name = spkg->name();
	ps->pkg = spkg;
	ps->data = data;
	ps->pkg_local = _pacman_db_get_pkgfromcache(handle->db_local, ps->pkg_name);

	return(ps);
}

int _pacman_syncpkg_delete(pmsyncpkg_t *ps)
{
	ASSERT(ps != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	if(ps->type == PM_SYNC_TYPE_REPLACE) {
		FREELISTPKGS(ps->data);
	} else {
		delete (Package *)ps->data;
	}
	free(ps);
	return 0;
}

/* Helper functions for _pacman_list_remove
 */
static
int _pacman_syncpkg_cmp(const void *s1, const void *s2)
{
	return(strcmp(((pmsyncpkg_t *)s1)->pkg->name(), ((pmsyncpkg_t *)s2)->pkg->name()));
}

int _pacman_sync_addtarget(pmtrans_t *trans, const char *name)
{
	char targline[PKG_FULLNAME_LEN];
	char *targ;
	pmlist_t *j;
	Package *pkg_local;
	Package *spkg = NULL;
	int cmp;
	Database *db_local = trans->handle->db_local;
	pmlist_t *dbs_sync = trans->handle->dbs_sync;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(name != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	STRNCPY(targline, name, PKG_FULLNAME_LEN);
	targ = strchr(targline, '/');
	if(targ) {
		*targ = '\0';
		targ++;
		for(j = dbs_sync; j && !spkg; j = j->next) {
			Database *dbs = j->data;
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
		for(j = dbs_sync; j && !spkg; j = j->next) {
			Database *dbs = j->data;
			spkg = _pacman_db_get_pkgfromcache(dbs, targ);
		}
		if(spkg == NULL) {
			/* Search provides */
			_pacman_log(PM_LOG_FLOW2, _("target '%s' not found -- looking for provisions"), targ);
			for(j = dbs_sync; j && !spkg; j = j->next) {
				Database *dbs = j->data;
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
			QUESTION(trans, PM_TRANS_CONV_LOCAL_NEWER, pkg_local, NULL, NULL, &resp);
			if(!resp) {
				_pacman_log(PM_LOG_WARNING, _("%s-%s: local version is newer -- skipping"), pkg_local->name(), pkg_local->version());
				return(0);
			}
		} else if(cmp == 0) {
			/* versions are identical -- get confirmation before adding */
			int resp = 0;
			QUESTION(trans, PM_TRANS_CONV_LOCAL_UPTODATE, pkg_local, NULL, NULL, &resp);
			if(!resp) {
				_pacman_log(PM_LOG_WARNING, _("%s-%s is up to date -- skipping"), pkg_local->name(), pkg_local->version());
				return(0);
			}
		}
	}

	/* add the package to the transaction */
	if(!_pacman_trans_find(trans, spkg->name())) {
		Package *dummy = NULL;
		pmsyncpkg_t *ps;

		if(pkg_local) {
			dummy = new Package(pkg_local->name(), pkg_local->version());
			if(dummy == NULL) {
				RET_ERR(PM_ERR_MEMORY, -1);
			}
		}
		ps = _pacman_syncpkg_new(PM_SYNC_TYPE_UPGRADE, spkg, dummy);
		if(ps == NULL) {
			delete dummy;
			RET_ERR(PM_ERR_MEMORY, -1);
		}
		_pacman_trans_add(trans, ps, 0);
	}

	return(0);
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

int _pacman_sync_prepare(pmtrans_t *trans, pmlist_t **data)
{
	pmlist_t *deps = NULL;
	pmlist_t *list = NULL; /* list allowing checkdeps usage with data from trans->packages */
	pmlist_t *trail = NULL; /* breadcrum list to avoid running into circles */
	pmlist_t *asked = NULL;
	pmlist_t *i, *j, *k, *l, *m;
	int ret = 0;
	Database *db_local = trans->handle->db_local;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->packages == NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	if(data) {
		*data = NULL;
	}

	for(i = trans->syncpkgs; i; i = i->next) {
		pmsyncpkg_t *ps = i->data;
		list = _pacman_list_add(list, ps->pkg);
	}

	if(!(trans->flags & PM_TRANS_FLAG_NODEPS)) {
		trail = _pacman_list_new();

		/* Resolve targets dependencies */
		EVENT(trans, PM_TRANS_EVT_RESOLVEDEPS_START, NULL, NULL);
		_pacman_log(PM_LOG_FLOW1, _("resolving targets dependencies"));
		for(i = trans->syncpkgs; i; i = i->next) {
			Package *spkg = ((pmsyncpkg_t *)i->data)->pkg;
			if(_pacman_resolvedeps(trans, spkg, list, trail, data) == -1) {
				/* pm_errno is set by resolvedeps */
				ret = -1;
				goto cleanup;
			}
		}

		for(i = list; i; i = i->next) {
			/* add the dependencies found by resolvedeps to the transaction set */
			Package *spkg = i->data;
			if(!_pacman_trans_find(trans, spkg->name())) {
				pmsyncpkg_t *ps = _pacman_syncpkg_new(PM_SYNC_TYPE_DEPEND, spkg, NULL);
				if(ps == NULL) {
					ret = -1;
					goto cleanup;
				}
				trans->syncpkgs = _pacman_list_add(trans->syncpkgs, ps);
				_pacman_log(PM_LOG_FLOW2, _("adding package %s-%s to the transaction targets"),
						spkg->name(), spkg->version());
			} else {
				/* remove the original targets from the list if requested */
				if((trans->flags & PM_TRANS_FLAG_DEPENDSONLY)) {
					/* they are just pointers so we don't have to free them */
					trans->syncpkgs = _pacman_list_remove(trans->syncpkgs, spkg, pkg_cmp, NULL);
				}
			}
		}

		/* re-order w.r.t. dependencies */
		k = l = NULL;
		for(i=trans->syncpkgs; i; i=i->next) {
			pmsyncpkg_t *s = (pmsyncpkg_t*)i->data;
			k = _pacman_list_add(k, s->pkg);
		}
		m = _pacman_sortbydeps(k, PM_TRANS_TYPE_ADD);
		for(i=m; i; i=i->next) {
			for(j=trans->syncpkgs; j; j=j->next) {
				pmsyncpkg_t *s = (pmsyncpkg_t*)j->data;
				if(s->pkg==i->data) {
					l = _pacman_list_add(l, s);
				}
			}
		}
		FREELISTPTR(k);
		FREELISTPTR(m);
		FREELISTPTR(trans->syncpkgs);
		trans->syncpkgs = l;

		EVENT(trans, PM_TRANS_EVT_RESOLVEDEPS_DONE, NULL, NULL);

		_pacman_log(PM_LOG_FLOW1, _("looking for unresolvable dependencies"));
		deps = _pacman_checkdeps(trans, PM_TRANS_TYPE_UPGRADE, list);
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

	if(!(trans->flags & PM_TRANS_FLAG_NOCONFLICTS)) {
		/* check for inter-conflicts and whatnot */
		EVENT(trans, PM_TRANS_EVT_INTERCONFLICTS_START, NULL, NULL);

		_pacman_log(PM_LOG_FLOW1, _("looking for conflicts"));
		deps = _pacman_checkconflicts(trans, list);
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
				for(j = trans->syncpkgs; j && !found; j = j->next) {
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

				ps = _pacman_trans_find(trans, miss->target);
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
						target = _pacman_list_is_strin(miss->target, trans->targets);
						depend = _pacman_list_is_strin(miss->depend.name, trans->targets);
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
							pmsyncpkg_t *rsync = _pacman_trans_find(trans, rmpkg);
							pmsyncpkg_t *spkg = NULL;

							_pacman_log(PM_LOG_FLOW2, _("removing '%s' from target list"), rmpkg);
							trans->syncpkgs = _pacman_list_remove(trans->syncpkgs, rsync, _pacman_syncpkg_cmp, (void **)&spkg);
							_pacman_syncpkg_delete(spkg);
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
						QUESTION(trans, PM_TRANS_CONV_CONFLICT_PKG, miss->target, miss->depend.name, NULL, &doremove);
						asked = _pacman_stringlist_append(asked, miss->depend.name);
						if(doremove) {
							pmsyncpkg_t *rsync = _pacman_trans_find(trans, miss->depend.name);
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
								trans->syncpkgs = _pacman_list_remove(trans->syncpkgs, rsync, _pacman_syncpkg_cmp, (void **)&spkg);
								_pacman_syncpkg_delete(spkg);
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
		EVENT(trans, PM_TRANS_EVT_INTERCONFLICTS_DONE, NULL, NULL);
	}

	FREELISTPTR(list);

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
		for(i = trans->syncpkgs; i; i = i->next) {
			pmsyncpkg_t *ps = i->data;
			if(ps->type == PM_SYNC_TYPE_REPLACE) {
				for(j = ps->data; j; j = j->next) {
					list = _pacman_list_add(list, j->data);
				}
			}
		}
		if(list) {
			_pacman_log(PM_LOG_FLOW1, _("checking dependencies of packages designated for removal"));
			deps = _pacman_checkdeps(trans, PM_TRANS_TYPE_REMOVE, list);
			if(deps) {
				int errorout = 0;
				for(i = deps; i; i = i->next) {
					pmdepmissing_t *miss = i->data;
					if(!_pacman_trans_find(trans, miss->depend.name)) {
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
									for(n = trans->syncpkgs; n && !pfound; n = n->next) {
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
	FREELISTPTR(list);
	FREELISTPTR(trail);
	FREELIST(asked);

	return(ret);
}

int _pacman_sync_commit(pmtrans_t *trans, pmlist_t **data)
{
	pmlist_t *i, *j;
	pmtrans_t *tr = NULL;
	int replaces = 0;
	Database *db_local = trans->handle->db_local;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	if(handle->sysupgrade) {
		_pacman_runhook("pre_sysupgrade", trans);
	}
	/* remove conflicting and to-be-replaced packages */
	tr = _pacman_trans_new();
	if(tr == NULL) {
		_pacman_log(PM_LOG_ERROR, _("could not create removal transaction"));
		pm_errno = PM_ERR_MEMORY;
		goto error;
	}

	if(_pacman_trans_init(tr, PM_TRANS_TYPE_REMOVE, PM_TRANS_FLAG_NODEPS, trans->cbs) == -1) {
		_pacman_log(PM_LOG_ERROR, _("could not initialize the removal transaction"));
		goto error;
	}

	for(i = trans->syncpkgs; i; i = i->next) {
		pmsyncpkg_t *ps = i->data;
		if(ps->type == PM_SYNC_TYPE_REPLACE) {
			for(j = ps->data; j; j = j->next) {
				Package *pkg = j->data;
				if(!_pacman_pkg_isin(pkg->name(), tr->packages)) {
					if(_pacman_trans_addtarget(tr, pkg->name()) == -1) {
						goto error;
					}
					replaces++;
				}
			}
		}
	}
	if(replaces) {
		_pacman_log(PM_LOG_FLOW1, _("removing conflicting and to-be-replaced packages"));
		if(_pacman_trans_prepare(tr, data) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not prepare removal transaction"));
			goto error;
		}
		/* we want the frontend to be aware of commit details */
		tr->cbs.event = trans->cbs.event;
		if(_pacman_trans_commit(tr, NULL) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not commit removal transaction"));
			goto error;
		}
	}
	FREETRANS(tr);

	/* install targets */
	_pacman_log(PM_LOG_FLOW1, _("installing packages"));
	tr = _pacman_trans_new();
	if(tr == NULL) {
		_pacman_log(PM_LOG_ERROR, _("could not create transaction"));
		pm_errno = PM_ERR_MEMORY;
		goto error;
	}
	if(_pacman_trans_init(tr, PM_TRANS_TYPE_UPGRADE, trans->flags | PM_TRANS_FLAG_NODEPS, trans->cbs) == -1) {
		_pacman_log(PM_LOG_ERROR, _("could not initialize transaction"));
		goto error;
	}
	for(i = trans->syncpkgs; i; i = i->next) {
		pmsyncpkg_t *ps = i->data;
		Package *spkg = ps->pkg;
		char str[PATH_MAX];
		snprintf(str, PATH_MAX, "%s%s/%s-%s-%s" PM_EXT_PKG, handle->root, handle->cachedir, spkg->name(), spkg->version(), spkg->arch);
		if(_pacman_trans_addtarget(tr, str) == -1) {
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
	if(_pacman_trans_prepare(tr, data) == -1) {
		_pacman_log(PM_LOG_ERROR, _("could not prepare transaction"));
		/* pm_errno is set by trans_prepare */
		goto error;
	}
	if(_pacman_trans_commit(tr, NULL) == -1) {
		_pacman_log(PM_LOG_ERROR, _("could not commit transaction"));
		goto error;
	}
	FREETRANS(tr);

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
	FREETRANS(tr);
	/* commiting failed, so this is still just a prepared transaction */
	return(-1);
}

int _pacman_trans_download_commit(pmtrans_t *trans, pmlist_t **data)
{
	pmlist_t *i, *j, *files = NULL;
	char ldir[PATH_MAX];
    int doremove, retval = 0, tries = 0;
	int varcache = 1;

	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

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

#if 0
const pmtrans_ops_t _pacman_sync_pmtrans_opts = {
	.addtarget = _pacman_sync_addtarget,
	.prepare = _pacman_sync_prepare,
	.commit = _pacman_sync_commit
};
#endif

const pmtrans_ops_t _pacman_sync_pmtrans_opts = {
	.addtarget = _pacman_sync_addtarget,
	.prepare = _pacman_sync_prepare,
	.commit = _pacman_trans_download_commit
};

/* vim: set ts=2 sw=2 noet: */
