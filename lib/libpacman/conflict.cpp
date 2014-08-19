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

#include "util/log.h"
#include "util/stringlist.h"
#include "fstdlib.h"

#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

using namespace libpacman;

/* Returns a pmlist_t* of pmdepmissing_t pointers.
 *
 * conflicts are always name only
 */
pmlist_t *_pacman_checkconflicts(pmtrans_t *trans, pmlist_t *packages)
{
	Package *info = NULL;
	pmlist_t *i, *j, *k;
	pmlist_t *baddeps = NULL;
	pmdepmissing_t *miss = NULL;
	int howmany, remain;
	double percent;
	Database *db_local = trans->m_handle->db_local;

	if(db_local == NULL) {
		return(NULL);
	}

	howmany = f_ptrlist_count(packages);

	for(i = packages; i; i = f_ptrlistitem_next(i)) {
		Package *tp = (Package *)f_ptrlistitem_data(i);
		if(tp == NULL) {
			continue;
		}
		remain = f_ptrlist_count(i);
		percent = (double)(howmany - remain + 1) / howmany;

		if(trans->m_type == PM_TRANS_TYPE_SYNC) {
			PROGRESS(trans, PM_TRANS_PROGRESS_INTERCONFLICTS_START, "",
					(percent * 100), howmany,
					howmany - remain + 1);
		}

		for(j = tp->conflicts(); j; j = f_ptrlistitem_next(j)) {
			const char *conflict = f_stringlistitem_to_str(j);
			if(!strcmp(tp->name(), conflict)) {
				/* a package cannot conflict with itself -- that's just not nice */
				continue;
			}
			/* CHECK 1: check targets against database */
			_pacman_log(PM_LOG_DEBUG, _("checkconflicts: targ '%s' vs db"), tp->name());
			for(k = _pacman_db_get_pkgcache(db_local); k; k = f_ptrlistitem_next(k)) {
				Package *dp = (Package *)f_ptrlistitem_data(k);
				if(!strcmp(dp->name(), tp->name())) {
					/* a package cannot conflict with itself -- that's just not nice */
					continue;
				}
				if(!strcmp(dp->name(), conflict)) {
					/* conflict */
					_pacman_log(PM_LOG_DEBUG, _("targs vs db: found %s as a conflict for %s"),
					          dp->name(), tp->name());
					miss = new __pmdepmissing_t(tp->name(), PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, dp->name(), NULL);
					baddeps = _pacman_depmisslist_add(baddeps, miss);
				} else {
					/* see if dp provides something in tp's conflict list */
					pmlist_t *m;
					for(m = dp->provides(); m; m = f_ptrlistitem_next(m)) {
						if(!strcmp(f_stringlistitem_to_str(m), conflict)) {
							/* confict */
							_pacman_log(PM_LOG_DEBUG, _("targs vs db: found %s as a conflict for %s"),
							          dp->name(), tp->name());
							miss = new __pmdepmissing_t(tp->name(), PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, dp->name(), NULL);
							baddeps = _pacman_depmisslist_add(baddeps, miss);
						}
					}
				}
			}
			/* CHECK 2: check targets against targets */
			_pacman_log(PM_LOG_DEBUG, _("checkconflicts: targ '%s' vs targs"), tp->name());
			for(k = packages; k; k = f_ptrlistitem_next(k)) {
				Package *otp = (Package *)f_ptrlistitem_data(k);
				if(!strcmp(otp->name(), tp->name())) {
					/* a package cannot conflict with itself -- that's just not nice */
					continue;
				}
				if(!strcmp(otp->name(), conflict)) {
					/* otp is listed in tp's conflict list */
					_pacman_log(PM_LOG_DEBUG, _("targs vs targs: found %s as a conflict for %s"),
					          otp->name(), tp->name());
					miss = new __pmdepmissing_t(tp->name(), PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, otp->name(), NULL);
					baddeps = _pacman_depmisslist_add(baddeps, miss);
				} else {
					/* see if otp provides something in tp's conflict list */
					pmlist_t *m;
					for(m = otp->provides(); m; m = f_ptrlistitem_next(m)) {
						if(!strcmp(f_stringlistitem_to_str(m), conflict)) {
							_pacman_log(PM_LOG_DEBUG, _("targs vs targs: found %s as a conflict for %s"),
							          otp->name(), tp->name());
							miss = new __pmdepmissing_t(tp->name(), PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, otp->name(), NULL);
							baddeps = _pacman_depmisslist_add(baddeps, miss);
						}
					}
				}
			}
		}
		/* CHECK 3: check database against targets */
		_pacman_log(PM_LOG_DEBUG, _("checkconflicts: db vs targ '%s'"), tp->name());
		for(k = _pacman_db_get_pkgcache(db_local); k; k = f_ptrlistitem_next(k)) {
			pmlist_t *conflicts = NULL;
			int usenewconflicts = 0;

			info = (Package *)f_ptrlistitem_data(k);
			if(!strcmp(info->name(), tp->name())) {
				/* a package cannot conflict with itself -- that's just not nice */
				continue;
			}
			/* If this package (*info) is also in our packages pmlist_t, use the
			 * conflicts list from the new package, not the old one (*info)
			 */
			for(j = packages; j; j = f_ptrlistitem_next(j)) {
				Package *pkg = (Package *)f_ptrlistitem_data(j);
				if(!strcmp(pkg->name(), info->name())) {
					/* Use the new, to-be-installed package's conflicts */
					conflicts = pkg->conflicts();
					usenewconflicts = 1;
				}
			}
			if(!usenewconflicts) {
				/* Use the old package's conflicts, it's the only set we have */
				conflicts = info->conflicts();
			}
			for(j = conflicts; j; j = f_ptrlistitem_next(j)) {
				if(!strcmp(tp->name(), f_stringlistitem_to_str(j))) {
					_pacman_log(PM_LOG_DEBUG, _("db vs targs: found %s as a conflict for %s"),
					          info->name(), tp->name());
					miss = new __pmdepmissing_t(tp->name(), PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, info->name(), NULL);
					baddeps = _pacman_depmisslist_add(baddeps, miss);
				} else {
					/* see if the db package conflicts with something we provide */
					pmlist_t *m;
					for(m = conflicts; m; m = f_ptrlistitem_next(m)) {
						pmlist_t *n;
						for(n = tp->provides(); n; n = f_ptrlistitem_next(n)) {
							if(!strcmp(f_stringlistitem_to_str(m), f_stringlistitem_to_str(n))) {
								_pacman_log(PM_LOG_DEBUG, _("db vs targs: found %s as a conflict for %s"),
								          info->name(), tp->name());
								miss = new __pmdepmissing_t(tp->name(), PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, info->name(), NULL);
								baddeps = _pacman_depmisslist_add(baddeps, miss);
							}
						}
					}
				}
			}
		}
	}

	return(baddeps);
}

