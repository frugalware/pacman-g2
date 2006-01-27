/*
 *  deps.c
 * 
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* pacman */
#include "util.h"
#include "log.h"
#include "error.h"
#include "list.h"
#include "package.h"
#include "db.h"
#include "cache.h"
#include "provide.h"
#include "deps.h"
#include "versioncmp.h"
#include "handle.h"

extern pmhandle_t *handle;

pmdepmissing_t *depmiss_new(const char *target, unsigned char type, unsigned char depmod,
                            const char *depname, const char *depversion)
{
	pmdepmissing_t *miss;

	miss = (pmdepmissing_t *)malloc(sizeof(pmdepmissing_t));
	if(miss == NULL) {
		return(NULL);
	}

	STRNCPY(miss->target, target, PKG_NAME_LEN);
	miss->type = type;
	miss->depend.mod = depmod;
	STRNCPY(miss->depend.name, depname, PKG_NAME_LEN);
	if(depversion) {
		STRNCPY(miss->depend.version, depversion, PKG_VERSION_LEN);
	} else {
		miss->depend.version[0] = 0;
	}

	return(miss);
}

int depmiss_isin(pmdepmissing_t *needle, PMList *haystack)
{
	PMList *i;

	for(i = haystack; i; i = i->next) {
		pmdepmissing_t *miss = i->data;
		if(!memcmp(needle, miss, sizeof(pmdepmissing_t))
		   && !memcmp(&needle->depend, &miss->depend, sizeof(pmdepend_t))) {
			return(1);
		}
	}

	return(0);
}

/* Re-order a list of target packages with respect to their dependencies.
 *
 * Example (PM_TRANS_TYPE_ADD):
 *   A depends on C
 *   B depends on A
 *   Target order is A,B,C,D
 *
 *   Should be re-ordered to C,A,B,D
 * 
 * mode should be either PM_TRANS_TYPE_ADD or PM_TRANS_TYPE_REMOVE.  This
 * affects the dependency order sortbydeps() will use.
 *
 * This function returns the new PMList* target list.
 *
 */ 
PMList *sortbydeps(PMList *targets, int mode)
{
	PMList *newtargs = NULL;
	PMList *i, *j, *k, *l;
	int change = 1;
	int numscans = 0;
	int numtargs = 0;

	if(targets == NULL) {
		return(NULL);
	}

	for(i = targets; i; i = i->next) {
		newtargs = pm_list_add(newtargs, i->data);
		numtargs++;
	}

	while(change) {
		PMList *tmptargs = NULL;
		change = 0;
		if(numscans > numtargs) {
			_alpm_log(PM_LOG_WARNING, "possible dependency cycle detected");
			continue;
		}
		numscans++;
		/* run thru targets, moving up packages as necessary */
		for(i = newtargs; i; i = i->next) {
			pmpkg_t *p = (pmpkg_t*)i->data;
			for(j = p->depends; j; j = j->next) {
				pmdepend_t dep;
				pmpkg_t *q = NULL;
				if(splitdep(j->data, &dep)) {
					continue;
				}
				/* look for dep.name -- if it's farther down in the list, then
				 * move it up above p
				 */
				for(k = i->next; k; k = k->next) {
					q = (pmpkg_t *)k->data;
					if(!strcmp(dep.name, q->name)) {
						if(!pkg_isin(q->name, tmptargs)) {
							change = 1;
							tmptargs = pm_list_add(tmptargs, q);
						}
						break;
					}
					for(l = q->provides; l; l = l->next) {
						if(!strcmp(dep.name, (char*)l->data)) {
							if(!pkg_isin((char*)l->data, tmptargs)) {
								change = 1;
								tmptargs = pm_list_add(tmptargs, q);
							}
							break;
						}
					}
				}
			}
			if(!pkg_isin(p->name, tmptargs)) {
				tmptargs = pm_list_add(tmptargs, p);
			}
		}
		FREELISTPTR(newtargs);
		newtargs = tmptargs;
	}

	if(mode == PM_TRANS_TYPE_REMOVE) {
		/* we're removing packages, so reverse the order */
		PMList *tmptargs = _alpm_list_reverse(newtargs);
		/* free the old one */
		FREELISTPTR(newtargs);
		newtargs = tmptargs;
	}

	return(newtargs);
}

