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
#include "util/log.h"
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

typedef struct __pmgraph_t {
	__pmgraph_t()
		: state(0), data(nullptr), parent(nullptr), childptr(nullptr)
	{ }

	Package *data;
	FList<__pmgraph_t *> children;

	int state; /* 0: untouched, -1: entered, other: leaving time */
	struct __pmgraph_t *parent; /* where did we come from? */
	decltype(children)::iterator childptr; /* points to a child in children list */
} pmgraph_t;

static pmgraph_t *_pacman_graph_new(void)
{
	return new pmgraph_t();
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
int _pacman_depmiss_isin(pmdepmissing_t *needle, FPtrList *haystack)
{
	for(auto i = haystack->begin(), end = haystack->end(); i != end; ++i) {
		pmdepmissing_t *miss = (pmdepmissing_t *)*i;
		if(!memcmp(needle, miss, sizeof(pmdepmissing_t))
		   && !memcmp(&needle->depend, &miss->depend, sizeof(pmdepend_t))) {
			return(1);
		}
	}
	return(0);
}

FPtrList &_pacman_depmisslist_add(FPtrList &misslist, pmdepmissing_t *miss)
{
	if(!_pacman_depmiss_isin(miss, &misslist)) {
		misslist.add(miss);
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
 * This function returns the new FPtrList* target list.
 *
 */
FList<Package *> _pacman_sortbydeps(const FList<Package *> &targets, int mode)
{
	FList<Package *> newtargs;
	FPtrList vertices;
	pmgraph_t *vertex;
	int found;

	if(targets.empty()) {
		return newtargs;
	}

	_pacman_log(PM_LOG_DEBUG, _("started sorting dependencies"));

	/* We create the vertices */
	for(auto i = targets.begin(), end = targets.end(); i != end; ++i) {
		pmgraph_t *v = _pacman_graph_new();
		v->data = *i;
		vertices.add(v);
	}

	/* We compute the edges */
	for(auto i = vertices.begin(), end = vertices.end(); i != end; ++i) {
		pmgraph_t *vertex_i = (pmgraph_t *)*i;
		Package *p_i = vertex_i->data;
		/* TODO this should be somehow combined with _pacman_checkdeps */
		for(auto j = vertices.begin(); j != end; ++j) {
			pmgraph_t *vertex_j = *j;
			Package *p_j = vertex_j->data;
			int child = 0;
			auto &depends = p_i->depends();
			for(auto k = depends.begin(), k_end = depends.end(); k != k_end && !child; ++k) {
				pmdepend_t depend;
				_pacman_splitdep((char *)*k, &depend);
				child = _pacman_depcmp(p_j, &depend);
			}
			if(child) {
				vertex_i->children.add(vertex_j);
			}
		}
		vertex_i->childptr = vertex_i->children.begin();
	}

	FPtrList::iterator vptr = vertices.begin(), end = vertices.end();
	vertex = *vptr;
	while(vptr != end) {
		/* mark that we touched the vertex */
		vertex->state = -1;
		found = 0;
		while(vertex->childptr != vertex->children.end() && !found) {
			pmgraph_t *nextchild = *vertex->childptr;
			++vertex->childptr;
			if (nextchild->state == 0) {
				found = 1;
				nextchild->parent = vertex;
				vertex = nextchild;
			} else if(nextchild->state == -1) {
				_pacman_log(PM_LOG_DEBUG, _("dependency cycle detected"));
			}
		}
		if(!found) {
			newtargs.add(vertex->data);
			/* mark that we've left this vertex */
			vertex->state = 1;
			vertex = vertex->parent;
			if(!vertex) {
				++vptr;
				while(vptr != end) {
					vertex = *vptr;
					if (vertex->state == 0) break;
					++vptr;
				}
			}
		}
	}
	_pacman_log(PM_LOG_DEBUG, _("sorting dependencies finished"));

	if(mode == PM_TRANS_TYPE_REMOVE) {
		/* we're removing packages, so reverse the order */
		newtargs.reverse();
	}
	return newtargs;
}

/* Returns a FPtrList* of missing_t pointers.
 *
 * dependencies can include versions with depmod operators.
 *
 */
FPtrList pmtrans_t::checkdeps(unsigned char op)
{
	return checkdeps(op, packages());
}

FPtrList pmtrans_t::checkdeps(unsigned char op, const FList<Package *> &packages)
{
	pmdepend_t depend;
	int cmp;
	FPtrList baddeps;
	pmdepmissing_t *miss = NULL;
	Database *db_local = m_handle->db_local;

	if(db_local == NULL) {
		return baddeps;
	}

	for(auto i = packages.begin(), end = packages.end(); i != end; ++i) {
		Package *tp = *i;
		Package *pkg_local;

		if(tp == NULL) {
			continue;
		}

		if(op == PM_TRANS_TYPE_UPGRADE) {
			pkg_local = db_local->find(tp->name());
		} else if (PM_TRANS_TYPE_REMOVE) {
			pkg_local = tp;
		}

		if(pkg_local != NULL) {
			bool found = false;
			auto &requiredby = pkg_local->requiredby();
			for(auto j = requiredby.begin(), j_end = requiredby.end(); j != j_end; ++j) {
				const char *requiredby_name = (const char *)*j;

				if(op == PM_TRANS_TYPE_UPGRADE) {
					/* PM_TRANS_TYPE_UPGRADE handles the backwards dependencies, ie, the packages
					 * listed in the requiredby field.
					 */
					Package *p;
				
					if((p = db_local->find(requiredby_name)) == NULL) {
						/* hmmm... package isn't installed.. */
						continue;
					}
					if(_pacman_pkg_isin(p->name(), packages)) {
						/* this package is also in the upgrade list, so don't worry about it */
						continue;
					}
					auto &depends = p->depends();
					for(auto k = depends.begin(), k_end = depends.end(); k != k_end; ++k) {
						const char *depend_name = (const char *)*k;

						/* don't break any existing dependencies (possible provides) */
						_pacman_splitdep(depend_name, &depend);
						if(_pacman_depcmp(pkg_local, &depend) && !_pacman_depcmp(tp, &depend)) {
							_pacman_log(PM_LOG_DEBUG, _("checkdeps: updated '%s' won't satisfy a dependency of '%s'"),
									pkg_local->name(), p->name());
							miss = new __pmdepmissing_t(p->name(), PM_DEP_TYPE_DEPEND, depend.mod,
									depend.name, depend.version);
							_pacman_depmisslist_add(baddeps, miss);
						}
					}
				} else if(op == PM_TRANS_TYPE_REMOVE) {
					/* check requiredby fields */
					if(!_pacman_pkg_isin(requiredby_name, packages)) {
						/* check if a package in trans->packages provides this package */
						for(auto k = syncpkgs.begin(), k_end = syncpkgs.end(); !found && k != k_end; ++k) {
							pmsyncpkg_t *ps = *k;

							if(ps->pkg_new != NULL && ps->pkg_new->provides(pkg_local->name())) {
								found = true;
							}
						}
						if(!found) {
							_pacman_log(PM_LOG_DEBUG, _("checkdeps: found %s which requires %s"), requiredby_name, pkg_local->name());
							miss = new __pmdepmissing_t(pkg_local->name(), PM_DEP_TYPE_REQUIRED, PM_DEP_MOD_ANY, requiredby_name, NULL);
							_pacman_depmisslist_add(baddeps, miss);
						}
					}
				}
			}
		}
		if(op == PM_TRANS_TYPE_ADD || op == PM_TRANS_TYPE_UPGRADE) {
			/* DEPENDENCIES -- look for unsatisfied dependencies */
			auto &depends = tp->depends();
			for(auto j = depends.begin(), j_end = depends.end(); j != j_end; ++j) {
				const char *depend_name = (const char *)*j;

				/* split into name/version pairs */
				_pacman_splitdep(depend_name, &depend);
				bool found = false;
				/* check database for literal packages */
				auto &cache = _pacman_db_get_pkgcache(db_local);
				for(auto k = cache.begin(), k_end = cache.end(); k != k_end && !found; ++k) {
					found = (*k)->match(depend);
				}
 				/* check database for provides matches */
 				if(!found) {
 					auto whatPackagesProvide = db_local->whatPackagesProvide(depend.name);
 					for(auto m = whatPackagesProvide.begin(), m_end = whatPackagesProvide.end(); m != m_end && !found; ++m) {
 						/* look for a match that isn't one of the packages we're trying
 						 * to install.  this way, if we match against a to-be-installed
 						 * package, we'll defer to the NEW one, not the one already
 						 * installed. */
 						Package *p = *m;
 						int skip = 0;
 						for(auto n = packages.begin(), n_end = packages.end(); n != n_end && !skip; ++n) {
 							Package *ptp = *n;
 							if(!strcmp(ptp->name(), p->name())) {
 								skip = 1;
 							}
 						}
 						if(skip) {
 							continue;
 						}
						found = p->match(depend);
					}
				}
 				/* check other targets */
 				for(auto k = packages.begin(), k_end = packages.end(); k != k_end && !found; ++k) {
 					Package *p = (Package *)*k;
					found = p->match(depend);
				}
				/* else if still not found... */
				if(!found) {
					_pacman_log(PM_LOG_DEBUG, _("checkdeps: found %s as a dependency for %s"),
					          depend.name, tp->name());
					miss = new __pmdepmissing_t(tp->name(), PM_DEP_TYPE_DEPEND, depend.mod, depend.name, depend.version);
					_pacman_depmisslist_add(baddeps, miss);
				}
			}
		}
	}

	return(baddeps);
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

/* return a new FPtrList target list containing all packages in the original
 * target list, as well as all their un-needed dependencies.  By un-needed,
 * I mean dependencies that are *only* required for packages in the target
 * list, so they can be safely removed.
 */
void pmtrans_t::removedeps(Database *db)
{
	if(db == NULL) {
		return;
	}
	FList<Package *> &targs = m_packages;

	bool again = false;
	for(auto i = targs.begin(), end = targs.end(); i != end; i = again ? targs.begin() : i.next()) {
		again = false;
		auto &depends = ((Package *)*i)->depends();
		for(auto j = depends.begin(), j_end = depends.end(); j != j_end; ++j) {
			pmdepend_t depend;
			Package *dep;
			int needed = 0;

			if(_pacman_splitdep((const char *)*j, &depend)) {
				continue;
			}

			dep = db->find(depend.name);
			if(dep == NULL) {
				/* package not found... look for a provisio instead */
				auto whatPackagesProvide = db->whatPackagesProvide(depend.name);
				if(whatPackagesProvide.empty()) {
					_pacman_log(PM_LOG_WARNING, _("cannot find package \"%s\" or anything that provides it!"), depend.name);
					continue;
				}
				dep = db->find(((Package *)*whatPackagesProvide.begin())->name());
				if(dep == NULL) {
					_pacman_log(PM_LOG_ERROR, _("dep is NULL!"));
					/* wtf */
					continue;
				}
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
			auto &requiredby = dep->requiredby();
			for(auto k = requiredby.begin(), k_end = requiredby.end(); k != k_end && !needed; ++k) {
				Package *dummy = db->find((const char *)*k);
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
				targs.add(pkg);
				_pacman_log(PM_LOG_FLOW2, _("adding '%s' to the targets"), pkg->name());
				again = true;
			}
		}
	}
}

/* populates *list with packages that need to be installed to satisfy all
 * dependencies (recursive) for syncpkg
 *
 * make sure *list and *trail are already initialized
 */
int pmtrans_t::resolvedeps(Package *syncpkg, FList<Package *> &list,
                      FList<Package *> &trail, FPtrList **data)
{
	FPtrList deps;
	FList<Package *> targ;

	if(handle->dbs_sync.empty() || syncpkg == NULL) {
		return(-1);
	}

	targ.add(syncpkg);
	deps = checkdeps(PM_TRANS_TYPE_ADD, targ);

	if(deps.empty()) {
		return 0;
	}

	for(auto i = deps.begin(), end = deps.end(); i != end; ++i) {
		int found = 0;
		pmdepmissing_t *miss = (pmdepmissing_t *)*i;
		Package *ps = NULL;

		/* check if one of the packages in *list already provides this dependency */
		for(auto j = list.begin(), j_end = list.end(); j != j_end && !found; ++j) {
			Package *sp = *j;
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
		for(auto j = handle->dbs_sync.begin(), j_end = handle->dbs_sync.end(); !ps && j != j_end; ++j) {
			ps = ((Database *)*j)->find(miss->depend.name);
		}
		/* check provides */
		for(auto j = handle->dbs_sync.begin(), j_end = handle->dbs_sync.end(); !ps && j != j_end; ++j) {
			auto provides = ((Database *)*j)->whatPackagesProvide(miss->depend.name);
			if(!provides.empty()) {
				ps = *provides.begin();
			}
		}
		if(ps == NULL) {
			_pacman_log(PM_LOG_ERROR, _("cannot resolve dependencies for \"%s\" (\"%s\" is not in the package set)"),
			          miss->target, miss->depend.name);
			if(data != NULL) {
				if((miss = _pacman_malloc(sizeof(pmdepmissing_t))) == NULL) {
					FREELIST(*data);
					goto error;
				}
				*miss = *(pmdepmissing_t *)*i;
				((FPtrList *)*data)->add(miss);
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
			if(_pacman_list_is_strin(ps->name(), &handle->ignorepkg)) {
				Package *dummypkg = new Package(miss->target, NULL);
				QUESTION(this, PM_TRANS_CONV_INSTALL_IGNOREPKG, dummypkg, ps, NULL, &usedep);
				dummypkg->release();
			}
			if(usedep) {
				trail.add(ps);
				if(resolvedeps(ps, list, trail, data)) {
					goto error;
				}
				_pacman_log(PM_LOG_DEBUG, _("pulling dependency %s (needed by %s)"),
				          ps->name(), syncpkg->name());
				list.add(ps);
			} else {
				_pacman_log(PM_LOG_ERROR, _("cannot resolve dependencies for \"%s\""), miss->target);
				if(data != NULL) {
					if((miss = _pacman_malloc(sizeof(pmdepmissing_t))) == NULL) {
						FREELIST(*data);
						goto error;
					}
					*miss = *(pmdepmissing_t *)*i;
					((FPtrList *)*data)->add(miss);
				}
				pm_errno = PM_ERR_UNSATISFIED_DEPS;
				goto error;
			}
		} else {
			/* cycle detected -- skip it */
			_pacman_log(PM_LOG_DEBUG, _("dependency cycle detected: %s"), ps->name());
		}
	}
	return 0;

error:
	return -1;
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

int inList(FStringList *lst, char *lItem) {
    auto ll = lst->begin(), end = lst->end();
    while(ll != end) {
        if(!strcmp(lItem, *ll)) {
           return 1;
        }
        ++ll;
    }
    return 0;
}

extern "C" {
int pacman_output_generate(FStringList *targets, FPtrList *dblist) {
    FStringList found;
    Package *pkg = NULL;
    char *match = NULL;
    int foundMatch = 0;
    unsigned int inforeq =  INFRQ_DEPENDS;
    for(auto j = dblist->begin(), end = dblist->end(); j != end; ++j) {
        Database *db = *j;
        do {
            foundMatch = 0;
            pkg = db->readpkg(inforeq);
            while(pkg != NULL) {
                const char *pname = pkg->name();
                if(targets != NULL && targets->remove((void*) pname, str_cmp, (const char **)&match)) {
                    foundMatch = 1;
										auto &depends = pkg->depends();
                    for(auto k = depends.begin(), k_end = depends.end(); k != k_end; ++k) {
                        char *fullDep = *k;
                        pmdepend_t depend;
                        if(_pacman_splitdep(fullDep, &depend)) {
                            continue;
                        }
                        strcpy(fullDep, depend.name);
                        if(!inList(&found, fullDep) && !inList(targets, fullDep)) {
                            targets = f_stringlist_add(targets, fullDep);
                        }
                    }
                    if(!inList(&found,pname)) {
                        printf("%s ", pname);
                        found.add(pname);
                    }
                    FREE(match);
                }
                pkg = db->readpkg(inforeq);
            }
            db->rewind();
        } while(foundMatch);
    }
    printf("\n");
    return 0;
}
}

/* vim: set ts=2 sw=2 noet: */
