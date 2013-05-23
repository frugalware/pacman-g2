/*
 *  sync.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2005-2008 by Miklos Vajna <vmiklos@frugalware.org>
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
#include <time.h>
#ifdef CYGWIN
#include <limits.h> /* PATH_MAX */
#endif
#include <dirent.h>

/* pacman-g2 */
#include "sync.h"

#include "log.h"
#include "error.h"
#include "package.h"
#include "db.h"
#include "cache.h"
#include "deps.h"
#include "conflict.h"
#include "provide.h"
#include "trans.h"
#include "util.h"
#include "versioncmp.h"
#include "handle.h"
#include "util.h"
#include "pacman.h"
#include "md5.h"
#include "sha1.h"
#include "handle.h"
#include "server.h"
#include "packages_transaction.h"

#include "fstringlist.h"

int _pacman_sync_commit(pmtrans_t *trans, pmlist_t **data)
{
	pmlist_t *i, *j;
	pmtrans_t *tr = NULL;
	int replaces = 0;
	pmdb_t *db_local = trans->handle->db_local;

	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	if(handle->sysupgrade) {
		_pacman_runhook("pre_sysupgrade", trans);
	}
	/* remove conflicting and to-be-replaced packages */
	tr = _pacman_trans_new();
	if(tr == NULL) {
		_pacman_log(PM_LOG_ERROR, _("could not create removal transaction"));
		goto error;
	}

	if(_pacman_trans_init(tr, PM_TRANS_TYPE_REMOVE, PM_TRANS_FLAG_NODEPS, trans->cbs) == -1) {
		_pacman_log(PM_LOG_ERROR, _("could not initialize the removal transaction"));
		goto error;
	}

	f_foreach (i, trans->packages) {
		pmsyncpkg_t *ps = i->data;

		f_foreach (j, ps->replaces) {
			pmpkg_t *pkg = j->data;
			if(__pacman_trans_get_trans_pkg(tr, pkg->name) == NULL) {
				if(_pacman_trans_addtarget(tr, pkg->name, PM_TRANS_TYPE_REMOVE, 0) == -1) {
					goto error;
				}
				replaces++;
			}
		}
	}
	if(replaces) {
		_pacman_log(PM_LOG_FLOW1, _("removing conflicting and to-be-replaced packages"));
		if(_pacman_trans_prepare(tr, data) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not prepare removal transaction"));
			goto error;
		}
		/* we want the frontend to be aware of commit details */
		tr->cbs.event = trans->cbs.event;
		if(_pacman_trans_commit(tr, NULL) == -1) {
			_pacman_log(PM_LOG_ERROR, _("could not commit removal transaction"));
			goto error;
		}
	}
	FREETRANS(tr);

	/* install targets */
	_pacman_log(PM_LOG_FLOW1, _("installing packages"));
	tr = _pacman_trans_new();
	if(tr == NULL) {
		_pacman_log(PM_LOG_ERROR, _("could not create transaction"));
		goto error;
	}
	if(_pacman_trans_init(tr, PM_TRANS_TYPE_UPGRADE, trans->flags | PM_TRANS_FLAG_NODEPS, trans->cbs) == -1) {
		_pacman_log(PM_LOG_ERROR, _("could not initialize transaction"));
		goto error;
	}
	f_foreach (i, trans->packages) {
		pmsyncpkg_t *ps = i->data;
		pmpkg_t *spkg = ps->pkg_new;
		char str[PATH_MAX];
		snprintf(str, PATH_MAX, "%s%s/%s-%s-%s" PM_EXT_PKG, handle->root, handle->cachedir, spkg->name, spkg->version, spkg->arch);
		if(_pacman_trans_addtarget(tr, str, PM_TRANS_TYPE_UPGRADE, ps->flags) == -1) {
			goto error;
		}
	}
	if(_pacman_trans_prepare(tr, data) == -1) {
		_pacman_log(PM_LOG_ERROR, _("could not prepare transaction"));
		/* pm_errno is set by trans_prepare */
		goto error;
	}
	if(_pacman_trans_commit(tr, NULL) == -1) {
		_pacman_log(PM_LOG_ERROR, _("could not commit transaction"));
		goto error;
	}
	FREETRANS(tr);

	/* propagate replaced packages' requiredby fields to their new owners */
	if(replaces) {
		_pacman_log(PM_LOG_FLOW1, _("updating database for replaced packages' dependencies"));
		f_foreach (i, trans->packages) {
			pmsyncpkg_t *ps = i->data;
			if (ps->replaces != NULL) {
				pmpkg_t *new = _pacman_db_get_pkgfromcache(db_local, ps->pkg_new->name);
				f_foreach (j, ps->replaces) {
					pmlist_t *k;
					pmpkg_t *old = j->data;
					/* merge lists */
					f_foreach (k, old->requiredby) {
						if(!f_stringlist_find (new->requiredby, k->data)) {
							/* replace old's name with new's name in the requiredby's dependency list */
							pmlist_t *m;
							pmpkg_t *depender = _pacman_db_get_pkgfromcache(db_local, k->data);
							if(depender == NULL) {
								/* If the depending package no longer exists in the local db,
								 * then it must have ALSO conflicted with ps->pkg.  If
								 * that's the case, then we don't have anything to propagate
								 * here. */
								continue;
							}
							f_foreach (m, depender->depends) {
								if(!strcmp(m->data, old->name)) {
									FREE(m->data);
									m->data = strdup(new->name);
								}
							}
							if(_pacman_db_write(db_local, depender, INFRQ_DEPENDS) == -1) {
								_pacman_log(PM_LOG_ERROR, _("could not update requiredby for database entry %s-%s"),
								          new->name, new->version);
							}
							/* add the new requiredby */
							new->requiredby = f_stringlist_append (new->requiredby, k->data);
						}
					}
				}
				if(_pacman_db_write(db_local, new, INFRQ_DEPENDS) == -1) {
					_pacman_log(PM_LOG_ERROR, _("could not update new database entry %s-%s"),
					          new->name, new->version);
				}
			}
		}
	}

	if(handle->sysupgrade) {
		_pacman_runhook("post_sysupgrade", trans);
	}
	return(0);

error:
	FREETRANS(tr);
	/* commiting failed, so this is still just a prepared transaction */
	return(-1);
}