/* Returns a PMList* of missing_t pointers.
 *
 * dependencies can include versions with depmod operators.
 *
 */
PMList *checkdeps(pmtrans_t *trans, pmdb_t *db, unsigned char op, PMList *packages)
{
	pmdepend_t depend;
	PMList *i, *j, *k;
	int cmp;
	int found = 0;
	PMList *baddeps = NULL;
	pmdepmissing_t *miss = NULL;

	if(db == NULL) {
		return(NULL);
	}

	if(op == PM_TRANS_TYPE_UPGRADE) {
		/* PM_TRANS_TYPE_UPGRADE handles the backwards dependencies, ie, the packages
		 * listed in the requiredby field.
		 */
		for(i = packages; i; i = i->next) {
			pmpkg_t *tp = i->data;
			pmpkg_t *oldpkg;
			if(tp == NULL) {
				continue;
			}

			if((oldpkg = db_get_pkgfromcache(db, tp->name)) == NULL) {
				continue;
			}
			for(j = oldpkg->requiredby; j; j = j->next) {
				char *ver;
				pmpkg_t *p;
				found = 0;
				if((p = db_get_pkgfromcache(db, j->data)) == NULL) {
					/* hmmm... package isn't installed.. */
					continue;
				}
				if(pkg_isin(p->name, packages)) {
					/* this package is also in the upgrade list, so don't worry about it */
					continue;
				}
				for(k = p->depends; k && !found; k = k->next) {
					/* find the dependency info in p->depends */
					splitdep(k->data, &depend);
					if(!strcmp(depend.name, oldpkg->name)) {
						found = 1;
					}
				}
				if(found == 0) {
					/* look for packages that list depend.name as a "provide" */
					PMList *provides = _alpm_db_whatprovides(db, depend.name);
					if(provides == NULL) {
						/* not found */
						continue;
					}
					/* we found an installed package that provides depend.name */
					FREELISTPTR(provides);
				}
				found = 0;
				if(depend.mod == PM_DEP_MOD_ANY) {
					found = 1;
				} else {
					/* note that we use the version from the NEW package in the check */
					ver = strdup(tp->version);
					if(!index(depend.version,'-')) {
						char *ptr;
						for(ptr = ver; *ptr != '-'; ptr++);
						*ptr = '\0';
					}
					cmp = versioncmp(ver, depend.version);
					switch(depend.mod) {
						case PM_DEP_MOD_EQ: found = (cmp == 0); break;
						case PM_DEP_MOD_GE: found = (cmp >= 0); break;
						case PM_DEP_MOD_LE: found = (cmp <= 0); break;
					}
					FREE(ver);
				}
				if(!found) {
					_alpm_log(PM_LOG_DEBUG, "checkdeps: found %s as required by %s", depend.name, p->name);
					miss = depmiss_new(p->name, PM_DEP_TYPE_REQUIRED, depend.mod, depend.name, depend.version);
					if(!depmiss_isin(miss, baddeps)) {
						baddeps = pm_list_add(baddeps, miss);
					} else {
						FREE(miss);
					}
				}
			}
		}
	}
	if(op == PM_TRANS_TYPE_ADD || op == PM_TRANS_TYPE_UPGRADE) {
		/* DEPENDENCIES -- look for unsatisfied dependencies */
		for(i = packages; i; i = i->next) {
			pmpkg_t *tp = i->data;
			if(tp == NULL) {
				continue;
			}

			for(j = tp->depends; j; j = j->next) {
				/* split into name/version pairs */
				splitdep((char *)j->data, &depend);
				found = 0;
				/* check database for literal packages */
				for(k = db_get_pkgcache(db); k && !found; k = k->next) {
					pmpkg_t *p = (pmpkg_t *)k->data;
					if(!strcmp(p->name, depend.name)) {
						if(depend.mod == PM_DEP_MOD_ANY) {
							/* accept any version */
							found = 1;
						} else {
							char *ver = strdup(p->version);
							/* check for a release in depend.version.  if it's
							 * missing remove it from p->version as well.
							 */
							if(!index(depend.version,'-')) {
								char *ptr;
								for(ptr = ver; *ptr != '-'; ptr++);
								*ptr = '\0';
							}
							cmp = versioncmp(ver, depend.version);
							switch(depend.mod) {
								case PM_DEP_MOD_EQ: found = (cmp == 0); break;
								case PM_DEP_MOD_GE: found = (cmp >= 0); break;
								case PM_DEP_MOD_LE: found = (cmp <= 0); break;
							}
							FREE(ver);
						}
					}
				}
				/* check other targets */
				for(k = packages; k && !found; k = k->next) {
					pmpkg_t *p = (pmpkg_t *)k->data;
					/* see if the package names match OR if p provides depend.name */
					if(!strcmp(p->name, depend.name) || pm_list_is_strin(depend.name, p->provides)) {
						if(depend.mod == PM_DEP_MOD_ANY) {
							/* accept any version */
							found = 1;
						} else {
							char *ver = strdup(p->version);
							/* check for a release in depend.version.  if it's
							 * missing remove it from p->version as well.
							 */
							if(!index(depend.version,'-')) {
								char *ptr;
								for(ptr = ver; *ptr != '-'; ptr++);
								*ptr = '\0';
							}
							cmp = versioncmp(ver, depend.version);
							switch(depend.mod) {
								case PM_DEP_MOD_EQ: found = (cmp == 0); break;
								case PM_DEP_MOD_GE: found = (cmp >= 0); break;
								case PM_DEP_MOD_LE: found = (cmp <= 0); break;
							}
							FREE(ver);
						}
					}
				}
				/* check database for provides matches */
				if(!found){
					k = _alpm_db_whatprovides(db, depend.name);
					if(k) {
						/* grab the first one (there should only really be one, anyway) */
						pmpkg_t *p = k->data;
						if(depend.mod == PM_DEP_MOD_ANY) {
							/* accept any version */
							found = 1;
						} else {
							char *ver = strdup(p->version);
							/* check for a release in depend.version.  if it's
							 * missing remove it from p->version as well.
							 */
							if(!index(depend.version,'-')) {
								char *ptr;
								for(ptr = ver; *ptr != '-'; ptr++);
								*ptr = '\0';
							}
							cmp = versioncmp(ver, depend.version);
							switch(depend.mod) {
								case PM_DEP_MOD_EQ: found = (cmp == 0); break;
								case PM_DEP_MOD_GE: found = (cmp >= 0); break;
								case PM_DEP_MOD_LE: found = (cmp <= 0); break;
							}
							FREE(ver);
						}
						FREELISTPTR(k);
					}
				}
				/* else if still not found... */
				if(!found) {
					_alpm_log(PM_LOG_DEBUG, "checkdeps: found %s as a dependency for %s",
					          depend.name, tp->name);
					miss = depmiss_new(tp->name, PM_DEP_TYPE_DEPEND, depend.mod, depend.name, depend.version);
					if(!depmiss_isin(miss, baddeps)) {
						baddeps = pm_list_add(baddeps, miss);
					} else {
						FREE(miss);
					}
				}
			}
		}
	} else if(op == PM_TRANS_TYPE_REMOVE) {
		/* check requiredby fields */
		for(i = packages; i; i = i->next) {
			pmpkg_t *tp = i->data;
			if(tp == NULL) {
				continue;
			}

			found=0;
			for(j = tp->requiredby; j; j = j->next) {
				if(!pm_list_is_strin((char *)j->data, packages)) {
					/* check if a package in trans->packages provides this package */
					for(k=trans->packages; !found && k; k=k->next) {
						pmpkg_t *spkg = NULL;
					if(trans->type == PM_TRANS_TYPE_SYNC) {
						pmsyncpkg_t *sync = k->data;
						spkg = sync->pkg;
					} else {
						spkg = k->data;
					}
						if(spkg && pm_list_is_strin(tp->name, spkg->provides)) {
							found=1;
						}
					}
					if(!found) {
						_alpm_log(PM_LOG_DEBUG, "checkdeps: found %s as required by %s", (char *)j->data, tp->name);
						miss = depmiss_new(tp->name, PM_DEP_TYPE_REQUIRED, PM_DEP_MOD_ANY, j->data, NULL);
						if(!depmiss_isin(miss, baddeps)) {
							baddeps = pm_list_add(baddeps, miss);
						} else {
							FREE(miss);
						}
					}
				}
			}
		}
	}

	return(baddeps);
}