/* Returns a pmlist_t* of file conflicts.
 *  Hooray for set-intersects!
 *  Pre-condition: both lists are sorted!
 */
static pmlist_t *chk_fileconflicts(pmlist_t *filesA, pmlist_t *filesB)
{
	pmlist_t *ret = NULL;
	pmlist_t *pA = filesA, *pB = filesB;

	while(pA && pB) {
		const char *strA = f_stringlistitem_to_str(pA);
		const char *strB = f_stringlistitem_to_str(pB);
		/* skip directories, we don't care about dir conflicts */
		if(strA[strlen(strA)-1] == '/') {
			pA = f_ptrlistitem_next(pA);
		} else if(strB[strlen(strB)-1] == '/') {
			pB = f_ptrlistitem_next(pB);
		} else {
			int cmp = strcmp(strA, strB);
			if(cmp < 0) {
				/* item only in filesA, ignore it */
				pA = f_ptrlistitem_next(pA);
			} else if(cmp > 0) {
				/* item only in filesB, ignore it */
				pB = f_ptrlistitem_next(pB);
			} else {
				/* item in both, record it */
				ret = _pacman_stringlist_append(ret, strA);
				pA = f_ptrlistitem_next(pA);
				pB = f_ptrlistitem_next(pB);
			}
	  }
	}

	return(ret);
}

