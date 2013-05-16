/*
 *  deps.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005, 2006, 2007 by Miklos Vajna <vmiklos@frugalware.org>
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
#ifdef __sun__
#include <strings.h>
#endif

/* pacman-g2 */
#include "deps.h"

#include "util.h"
#include "log.h"
#include "error.h"
#include "package.h"
#include "db.h"
#include "cache.h"
#include "provide.h"
#include "versioncmp.h"
#include "handle.h"

#include "fstringlist.h"

static
int _pacman_transpkg_resolvedeps (pmtrans_t *trans, pmtranspkg_t *transpkg, pmlist_t **data);

static pmgraph_t *_pacman_graph_new(void)
{
	pmgraph_t *graph = _pacman_zalloc(sizeof(pmgraph_t));

	return(graph);
}

static void _pacman_graph_free(void *data)
{
	pmgraph_t *graph = data;
	FREELISTPTR(graph->children);
	FREE(graph);
}

int _pacman_splitdep(const char *depstr, pmdepend_t *depend)
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
	} else if((ptr = strstr(str, "<"))) {
		depend->mod = PM_DEP_MOD_LT;
	} else if((ptr = strstr(str, ">"))) {
		depend->mod = PM_DEP_MOD_GT;
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
	if(depend->mod == PM_DEP_MOD_GE || depend->mod == PM_DEP_MOD_LE) {
		ptr++;
	}
	STRNCPY(depend->version, ptr, PKG_VERSION_LEN);
	FREE(str);

	return(0);
}

static
int _pacman_pkg_is_depend_satisfied (pmpkg_t *pkg, pmdepend_t *dep)
{
	int cmp = 1;
	const char *mod = "~=";

	if (strcmp(pkg->name, dep->name) == 0) {
		if(dep->mod == PM_DEP_MOD_ANY) {
			cmp = 0;
		} else {
			int equal;

			/* check for a release in depend.version. if it's
			 * missing remove it from p->version as well.
			 */
			cmp = _pacman_versioncmp(_pacman_pkg_getinfo(pkg, PM_PKG_VERSION), dep->version);
			switch(dep->mod) {
				case PM_DEP_MOD_EQ: equal = (cmp == 0); break;
				case PM_DEP_MOD_GE: equal = (cmp >= 0); break;
				case PM_DEP_MOD_LE: equal = (cmp <= 0); break;
				case PM_DEP_MOD_LT: equal = (cmp < 0); break;
				case PM_DEP_MOD_GT: equal = (cmp > 0); break;
				default: equal = 1; break;
			}
			cmp = !equal; /* reverse test so 0 means success */
		}
	} else if (f_stringlist_find (_pacman_pkg_getinfo(pkg, PM_PKG_PROVIDES), dep->name) != NULL) {
		/* accepts any version for provides */
		cmp = 0;
	} else {
		return 1;
	}

	switch(dep->mod) {
		case PM_DEP_MOD_EQ: mod = "=="; break;
		case PM_DEP_MOD_GE: mod = ">="; break;
		case PM_DEP_MOD_LE: mod = "<="; break;
		case PM_DEP_MOD_LT: mod = "<"; break;
		case PM_DEP_MOD_GT: mod = ">"; break;
		default: break;
	}

	if(strlen(dep->version) > 0) {
		_pacman_log(PM_LOG_DEBUG, _("depcmp: %s-%s %s %s-%s => %s"),
							_pacman_pkg_getinfo(pkg, PM_PKG_NAME), _pacman_pkg_getinfo(pkg, PM_PKG_NAME),
							mod, dep->name, dep->version,
							(cmp == 0 ? "match" : "no match"));
	} else {
		_pacman_log(PM_LOG_DEBUG, _("depcmp: %s-%s %s %s => %s"),
							_pacman_pkg_getinfo(pkg, PM_PKG_NAME), _pacman_pkg_getinfo(pkg, PM_PKG_VERSION),
							mod, dep->name,
							(cmp == 0 ? "match" : "no match"));
	}

	return cmp;
}