int splitdep(char *depstr, pmdepend_t *depend)
{
	char *str = NULL;
	char *ptr = NULL;

	if(depstr == NULL || depend == NULL) {
		return(-1);
	}

	depend->mod = 0;
	depend->name[0] = 0;
	depend->version[0] = 0;

	str = strdup(depstr);

	if((ptr = strstr(str, ">="))) {
		depend->mod = PM_DEP_MOD_GE;
	} else if((ptr = strstr(str, "<="))) {
		depend->mod = PM_DEP_MOD_LE;
	} else if((ptr = strstr(str, "="))) {
		depend->mod = PM_DEP_MOD_EQ;
	} else {
		/* no version specified - accept any */
		depend->mod = PM_DEP_MOD_ANY;
		STRNCPY(depend->name, str, PKG_NAME_LEN);
	}

	if(ptr == NULL) {
		FREE(str);
		return(0);
	}
	*ptr = '\0';
	STRNCPY(depend->name, str, PKG_NAME_LEN);
	ptr++;
	if(depend->mod != PM_DEP_MOD_EQ) {
		ptr++;
	}
	STRNCPY(depend->version, ptr, PKG_VERSION_LEN);
	FREE(str);

	return(0);
}

/* return a new PMList target list containing all packages in the original
 * target list, as well as all their un-needed dependencies.  By un-needed,
 * I mean dependencies that are *only* required for packages in the target
 * list, so they can be safely removed.  This function is recursive.
 */
