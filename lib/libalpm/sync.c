/*
 *  sync.c
 * 
 *  Copyright (c) 2002-2005 by Judd Vinet <jvinet@zeroflux.org>
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
#ifdef CYGWIN
#include <limits.h> /* PATH_MAX */
#endif
#include <libtar.h>
#include <zlib.h>
/* pacman */
#include "log.h"
#include "util.h"
#include "error.h"
#include "list.h"
#include "package.h"
#include "db.h"
#include "cache.h"
#include "deps.h"
#include "trans.h"
#include "sync.h"
#include "rpmvercmp.h"
#include "handle.h"

extern pmhandle_t *handle;

pmsyncpkg_t *sync_new(int type, pmpkg_t *spkg, void *data)
{
	pmsyncpkg_t *sync;

	if((sync = (pmsyncpkg_t *)malloc(sizeof(pmsyncpkg_t))) == NULL) {
		return(NULL);
	}

	sync->type = type;
	sync->pkg = spkg;
	sync->data = data;
	
	return(sync);
}

void sync_free(pmsyncpkg_t *sync)
{
	if(sync == NULL) {
		return;
	}

	if(sync->type == PM_SYNC_TYPE_REPLACE) {
		FREELISTPKGS(sync->data);
	} else {
		FREEPKG(sync->data);
	}
	free(sync);
}

/* Test for existance of a package in a PMList* of pmsyncpkg_t*
 * If found, return a pointer to the respective pmsyncpkg_t*
 */
static pmsyncpkg_t* find_pkginsync(char *needle, PMList *haystack)
{
	PMList *i;
	pmsyncpkg_t *sync = NULL;
	int found = 0;

	for(i = haystack; i && !found; i = i->next) {
		sync = i->data;
		if(sync && !strcmp(sync->pkg->name, needle)) {
			found = 1;
		}
	}
	if(!found) {
		sync = NULL;
	}

	return(sync);
}

/* It returns a PMList of packages extracted from the given archive
 * (the archive must have been generated by gensync)
 */
PMList *sync_load_dbarchive(char *archive)
{
	PMList *lp = NULL;
	DIR *dir = NULL;
	TAR *tar = NULL;
	tartype_t gztype = {
		(openfunc_t)_alpm_gzopen_frontend,
		(closefunc_t)gzclose,
		(readfunc_t)gzread,
		(writefunc_t)gzwrite
	};

	if(tar_open(&tar, archive, &gztype, O_RDONLY, 0, TAR_GNU) == -1) {
		pm_errno = PM_ERR_NOT_A_FILE;
		goto error;
	}

	/* readdir tmp_dir */
	/* for each subdir, parse %s/desc and %s/depends */

	tar_close(tar);

	return(lp);

error:
	if(tar) {
		tar_close(tar);
	}
	if(dir) {
		closedir(dir);
	}
	return(NULL);
}

