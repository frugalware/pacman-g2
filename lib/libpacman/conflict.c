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

#if defined(__APPLE__) || defined(__OpenBSD__)
#include <sys/syslimits.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>

/* pacman-g2 */
#include "conflict.h"

#include "util.h"
#include "error.h"
#include "log.h"
#include "cache.h"
#include "deps.h"

#include "fstringlist.h"

/* Returns a pmlist_t* of pmdepmissing_t pointers.
 *
 * conflicts are always name only
 */
pmlist_t *_pacman_checkconflicts(pmtrans_t *trans) {
	pmpkg_t *info = NULL;
	pmlist_t *i, *j, *k;
	pmlist_t *baddeps = NULL;
	pmdepmissing_t *miss = NULL;
	int howmany, remain;
	double percent;

	howmany = f_ptrlist_count (trans->packages);

	f_foreach (i, trans->packages) {
		pmtranspkg_t *transpkg = i->data;
		const char *transpkg_name = __pacman_transpkg_name (transpkg);

		remain = f_ptrlist_count (i);
		percent = (double)(howmany - remain + 1) / howmany;

		if(trans->type == PM_TRANS_TYPE_SYNC) {
			PROGRESS(trans, PM_TRANS_PROGRESS_INTERCONFLICTS_START, "",
					(percent * 100), howmany,
					howmany - remain + 1);
		}

		if (!(transpkg->type & PM_TRANS_TYPE_ADD)) {
			continue;
		}

		for(j = _pacman_pkg_getinfo(transpkg->pkg_new, PM_PKG_CONFLICTS); j; j = j->next) {
			if(!strcmp(transpkg_name, j->data)) {
				/* a package cannot conflict with itself -- that's just not nice */
				continue;
			}
			/* CHECK 1: check targets against database */
			_pacman_log(PM_LOG_DEBUG, _("checkconflicts: targ '%s' vs db"), transpkg_name);
			for(k = _pacman_db_get_pkgcache(trans->handle->db_local); k; k = k->next) {
				pmpkg_t *dp = (pmpkg_t *)k->data;
				if(!strcmp(dp->name, transpkg_name)) {
					/* a package cannot conflict with itself -- that's just not nice */
					continue;
				}
				if(!strcmp(j->data, dp->name)) {
					/* conflict */
					_pacman_depmissinglist_add (&baddeps, transpkg_name, PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, dp->name, NULL);
				} else {
					/* see if dp provides something in tp's conflict list */
					pmlist_t *m;
					for(m = _pacman_pkg_getinfo(dp, PM_PKG_PROVIDES); m; m = m->next) {
						if(!strcmp(m->data, j->data)) {
							/* confict */
							_pacman_depmissinglist_add (&baddeps, transpkg_name, PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, dp->name, NULL);
						}
					}
				}
			}
			/* CHECK 2: check targets against targets */
			_pacman_log(PM_LOG_DEBUG, _("checkconflicts: targ '%s' vs targs"), transpkg_name);
			f_foreach (k, trans->packages) {
				pmtranspkg_t *other_transpkg = k->data;
				const char *other_transpkg_name = __pacman_transpkg_name (other_transpkg);

				if (other_transpkg == transpkg ||
						!(other_transpkg->type & PM_TRANS_TYPE_ADD)) {
					/* a package cannot conflict with itself -- that's just not nice */
					continue;
				}
				if(!strcmp(other_transpkg_name, (char *)j->data)) {
					/* otp is listed in tp's conflict list */
					_pacman_depmissinglist_add (&baddeps, transpkg_name, PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, other_transpkg_name, NULL);
				} else {
					/* see if otp provides something in tp's conflict list */
					pmlist_t *m;
					for(m = _pacman_pkg_getinfo(other_transpkg->pkg_new, PM_PKG_PROVIDES); m; m = m->next) {
						if(!strcmp(m->data, j->data)) {
							_pacman_depmissinglist_add (&baddeps, transpkg_name, PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, other_transpkg_name, NULL);
						}
					}
				}
			}
		}
		/* CHECK 3: check database against targets */
		_pacman_log(PM_LOG_DEBUG, _("checkconflicts: db vs targ '%s'"), transpkg_name);
		for(k = _pacman_db_get_pkgcache(trans->handle->db_local); k; k = k->next) {
			pmlist_t *conflicts = NULL;
			int usenewconflicts = 0;

			info = k->data;
			if(!strcmp(info->name, transpkg_name)) {
				/* a package cannot conflict with itself -- that's just not nice */
				continue;
			}
			/* If this package (*info) is also in our packages pmlist_t, use the
			 * conflicts list from the new package, not the old one (*info)
			 */
			f_foreach (j, trans->packages) {
				pmtranspkg_t *transpkg = j->data;

				if (transpkg->type & PM_TRANS_TYPE_ADD &&
						!strcmp(transpkg->pkg_new->name, info->name)) {
					/* Use the new, to-be-installed package's conflicts */
					conflicts = _pacman_pkg_getinfo(transpkg->pkg_new, PM_PKG_CONFLICTS);
					usenewconflicts = 1;
				}
			}
			if(!usenewconflicts) {
				/* Use the old package's conflicts, it's the only set we have */
				conflicts = _pacman_pkg_getinfo(info, PM_PKG_CONFLICTS);
			}
			f_foreach (j, conflicts) {
				if(!strcmp((char *)j->data, transpkg_name)) {
					_pacman_depmissinglist_add (&baddeps, transpkg_name, PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, info->name, NULL);
				} else {
					/* see if the db package conflicts with something we provide */
					pmlist_t *m;

					f_foreach (m, conflicts) {
						pmlist_t *n;
						for(n = _pacman_pkg_getinfo(transpkg->pkg_new, PM_PKG_PROVIDES); n; n = n->next) {
							if(!strcmp(m->data, n->data)) {
								_pacman_depmissinglist_add (&baddeps, transpkg_name, PM_DEP_TYPE_CONFLICT, PM_DEP_MOD_ANY, info->name, NULL);
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
		const char *strA = pA->data;
		const char *strB = pB->data;
		/* skip directories, we don't care about dir conflicts */
		if(strA[strlen(strA)-1] == '/') {
			pA = pA->next;
		} else if(strB[strlen(strB)-1] == '/') {
			pB = pB->next;
		} else {
			int cmp = strcmp(strA, strB);
			if(cmp < 0) {
				/* item only in filesA, ignore it */
				pA = pA->next;
			} else if(cmp > 0) {
				/* item only in filesB, ignore it */
				pB = pB->next;
			} else {
				/* item in both, record it */
				ret = _pacman_list_add(ret, strdup(strA));
				pA = pA->next;
				pB = pB->next;
			}
	  }
	}
	return(ret);
}

pmlist_t *_pacman_db_find_conflicts(pmtrans_t *trans, pmlist_t **skip_list)
{
	pmlist_t *i, *j, *k;
	char path[PATH_MAX+1];
	struct stat buf;
	pmlist_t *conflicts = NULL;
	pmpkg_t *dbpkg;
	double percent;
	int remain;
	pmdb_t *db = trans->handle->db_local;
	int howmany = f_ptrlist_count (trans->packages);

	if (howmany == 0) {
		return(NULL);
	}

	/* CHECK 1: check every target against every target */
	f_foreach (i, trans->packages) {
		pmtranspkg_t *p1 = i->data;

		if (p1->pkg_new == NULL) {
			continue;
		}

		remain = f_ptrlist_count (i);
		percent = (double)(howmany - remain + 1) / howmany;
		PROGRESS(trans, PM_TRANS_PROGRESS_CONFLICTS_START, "", (percent * 100), howmany, howmany - remain + 1);
		for(j = i->next; j; j = j->next) {
			pmtranspkg_t *p2 = j->data;
			if (p2->pkg_new != NULL) {
				pmlist_t *ret = chk_fileconflicts(p1->pkg_new->files, p2->pkg_new->files);

				f_foreach (k, ret) {
						pmconflict_t *conflict = _pacman_malloc (sizeof (*conflict));

						if(conflict == NULL) {
							continue;
						}
						conflict->type = PM_CONFLICT_TYPE_TARGET;
						STRNCPY(conflict->target, p1->pkg_new->name, PKG_NAME_LEN);
						STRNCPY(conflict->file, k->data, CONFLICT_FILE_LEN);
						STRNCPY(conflict->ctarget, p2->pkg_new->name, PKG_NAME_LEN);
						conflicts = _pacman_list_add(conflicts, conflict);
				}
				FREELIST(ret);
			}
		}

		/* CHECK 2: check every target against the filesystem */
		dbpkg = NULL;
		f_foreach (j, p1->pkg_new->files) {
			char *filestr = (char*)j->data;

			snprintf(path, PATH_MAX, "%s%s", trans->handle->root, filestr);
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
						dbpkg = _pacman_db_get_pkgfromcache(db, p1->pkg_new->name);
					}
					if(dbpkg && !(dbpkg->infolevel & INFRQ_FILES)) {
						_pacman_log(PM_LOG_DEBUG, _("loading FILES info for '%s'"), dbpkg->name);
						_pacman_db_read(db, INFRQ_FILES, dbpkg);
					}
					if(dbpkg && f_stringlist_find (dbpkg->files, j->data)) {
						ok = 1;
					}
					/* Check if the conflicting file has been moved to another package/target */
					if(!ok) {
						/* Look at all the targets */
						f_foreach (k, trans->packages) {
							pmtranspkg_t *p2 = k->data;

							/* As long as they're not the current package */
							if(p2->pkg_new && p2 != p1) {
								if(p2->pkg_local && !(p2->pkg_local->infolevel & INFRQ_FILES)) {
									_pacman_log(PM_LOG_DEBUG, _("loading FILES info for '%s'"), p2->pkg_local->name);
									_pacman_db_read(db, INFRQ_FILES, p2->pkg_local);
								}
								/* If it used to exist in there, but doesn't anymore */
								if (p2->pkg_local &&
										!f_stringlist_find (p2->pkg_new->files, filestr) &&
										f_stringlist_find (p2->pkg_local->files, filestr)) {
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
									*skip_list = _pacman_list_add(*skip_list, strdup(filestr));
									ok = 1;
									break;
								}
							}
						}
					}
				}
				if(!ok) {
					pmconflict_t *conflict = _pacman_malloc (sizeof (*conflict));

					if(conflict == NULL) {
						continue;
					}
					conflict->type = PM_CONFLICT_TYPE_FILE;
					STRNCPY(conflict->target, p1->pkg_new->name, PKG_NAME_LEN);
					STRNCPY(conflict->file, filestr, CONFLICT_FILE_LEN);
					conflict->ctarget[0] = 0;
					conflicts = _pacman_list_add(conflicts, conflict);
				}
			}
		}
	}

	return(conflicts);
}

/* vim: set ts=2 sw=2 noet: */