PMList* removedeps(pmdb_t *db, PMList *targs)
{
	PMList *i, *j, *k;
	PMList *newtargs = targs;

	if(db == NULL) {
		return(newtargs);
	}

	for(i = targs; i; i = i->next) {
		for(j = ((pmpkg_t *)i->data)->depends; j; j = j->next) {
			pmdepend_t depend;
			pmpkg_t *dep;
			int needed = 0;

			if(splitdep(j->data, &depend)) {
				continue;
			}

			dep = db_get_pkgfromcache(db, depend.name);
			if(dep == NULL) {
				/* package not found... look for a provisio instead */
				k = _alpm_db_whatprovides(db, depend.name);
				if(k == NULL) {
					_alpm_log(PM_LOG_WARNING, "cannot find package \"%s\" or anything that provides it!", depend.name);
					continue;
				}
				dep = db_get_pkgfromcache(db, ((pmpkg_t *)k->data)->name);
				if(dep == NULL) {
					_alpm_log(PM_LOG_ERROR, "dep is NULL!");
					/* wtf */
					continue;
				}
				FREELISTPTR(k);
			}
			if(pkg_isin(dep->name, targs)) {
				continue;
			}

			/* see if it was explicitly installed */
			if(dep->reason == PM_PKG_REASON_EXPLICIT) {
				_alpm_log(PM_LOG_FLOW2, "excluding %s -- explicitly installed", dep->name);
				needed = 1;
			}

			/* see if other packages need it */
			for(k = dep->requiredby; k && !needed; k = k->next) {
				pmpkg_t *dummy = db_get_pkgfromcache(db, k->data);
				if(!pkg_isin(dummy->name, targs)) {
					needed = 1;
				}
			}
			if(!needed) {
				char *name;
				pmpkg_t *pkg = pkg_new(dep->name, dep->version);
				if(pkg == NULL) {
					_alpm_log(PM_LOG_ERROR, "could not allocate memory for a package structure");
					continue;
				}
				asprintf(&name, "%s-%s", dep->name, dep->version);
				/* add it to the target list */
				db_read(db, name, INFRQ_ALL, pkg);
				newtargs = pm_list_add(newtargs, pkg);
				_alpm_log(PM_LOG_FLOW2, "adding %s to the targets", pkg->name);
				newtargs = removedeps(db, newtargs);
				FREE(name);
			}
		}
	}

	return(newtargs);
}

/* populates *list with packages that need to be installed to satisfy all
 * dependencies (recursive) for syncpkg
 *
 * make sure *list and *trail are already initialized
 */