static
int _pacman_trans_check_integrity (pmtrans_t *trans, pmlist_t **data) {
	int retval = 0;
	pmlist_t *i;

	if(trans->flags & PM_TRANS_FLAG_NOINTEGRITY) {
		return 0;
	}

	EVENT(trans, PM_TRANS_EVT_INTEGRITY_START, NULL, NULL);

	f_foreach (i, trans->packages) {
		pmsyncpkg_t *ps = i->data;
		pmpkg_t *spkg = ps->pkg_new;
		char str[PATH_MAX], pkgname[PATH_MAX];
		char *md5sum1, *md5sum2, *sha1sum1, *sha1sum2;
		char *ptr=NULL;

		_pacman_pkg_filename(pkgname, sizeof(pkgname), spkg);
		md5sum1 = spkg->md5sum;
		sha1sum1 = spkg->sha1sum;

		if((md5sum1 == NULL) && (sha1sum1 == NULL)) {
			if((ptr = (char *)malloc(512)) == NULL) {
				RET_ERR(PM_ERR_MEMORY, -1);
			}
			snprintf(ptr, 512, _("can't get md5 or sha1 checksum for package %s\n"), pkgname);
			*data = _pacman_list_add(*data, ptr);
			retval = 1;
			continue;
		}
		snprintf(str, PATH_MAX, "%s/%s/%s", handle->root, handle->cachedir, pkgname);
		md5sum2 = _pacman_MDFile(str);
		sha1sum2 = _pacman_SHAFile(str);
		if(md5sum2 == NULL && sha1sum2 == NULL) {
			if((ptr = (char *)malloc(512)) == NULL) {
				RET_ERR(PM_ERR_MEMORY, -1);
			}
			snprintf(ptr, 512, _("can't get md5 or sha1 checksum for package %s\n"), pkgname);
			*data = _pacman_list_add(*data, ptr);
			retval = 1;
			continue;
		}
		if((strcmp(md5sum1, md5sum2) != 0) && (strcmp(sha1sum1, sha1sum2) != 0)) {
			int doremove = 0;

			_pacman_log(PM_LOG_DEBUG, _("expected md5:  '%s'"), md5sum1);
			_pacman_log(PM_LOG_DEBUG, _("actual md5:    '%s'"), md5sum2);
			_pacman_log(PM_LOG_DEBUG, _("expected sha1: '%s'"), sha1sum1);
			_pacman_log(PM_LOG_DEBUG, _("actual sha1:   '%s'"), sha1sum2);

			if((ptr = (char *)malloc(512)) == NULL) {
				RET_ERR(PM_ERR_MEMORY, -1);
			}
			if(trans->flags & PM_TRANS_FLAG_ALLDEPS) {
				doremove=1;
			} else {
				QUESTION(trans, PM_TRANS_CONV_CORRUPTED_PKG, pkgname, NULL, NULL, &doremove);
			}
			if(doremove) {
				snprintf(str, PATH_MAX, "%s%s/%s-%s-%s" PM_EXT_PKG, handle->root, handle->cachedir, spkg->name, spkg->version, spkg->arch);
				unlink(str);
				snprintf(ptr, 512, _("archive %s was corrupted (bad MD5 or SHA1 checksum)\n"), pkgname);
			} else {
				snprintf(ptr, 512, _("archive %s is corrupted (bad MD5 or SHA1 checksum)\n"), pkgname);
			}
			*data = _pacman_list_add(*data, ptr);
			retval = 1;
		}
		FREE(md5sum2);
		FREE(sha1sum2);
	}
	if(retval == 0) {
		EVENT(trans, PM_TRANS_EVT_INTEGRITY_DONE, NULL, NULL);
	}
	return retval;
}

