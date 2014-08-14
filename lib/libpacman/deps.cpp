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

/* pacman-g2 */
#include "deps.h"

#include "db/localdb_files.h"
#include "util/list.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "fstdlib.h"
#include "fstring.h"
#include "util.h"
#include "error.h"
#include "package.h"
#include "pacman_p.h"
#include "db.h"
#include "cache.h"
#include "versioncmp.h"
#include "handle.h"

using namespace libpacman;

static pmgraph_t *_pacman_graph_new(void)
{
	pmgraph_t *graph = _pacman_zalloc(sizeof(pmgraph_t));

	return(graph);
}

static void _pacman_graph_free(void *data)
{
	pmgraph_t *graph = data;
	FREELISTPTR(graph->children);
	free(graph);
}

__pmdepmissing_t::__pmdepmissing_t(const char *target, unsigned char type, unsigned char depmod,
		const char *depname, const char *depversion)
{
	STRNCPY(this->target, target, PKG_NAME_LEN);
	this->type = type;
	depend.mod = depmod;
	STRNCPY(depend.name, depname, PKG_NAME_LEN);
	if(depversion) {
		STRNCPY(depend.version, depversion, PKG_VERSION_LEN);
	} else {
		depend.version[0] = 0;
	}
}

static
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

pmlist_t *_pacman_depmisslist_add(pmlist_t *misslist, pmdepmissing_t *miss)
{
	if(!_pacman_depmiss_isin(miss, misslist)) {
		misslist = _pacman_list_add(misslist, miss);
	} else {
		delete miss;
	}
	return misslist;
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
 *
 */
pmlist_t *_pacman_sortbydeps(pmlist_t *targets, int mode)
{
	pmlist_t *newtargs = NULL;
	pmlist_t *i, *j, *k;
	pmlist_t *vertices = NULL;
	pmlist_t *vptr;
	pmgraph_t *vertex;
	int found;

	if(targets == NULL) {
		return(NULL);
	}

	_pacman_log(PM_LOG_DEBUG, _("started sorting dependencies"));

	/* We create the vertices */
	for(i = targets; i; i = i->next) {
		pmgraph_t *v = _pacman_graph_new();
		v->data = (void *)i->data;
		vertices = _pacman_list_add(vertices, v);
	}

	/* We compute the edges */
	for(i = vertices; i; i = i->next) {
		pmgraph_t *vertex_i = i->data;
		Package *p_i = vertex_i->data;
		/* TODO this should be somehow combined with _pacman_checkdeps */
		for(j = vertices; j; j = j->next) {
			pmgraph_t *vertex_j = j->data;
			Package *p_j = vertex_j->data;
			int child = 0;
			for(k = p_i->depends(); k && !child; k = k->next) {
				pmdepend_t depend;
				_pacman_splitdep(k->data, &depend);
				child = _pacman_depcmp(p_j, &depend);
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
		pmlist_t *tmptargs = _pacman_list_reverse(newtargs);
		/* free the old one */
		FREELISTPTR(newtargs);
		newtargs = tmptargs;
	}

	_FREELIST(vertices, _pacman_graph_free);

	return(newtargs);
}

/* Returns a pmlist_t* of missing_t pointers.
 *
 * dependencies can include versions with depmod operators.
 *
 */
pmlist_t *_pacman_checkdeps(pmtrans_t *trans, unsigned char op, pmlist_t *packages)
{
	pmdepend_t depend;
	pmlist_t *i, *j, *k;
	int cmp;
	int found = 0;
	pmlist_t *baddeps = NULL;
	pmdepmissing_t *miss = NULL;
	Database *db_local = trans->m_handle->db_local;

	if(db_local == NULL) {
		return(NULL);
	}

	for(i = packages; i; i = i->next) {
		Package *tp = i->data;
		Package *pkg_local;

		if(tp == NULL) {
			continue;
		}

		if(op == PM_TRANS_TYPE_UPGRADE) {
			pkg_local = _pacman_db_get_pkgfromcache(db_local, tp->name());
		} else if (PM_TRANS_TYPE_REMOVE) {
			pkg_local = tp;
		}

		if(pkg_local != NULL) {
			found = 0;
			for(j = pkg_local->requiredby(); j; j = j->next) {
				const char *requiredby_name = f_stringlistitem_to_str(j);

				if(op == PM_TRANS_TYPE_UPGRADE) {
					/* PM_TRANS_TYPE_UPGRADE handles the backwards dependencies, ie, the packages
					 * listed in the requiredby field.
					 */
					Package *p;
				
					if((p = _pacman_db_get_pkgfromcache(db_local, requiredby_name)) == NULL) {
						/* hmmm... package isn't installed.. */
						continue;
					}
					if(_pacman_pkg_isin(p->name(), packages)) {
						/* this package is also in the upgrade list, so don't worry about it */
						continue;
					}
					for(k = p->depends(); k; k = k->next) {
						const char *depend_name = f_stringlistitem_to_str(k);

						/* don't break any existing dependencies (possible provides) */
						_pacman_splitdep(depend_name, &depend);
						if(_pacman_depcmp(pkg_local, &depend) && !_pacman_depcmp(tp, &depend)) {
							_pacman_log(PM_LOG_DEBUG, _("checkdeps: updated '%s' won't satisfy a dependency of '%s'"),
									pkg_local->name(), p->name());
							miss = new __pmdepmissing_t(p->name(), PM_DEP_TYPE_DEPEND, depend.mod,
									depend.name, depend.version);
							baddeps = _pacman_depmisslist_add(baddeps, miss);
						}
					}
				} else if(op == PM_TRANS_TYPE_REMOVE) {
					/* check requiredby fields */
					if(!_pacman_pkg_isin(requiredby_name, packages)) {
						/* check if a package in trans->packages provides this package */
						for(k=trans->packages; !found && k; k=k->next) {
							Package *spkg = k->data;

							if(spkg && spkg->provides(pkg_local->name())) {
								found=1;
							}
						}
						for(k=trans->syncpkgs; !found && k; k=k->next) {
							pmsyncpkg_t *ps = k->data;

							if(ps->pkg_new->provides(pkg_local->name())) {
								found=1;
							}
						}
						if(!found) {
							_pacman_log(PM_LOG_DEBUG, _("checkdeps: found %s which requires %s"), requiredby_name, pkg_local->name());
							miss = new __pmdepmissing_t(pkg_local->name(), PM_DEP_TYPE_REQUIRED, PM_DEP_MOD_ANY, requiredby_name, NULL);
							baddeps = _pacman_depmisslist_add(baddeps, miss);
						}
					}
				}
			}
		}
		if(op == PM_TRANS_TYPE_ADD || op == PM_TRANS_TYPE_UPGRADE) {
			/* DEPENDENCIES -- look for unsatisfied dependencies */
			for(j = tp->depends(); j; j = j->next) {
				const char *depend_name = f_stringlistitem_to_str(j);

				/* split into name/version pairs */
				_pacman_splitdep(depend_name, &depend);
				found = 0;
				/* check database for literal packages */
				for(k = _pacman_db_get_pkgcache(db_local); k && !found; k = k->next) {
					Package *p = (Package *)k->data;
					if(!strcmp(p->name(), depend.name)) {
						if(depend.mod == PM_DEP_MOD_ANY) {
							/* accept any version */
							found = 1;
						} else {
							char *ver = strdup(p->version());
							/* check for a release in depend.version.  if it's
							 * missing remove it from p->version as well.
							 */
							if(!index(depend.version,'-')) {
								char *ptr;
								for(ptr = ver; *ptr != '-'; ptr++);
								*ptr = '\0';
							}
							cmp = _pacman_versioncmp(ver, depend.version);
							switch(depend.mod) {
								case PM_DEP_MOD_EQ: found = (cmp == 0); break;
								case PM_DEP_MOD_GE: found = (cmp >= 0); break;
								case PM_DEP_MOD_LE: found = (cmp <= 0); break;
								case PM_DEP_MOD_LT: found = (cmp < 0); break;
								case PM_DEP_MOD_GT: found = (cmp > 0); break;
							}
							free(ver);
						}
					}
				}
 				/* check database for provides matches */
 				if(!found) {
 					pmlist_t *m;
 					k = _pacman_db_whatprovides(db_local, depend.name);
 					for(m = k; m && !found; m = m->next) {
 						/* look for a match that isn't one of the packages we're trying
 						 * to install.  this way, if we match against a to-be-installed
 						 * package, we'll defer to the NEW one, not the one already
 						 * installed. */
 						Package *p = m->data;
 						pmlist_t *n;
 						int skip = 0;
 						for(n = packages; n && !skip; n = n->next) {
 							Package *ptp = n->data;
 							if(!strcmp(ptp->name(), p->name())) {
 								skip = 1;
 							}
 						}
 						if(skip) {
 							continue;
 						}

						if(depend.mod == PM_DEP_MOD_ANY) {
							/* accept any version */
							found = 1;
						} else {
							char *ver = strdup(p->version());
							/* check for a release in depend.version.  if it's
							 * missing remove it from p->version as well.
							 */
							if(!index(depend.version,'-')) {
								char *ptr;
								for(ptr = ver; *ptr != '-'; ptr++);
								*ptr = '\0';
							}
							cmp = _pacman_versioncmp(ver, depend.version);
							switch(depend.mod) {
								case PM_DEP_MOD_EQ: found = (cmp == 0); break;
								case PM_DEP_MOD_GE: found = (cmp >= 0); break;
								case PM_DEP_MOD_LE: found = (cmp <= 0); break;
								case PM_DEP_MOD_LT: found = (cmp < 0); break;
								case PM_DEP_MOD_GT: found = (cmp > 0); break;
							}
							free(ver);
						}
					}
					FREELISTPTR(k);
				}
 				/* check other targets */
 				for(k = packages; k && !found; k = k->next) {
 					Package *p = (Package *)k->data;
 					/* see if the package names match OR if p provides depend.name */
 					if(!strcmp(p->name(), depend.name) || p->provides(depend.name)) {
						if(depend.mod == PM_DEP_MOD_ANY ||
								p->provides(depend.name)) {
							/* depend accepts any version or p provides depend (provides - by
							 * definition - is for all versions) */
							found = 1;
						} else {
							char *ver = strdup(p->version());
							/* check for a release in depend.version.  if it's
							 * missing remove it from p->version as well.
							 */
							if(!index(depend.version,'-')) {
								char *ptr;
								for(ptr = ver; *ptr != '-'; ptr++);
								*ptr = '\0';
							}
							cmp = _pacman_versioncmp(ver, depend.version);
							switch(depend.mod) {
								case PM_DEP_MOD_EQ: found = (cmp == 0); break;
								case PM_DEP_MOD_GE: found = (cmp >= 0); break;
								case PM_DEP_MOD_LE: found = (cmp <= 0); break;
								case PM_DEP_MOD_LT: found = (cmp < 0); break;
								case PM_DEP_MOD_GT: found = (cmp > 0); break;
							}
							free(ver);
						}
					}
				}
				/* else if still not found... */
				if(!found) {
					_pacman_log(PM_LOG_DEBUG, _("checkdeps: found %s as a dependency for %s"),
					          depend.name, tp->name());
					miss = new __pmdepmissing_t(tp->name(), PM_DEP_TYPE_DEPEND, depend.mod, depend.name, depend.version);
					baddeps = _pacman_depmisslist_add(baddeps, miss);
				}
			}
		}
	}

	return(baddeps);
}

int _pacman_splitdep(char *depstr, pmdepend_t *depend)
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
		goto out;
	}
	*ptr = '\0';
	STRNCPY(depend->name, str, PKG_NAME_LEN);
	ptr++;
	if(depend->mod == PM_DEP_MOD_GE || depend->mod == PM_DEP_MOD_LE) {
		ptr++;
	}
	STRNCPY(depend->version, ptr, PKG_VERSION_LEN);

out:
	free(str);
	return(0);
}

/* return a new pmlist_t target list containing all packages in the original
 * target list, as well as all their un-needed dependencies.  By un-needed,
 * I mean dependencies that are *only* required for packages in the target
 * list, so they can be safely removed.  This function is recursive.
 */
pmlist_t *_pacman_removedeps(Database *db, pmlist_t *targs)
{
	pmlist_t *i, *j, *k;
	pmlist_t *newtargs = targs;

	if(db == NULL) {
		return(newtargs);
	}

	for(i = targs; i; i = i->next) {
		for(j = ((Package *)i->data)->depends(); j; j = j->next) {
			pmdepend_t depend;
			Package *dep;
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
				dep = _pacman_db_get_pkgfromcache(db, ((Package *)k->data)->name());
				if(dep == NULL) {
					_pacman_log(PM_LOG_ERROR, _("dep is NULL!"));
					/* wtf */
					continue;
				}
				FREELISTPTR(k);
			}
			if(_pacman_pkg_isin(dep->name(), targs)) {
				continue;
			}

			/* see if it was explicitly installed */
			if(dep->reason() == PM_PKG_REASON_EXPLICIT) {
				_pacman_log(PM_LOG_FLOW2, _("excluding %s -- explicitly installed"), dep->name());
				needed = 1;
			}

			/* see if other packages need it */
			for(k = dep->requiredby(); k && !needed; k = k->next) {
				Package *dummy = _pacman_db_get_pkgfromcache(db, f_stringlistitem_to_str(k));
				if(!_pacman_pkg_isin(dummy->name(), targs)) {
					needed = 1;
				}
			}
			if(!needed) {
				Package *pkg = new Package(dep->name(), dep->version());
				if(pkg == NULL) {
					continue;
				}
				/* add it to the target list */
				_pacman_log(PM_LOG_DEBUG, _("loading ALL info for '%s'"), pkg->name());
				pkg->read(INFRQ_ALL);
				newtargs = _pacman_list_add(newtargs, pkg);
				_pacman_log(PM_LOG_FLOW2, _("adding '%s' to the targets"), pkg->name());
				newtargs = _pacman_removedeps(db, newtargs);
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
int _pacman_resolvedeps(pmtrans_t *trans, Package *syncpkg, pmlist_t *list,
                      pmlist_t *trail, pmlist_t **data)
{
	pmlist_t *i, *j;
	pmlist_t *targ;
	pmlist_t *deps = NULL;
	Handle *handle = trans->m_handle;
	pmlist_t *dbs_sync = handle->dbs_sync;

	if(dbs_sync == NULL || syncpkg == NULL) {
		return(-1);
	}

	targ = _pacman_list_add(NULL, syncpkg);
	deps = _pacman_checkdeps(trans, PM_TRANS_TYPE_ADD, targ);
	FREELISTPTR(targ);

	if(deps == NULL) {
		return(0);
	}

	for(i = deps; i; i = i->next) {
		int found = 0;
		pmdepmissing_t *miss = i->data;
		Package *ps = NULL;

		/* check if one of the packages in *list already provides this dependency */
		for(j = list; j && !found; j = j->next) {
			Package *sp = (Package *)j->data;
			if(sp->provides(miss->depend.name)) {
				_pacman_log(PM_LOG_DEBUG, _("%s provides dependency %s -- skipping"),
				          sp->name(), miss->depend.name);
				found = 1;
			}
		}
		if(found) {
			continue;
		}

		/* find the package in one of the repositories */
		/* check literals */
		for(j = dbs_sync; !ps && j; j = j->next) {
			ps = _pacman_db_get_pkgfromcache(j->data, miss->depend.name);
		}
		/* check provides */
		for(j = dbs_sync; !ps && j; j = j->next) {
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
		if(_pacman_pkg_isin(ps->name(), list)) {
			/* this dep is already in the target list */
			_pacman_log(PM_LOG_DEBUG, _("dependency %s is already in the target list -- skipping"),
			          ps->name());
			continue;
		}

		if(!_pacman_pkg_isin(ps->name(), trail)) {
			/* check pmo_ignorepkg and pmo_s_ignore to make sure we haven't pulled in
			 * something we're not supposed to.
			 */
			int usedep = 1;
			if(_pacman_list_is_strin(ps->name(), handle->ignorepkg)) {
				Package *dummypkg = new Package(miss->target, NULL);
				QUESTION(trans, PM_TRANS_CONV_INSTALL_IGNOREPKG, dummypkg, ps, NULL, &usedep);
				dummypkg->release();
			}
			if(usedep) {
				trail = _pacman_list_add(trail, ps);
				if(_pacman_resolvedeps(trans, ps, list, trail, data)) {
					goto error;
				}
				_pacman_log(PM_LOG_DEBUG, _("pulling dependency %s (needed by %s)"),
				          ps->name(), syncpkg->name());
				list = _pacman_list_add(list, ps);
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
		} else {
			/* cycle detected -- skip it */
			_pacman_log(PM_LOG_DEBUG, _("dependency cycle detected: %s"), ps->name());
		}
	}

	FREELIST(deps);

	return(0);

error:
	FREELIST(deps);
	return(-1);
}

int _pacman_depcmp(Package *pkg, pmdepend_t *dep)
{
	int equal = 0, cmp;
	const char *mod = "~=";

	if(strcmp(pkg->name(), dep->name) == 0
	    	|| pkg->provides(dep->name)) {
			if(dep->mod == PM_DEP_MOD_ANY) {
				equal = 1;
			} else {
				cmp = _pacman_versioncmp(pkg->version(), dep->version);
				switch(dep->mod) {
					case PM_DEP_MOD_EQ: equal = (cmp == 0); break;
					case PM_DEP_MOD_GE: equal = (cmp >= 0); break;
					case PM_DEP_MOD_LE: equal = (cmp <= 0); break;
					case PM_DEP_MOD_LT: equal = (cmp < 0); break;
					case PM_DEP_MOD_GT: equal = (cmp > 0); break;
					default: equal = 1; break;
				}
			}

		switch(dep->mod) {
			case PM_DEP_MOD_EQ: mod = "=="; break;
			case PM_DEP_MOD_GE: mod = ">="; break;
			case PM_DEP_MOD_LE: mod = "<="; break;
			case PM_DEP_MOD_LT: mod = "<"; break;
			case PM_DEP_MOD_GT: mod = ">"; break;
			default: break;
		}

		if(!_pacman_strempty(dep->version)) {
			_pacman_log(PM_LOG_DEBUG, _("depcmp: %s-%s %s %s-%s => %s"),
								pkg->name(), pkg->version(),
								mod, dep->name, dep->version,
								(equal ? "match" : "no match"));
		} else {
			_pacman_log(PM_LOG_DEBUG, _("depcmp: %s-%s %s %s => %s"),
								pkg->name(), pkg->version(),
								mod, dep->name,
								(equal ? "match" : "no match"));
		}
	}

	return equal;
}

/* Helper function for comparing strings
 */
static int str_cmp(const void *s1, const void *s2)
{
    return(strcmp(s1, s2));
}

int inList(pmlist_t *lst, char *lItem) {
    pmlist_t *ll;
    ll = lst;
    while(ll) {
        if(!strcmp(lItem, (char *)ll->data)) {
           return 1;
        }
        ll = ll->next;
    }
    return 0;
}

extern "C" {
int pacman_output_generate(pmlist_t *targets, pmlist_t *dblist) {
    pmlist_t *found = NULL;
    pmlist_t *k = NULL, *j = NULL;
    Package *pkg = NULL;
    char *match = NULL;
    int foundMatch = 0;
    unsigned int inforeq =  INFRQ_DEPENDS;
    for(j = dblist; j; j = j->next) {
        Database *db = j->data;
        do {
            foundMatch = 0;
            pkg = db->readpkg(inforeq);
            while(pkg != NULL) {
                const char *pname = pkg->name();
                targets = _pacman_list_remove(targets, (void*) pname, str_cmp, (void **)&match);
                if(match) {
                    foundMatch = 1;
                    for(k = pkg->depends(); k; k = k->next) {
                        char *fullDep = (char *)k->data;
                        pmdepend_t depend;
                        if(_pacman_splitdep(fullDep, &depend)) {
                            continue;
                        }
                        strcpy(fullDep, depend.name);
                        if(!inList(found, fullDep) && !inList(targets, fullDep)) {
                            targets = _pacman_list_add(targets, fullDep);
                        }
                    }
                    if(!inList(found,pname)) {
                        printf("%s ", pname);
                        found = _pacman_list_add(found, pname);
                    }
                    FREE(match);
                }
                pkg = db->readpkg(inforeq);
            }
            db->rewind();
        } while(foundMatch);
    }
    FREELISTPTR(found);
    FREELISTPTR(targets);
    printf("\n");
    return 0;
}
}

/* vim: set ts=2 sw=2 noet: */