int sync_sysupgrade(pmtrans_t *trans, pmdb_t *db_local, PMList *dbs_sync)
{
	PMList *i, *j, *k;

	/* check for "recommended" package replacements */
	for(i = dbs_sync; i; i = i->next) {
		PMList *j;
		for(j = db_get_pkgcache(i->data); j; j = j->next) {
			pmpkg_t *spkg = j->data;
			for(k = spkg->replaces; k; k = k->next) {
				PMList *m;
				for(m = db_get_pkgcache(db_local); m; m = m->next) {
					pmpkg_t *lpkg = m->data;
					if(!strcmp(k->data, lpkg->name)) {
						if(pm_list_is_strin(lpkg->name, handle->ignorepkg)) {
							_alpm_log(PM_LOG_WARNING, "%s-%s: ignoring package upgrade (to be replaced by %s-%s)",
								lpkg->name, lpkg->version, spkg->name, spkg->version);
						} else {
							/* get confirmation for the replacement */
							int doreplace = 0;
							QUESTION(trans, PM_TRANS_CONV_REPLACE_PKG, lpkg, spkg, ((pmdb_t *)i->data)->treename, &doreplace);

							if(doreplace) {
								/* if confirmed, add this to the 'final' list, designating 'lpkg' as
								 * the package to replace.
								 */
								pmsyncpkg_t *sync;
								pmpkg_t *dummy = pkg_dummy(lpkg->name, NULL);
								if(dummy == NULL) {
									pm_errno = PM_ERR_MEMORY;
									goto error;
								}
								dummy->requiredby = _alpm_list_strdup(lpkg->requiredby);
								/* check if spkg->name is already in the packages list. */
								sync = find_pkginsync(spkg->name, trans->packages);
								if(sync) {
									/* found it -- just append to the replaces list */
									sync->data = pm_list_add(sync->data, dummy);
								} else {
									/* none found -- enter pkg into the final sync list */
									sync = sync_new(PM_SYNC_TYPE_REPLACE, spkg, NULL);
									if(sync == NULL) {
										FREEPKG(dummy);
										pm_errno = PM_ERR_MEMORY;
										goto error;
									}
									sync->data = pm_list_add(sync->data, dummy);
									trans->packages = pm_list_add(trans->packages, sync);
								}
								_alpm_log(PM_LOG_DEBUG, "%s-%s elected for upgrade (to be replaced by %s-%s)",
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
	for(i = db_get_pkgcache(db_local); i; i = i->next) {
		int cmp;
		pmpkg_t *local = i->data;
		pmpkg_t *spkg = NULL;
		pmsyncpkg_t *sync;

		for(j = dbs_sync; !spkg && j; j = j->next) {
			for(k = db_get_pkgcache(j->data); !spkg && k; k = k->next) {
				pmpkg_t *sp = k->data;
				if(!strcmp(local->name, sp->name)) {
					spkg = sp;
				}
			}
		}
		if(spkg == NULL) {
			/*_alpm_log(PM_LOG_ERROR, "%s: not found in sync db -- skipping.", local->name);*/
			continue;
		}

		/* compare versions and see if we need to upgrade */
		cmp = rpmvercmp(local->version, spkg->version);
		if(cmp > 0 && !spkg->force) {
			/* local version is newer */
			_alpm_log(PM_LOG_FLOW1, "%s-%s: local version is newer",
				local->name, local->version);
		} else if(cmp == 0) {
			/* versions are identical */
		} else if(pm_list_is_strin(i->data, handle->ignorepkg)) {
			/* package should be ignored (IgnorePkg) */
			_alpm_log(PM_LOG_FLOW1, "%s-%s: ignoring package upgrade (%s)",
				local->name, local->version, spkg->version);
		} else {
			pmpkg_t *dummy = pkg_new();
			STRNCPY(dummy->name, local->name, PKG_NAME_LEN);
			STRNCPY(dummy->version, local->version, PKG_VERSION_LEN);
			sync = sync_new(PM_SYNC_TYPE_UPGRADE, spkg, dummy);
			if(sync == NULL) {
				FREEPKG(dummy);
				pm_errno = PM_ERR_MEMORY;
				goto error;
			}
			_alpm_log(PM_LOG_DEBUG, "%s-%s elected for upgrade (%s => %s)",
				local->name, local->version, local->version, spkg->version);
			trans->packages = pm_list_add(trans->packages, sync);
		}
	}

	return(0);

error:
	return(-1);
}

int sync_addtarget(pmtrans_t *trans, pmdb_t *db_local, PMList *dbs_sync, char *name)
{
	char targline[PKG_FULLNAME_LEN];
	char *targ;
	PMList *j;
	pmpkg_t *local;
	pmpkg_t *spkg = NULL;
	pmsyncpkg_t *sync;
	int cmp;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(name != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	STRNCPY(targline, name, PKG_FULLNAME_LEN);
	targ = strchr(targline, '/');
	if(targ) {
		*targ = '\0';
		targ++;
		for(j = dbs_sync; j && !spkg; j = j->next) {
			pmdb_t *dbs = j->data;
			if(strcmp(dbs->treename, targline) == 0) {
				spkg = db_get_pkgfromcache(dbs, targ);
				if(spkg == NULL) {
					RET_ERR(PM_ERR_PKG_NOT_FOUND, -1);
				}
			}
		}
	} else {
		targ = targline;
		for(j = dbs_sync; j && !spkg; j = j->next) {
			pmdb_t *dbs = j->data;
			spkg = db_get_pkgfromcache(dbs, targ);
		}
	}
	if(spkg == NULL) {
		RET_ERR(PM_ERR_PKG_NOT_FOUND, -1);
	}

	local = db_get_pkgfromcache(db_local, name);
	if(local) {
		cmp = alpm_pkg_vercmp(local->version, spkg->version);
		if(cmp > 0) {
			/* local version is newer -- get confirmation before adding */
			int resp = 0;
			QUESTION(trans, PM_TRANS_CONV_LOCAL_NEWER, local, NULL, NULL, &resp);
			if(!resp) {
				_alpm_log(PM_LOG_WARNING, "%s-%s: local version is newer -- skipping", local->name, local->version);
				return(0);
			}
		} else if(cmp == 0) {
			/* versions are identical -- get confirmation before adding */
			int resp = 0;
			QUESTION(trans, PM_TRANS_CONV_LOCAL_UPTODATE, local, NULL, NULL, &resp);
			if(!resp) {
				_alpm_log(PM_LOG_WARNING, "%s-%s is up to date -- skipping", local->name, local->version);
				return(0);
			}
		}
	}

	/* add the package to the transaction */
	if(!find_pkginsync(spkg->name, trans->packages)) {
		pmpkg_t *dummy = NULL;
		if(local) {
			dummy = pkg_new();
			if(dummy == NULL) {
				RET_ERR(PM_ERR_MEMORY, -1);
			}
			STRNCPY(dummy->name, local->name, PKG_NAME_LEN);
			STRNCPY(dummy->version, local->version, PKG_VERSION_LEN);
		}
		sync = sync_new(PM_SYNC_TYPE_UPGRADE, spkg, dummy);
		if(sync == NULL) {
			FREEPKG(dummy);
			RET_ERR(PM_ERR_MEMORY, -1);
		}
		trans->packages = pm_list_add(trans->packages, sync);
	}

	return(0);
}

int sync_prepare(pmtrans_t *trans, pmdb_t *db_local, PMList *dbs_sync, PMList **data)
{
	PMList *deps = NULL;
	PMList *list = NULL;
	PMList *trail = NULL;
	PMList *i;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(data != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	*data = NULL;

	if(!(trans->flags & PM_TRANS_FLAG_NODEPS)) {
		for(i = trans->packages; i; i = i->next) {
			pmsyncpkg_t *sync = i->data;
			list = pm_list_add(list, sync->pkg);
		}
		trail = pm_list_new();

		/* Resolve targets dependencies */
		EVENT(trans, PM_TRANS_EVT_RESOLVEDEPS_START, NULL, NULL);
		_alpm_log(PM_LOG_FLOW1, "resolving targets dependencies");
		for(i = trans->packages; i; i = i->next) {
			pmsyncpkg_t *sync = i->data;
			pmpkg_t *spkg = sync->pkg;
			_alpm_log(PM_LOG_FLOW1, "resolving dependencies for package %s", spkg->name);
			if(resolvedeps(db_local, dbs_sync, spkg, list, trail, trans) == -1) {
				/* pm_errno is set by resolvedeps */
				goto error;
			}
		}
		for(i = list; i; i = i->next) {
			pmpkg_t *spkg = i->data;
			if(!find_pkginsync(spkg->name, trans->packages)) {
				pmsyncpkg_t *sync = sync_new(PM_SYNC_TYPE_DEPEND, spkg, NULL);
				trans->packages = pm_list_add(trans->packages, sync);
			}
		}
		EVENT(trans, PM_TRANS_EVT_RESOLVEDEPS_DONE, NULL, NULL);

		/* check for inter-conflicts and whatnot */
		EVENT(trans, PM_TRANS_EVT_INTERCONFLICTS_START, NULL, NULL);
		deps = checkdeps(db_local, PM_TRANS_TYPE_UPGRADE, list);
		if(deps) {
			int found;
			PMList *j, *k, *exfinal = NULL;
			int errorout = 0;
			_alpm_log(PM_LOG_FLOW1, "looking for unresolvable dependencies");
			for(i = deps; i; i = i->next) {
				pmdepmissing_t *miss = i->data;
				if(miss->type == PM_DEP_TYPE_DEPEND || miss->type == PM_DEP_TYPE_REQUIRED) {
					if(!errorout) {
						errorout = 1;
					}
					if((miss = (pmdepmissing_t *)malloc(sizeof(pmdepmissing_t))) == NULL) {
						FREELIST(deps);
						FREELIST(*data);
						RET_ERR(PM_ERR_MEMORY, -1);
					}
					*miss = *(pmdepmissing_t *)i->data;
					*data = pm_list_add(*data, miss);
				}
			}
			if(errorout) {
				FREELIST(deps);
				RET_ERR(PM_ERR_UNSATISFIED_DEPS, -1);
			}

			/* no unresolvable deps, so look for conflicts */
			errorout = 0;
			_alpm_log(PM_LOG_FLOW1, "looking for conflicts");
			for(i = deps; i && !errorout; i = i->next) {
				pmdepmissing_t *miss = i->data;
				if(miss->type != PM_DEP_TYPE_CONFLICT) {
					continue;
				}
				/* make sure this package wasn't already removed from the final list */
				if(pm_list_is_in(miss->target, exfinal)) {
					continue;
				}

				/* check if the conflicting package is one that's about to be removed/replaced.
				 * if so, then just ignore it
				 */
				found = 0;
				for(j = trans->packages; j && !found; j = j->next) {
					pmsyncpkg_t *sync = j->data;
					if(sync->type != PM_SYNC_TYPE_REPLACE) {
						continue;
					}
					for(k = sync->data; k && !found; k = k->next) {
						pmpkg_t *p = k->data;
						if(!strcmp(p->name, miss->depend.name)) {
							found = 1;
						}
					}
				}

				/* ORE
				if we didn't find it in any sync->replaces lists, then it's a conflict */
			}

			if(errorout) {
				FREELIST(deps);
				RET_ERR(PM_ERR_CONFLICTING_DEPS, -1);
			}
		}
		EVENT(trans, PM_TRANS_EVT_INTERCONFLICTS_DONE, NULL, NULL);

		FREELISTPTR(list);
		FREELISTPTR(trail);
	}

	/* ORE
	any packages in rmtargs need to be removed from final.
	rather than ripping out nodes from final, we just copy over
	our "good" nodes to a new list and reassign. */

	/* XXX: this fails for cases where a requested package wants
	 *      a dependency that conflicts with an older version of
	 *      the package.  It will be removed from final, and the user
	 *      has to re-request it to get it installed properly.
	 *
	 *      Not gonna happen very often, but should be dealt with...
	 */

	/* ORE
	Check dependencies of packages in rmtargs and make sure
	we won't be breaking anything by removing them.
	If a broken dep is detected, make sure it's not from a
	package that's in our final (upgrade) list. */
	/*_alpm_log(PM_LOG_DEBUG, "checking dependencies of packages designated for removal");*/

	return(0);

error:
	FREELISTPTR(list);
	FREELISTPTR(trail);
	FREELIST(deps);
	return(-1);
}

int sync_commit(pmtrans_t *trans, pmdb_t *db_local)
{
	PMList *i;
	PMList *data;
	pmtrans_t *tr = NULL;
	int replaces = 0;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	/* ORE
	remove any conflicting packages (WITHOUT dep checks) */

	/* remove to-be-replaced packages */
	tr = trans_new(PM_TRANS_TYPE_REMOVE, PM_TRANS_FLAG_NODEPS);
	/* ORE */
	if(tr == NULL) {
		_alpm_log(PM_LOG_ERROR, "could not create removal transaction");
		pm_errno = PM_ERR_XXX;
		goto error;
	}
	for(i = trans->packages; i; i = i->next) {
		pmsyncpkg_t *sync = i->data;
		if(sync->type == PM_SYNC_TYPE_REPLACE) {
			PMList *pkgs = sync->data;
			PMList *j;
			for(j = pkgs; j; j = j->next) {
				pmpkg_t *pkg = j->data;
				if(!pkg_isin(pkg, tr->packages)) {
					if(trans_addtarget(tr, pkg->name) == -1) {
						goto error;
					}
				}
			}
			replaces++;
		}
	}
	if(replaces) {
		_alpm_log(PM_LOG_FLOW1, "removing to-be-replaced packages");
		if(trans_prepare(tr, &data) == -1) {
			_alpm_log(PM_LOG_ERROR, "could not prepare removal transaction");
			pm_errno = PM_ERR_XXX;
			goto error;
		}
		/* we want the frontend to be aware of commit details */
		tr->cb_event = trans->cb_event;
		if(trans_commit(tr) == -1) {
			_alpm_log(PM_LOG_ERROR, "could not commit removal transaction");
			pm_errno = PM_ERR_XXX;
			goto error;
		}
	}
	FREETRANS(tr);

	/* install targets */
	_alpm_log(PM_LOG_FLOW1, "installing packages");
	tr = trans_new();
	if(tr == NULL) {
		_alpm_log(PM_LOG_ERROR, "could not create transaction");
		pm_errno = PM_ERR_XXX;
		goto error;
	}
	if(trans_init(tr, PM_TRANS_TYPE_UPGRADE, trans->flags | PM_TRANS_FLAG_NODEPS, NULL, NULL) == -1) {
		_alpm_log(PM_LOG_ERROR, "could not initialize transaction");
		pm_errno = PM_ERR_XXX;
		goto error;
	}
	for(i = trans->packages; i; i = i->next) {
		pmsyncpkg_t *sync = i->data;
		pmpkg_t *spkg = sync->pkg;
		char str[PATH_MAX];
		snprintf(str, PATH_MAX, "%s%s/%s-%s" PM_EXT_PKG, handle->root, handle->cachedir, spkg->name, spkg->version);
		if(trans_addtarget(tr, str) == -1) {
			goto error;
		}
		/* using list_last() is ok because addtarget() adds the new target at the
		 * end of the tr->packages list */
		spkg = pm_list_last(tr->packages)->data;
		if(sync->type == PM_SYNC_TYPE_DEPEND) {
			/* ORE
			 * if called from makepkg, reason should be set to PM_PKG_REASON_DEPEND */
			spkg->reason = PM_PKG_REASON_DEPEND;
		} else {
			spkg->reason = PM_PKG_REASON_EXPLICIT;
		}
	}
	if(trans_prepare(tr, &data) == -1) {
		_alpm_log(PM_LOG_ERROR, "could not prepare transaction");
		pm_errno = PM_ERR_XXX;
		goto error;
	}
	/* we want the frontend to be aware of commit details */
	tr->cb_event = trans->cb_event;
	if(trans_commit(tr) == -1) {
		_alpm_log(PM_LOG_ERROR, "could not commit transaction");
		pm_errno = PM_ERR_XXX;
		goto error;
	}
	FREETRANS(tr);

	/* propagate replaced packages' requiredby fields to their new owners */
	if(replaces) {
		_alpm_log(PM_LOG_FLOW1, "updating database for replaced packages dependencies");
		for(i = trans->packages; i; i = i->next) {
			pmsyncpkg_t *sync = i->data;
			if(sync->type == PM_SYNC_TYPE_REPLACE) {
				PMList *j;
				pmpkg_t *new = db_get_pkgfromcache(db_local, sync->pkg->name);
				for(j = sync->data; j; j = j->next) {
					PMList *k;
					pmpkg_t *old = j->data;
					/* merge lists */
					for(k = old->requiredby; k; k = k->next) {
						if(!pm_list_is_strin(k->data, new->requiredby)) {
							/* replace old's name with new's name in the requiredby's dependency list */
							PMList *m;
							pmpkg_t *depender = db_get_pkgfromcache(db_local, k->data);
							if(depender == NULL) {
								/* If the depending package no longer exists in the local db,
								 * then it must have ALSO conflicted with sync->pkg.  If
								 * that's the case, then we don't have anything to propagate
								 * here. */
								continue;
							}
							for(m = depender->depends; m; m = m->next) {
								if(!strcmp(m->data, old->name)) {
									FREE(m->data);
									m->data = strdup(new->name);
								}
							}
							if(db_write(db_local, depender, INFRQ_DEPENDS) == -1) {
								_alpm_log(PM_LOG_ERROR, "could not update 'requiredby' database entry %s/%s-%s", db_local->treename, new->name, new->version);
							}
							/* add the new requiredby */
							new->requiredby = pm_list_add(new->requiredby, strdup(k->data));
						}
					}
				}
				if(db_write(db_local, new, INFRQ_DEPENDS) == -1) {
					_alpm_log(PM_LOG_ERROR, "could not update new database entry %s/%s-%s", db_local->treename, new->name, new->version);
				}
			}
		}
	}

	return(0);

error:
	FREETRANS(tr);
	return(-1);
}

/* vim: set ts=2 sw=2 noet: */
