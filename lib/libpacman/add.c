/*
 *  add.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2006 by David Kimpe <dnaku@frugalware.org>
 *  Copyright (c) 2005, 2006 by Miklos Vajna <vmiklos@frugalware.org>
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
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* pacman-g2 */
#include "add.h"

#include "db/fakedb.h"
#include "hash/md5.h"
#include "hash/sha1.h"
#include "io/archive.h"
#include "util/list.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "trans.h"
#include "util.h"
#include "error.h"
#include "cache.h"
#include "deps.h"
#include "versioncmp.h"
#include "package.h"
#include "db.h"
#include "conflict.h"
#include "remove.h"
#include "handle.h"
#include "packages_transaction.h"

pmpkg_t *_pacman_filedb_load(pmdb_t *db, const char *name)
{
	struct stat buf;

	if(stat(name, &buf)) {
		pm_errno = PM_ERR_NOT_A_FILE;
		return NULL;
	}

	_pacman_log(PM_LOG_FLOW2, _("loading target '%s'"), name);
	return _pacman_pkg_load(name);
}

int _pacman_add_addtarget(pmtrans_t *trans, const char *name)
{
	pmlist_t *i;
	pmpkg_t *pkg_new, *pkg_local;
	pmdb_t *db_local = trans->handle->db_local;

	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(!_pacman_strempty(name), RET_ERR(PM_ERR_WRONG_ARGS, -1));

	/* Check if we need to add a fake target to the transaction. */
	if(strchr(name, '|')) {
		return(_pacman_fakedb_addtarget(trans, name));
	}

	pkg_new = _pacman_filedb_load(NULL, name);
	if(pkg_new == NULL || _pacman_pkg_is_valid(pkg_new, trans, name) != 0) {
		/* pm_errno is already set by _pacman_filedb_load() */
		goto error;
	}

	pkg_local = _pacman_db_get_pkgfromcache(db_local, pkg_new->name);
	if(trans->type != PM_TRANS_TYPE_UPGRADE) {
		/* only install this package if it is not already installed */
		if(pkg_local != NULL) {
			pm_errno = PM_ERR_PKG_INSTALLED;
			goto error;
		}
	} else {
		if(trans->flags & PM_TRANS_FLAG_FRESHEN) {
			/* only upgrade/install this package if it is already installed and at a lesser version */
			if(pkg_local == NULL || _pacman_versioncmp(pkg_local->version, pkg_new->version) >= 0) {
				pm_errno = PM_ERR_PKG_CANT_FRESH;
				goto error;
			}
		}
	}

	/* check if an older version of said package is already in transaction packages.
	 * if so, replace it in the list */
	for(i = trans->packages; i; i = i->next) {
		pmpkg_t *pkg = i->data;
		if(strcmp(pkg->name, pkg_new->name) == 0) {
			if(_pacman_versioncmp(pkg->version, pkg_new->version) < 0) {
				_pacman_log(PM_LOG_WARNING, _("replacing older version %s-%s by %s in target list"),
				          pkg->name, pkg->version, pkg_new->version);
				f_ptrswap(&i->data, (void **)&pkg_new);
			} else {
				_pacman_log(PM_LOG_WARNING, _("newer version %s-%s is in the target list -- skipping"),
				          pkg->name, pkg->version, pkg_new->version);
			}
			FREEPKG(pkg_new);
			return(0);
		}
	}

	if(trans->flags & PM_TRANS_FLAG_ALLDEPS) {
		pkg_new->reason = PM_PKG_REASON_DEPEND;
	}

	/* copy over the install reason */
	if(pkg_local) {
		pkg_new->reason = (long)_pacman_pkg_getinfo(pkg_local, PM_PKG_REASON);
	}

	/* add the package to the transaction */
	trans->packages = _pacman_list_add(trans->packages, pkg_new);

	return(0);

error:
	FREEPKG(pkg_new);
	return(-1);
}