int resolvedeps(pmdb_t *local, PMList *dbs_sync, pmpkg_t *syncpkg, PMList *list,
                PMList *trail, pmtrans_t *trans, PMList **data)
{
	PMList *i, *j;
	PMList *targ;
	PMList *deps = NULL;

	if(local == NULL || dbs_sync == NULL || syncpkg == NULL) {
		return(-1);
	}

	targ = pm_list_add(NULL, syncpkg);
	deps = checkdeps(trans, local, PM_TRANS_TYPE_ADD, targ);
	FREELISTPTR(targ);

	if(deps == NULL) {
		return(0);
	}

	for(i = deps; i; i = i->next) {
		int found = 0;
		pmdepmissing_t *miss = i->data;
		pmpkg_t *sync = NULL;

		/* XXX: conflicts are now treated specially in the _add and _sync functions */

		/*if(miss->type == PM_DEP_TYPE_CONFLICT) {
			_alpm_log(PM_LOG_ERROR, "cannot resolve dependencies for \"%s\" (it conflict with %s)", miss->target, miss->depend.name);
			RET_ERR(???, -1);
		} else*/


		/* check if one of the packages in *list already provides this dependency */
		for(j = list; j && !found; j = j->next) {
			pmpkg_t *sp = (pmpkg_t *)j->data;
			if(pm_list_is_strin(miss->depend.name, sp->provides)) {
				_alpm_log(PM_LOG_DEBUG, "%s provides dependency %s -- skipping",
				          sp->name, miss->depend.name);
				found = 1;
			}
		}
		if(found) {
			continue;
		}

		/* find the package in one of the repositories */
		/* check literals */
		for(j = dbs_sync; !sync && j; j = j->next) {
			sync = db_get_pkgfromcache(j->data, miss->depend.name);
		}
		/* check provides */
		for(j = dbs_sync; !sync && j; j = j->next) {
			PMList *provides;
			provides = _alpm_db_whatprovides(j->data, miss->depend.name);
			if(provides) {
				sync = provides->data;
			}
			FREELISTPTR(provides);
		}
		if(sync == NULL) {
			_alpm_log(PM_LOG_ERROR, "cannot resolve dependencies for \"%s\" (\"%s\" is not in the package set)",
			          miss->target, miss->depend.name);
			if(data) {
				if((miss = (pmdepmissing_t *)malloc(sizeof(pmdepmissing_t))) == NULL) {
					FREELIST(*data);
					pm_errno = PM_ERR_MEMORY;
					goto error;
				}
				*miss = *(pmdepmissing_t *)i->data;
				*data = pm_list_add(*data, miss);
			}
			pm_errno = PM_ERR_UNSATISFIED_DEPS;
			goto error;
		}
		if(pkg_isin(sync->name, list)) {
			/* this dep is already in the target list */
			_alpm_log(PM_LOG_DEBUG, "dependency %s is already in the target list -- skipping",
			          sync->name);
			continue;
		}

		if(!pkg_isin(sync->name, trail)) {
			/* check pmo_ignorepkg and pmo_s_ignore to make sure we haven't pulled in
			 * something we're not supposed to.
			 */
			int usedep = 1;
			if(pm_list_is_strin(sync->name, handle->ignorepkg)) {
				pmpkg_t *dummypkg = pkg_new(miss->target, NULL);
				QUESTION(trans, PM_TRANS_CONV_INSTALL_IGNOREPKG, dummypkg, sync, NULL, &usedep);
				FREEPKG(dummypkg);
			}
			if(usedep) {
				trail = pm_list_add(trail, sync);
				if(resolvedeps(local, dbs_sync, sync, list, trail, trans, data)) {
					goto error;
				}
				_alpm_log(PM_LOG_DEBUG, "pulling dependency %s (needed by %s)",
				          sync->name, syncpkg->name);
				list = pm_list_add(list, sync);
			} else {
				_alpm_log(PM_LOG_ERROR, "cannot resolve dependencies for \"%s\"", miss->target);
				if(data) {
					if((miss = (pmdepmissing_t *)malloc(sizeof(pmdepmissing_t))) == NULL) {
						FREELIST(*data);
						pm_errno = PM_ERR_MEMORY;
						goto error;
					}
					*miss = *(pmdepmissing_t *)i->data;
					*data = pm_list_add(*data, miss);
				}
				pm_errno = PM_ERR_UNSATISFIED_DEPS;
				goto error;
			}
		} else {
			/* cycle detected -- skip it */
			_alpm_log(PM_LOG_DEBUG, "dependency cycle detected: %s", sync->name);
		}
	}

	FREELIST(deps);

	return(0);

error:
	FREELIST(deps);
	return(-1);
}

/* vim: set ts=2 sw=2 noet: */