pmlist_t *_pacman_db_find_conflicts(pmtrans_t *trans)
{
	pmlist_t *i, *j, *k;
	char *filestr = NULL;
	char path[PATH_MAX+1];
	struct stat buf;
	pmlist_t *conflicts = NULL;
	pmlist_t *targets = trans->packages;
	Package *p, *dbpkg;
	double percent;
	int howmany, remain;
	Database *db_local = trans->m_handle->db_local;
	const char *root = trans->m_handle->root;

	if(db_local == NULL || targets == NULL || root == NULL) {
		return(NULL);
	}
	howmany = f_ptrlist_count(targets);

	/* CHECK 1: check every target against every target */
	for(i = targets; i; i = f_ptrlistitem_next(i)) {
		Package *p1 = (Package*)f_ptrlistitem_data(i);
		remain = f_ptrlist_count(i);
		percent = (double)(howmany - remain + 1) / howmany;
		PROGRESS(trans, PM_TRANS_PROGRESS_CONFLICTS_START, "", (percent * 100), howmany, howmany - remain + 1);
		for(j = i; j; j = f_ptrlistitem_next(j)) {
			Package *p2 = (Package*)f_ptrlistitem_data(j);
			if(strcmp(p1->name(), p2->name())) {
				pmlist_t *ret = chk_fileconflicts(p1->files(), p2->files());
				for(k = ret; k; k = f_ptrlistitem_next(k)) {
						pmconflict_t *conflict = _pacman_malloc(sizeof(pmconflict_t));
						if(conflict == NULL) {
							continue;
						}
						conflict->type = PM_CONFLICT_TYPE_TARGET;
						STRNCPY(conflict->target, p1->name(), PKG_NAME_LEN);
						STRNCPY(conflict->file, f_stringlistitem_to_str(k), CONFLICT_FILE_LEN);
						STRNCPY(conflict->ctarget, p2->name(), PKG_NAME_LEN);
						conflicts = f_ptrlist_add(conflicts, conflict);
				}
				FREELIST(ret);
			}
		}

		/* CHECK 2: check every target against the filesystem */
		p = (Package*)f_ptrlistitem_data(i);
		dbpkg = NULL;
		for(j = p->files(); j; j = f_ptrlistitem_next(j)) {
			filestr = f_stringlistitem_to_str(j);
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
					if(dbpkg && _pacman_list_is_strin(filestr, dbpkg->files())) {
						ok = 1;
					}
					/* Check if the conflicting file has been moved to another package/target */
					if(!ok) {
						/* Look at all the targets */
						for(k = targets; k && !ok; k = f_ptrlistitem_next(k)) {
							Package *p2 = (Package *)f_ptrlistitem_data(k);
							/* As long as they're not the current package */
							if(strcmp(p2->name(), p->name())) {
								Package *dbpkg2 = NULL;
								dbpkg2 = db_local->find(p2->name());
								if(dbpkg2 && !(dbpkg2->flags & INFRQ_FILES)) {
									_pacman_log(PM_LOG_DEBUG, _("loading FILES info for '%s'"), dbpkg2->name());
									dbpkg2->read(INFRQ_FILES);
								}
								/* If it used to exist in there, but doesn't anymore */
								if(dbpkg2 && !_pacman_list_is_strin(filestr, p2->files()) && _pacman_list_is_strin(filestr, dbpkg2->files())) {
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
									trans->skiplist = _pacman_stringlist_append(trans->skiplist, filestr);
								}
							}
						}
					}
				}
				if(!ok) {
					pmconflict_t *conflict = _pacman_malloc(sizeof(pmconflict_t));
					if(conflict == NULL) {
						continue;
					}
					conflict->type = PM_CONFLICT_TYPE_FILE;
					STRNCPY(conflict->target, p->name(), PKG_NAME_LEN);
					STRNCPY(conflict->file, filestr, CONFLICT_FILE_LEN);
					conflict->ctarget[0] = 0;
					conflicts = f_ptrlist_add(conflicts, conflict);
				}
			}
		}
	}

	return(conflicts);
}

/* vim: set ts=2 sw=2 noet: */