int _pacman_add_prepare(pmtrans_t *trans, pmlist_t **data)
{
	pmlist_t *lp;
	pmdb_t *db_local = trans->handle->db_local;

	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	/* Check dependencies
	 */
	if(!(trans->flags & PM_TRANS_FLAG_NODEPS)) {
		EVENT(trans, PM_TRANS_EVT_CHECKDEPS_START, NULL, NULL);

		/* look for unsatisfied dependencies */
		_pacman_log(PM_LOG_FLOW1, _("looking for unsatisfied dependencies"));
		lp = _pacman_checkdeps(trans, trans->type, trans->packages);
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
		lp = _pacman_checkconflicts(trans, trans->packages);
		if(lp != NULL) {
			if(data) {
				*data = lp;
			} else {
				FREELIST(lp);
			}
			RET_ERR(PM_ERR_CONFLICTING_DEPS, -1);
		}

		/* re-order w.r.t. dependencies */
		_pacman_log(PM_LOG_FLOW1, _("sorting by dependencies"));
		lp = _pacman_sortbydeps(trans->packages, PM_TRANS_TYPE_ADD);
		/* free the old alltargs */
		FREELISTPTR(trans->packages);
		trans->packages = lp;

		EVENT(trans, PM_TRANS_EVT_CHECKDEPS_DONE, NULL, NULL);
	}

	/* Cleaning up
	 */
	EVENT(trans, PM_TRANS_EVT_CLEANUP_START, NULL, NULL);
	_pacman_log(PM_LOG_FLOW1, _("cleaning up"));
	for (lp=trans->packages; lp!=NULL; lp=lp->next) {
		pmpkg_t *pkg_new=(pmpkg_t *)lp->data;
		pmlist_t *rmlist;

		for (rmlist=pkg_new->removes; rmlist!=NULL; rmlist=rmlist->next) {
			char rm_fname[PATH_MAX];

			snprintf(rm_fname, PATH_MAX, "%s%s", handle->root, (char *)rmlist->data);
			remove(rm_fname);
		}
	}
	EVENT(trans, PM_TRANS_EVT_CLEANUP_DONE, NULL, NULL);

	/* Check for file conflicts
	 */
	if(!(trans->flags & PM_TRANS_FLAG_FORCE)) {
		pmlist_t *skiplist = NULL;

		EVENT(trans, PM_TRANS_EVT_FILECONFLICTS_START, NULL, NULL);

		_pacman_log(PM_LOG_FLOW1, _("looking for file conflicts"));
		lp = _pacman_db_find_conflicts(trans, handle->root, &skiplist);
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

	return(0);
}

int _pacman_add_commit(pmtrans_t *trans, pmlist_t **data)
{
	int i, ret = 0;
	int remain, howmany, archive_ret;
	double percent;
	char expath[PATH_MAX], cwd[PATH_MAX] = "";
	unsigned char cb_state;
	time_t t;
	pmlist_t *targ, *lp;
	pmdb_t *db_local = trans->handle->db_local;

	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	howmany = _pacman_list_count(trans->packages);
	for(targ = trans->packages; targ; targ = targ->next) {
		unsigned short pmo_upgrade;
		char pm_install[PATH_MAX];
		pmpkg_t *pkg_new = (pmpkg_t *)targ->data;
		pmpkg_t *oldpkg = NULL;
		remain = _pacman_list_count(targ);

		if(handle->trans->state == STATE_INTERRUPTED) {
			break;
		}

		pmo_upgrade = (trans->type == PM_TRANS_TYPE_UPGRADE) ? 1 : 0;

		/* see if this is an upgrade.  if so, remove the old package first */
		if(pmo_upgrade) {
			pmpkg_t *pkg_local = _pacman_db_get_pkgfromcache(db_local, pkg_new->name);
			if(pkg_local) {
				EVENT(trans, PM_TRANS_EVT_UPGRADE_START, pkg_new, NULL);
				cb_state = PM_TRANS_PROGRESS_UPGRADE_START;
				_pacman_log(PM_LOG_FLOW1, _("upgrading package %s-%s"), pkg_new->name, pkg_new->version);

				/* we'll need to save some record for backup checks later */
				oldpkg = _pacman_pkg_new(pkg_local->name, pkg_local->version);
				if(oldpkg) {
					if(!(pkg_local->infolevel & INFRQ_FILES)) {
						_pacman_log(PM_LOG_DEBUG, _("loading FILES info for '%s'"), pkg_local->name);
						_pacman_db_read(db_local, pkg_local, INFRQ_FILES);
					}
					oldpkg->backup = _pacman_list_strdup(pkg_local->backup);
					strncpy(oldpkg->name, pkg_local->name, PKG_NAME_LEN);
					strncpy(oldpkg->version, pkg_local->version, PKG_VERSION_LEN);
				}

				/* pre_upgrade scriptlet */
				if(pkg_new->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
					_pacman_runscriptlet(handle->root, pkg_new->data, "pre_upgrade", pkg_new->version, oldpkg ? oldpkg->version : NULL,
						trans);
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
					if(_pacman_remove_addtarget(tr, pkg_new->name) == -1) {
						FREETRANS(tr);
						RET_ERR(PM_ERR_TRANS_ABORT, -1);
					}
					/* copy the skiplist over */
					tr->skiplist = _pacman_list_strdup(trans->skiplist);
					if(_pacman_remove_commit(tr, NULL) == -1) {
						FREETRANS(tr);
						RET_ERR(PM_ERR_TRANS_ABORT, -1);
					}
					FREETRANS(tr);
				}
			} else {
				/* no previous package version is installed, so this is actually
				 * just an install.  */
				pmo_upgrade = 0;
			}
		}
		if(!pmo_upgrade) {
			EVENT(trans, PM_TRANS_EVT_ADD_START, pkg_new, NULL);
			cb_state = PM_TRANS_PROGRESS_ADD_START;
			_pacman_log(PM_LOG_FLOW1, _("adding package %s-%s"), pkg_new->name, pkg_new->version);

			/* pre_install scriptlet */
			if(pkg_new->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
				_pacman_runscriptlet(handle->root, pkg_new->data, "pre_install", pkg_new->version, NULL, trans);
			}
		} else {
			_pacman_log(PM_LOG_FLOW1, _("adding new package %s-%s"), pkg_new->name, pkg_new->version);
		}

		if(!(trans->flags & PM_TRANS_FLAG_DBONLY)) {
			int errors = 0;
			register struct archive *archive;
			struct archive_entry *entry;

			_pacman_log(PM_LOG_FLOW1, _("extracting files"));

			/* Extract the package */
			if((archive = _pacman_archive_read_open_all_file(pkg_new->data)) == NULL) {
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
				int nb = 0, notouch = 0;
				char *hash_orig = NULL;
				struct stat buf;
				char pathname[PATH_MAX];

				STRNCPY(pathname, archive_entry_pathname (entry), PATH_MAX);

				if (pkg_new->size != 0)
					percent = (double)archive_position_uncompressed(archive) / pkg_new->size;
				PROGRESS(trans, cb_state, pkg_new->name, (int)(percent * 100), howmany, (howmany - remain +1));

				if(!strcmp(pathname, ".PKGINFO") || !strcmp(pathname, ".FILELIST")) {
					archive_read_data_skip (archive);
					continue;
				}

				/*if(!strcmp(pathname, "._install") || !strcmp(pathname, ".INSTALL")) {
				*	 the install script goes inside the db
				*	snprintf(expath, PATH_MAX, "%s/%s-%s/install", db_local->path, pkg_new->name, pkg_new->version); */
				if(!strcmp(pathname, "._install") || !strcmp(pathname, ".INSTALL") ||
					!strcmp(pathname, ".CHANGELOG")) {
					if(!strcmp(pathname, ".CHANGELOG")) {
						/* the changelog goes inside the db */
						snprintf(expath, PATH_MAX, "%s/%s-%s/changelog", db_local->path,
							pkg_new->name, pkg_new->version);
					} else {
						/* the install script goes inside the db */
						snprintf(expath, PATH_MAX, "%s/%s-%s/install", db_local->path,
							pkg_new->name, pkg_new->version);
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
				if(_pacman_list_is_strin(pathname, handle->noextract)) {
					pacman_logaction(_("notice: %s is in NoExtract -- skipping extraction"), pathname);
					archive_read_data_skip (archive);
					continue;
				}

				if(!lstat(expath, &buf)) {
					/* file already exists */
					if(S_ISLNK(buf.st_mode)) {
						continue;
					} else if(!S_ISDIR(buf.st_mode)) {
						if(_pacman_list_is_strin(pathname, handle->noupgrade)) {
							notouch = 1;
						} else {
							if(!pmo_upgrade || oldpkg == NULL) {
								nb = _pacman_list_is_strin(pathname, pkg_new->backup);
							} else {
								/* op == PM_TRANS_TYPE_UPGRADE */
								hash_orig = _pacman_pkg_fileneedbackup(oldpkg, pathname);
								if(!_pacman_strempty(hash_orig)) {
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
					/* append the new md5 or sha1 hash to it's respective entry in pkg_new->backup
					 * (it will be the new orginal)
					 */
					for(lp = pkg_new->backup; lp; lp = lp->next) {
						char *fn;
						char *file = lp->data;

						if(!file) continue;
						if(!strcmp(file, pathname)) {
							if(!_pacman_strempty(pkg_new->sha1sum)) {
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

					if (!_pacman_strempty(pkg_new->sha1sum)) {
						_pacman_log(PM_LOG_DEBUG, _("checking md5 hashes for %s"), pathname);
						_pacman_log(PM_LOG_DEBUG, _("current:  %s"), md5_local);
						_pacman_log(PM_LOG_DEBUG, _("new:      %s"), md5_pkg);
					} else {	
						_pacman_log(PM_LOG_DEBUG, _("checking sha1 hashes for %s"), pathname);
						_pacman_log(PM_LOG_DEBUG, _("current:  %s"), sha1_local);
						_pacman_log(PM_LOG_DEBUG, _("new:      %s"), sha1_pkg);
					}
					if(!_pacman_strempty(hash_orig)) {
						_pacman_log(PM_LOG_DEBUG, _("original: %s"), hash_orig);
					}

					if(!pmo_upgrade) {
						/* PM_ADD */

						/* if a file already exists with a different md5 or sha1 hash,
						 * then we rename it to a .pacorig extension and continue */
						if(strcmp(md5_local, md5_pkg) || strcmp(sha1_local, sha1_pkg)) {
							char newpath[PATH_MAX];
							snprintf(newpath, PATH_MAX, "%s.pacorig", expath);
							if(rename(expath, newpath)) {
								archive_entry_set_pathname (entry, expath);
								_pacman_log(PM_LOG_ERROR, _("could not rename %s (%s)"), pathname, strerror(errno));
							}
							if(_pacman_copyfile(temp, expath)) {
								archive_entry_set_pathname (entry, expath);
								_pacman_log(PM_LOG_ERROR, _("could not copy %s to %s (%s)"), temp, pathname, strerror(errno));
								errors++;
							} else {
								archive_entry_set_pathname (entry, expath);
								_pacman_log(PM_LOG_WARNING, _("%s saved as %s.pacorig"), pathname, pathname);
							}
						}
					} else if(!_pacman_strempty(hash_orig)) {
						/* PM_UPGRADE */
						int installnew = 0;

						/* the fun part */
						if(!strcmp(hash_orig, md5_local) || !strcmp(hash_orig, sha1_local)) {
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
							} else {
								_pacman_log(PM_LOG_WARNING, _("%s installed as %s"), expath, newpath);
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

					FREE(md5_local);
					FREE(md5_pkg);
					FREE(sha1_local);
					FREE(sha1_pkg);
					FREE(hash_orig);
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
						errors++;
					}
					/* calculate an md5 or sha1 hash if this is in pkg_new->backup */
					for(lp = pkg_new->backup; lp; lp = lp->next) {
						char *fn, *md5, *sha1;
						char path[PATH_MAX];
						char *file = lp->data;

						if(!file) continue;
						if(!strcmp(file, pathname)) {
							_pacman_log(PM_LOG_DEBUG, _("appending backup entry"));
							snprintf(path, PATH_MAX, "%s%s", handle->root, file);
							if (!_pacman_strempty(pkg_new->sha1sum)) {
								md5 = _pacman_MDFile(path);
								/* 32 for the hash, 1 for the terminating NULL, and 1 for the tab delimiter */
								if((fn = (char *)malloc(strlen(file)+34)) == NULL) {
									RET_ERR(PM_ERR_MEMORY, -1);
								}
								sprintf(fn, "%s\t%s", file, md5);
								FREE(md5);
							} else {
								/* 41 for the hash, 1 for the terminating NULL, and 1 for the tab delimiter */
								sha1 = _pacman_SHAFile(path);
								if((fn = (char *)malloc(strlen(file)+43)) == NULL) {
									RET_ERR(PM_ERR_MEMORY, -1);
								}
								sprintf(fn, "%s\t%s", file, sha1);
								FREE(sha1);
							}
							FREE(file);
							lp->data = fn;
						}
					}
				}
			}
			if(archive_ret == ARCHIVE_FATAL) {
				errors++;
			}
			if(!_pacman_strempty(cwd)) {
				chdir(cwd);
			}
			archive_read_finish (archive);

			if(errors) {
				ret = 1;
				_pacman_log(PM_LOG_WARNING, _("errors occurred while %s %s"),
					(pmo_upgrade ? _("upgrading") : _("installing")), pkg_new->name);
			} else {
			PROGRESS(trans, cb_state, pkg_new->name, 100, howmany, howmany - remain + 1);
			}
		}

		/* Add the package to the database */
		t = time(NULL);

		/* Update the requiredby field by scanning the whole database
		 * looking for packages depending on the package to add */
		for(lp = _pacman_db_get_pkgcache(db_local); lp; lp = lp->next) {
			pmpkg_t *tmpp = lp->data;
			pmlist_t *tmppm = NULL;
			if(tmpp == NULL) {
				continue;
			}
			for(tmppm = tmpp->depends; tmppm; tmppm = tmppm->next) {
				pmdepend_t depend;
				if(_pacman_splitdep(tmppm->data, &depend)) {
					continue;
				}
				if(tmppm->data && (!strcmp(depend.name, pkg_new->name) || _pacman_list_is_strin(depend.name, pkg_new->provides))) {
					_pacman_log(PM_LOG_DEBUG, _("adding '%s' in requiredby field for '%s'"), tmpp->name, pkg_new->name);
					pkg_new->requiredby = _pacman_stringlist_append(pkg_new->requiredby, tmpp->name);
				}
			}
		}

		/* make an install date (in UTC) */
		STRNCPY(pkg_new->installdate, asctime(gmtime(&t)), sizeof(pkg_new->installdate));
		/* remove the extra line feed appended by asctime() */
		pkg_new->installdate[strlen(pkg_new->installdate)-1] = 0;

		_pacman_log(PM_LOG_FLOW1, _("updating database"));
		_pacman_log(PM_LOG_FLOW2, _("adding database entry '%s'"), pkg_new->name);
		if(_pacman_db_write(db_local, pkg_new, INFRQ_ALL)) {
			_pacman_log(PM_LOG_ERROR, _("error updating database for %s-%s!"),
			          pkg_new->name, pkg_new->version);
			RET_ERR(PM_ERR_DB_WRITE, -1);
		}
		if(_pacman_db_add_pkgincache(db_local, pkg_new) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not add entry '%s' in cache"), pkg_new->name);
		}

		/* update dependency packages' REQUIREDBY fields */
		if(pkg_new->depends) {
			_pacman_log(PM_LOG_FLOW2, _("updating dependency packages 'requiredby' fields"));
		}
		for(lp = pkg_new->depends; lp; lp = lp->next) {
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
			_pacman_log(PM_LOG_DEBUG, _("adding '%s' in requiredby field for '%s'"), pkg_new->name, depinfo->name);
			depinfo->requiredby = _pacman_stringlist_append(_pacman_pkg_getinfo(depinfo, PM_PKG_REQUIREDBY), pkg_new->name);
			if(_pacman_db_write(db_local, depinfo, INFRQ_DEPENDS)) {
				_pacman_log(PM_LOG_ERROR, _("could not update 'requiredby' database entry %s-%s"),
				          depinfo->name, depinfo->version);
			}
		}

		EVENT(trans, PM_TRANS_EVT_EXTRACT_DONE, NULL, NULL);

		/* run the post-install script if it exists  */
		if(pkg_new->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
			/* must run ldconfig here because some scriptlets fail due to missing libs otherwise */
			_pacman_ldconfig(handle->root);
			snprintf(pm_install, PATH_MAX, "%s%s/%s/%s-%s/install", handle->root, handle->dbpath, db_local->treename, pkg_new->name, pkg_new->version);
			if(pmo_upgrade) {
				_pacman_runscriptlet(handle->root, pm_install, "post_upgrade", pkg_new->version, oldpkg ? oldpkg->version : NULL, trans);
			} else {
				_pacman_runscriptlet(handle->root, pm_install, "post_install", pkg_new->version, NULL, trans);
			}
		}

		EVENT(trans, (pmo_upgrade) ? PM_TRANS_EVT_UPGRADE_DONE : PM_TRANS_EVT_ADD_DONE, pkg_new, oldpkg);

		FREEPKG(oldpkg);
	}

	/* run ldconfig if it exists */
	if(handle->trans->state != STATE_INTERRUPTED) {
		_pacman_ldconfig(handle->root);
	}

	return(ret);
}

const pmtrans_ops_t _pacman_add_pmtrans_opts = {
	.addtarget = _pacman_add_addtarget,
	.prepare = _pacman_add_prepare,
	.commit = _pacman_add_commit
};

/* vim: set ts=2 sw=2 noet: */
