/*
 *  conflict.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2006 by David Kimpe <dnaku@frugalware.org>
 *  Copyright (c) 2006, 2007 by Miklos Vajna <vmiklos@frugalware.org>
 *  Copyright (c) 2006 by Christian Hamar <krics@linuxforum.hu>
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
#include "conflict.h"

#include "db/localdb_files.h"
#include "util.h"
#include "error.h"
#include "cache.h"
#include "deps.h"

#include "util/falgorithm.h"
#include "util/log.h"
#include "fstdlib.h"

#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

using namespace libpacman;

/* Returns a FPtrList* of pmdepmissing_t pointers.
 *
 * conflicts are always name only
 */
FPtrList pmtrans_t::checkconflicts()
{
	return checkconflicts(packages());
}

FPtrList pmtrans_t::checkconflicts(const FList<Package *> &packages)
{
	Package *info = NULL;
	FPtrList baddeps;
	pmdepmissing_t *miss = NULL;
	int howmany, remain;
	double percent;
	Database *db_local = m_handle->db_local;

	if(db_local == NULL) {
		return baddeps;
	}

	howmany = packages.size();

	for(auto i = packages.begin(), end = packages.end(); i != end; ++i) {
		Package *tp = *i;
		if(tp == NULL) {
			continue;
		}
		remain = flib::count(i, end);
		percent = (double)(howmany - remain + 1) / howmany;

		if(m_type == PM_TRANS_TYPE_SYNC) {
			PROGRESS(this, PM_TRANS_PROGRESS_INTERCONFLICTS_START, "",
					(percent * 100), howmany,
					howmany - remain + 1);
		}

		auto &conflicts = tp->conflicts();
		for(auto j = conflicts.begin(), j_end = conflicts.end(); j != j_end; ++j) {
			const char *conflict = (const char *)*j;
			if(!strcmp(tp->name(), conflict)) {
				/* a package cannot conflict with itself -- that's just not nice */
				continue;
			}
			/* CHECK 1: check targets against database */
			_pacman_log(PM_LOG_DEBUG, _("checkconflicts: targ '%s' vs db"), tp->name());
			auto &cache = _pacman_db_get_pkgcache(db_local);
			for(auto k = cache.begin(), k_end = cache.end(); k != k_end; ++k) {
				Package *dp = (Package *)*k;
				if(!strcmp(dp->name(), tp->name())) {
					/* a package cannot conflict with itself -- that's just not nice */
					continue;
				}
				if(!strcmp(dp->name(), conflict)) {
					/* conflict */
					_pacman_log(PM_LOG_DEBUG, _("targs vs db: found %s as a conflict for %s"),
					          dp->name(), tp->name());
					miss = new __pmdepmissing_t(tp->name(), PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, dp->name(), NULL);
					_pacman_depmisslist_add(baddeps, miss);
				} else {
					/* see if dp provides something in tp's conflict list */
					auto &provides = dp->provides();
					for(auto m = provides.begin(), m_end = provides.end(); m != m_end; ++m) {
						if(!strcmp((const char *)*m, conflict)) {
							/* confict */
							_pacman_log(PM_LOG_DEBUG, _("targs vs db: found %s as a conflict for %s"),
							          dp->name(), tp->name());
							miss = new __pmdepmissing_t(tp->name(), PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, dp->name(), NULL);
							_pacman_depmisslist_add(baddeps, miss);
						}
					}
				}
			}
			/* CHECK 2: check targets against targets */
			_pacman_log(PM_LOG_DEBUG, _("checkconflicts: targ '%s' vs targs"), tp->name());
			for(auto k = packages.begin(), k_end = packages.end(); k != k_end; ++k) {
				Package *otp = (Package *)*k;
				if(!strcmp(otp->name(), tp->name())) {
					/* a package cannot conflict with itself -- that's just not nice */
					continue;
				}
				if(!strcmp(otp->name(), conflict)) {
					/* otp is listed in tp's conflict list */
					_pacman_log(PM_LOG_DEBUG, _("targs vs targs: found %s as a conflict for %s"),
					          otp->name(), tp->name());
					miss = new __pmdepmissing_t(tp->name(), PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, otp->name(), NULL);
					_pacman_depmisslist_add(baddeps, miss);
				} else {
					/* see if otp provides something in tp's conflict list */
					auto &provides = otp->provides();
					for(auto m = provides.begin(), m_end = provides.end(); m != m_end; ++m) {
						if(!strcmp((const char *)*m, conflict)) {
							_pacman_log(PM_LOG_DEBUG, _("targs vs targs: found %s as a conflict for %s"),
							          otp->name(), tp->name());
							miss = new __pmdepmissing_t(tp->name(), PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, otp->name(), NULL);
							_pacman_depmisslist_add(baddeps, miss);
						}
					}
				}
			}
		}
		/* CHECK 3: check database against targets */
		_pacman_log(PM_LOG_DEBUG, _("checkconflicts: db vs targ '%s'"), tp->name());
		auto &cache = _pacman_db_get_pkgcache(db_local);
		for(auto k = cache.begin(), k_end = cache.end(); k != k_end; ++k) {
			FStringList *conflicts;
			int usenewconflicts = 0;

			info = (Package *)*k;
			if(!strcmp(info->name(), tp->name())) {
				/* a package cannot conflict with itself -- that's just not nice */
				continue;
			}
			/* If this package (*info) is also in our packages FPtrList, use the
			 * conflicts list from the new package, not the old one (*info)
			 */
			for(auto j = packages.begin(), j_end = packages.end(); j != j_end; ++j) {
				Package *pkg = (Package *)*j;
				if(!strcmp(pkg->name(), info->name())) {
					/* Use the new, to-be-installed package's conflicts */
					conflicts = &pkg->conflicts();
					usenewconflicts = 1;
				}
			}
			if(!usenewconflicts) {
				/* Use the old package's conflicts, it's the only set we have */
				conflicts = &info->conflicts();
			}
			for(auto j = conflicts->begin(), j_end = conflicts->end(); j != j_end; ++j) {
				if(!strcmp(tp->name(), (const char *)*j)) {
					_pacman_log(PM_LOG_DEBUG, _("db vs targs: found %s as a conflict for %s"),
					          info->name(), tp->name());
					miss = new __pmdepmissing_t(tp->name(), PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, info->name(), NULL);
					_pacman_depmisslist_add(baddeps, miss);
				} else {
					/* see if the db package conflicts with something we provide */
					for(auto m = conflicts->begin(), m_end = conflicts->end(); m != m_end; ++m) {
						auto &provides = tp->provides();
						for(auto n = provides.begin(), n_end = provides.end(); n != n_end; ++n) {
							if(!strcmp((const char *)*m, (const char *)*n)) {
								_pacman_log(PM_LOG_DEBUG, _("db vs targs: found %s as a conflict for %s"),
								          info->name(), tp->name());
								miss = new __pmdepmissing_t(tp->name(), PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, info->name(), NULL);
								_pacman_depmisslist_add(baddeps, miss);
							}
						}
					}
				}
			}
		}
	}
	return baddeps;
}