static
int _pacman_transpkg_is_depend_satisfied (pmtranspkg_t *transpkg, pmdepend_t *depend) {
	if (transpkg->type & PM_TRANS_TYPE_ADD &&
			_pacman_pkg_is_depend_satisfied (transpkg->pkg_new, depend) == 0) {
		return 0;
	}
	return 1;
}

static
int _pacman_trans_is_depend_satisfied (pmtrans_t *trans, pmdepend_t *depend) {
	pmlist_t *i;
	pmtranspkg_t *transpkg = __pacman_trans_get_trans_pkg (trans, depend->name);

	/* Check against named depend in transaction list if it's not removed */
	if (transpkg != NULL &&
			transpkg->type != PM_TRANS_TYPE_REMOVE) {
		return _pacman_transpkg_is_depend_satisfied (transpkg, depend);
	}

	/* Else check transaction for provides */
	if (f_list_detect (trans->packages, (FDetectFunc)_pacman_transpkg_is_depend_satisfied, depend) != NULL) {
		return 0;
	}

	/* Else check local database for provides excluding packages in transaction */
	for(i = _pacman_db_get_pkgcache(trans->handle->db_local); i != NULL; i = i->next) {
		pmpkg_t *pkg = i->data;

		if (__pacman_trans_get_trans_pkg (trans, pkg->name) == NULL &&
				_pacman_pkg_is_depend_satisfied (pkg, depend) == 0) {
			return 0;
		}
	}
	return 1;
}