int _pacman_trans_download_commit(pmtrans_t *trans, pmlist_t **data)
{
	pmlist_t *i, *j, *files = NULL;
	char ldir[PATH_MAX];
	int retval, tries = 0;
	int varcache = 1;

//	ASSERT(db_local != NULL, RET_ERR(PM_ERR_DB_NULL, -1));
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	trans->state = STATE_DOWNLOADING;
	/* group sync records by repository and download */
	snprintf(ldir, PATH_MAX, "%s%s", handle->root, handle->cachedir);

	for(tries = 0; tries < handle->maxtries; tries++) {
		retval = 0;
		FREELIST(*data);
		int done = 1;
		f_foreach (i, handle->dbs_sync) {
			struct stat buf;
			pmdb_t *current = i->data;

			f_foreach (j, trans->packages) {
				pmsyncpkg_t *ps = j->data;
				pmpkg_t *spkg = ps->pkg_new;
				pmdb_t *dbs = spkg->db;

				if(current == dbs) {
					char filename[PATH_MAX];
					char lcpath[PATH_MAX];
					_pacman_pkg_filename(filename, sizeof(filename), spkg);
					snprintf(lcpath, sizeof(lcpath), "%s/%s", ldir, filename);

					if(trans->flags & PM_TRANS_FLAG_PRINTURIS) {
						if (!(trans->flags & PM_TRANS_FLAG_PRINTURIS_CACHED)) {
							if (stat(lcpath, &buf) == 0) {
								continue;
							}
						}

						EVENT(trans, PM_TRANS_EVT_PRINTURI, pacman_db_getinfo(current, PM_DB_FIRSTSERVER), filename);
					} else {
						if(stat(lcpath, &buf)) {
							/* file is not in the cache dir, so add it to the list */
							files = f_stringlist_append (files, filename);
						} else {
							_pacman_log(PM_LOG_DEBUG, _("%s is already in the cache\n"), filename);
						}
					}
				}
			}

			if(files) {
				EVENT(trans, PM_TRANS_EVT_RETRIEVE_START, current->treename, NULL);
				if(stat(ldir, &buf)) {
					/* no cache directory.... try creating it */
					_pacman_log(PM_LOG_WARNING, _("no %s cache exists.  creating...\n"), ldir);
					pacman_logaction(_("warning: no %s cache exists.  creating..."), ldir);
					if(_pacman_makepath(ldir)) {
						/* couldn't mkdir the cache directory, so fall back to /tmp and unlink
						 * the package afterwards.
						 */
						_pacman_log(PM_LOG_WARNING, _("couldn't create package cache, using /tmp instead\n"));
						pacman_logaction(_("warning: couldn't create package cache, using /tmp instead"));
						snprintf(ldir, PATH_MAX, "%s/tmp", handle->root);
						if(_pacman_handle_set_option(handle, PM_OPT_CACHEDIR, (long)"/tmp") == -1) {
							_pacman_log(PM_LOG_WARNING, _("failed to set option CACHEDIR (%s)\n"), pacman_strerror(pm_errno));
							RET_ERR(PM_ERR_RETRIEVE, -1);
						}
						varcache = 0;
					}
				}
				if(_pacman_downloadfiles(current->servers, ldir, files, tries) == -1) {
					_pacman_log(PM_LOG_WARNING, _("failed to retrieve some files from %s\n"), current->treename);
					retval=1;
					done = 0;
				}
				FREELIST(files);
			}
		}
		if (!done)
			continue;
		if(trans->flags & PM_TRANS_FLAG_PRINTURIS) {
			return(0);
		}
		if ((retval = _pacman_trans_check_integrity (trans, data)) == 0) {
			break;
		}
	}
	
	if(retval != 0) {
		pm_errno = PM_ERR_PKG_CORRUPTED;
		goto error;
	}
	if(trans->flags & PM_TRANS_FLAG_DOWNLOADONLY) {
		return(0);
	}
	if(!retval) {
		retval = _pacman_sync_commit(trans, data);
		if(retval) {
			goto error;
		}
	}

	if(!varcache && !(trans->flags & PM_TRANS_FLAG_DOWNLOADONLY)) {
		/* delete packages */
		f_foreach (i, files) {
			unlink(i->data);
		}
	}
	return(retval);

error:
	/* commiting failed, so this is still just a prepared transaction */
	return(-1);
}

const pmtrans_ops_t _pacman_sync_pmtrans_opts = {
	.commit = _pacman_trans_download_commit
};

/* vim: set ts=2 sw=2 noet: */