/* Returns a FPtrList* of file conflicts.
 *  Hooray for set-intersects!
 *  Pre-condition: both lists are sorted!
 */
static FStringList chk_fileconflicts(const FStringList &filesA, const FStringList &filesB)
{
	FStringList ret;
	auto pA = filesA.begin(), pB = filesB.begin();

	while(pA != filesA.end() && pB != filesB.end()) {
		const char *strA = (const char *)*pA;
		const char *strB = (const char *)*pB;
		/* skip directories, we don't care about dir conflicts */
		if(strA[strlen(strA)-1] == '/') {
			++pA;
		} else if(strB[strlen(strB)-1] == '/') {
			++pB;
		} else {
			int cmp = strcmp(strA, strB);
			if(cmp < 0) {
				/* item only in filesA, ignore it */
				++pA;
			} else if(cmp > 0) {
				/* item only in filesB, ignore it */
				++pB;
			} else {
				/* item in both, record it */
				ret.add(strA);
				++pA;
				++pB;
			}
	  }
	}
	return ret;
}

FPtrList pmtrans_t::find_conflicts()
{
	char path[PATH_MAX+1];
	struct stat buf;
	FPtrList conflicts;
	Package *p, *dbpkg;
	double percent;
	int howmany, remain;
	Database *db_local = m_handle->db_local;
	const char *root = m_handle->root;

	if(db_local == NULL || empty() || root == NULL) {
		return conflicts;
	}
	howmany = m_packages.size();

	/* CHECK 1: check every target against every target */
	for(auto i = m_packages.begin(), end = m_packages.end(); i != end; ++i) {
		Package *p1 = (Package*)*i;
		remain = flib::count(i, end);
		percent = (double)(howmany - remain + 1) / howmany;
		PROGRESS(this, PM_TRANS_PROGRESS_CONFLICTS_START, "", (percent * 100), howmany, howmany - remain + 1);
		for(auto j = i; j != end; ++j) {
			Package *p2 = (Package*)*j;
			if(strcmp(p1->name(), p2->name())) {
				auto ret = chk_fileconflicts(p1->files(), p2->files());
				for(auto k = ret.begin(), k_end = ret.end(); k != k_end; ++k) {
						pmconflict_t *conflict = new pmconflict_t();
						if(conflict == NULL) {
							continue;
						}
						conflict->type = PM_CONFLICT_TYPE_TARGET;
						STRNCPY(conflict->target, p1->name(), PKG_NAME_LEN);
						STRNCPY(conflict->file, (const char *)*k, CONFLICT_FILE_LEN);
						STRNCPY(conflict->ctarget, p2->name(), PKG_NAME_LEN);
						conflicts.add(conflict);
				}
			}
		}

		/* CHECK 2: check every target against the filesystem */
		p = (Package*)*i;
		dbpkg = NULL;
		auto &files = p->files();
		for(auto j = files.begin(), j_end = files.end(); j != j_end; ++j) {
			const char *filestr = *j;
			snprintf(path, PATH_MAX, "%s%s", root, filestr);
			/* is this target a file or directory? */
			if(path[strlen(path)-1] == '/') {
				path[strlen(path)-1] = '\0';
			}
			if(!lstat(path, &buf)) {
				int ok = 0;
				/* re-fetch with stat() instead of lstat() */
				stat(path, &buf);
				if(S_ISDIR(buf.st_mode)) {
					/* if it's a directory, then we have no conflict */
					ok = 1;
				} else {
					if(dbpkg == NULL) {
						dbpkg = db_local->find(p->name());
					}
					if(dbpkg && !(dbpkg->flags & INFRQ_FILES)) {
						_pacman_log(PM_LOG_DEBUG, _("loading FILES info for '%s'"), dbpkg->name());
						dbpkg->read(INFRQ_FILES);
					}
					if(dbpkg && _pacman_list_is_strin(filestr, &dbpkg->files())) {
						ok = 1;
					}
					/* Check if the conflicting file has been moved to another package/target */
					if(!ok) {
						/* Look at all the targets */
						for(auto k = m_packages.begin(), k_end = m_packages.end(); k != k_end && !ok; ++k) {
							Package *p2 = *k;
							/* As long as they're not the current package */
							if(strcmp(p2->name(), p->name())) {
								Package *dbpkg2 = NULL;
								dbpkg2 = db_local->find(p2->name());
								if(dbpkg2 && !(dbpkg2->flags & INFRQ_FILES)) {
									_pacman_log(PM_LOG_DEBUG, _("loading FILES info for '%s'"), dbpkg2->name());
									dbpkg2->read(INFRQ_FILES);
								}
								/* If it used to exist in there, but doesn't anymore */
								if(dbpkg2 && !_pacman_list_is_strin(filestr, &p2->files()) && _pacman_list_is_strin(filestr, &dbpkg2->files())) {
									ok = 1;
									/* Add to the "skip list" of files that we shouldn't remove during an upgrade.
									 *
									 * This is a workaround for the following scenario:
									 *
									 *    - the old package A provides file X
									 *    - the new package A does not
									 *    - the new package B provides file X
									 *    - package A depends on B, so B is upgraded first
									 *
									 * Package B is upgraded, so file X is installed.  Then package A
									 * is upgraded, and it *removes* file X, since it no longer exists
									 * in package A.
									 *
									 * Our workaround is to scan through all "old" packages and all "new"
									 * ones, looking for files that jump to different packages.
									 */
									skiplist.add(filestr);
								}
							}
						}
					}
				}
				if(!ok) {
					pmconflict_t *conflict = new pmconflict_t();
					if(conflict == NULL) {
						continue;
					}
					conflict->type = PM_CONFLICT_TYPE_FILE;
					STRNCPY(conflict->target, p->name(), PKG_NAME_LEN);
					STRNCPY(conflict->file, filestr, CONFLICT_FILE_LEN);
					conflict->ctarget[0] = 0;
					conflicts.add(conflict);
				}
			}
		}
	}
	return conflicts;
}

/* vim: set ts=2 sw=2 noet: */