pmdepmissing_t *_pacman_depmiss_new(const char *target, unsigned char type, unsigned char depmod,
                                  const char *depname, const char *depversion)
{
	pmdepmissing_t *miss = _pacman_malloc(sizeof(pmdepmissing_t));

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

int _pacman_depmiss_isin(pmdepmissing_t *needle, pmlist_t *haystack)
{
	pmlist_t *i;

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
 * This function returns the new pmlist_t* target list.
 */
void _pacman_sortbydeps(pmtrans_t *trans, int mode)
{
	pmlist_t *newtargs = NULL;
	pmlist_t *i, *j, *k;
	pmlist_t *vertices = NULL;
	pmlist_t *vptr;
	pmgraph_t *vertex;
	int found;

	if (trans->_packages == NULL) {
		return;
	}

	_pacman_log(PM_LOG_DEBUG, _("started sorting dependencies"));

	/* We create the vertices */
	for (i = trans->_packages; i; i = i->next) {
		pmgraph_t *v = _pacman_graph_new();
		v->data = (void *)i->data;
		vertices = _pacman_list_add(vertices, v);
	}

	/* We compute the edges */
	for(i = vertices; i; i = i->next) {
		pmgraph_t *vertex_i = i->data;
		pmpkg_t *p_i = vertex_i->data;
		/* TODO this should be somehow combined with _pacman_checkdeps */
		for(j = vertices; j; j = j->next) {
			pmgraph_t *vertex_j = j->data;
			pmpkg_t *p_j = vertex_j->data;
			int child = 0;
			for(k = _pacman_pkg_getinfo(p_i, PM_PKG_DEPENDS); k && !child; k = k->next) {
				pmdepend_t depend;
				_pacman_splitdep(k->data, &depend);
				child = _pacman_pkg_is_depend_satisfied (p_j, &depend) == 0;
			}
			if(child) {
				vertex_i->children = _pacman_list_add(vertex_i->children, vertex_j);
			}
		}
		vertex_i->childptr = vertex_i->children;
	}

	vptr = vertices;
	vertex = vertices->data;
	while(vptr) {
		/* mark that we touched the vertex */
		vertex->state = -1;
		found = 0;
		while(vertex->childptr && !found) {
			pmgraph_t *nextchild = (vertex->childptr)->data;
			vertex->childptr = (vertex->childptr)->next;
			if (nextchild->state == 0) {
				found = 1;
				nextchild->parent = vertex;
				vertex = nextchild;
			} else if(nextchild->state == -1) {
				_pacman_log(PM_LOG_DEBUG, _("dependency cycle detected"));
			}
		}
		if(!found) {
			newtargs = _pacman_list_add(newtargs, vertex->data);
			/* mark that we've left this vertex */
			vertex->state = 1;
			vertex = vertex->parent;
			if(!vertex) {
				vptr = vptr->next;
				while(vptr) {
					vertex = vptr->data;
					if (vertex->state == 0) break;
					vptr = vptr->next;
				}
			}
		}
	}
	_pacman_log(PM_LOG_DEBUG, _("sorting dependencies finished"));

	if(mode == PM_TRANS_TYPE_REMOVE) {
		/* we're removing packages, so reverse the order */
		pmlist_t *tmptargs = f_list_reverse(newtargs);
		/* free the old one */
		FREELISTPTR(newtargs);
		newtargs = tmptargs;
	}

	_FREELIST(vertices, _pacman_graph_free);

	trans->_packages = newtargs;
}

static
int _pacman_trans_check_package_depends (pmtrans_t *trans, pmpkg_t *pkg, pmlist_t **missdeps) {
	pmlist_t *i;

	for(i = _pacman_pkg_getinfo (pkg, PM_PKG_DEPENDS); i; i = i->next) {
		const char *pkg_depend = i->data;
		pmdepend_t depend;
		pmdepmissing_t *miss;

		/* split into name/version pairs */
		_pacman_splitdep (pkg_depend, &depend);
		if (_pacman_trans_is_depend_satisfied (trans, &depend) != 0) {
			_pacman_log (PM_LOG_DEBUG, _("checkdeps: found %s as a dependency for %s"),
					depend.name, pkg->name);
			miss = _pacman_depmiss_new(pkg->name, PM_DEP_TYPE_DEPEND, depend.mod, depend.name, depend.version);
			if (!_pacman_depmiss_isin(miss, *missdeps)) {
				*missdeps = _pacman_list_add(*missdeps, miss);
			} else {
				FREE(miss);
			}
		}
	}
	return 0;
}

/* Returns a pmlist_t* of missing_t pointers in **baddeps.
 *
 * dependencies can include versions with depmod operators.
 */
static
int _pacman_transpkg_checkdeps(pmtrans_t *trans, pmtranspkg_t *transpkg, pmlist_t **baddeps) {
	pmdb_t *db = trans->handle->db_local;

	if(db == NULL) {
		return -1;
	}

	if (transpkg->type & PM_TRANS_TYPE_ADD) {
		_pacman_trans_check_package_depends (trans, transpkg->pkg_new, baddeps);
	}
	if (transpkg->type & PM_TRANS_TYPE_REMOVE) {
		pmlist_t *i;

		/* Check reverse depends using PM_PKG_REQUIREDBY */
		for(i = _pacman_pkg_getinfo(transpkg->pkg_local, PM_PKG_REQUIREDBY); i; i = i->next) {
			const char *required_pkg_name = i->data;
			pmtranspkg_t *required_transpkg = __pacman_trans_get_trans_pkg (trans, required_pkg_name);
			pmpkg_t *required_pkg_local;

			if (required_transpkg != NULL) {
				if (transpkg->type & PM_TRANS_TYPE_ADD &&
						required_transpkg->type & PM_TRANS_TYPE_ADD) {
					/* The required package is also to be upgraded, so don't worry about it */
					continue;
				}
				if (transpkg->type == PM_TRANS_TYPE_REMOVE &&
						required_transpkg->type == PM_TRANS_TYPE_REMOVE) {
					/* The required package is also to be deleted, so don't worry about it */
					continue;
				}
			}

			if((required_pkg_local = _pacman_db_get_pkgfromcache(db, required_pkg_name)) == NULL) {
				/* hmmm... package isn't installed.. */
				continue;
			}

			_pacman_trans_check_package_depends (trans, required_pkg_local, baddeps);
		}
	}
	return 0;
}

int _pacman_trans_checkdeps (pmtrans_t *trans, pmlist_t **depmisslist) {
	pmlist_t *i;

	for (i = trans->packages; i != NULL; i = i->next) {
		pmtranspkg_t *transpkg = i->data;

		if (_pacman_transpkg_checkdeps (trans, transpkg, depmisslist) != 0) {
			return -1;
		}
	}
	return 0;
}

/* Returns a pmlist_t* of missing_t pointers.
 *
 * dependencies can include versions with depmod operators.
 */
pmlist_t *_pacman_checkdeps (pmtrans_t *trans, unsigned char op, pmlist_t *packages) {
	pmlist_t *i, *ret = NULL;

	for (i = packages; i; i = i->next) {
		pmtranspkg_t transpkg;
		transpkg.type = op;
		if (op & PM_TRANS_TYPE_ADD) {
			transpkg.pkg_new = i->data;
			transpkg.pkg_local = _pacman_db_get_pkgfromcache(trans->handle->db_local, transpkg.pkg_new->name);
		} else {
			transpkg.pkg_new = NULL;
			transpkg.pkg_local = i->data;
		}
		_pacman_transpkg_checkdeps(trans, &transpkg, &ret);
	}
	return ret;
}

/* return a new pmlist_t target list containing all packages in the original
 * target list, as well as all their un-needed dependencies.  By un-needed,
 * I mean dependencies that are *only* required for packages in the target
 * list, so they can be safely removed.  This function is recursive.
 */
void _pacman_removedeps(pmtrans_t *trans)
{
	pmlist_t *i, *j, *k;
	pmdb_t *db = trans->handle->db_local;

	if(db == NULL) {
		return;
	}

	for (i = trans->packages; i; i = f_list_next(i), NULL) {
		pmtranspkg_t *transpkg = i->data;
		for(j = (_pacman_pkg_getinfo(transpkg->pkg_local, PM_PKG_DEPENDS)); j; j = j->next) {
			pmdepend_t depend;
			pmpkg_t *dep;
			int needed = 0;

			if(_pacman_splitdep(j->data, &depend)) {
				continue;
			}

			dep = _pacman_db_get_pkgfromcache(db, depend.name);
			if(dep == NULL) {
				/* package not found... look for a provisio instead */
				k = _pacman_db_whatprovides(db, depend.name);
				if(k == NULL) {
					_pacman_log(PM_LOG_WARNING, _("cannot find package \"%s\" or anything that provides it!"), depend.name);
					continue;
				}
				dep = _pacman_db_get_pkgfromcache(db, ((pmpkg_t *)k->data)->name);
				if(dep == NULL) {
					_pacman_log(PM_LOG_ERROR, _("dep is NULL!"));
					/* wtf */
					continue;
				}
				FREELISTPTR(k);
			}
			if(__pacman_trans_get_trans_pkg (trans, dep->name) != NULL) {
				continue;
			}

			/* see if it was explicitly installed */
			if(dep->reason == PM_PKG_REASON_EXPLICIT) {
				_pacman_log(PM_LOG_FLOW2, _("excluding %s -- explicitly installed"), dep->name);
				needed = 1;
			}

			/* see if other packages need it */
			for(k = _pacman_pkg_getinfo(dep, PM_PKG_REQUIREDBY); k && !needed; k = k->next) {
				pmpkg_t *dummy = _pacman_db_get_pkgfromcache(db, k->data);
				if(__pacman_trans_get_trans_pkg (trans, dummy->name) == NULL) {
					needed = 1;
				}
			}
			if(!needed) {
				pmpkg_t *pkg = _pacman_pkg_new(dep->name, dep->version);
				if(pkg == NULL) {
					continue;
				}
				/* add it to the target list */
				_pacman_trans_addtarget (trans, dep->name, transpkg->type, transpkg->flags);
			}
		}
	}
}

static
int _pacman_transpkg_remove_dependsonly (pmtrans_t *trans) {
	FList *excludes = NULL;

	f_list_exclude (&trans->packages, &excludes, (FDetectFunc)_pacman_transpkg_has_flags, (void*)PM_TRANS_FLAG_DEPENDSONLY);
	f_list_delete (excludes, NULL, NULL);
	return 0;
}

/* populates *trans with packages that need to be installed to satisfy all
 * dependencies (recursive) for syncpkg
 */
static
int _pacman_resolvedeps (pmtrans_t *trans, pmlist_t *deps, pmlist_t **data)
{
	pmlist_t *i, *j;

	if(deps == NULL) {
		return(0);
	}

	_pacman_log(PM_LOG_FLOW1, _("resolving targets dependencies"));
	for(i = deps; i; i = i->next) {
		pmdepmissing_t *miss = i->data;
		pmpkg_t *ps = NULL;

		/* find the package in one of the repositories */
		/* check literals */
		for(j = trans->handle->dbs_sync; !ps && j; j = j->next) {
			ps = _pacman_db_get_pkgfromcache(j->data, miss->depend.name);
		}
		/* check provides */
		for(j = trans->handle->dbs_sync; !ps && j; j = j->next) {
			pmlist_t *provides;
			provides = _pacman_db_whatprovides(j->data, miss->depend.name);
			if(provides) {
				ps = provides->data;
			}
			FREELISTPTR(provides);
		}
		if(ps == NULL) {
			_pacman_log(PM_LOG_ERROR, _("cannot resolve dependencies for \"%s\" (\"%s\" is not in the package set)"),
			          miss->target, miss->depend.name);
			if(data) {
				if((miss = _pacman_malloc(sizeof(pmdepmissing_t))) == NULL) {
					FREELIST(*data);
					goto error;
				}
				*miss = *(pmdepmissing_t *)i->data;
				*data = _pacman_list_add(*data, miss);
			}
			pm_errno = PM_ERR_UNSATISFIED_DEPS;
			goto error;
		}
		if (__pacman_trans_get_trans_pkg (trans, ps->name) != NULL) {
			/* this dep is already in the target list */
			_pacman_log(PM_LOG_DEBUG, _("dependency %s is already in the target list -- skipping"),
			          ps->name);
			continue;
		} else {
			/* check pmo_ignorepkg and pmo_s_ignore to make sure we haven't pulled in
			 * something we're not supposed to.
			 */
			int usedep = 1;
			if(f_stringlist_find (handle->ignorepkg, ps->name)) {
				pmpkg_t *dummypkg = _pacman_pkg_new(miss->target, NULL);
				QUESTION(trans, PM_TRANS_CONV_INSTALL_IGNOREPKG, dummypkg, ps, NULL, &usedep);
				FREEPKG(dummypkg);
			}
			if(usedep) {
				pmtranspkg_t *transpkg = _pacman_trans_add_pkg (trans, ps, PM_TRANS_TYPE_UPGRADE, 0);

				_pacman_log(PM_LOG_DEBUG, _("pulling dependency %s"), ps->name);
				if (_pacman_transpkg_resolvedeps(trans, transpkg, data) != 0) {
					goto error;
				}
			} else {
				_pacman_log(PM_LOG_ERROR, _("cannot resolve dependencies for \"%s\""), miss->target);
				if(data) {
					if((miss = _pacman_malloc(sizeof(pmdepmissing_t))) == NULL) {
						FREELIST(*data);
						goto error;
					}
					*miss = *(pmdepmissing_t *)i->data;
					*data = _pacman_list_add(*data, miss);
				}
				pm_errno = PM_ERR_UNSATISFIED_DEPS;
				goto error;
			}
		}
	}

	FREELIST(deps);

	return(0);

error:
	FREELIST(deps);
	return(-1);
}

static
int _pacman_transpkg_resolvedeps (pmtrans_t *trans, pmtranspkg_t *transpkg, pmlist_t **data) {
	pmlist_t *deps = NULL;

	if (_pacman_transpkg_checkdeps (trans, transpkg, &deps) != 0 ||
			_pacman_resolvedeps (trans, deps, data) != 0) {
		/* pm_errno is allready set */
		return -1;
	}
	return 0;
}

int _pacman_trans_resolvedeps (pmtrans_t *trans, pmlist_t **data) {
	pmlist_t *deps = NULL;

	if (trans == NULL) {
		return(-1);
	}

	if (_pacman_trans_checkdeps (trans, &deps) != 0 ||
			_pacman_resolvedeps (trans, deps, data) != 0 ||
			_pacman_transpkg_remove_dependsonly (trans) != 0) {
		/* pm_errno is allready set */
		return -1;
	}
	return 0;
}

/* vim: set ts=2 sw=2 noet: */
