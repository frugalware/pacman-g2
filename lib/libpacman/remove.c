/*
 *  remove.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2006 by David Kimpe <dnaku@frugalware.org>
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

#if defined(__APPLE__) || defined(__OpenBSD__)
#include <sys/syslimits.h>
#endif
#if defined(__APPLE__) || defined(__OpenBSD__) || defined(__sun__)
#include <sys/stat.h>
#endif

#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <zlib.h>

/* pacman-g2 */
#include "remove.h"

#include "trans.h"
#include "util.h"
#include "error.h"
#include "deps.h"
#include "versioncmp.h"
#include "md5.h"
#include "sha1.h"
#include "log.h"
#include "backup.h"
#include "package.h"
#include "db.h"
#include "cache.h"
#include "provide.h"
#include "handle.h"
#include "pacman.h"
#include "packages_transaction.h"

int _pacman_remove_commit(pmtrans_t *trans, pmlist_t **data)
{
	pmpkg_t *info;
	struct stat buf;
	pmlist_t *targ, *lp;
	char line[PATH_MAX+1];
	int howmany, remain;
	pmdb_t *db = trans->handle->db_local;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	howmany = _pacman_list_count(trans->packages);

	for(targ = trans->packages; targ; targ = targ->next) {
		int position = 0;
		char pm_install[PATH_MAX];
		info = ((pmtranspkg_t*)targ->data)->pkg_local;

		if(handle->trans->state == STATE_INTERRUPTED) {
			break;
		}

		remain = _pacman_list_count(targ);

		if(trans->type != PM_TRANS_TYPE_UPGRADE) {
			EVENT(trans, PM_TRANS_EVT_REMOVE_START, info, NULL);
			_pacman_log(PM_LOG_FLOW1, _("removing package %s-%s"), info->name, info->version);

			/* run the pre-remove scriptlet if it exists */
			if(info->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
				snprintf(pm_install, PATH_MAX, "%s/%s-%s/install", db->path, info->name, info->version);
				_pacman_runscriptlet(handle->root, pm_install, "pre_remove", info->version, NULL, trans);
			}
		}

		if(!(trans->flags & PM_TRANS_FLAG_DBONLY)) {
			int filenum = _pacman_list_count(info->files);
			_pacman_log(PM_LOG_FLOW1, _("removing files"));

			/* iterate through the list backwards, unlinking files */
			for(lp = _pacman_list_last(info->files); lp; lp = lp->prev) {
				int nb = 0;
				double percent;
				char *file = lp->data;
				char *md5 =_pacman_needbackup(file, info->backup);
				char *sha1 =_pacman_needbackup(file, info->backup);

				if (position != 0) {
				percent = (double)position / filenum;
				}
				if(md5 && sha1) {
					nb = 1;
					FREE(md5);
					FREE(sha1);
				}
				if(!nb && trans->type == PM_TRANS_TYPE_UPGRADE) {
					/* check noupgrade */
					if(_pacman_strlist_find(handle->noupgrade, file)) {
						nb = 1;
					}
				}
				snprintf(line, PATH_MAX, "%s%s", handle->root, file);
				if(lstat(line, &buf)) {
					_pacman_log(PM_LOG_DEBUG, _("file %s does not exist"), file);
					continue;
				}
				if(S_ISDIR(buf.st_mode)) {
					if(rmdir(line)) {
						/* this is okay, other packages are probably using it. */
						_pacman_log(PM_LOG_DEBUG, _("keeping directory %s"), file);
					} else {
						_pacman_log(PM_LOG_FLOW2, _("removing directory %s"), file);
					}
				} else {
					/* check the "skip list" before removing the file.
					 * see the big comment block in db_find_conflicts() for an
					 * explanation. */
					int skipit = 0;
					pmlist_t *j;
					for(j = trans->skiplist; j; j = j->next) {
						if(!strcmp(file, (char*)j->data)) {
							skipit = 1;
						}
					}
					if(skipit) {
						_pacman_log(PM_LOG_FLOW2, _("skipping removal of %s as it has moved to another package"),
							file);
					} else {
						/* if the file is flagged, back it up to .pacsave */
						if(nb) {
							if(trans->type == PM_TRANS_TYPE_UPGRADE) {
								/* we're upgrading so just leave the file as is. pacman_add() will handle it */
							} else {
								if(!(trans->flags & PM_TRANS_FLAG_NOSAVE)) {
									char newpath[PATH_MAX];
									snprintf(newpath, PATH_MAX, "%s.pacsave", line);
									rename(line, newpath);
									_pacman_log(PM_LOG_WARNING, _("%s saved as %s"), line, newpath);
									pacman_logaction(_("%s saved as %s"), line, newpath);
								} else {
									_pacman_log(PM_LOG_FLOW2, _("unlinking %s"), file);
									if(unlink(line)) {
										_pacman_log(PM_LOG_ERROR, _("cannot remove file %s"), file);
									}
								}
							}
						} else {
							_pacman_log(PM_LOG_FLOW2, _("unlinking %s"), file);
							/* Need at here because we count only real unlinked files ? */
							PROGRESS(trans, PM_TRANS_PROGRESS_REMOVE_START, info->name, (int)(percent * 100), howmany, howmany - remain + 1);
							position++;
							if(unlink(line)) {
								_pacman_log(PM_LOG_ERROR, _("cannot remove file %s"), file);
							}
						}
					}
				}
			}
		}

		PROGRESS(trans, PM_TRANS_PROGRESS_REMOVE_START, info->name, 100, howmany, howmany - remain + 1);
		if(trans->type != PM_TRANS_TYPE_UPGRADE) {
			/* run the post-remove script if it exists */
			if(info->scriptlet && !(trans->flags & PM_TRANS_FLAG_NOSCRIPTLET)) {
				/* must run ldconfig here because some scriptlets fail due to missing libs otherwise */
				_pacman_ldconfig(handle->root);
				snprintf(pm_install, PATH_MAX, "%s/%s-%s/install", db->path, info->name, info->version);
				_pacman_runscriptlet(handle->root, pm_install, "post_remove", info->version, NULL, trans);
			}
		}

		/* remove the package from the database */
		_pacman_log(PM_LOG_FLOW1, _("updating database"));
		_pacman_log(PM_LOG_FLOW2, _("removing database entry '%s'"), info->name);
		if(_pacman_db_remove(db, info) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not remove database entry %s-%s"), info->name, info->version);
		}
		if(_pacman_db_remove_pkgfromcache(db, info) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not remove entry '%s' from cache"), info->name);
		}

		/* update dependency packages' REQUIREDBY fields */
		_pacman_log(PM_LOG_FLOW2, _("updating dependency packages 'requiredby' fields"));
		for(lp = info->depends; lp; lp = lp->next) {
			pmpkg_t *depinfo = NULL;
			pmdepend_t depend;
			char *data;
			if(_pacman_splitdep((char*)lp->data, &depend)) {
				continue;
			}
			/* if this dependency is in the transaction targets, no need to update
			 * its requiredby info: it is in the process of being removed (if not
			 * already done!)
			 */
			if(__pacman_trans_get_trans_pkg(trans, depend.name) != NULL) {
				continue;
			}
			depinfo = _pacman_db_get_pkgfromcache(db, depend.name);
			if(depinfo == NULL) {
				/* look for a provides package */
				pmlist_t *provides = _pacman_db_whatprovides(db, depend.name);
				if(provides) {
					/* TODO: should check _all_ packages listed in provides, not just
					 *			 the first one.
					 */
					/* use the first one */
					depinfo = _pacman_db_get_pkgfromcache(db, ((pmpkg_t *)provides->data)->name);
					FREELISTPTR(provides);
				}
				if(depinfo == NULL) {
					_pacman_log(PM_LOG_ERROR, _("could not find dependency '%s'"), depend.name);
					/* wtf */
					continue;
				}
			}
			/* splice out this entry from requiredby */
			depinfo->requiredby = _pacman_list_remove(_pacman_pkg_getinfo(depinfo, PM_PKG_REQUIREDBY), info->name, strcmp, (void **)&data);
			FREE(data);
			_pacman_log(PM_LOG_DEBUG, _("updating 'requiredby' field for package '%s'"), depinfo->name);
			if(_pacman_db_write(db, depinfo, INFRQ_DEPENDS)) {
				_pacman_log(PM_LOG_ERROR, _("could not update 'requiredby' database entry %s-%s"),
					depinfo->name, depinfo->version);
			}
		}

		if(trans->type != PM_TRANS_TYPE_UPGRADE) {
			EVENT(trans, PM_TRANS_EVT_REMOVE_DONE, info, NULL);
		}
	}

	/* run ldconfig if it exists */
	if((trans->type != PM_TRANS_TYPE_UPGRADE) && (handle->trans->state != STATE_INTERRUPTED)) {
		_pacman_ldconfig(handle->root);
	}

	return(0);
}

const pmtrans_ops_t _pacman_remove_pmtrans_opts = {
	.commit = _pacman_remove_commit
};

/* vim: set ts=2 sw=2 noet: */
