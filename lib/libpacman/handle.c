/*
 *  handle.c
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
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>
#include <ftplib.h>
#include <locale.h>

/* pacman-g2 */
#include "handle.h"

#include "util.h"
#include "log.h"
#include "error.h"
#include "trans.h"
#include "pacman.h"
#include "server.h"

static
void _pacman_handle_fini(pmhandle_t *ph)
{
	/* close logfiles */
	if(ph->logfd) {
		fclose(ph->logfd);
		ph->logfd = NULL;
	}
	if(ph->usesyslog) {
		ph->usesyslog = 0;
		closelog();
	}

	/* free memory */
	FREETRANS(ph->trans);
	FREE(ph->root);
	FREE(ph->dbpath);
	FREE(ph->cachedir);
	FREE(ph->hooksdir);
	FREE(ph->language);
	FREE(ph->logfile);
	FREE(ph->proxyhost);
	FREE(ph->xfercommand);
	FREELIST(ph->dbs_sync);
	FREELIST(ph->noupgrade);
	FREELIST(ph->noextract);
	FREELIST(ph->ignorepkg);
	FREELIST(ph->holdpkg);
	FREELIST(ph->needles);
}

static const
FObjectOperations _pacman_handle_ops = {
	.fini = _pacman_handle_fini,
};

pmhandle_t *_pacman_handle_new()
{
	pmhandle_t *ph = _pacman_zalloc(sizeof(pmhandle_t));

	if(ph == NULL) {
		return(NULL);
	}

	f_object_init (&ph->base, &_pacman_handle_ops);

	ph->lckfd = -1;
	ph->maxtries = 1;

#ifndef CYGWIN
	/* see if we're root or not */
	ph->uid = geteuid();
#ifndef FAKEROOT
	if(!ph->uid && getenv("FAKEROOTKEY")) {
		/* fakeroot doesn't count, we're non-root */
		ph->uid = 99;
	}
#endif

	/* see if we're root or not (fakeroot does not count) */
#ifndef FAKEROOT
	if(ph->uid == 0 && !getenv("FAKEROOTKEY")) {
#else
	if(ph->uid == 0) {
#endif
		ph->access = PM_ACCESS_RW;
	} else {
		ph->access = PM_ACCESS_RO;
	}
#else
	ph->access = PM_ACCESS_RW;
#endif

	ph->dbpath = strdup(PM_DBPATH);
	ph->cachedir = strdup(PM_CACHEDIR);
	ph->hooksdir = strdup(PM_HOOKSDIR);

	ph->language = strdup(setlocale(LC_ALL, NULL));

	return(ph);
}

void _pacman_handle_free(pmhandle_t *ph)
{
	f_object_delete (&ph->base);
}

int _pacman_handle_set_option(pmhandle_t *ph, unsigned char val, unsigned long data)
{

	char logdir[PATH_MAX], path[PATH_MAX], *p, *q;

	/* Sanity checks */
	ASSERT(ph != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	switch(val) {
		case PM_OPT_DBPATH:
			if(ph->dbpath) {
				FREE(ph->dbpath);
			}
			ph->dbpath = strdup((data && strlen((char *)data) != 0) ? (char *)data : PM_DBPATH);
			_pacman_log(PM_LOG_FLOW2, _("PM_OPT_DBPATH set to '%s'"), ph->dbpath);
		break;
		case PM_OPT_CACHEDIR:
			if(ph->cachedir) {
				FREE(ph->cachedir);
			}
			ph->cachedir = strdup((data && strlen((char *)data) != 0) ? (char *)data : PM_CACHEDIR);
			_pacman_log(PM_LOG_FLOW2, _("PM_OPT_CACHEDIR set to '%s'"), ph->cachedir);
		break;
		case PM_OPT_HOOKSDIR:
			if(ph->hooksdir) {
				FREE(ph->hooksdir);
			}
			ph->hooksdir = strdup((data && strlen((char *)data) != 0) ? (char *)data : PM_HOOKSDIR);
			_pacman_log(PM_LOG_FLOW2, _("PM_OPT_HOOKSDIR set to '%s'"), ph->hooksdir);
		break;
		case PM_OPT_LOGFILE:
			if((char *)data == NULL || ph->uid != 0) {
				return(0);
			}
			if(ph->logfile) {
				FREE(ph->logfile);
			}
			if(ph->logfd) {
				if(fclose(ph->logfd) != 0) {
					ph->logfd = NULL;
					RET_ERR(PM_ERR_OPT_LOGFILE, -1);
				}
				ph->logfd = NULL;
			}

			snprintf(path, PATH_MAX, "%s/%s", ph->root, (char *)data);
			p = strdup((char*)data);
			q = strrchr(p, '/');
			if (q) {
				*q = '\0';
			}
			snprintf(logdir, PATH_MAX, "%s/%s", ph->root, p);
			free(p);
			_pacman_makepath(logdir);
			if((ph->logfd = fopen(path, "a")) == NULL) {
				_pacman_log(PM_LOG_ERROR, _("can't open log file %s"), path);
				RET_ERR(PM_ERR_OPT_LOGFILE, -1);
			}
			ph->logfile = strdup(path);
			_pacman_log(PM_LOG_FLOW2, _("PM_OPT_LOGFILE set to '%s'"), path);
		break;
		case PM_OPT_NOUPGRADE:
			if((char *)data && strlen((char *)data) != 0) {
				ph->noupgrade = f_stringlist_append (ph->noupgrade, (char *)data);
				_pacman_log(PM_LOG_FLOW2, _("'%s' added to PM_OPT_NOUPGRADE"), (char *)data);
			} else {
				FREELIST(ph->noupgrade);
				_pacman_log(PM_LOG_FLOW2, _("PM_OPT_NOUPGRADE flushed"));
			}
		break;
		case PM_OPT_NOEXTRACT:
			if((char *)data && strlen((char *)data) != 0) {
				ph->noextract = f_stringlist_append (ph->noextract, (char *)data);
				_pacman_log(PM_LOG_FLOW2, _("'%s' added to PM_OPT_NOEXTRACT"), (char *)data);
			} else {
				FREELIST(ph->noextract);
				_pacman_log(PM_LOG_FLOW2, _("PM_OPT_NOEXTRACT flushed"));
			}
		break;
		case PM_OPT_IGNOREPKG:
			if((char *)data && strlen((char *)data) != 0) {
				ph->ignorepkg = f_stringlist_append (ph->ignorepkg, (char *)data);
				_pacman_log(PM_LOG_FLOW2, _("'%s' added to PM_OPT_IGNOREPKG"), (char *)data);
			} else {
				FREELIST(ph->ignorepkg);
				_pacman_log(PM_LOG_FLOW2, _("PM_OPT_IGNOREPKG flushed"));
			}
		break;
		case PM_OPT_HOLDPKG:
			if((char *)data && strlen((char *)data) != 0) {
				ph->holdpkg = f_stringlist_append (ph->holdpkg, (char *)data);
				_pacman_log(PM_LOG_FLOW2, _("'%s' added to PM_OPT_HOLDPKG"), (char *)data);
			} else {
				FREELIST(ph->holdpkg);
				_pacman_log(PM_LOG_FLOW2, _("PM_OPT_HOLDPKG flushed"));
			}
		break;
		case PM_OPT_NEEDLES:
			if((char *)data && strlen((char *)data) != 0) {
				ph->needles = f_stringlist_append (ph->needles, (char *)data);
				_pacman_log(PM_LOG_FLOW2, _("'%s' added to PM_OPT_NEEDLES"), (char *)data);
			} else {
				FREELIST(ph->needles);
				_pacman_log(PM_LOG_FLOW2, _("PM_OPT_NEEDLES flushed"));
			}
		break;
		case PM_OPT_USESYSLOG:
			if(data != 0 && data != 1) {
				RET_ERR(PM_ERR_OPT_USESYSLOG, -1);
			}
			if(ph->usesyslog == data) {
				return(0);
			}
			if(ph->usesyslog) {
				closelog();
			} else {
				openlog("libpacman", 0, LOG_USER);
			}
			ph->usesyslog = (unsigned short)data;
			_pacman_log(PM_LOG_FLOW2, _("PM_OPT_USESYSLOG set to '%d'"), ph->usesyslog);
		break;
		case PM_OPT_LOGCB:
			pm_logcb = (pacman_cb_log)data;
		break;
		case PM_OPT_DLCB:
			pm_dlcb = (FtpCallback)data;
		break;
		case PM_OPT_DLFNM:
			pm_dlfnm = (char *)data;
		break;
		case PM_OPT_DLOFFSET:
			pm_dloffset = (int *)data;
		break;
		case PM_OPT_DLT0:
			pm_dlt0 = (struct timeval *)data;
		break;
		case PM_OPT_DLT:
			pm_dlt = (struct timeval *)data;
		break;
		case PM_OPT_DLRATE:
			pm_dlrate = (float *)data;
		break;
		case PM_OPT_DLXFERED1:
			pm_dlxfered1 = (int *)data;
		break;
		case PM_OPT_DLETA_H:
			pm_dleta_h = (unsigned int *)data;
		break;
		case PM_OPT_DLETA_M:
			pm_dleta_m = (unsigned int *)data;
		break;
		case PM_OPT_DLETA_S:
			pm_dleta_s = (unsigned int *)data;
		break;
		case PM_OPT_DLREMAIN:
			ph->dlremain = (int *)data;
		break;
		case PM_OPT_DLHOWMANY:
			ph->dlhowmany = (int *)data;
		break;
		case PM_OPT_UPGRADEDELAY:
			ph->upgradedelay = data;
		break;
		case PM_OPT_OLDDELAY:
			ph->olddelay = data;
		break;
		case PM_OPT_LOGMASK:
			pm_logmask = (unsigned char)data;
			_pacman_log(PM_LOG_FLOW2, _("PM_OPT_LOGMASK set to '%02x'"), (unsigned char)data);
		break;
		case PM_OPT_PROXYHOST:
			if(ph->proxyhost) {
				FREE(ph->proxyhost);
			}
			p = strstr((char*)data, "://");
			if(p) {
				p += 3;
				if(p == NULL || *p == '\0') {
					RET_ERR(PM_ERR_SERVER_BAD_LOCATION, -1);
				}
				data = (long)p;
			}
#if defined(__APPLE__) || defined(__OpenBSD__)
			ph->proxyhost = strdup((char*)data);
#else
			ph->proxyhost = strndup((char*)data, PATH_MAX);
#endif
			_pacman_log(PM_LOG_FLOW2, _("PM_OPT_PROXYHOST set to '%s'"), ph->proxyhost);
		break;
		case PM_OPT_PROXYPORT:
			ph->proxyport = (unsigned short)data;
			_pacman_log(PM_LOG_FLOW2, _("PM_OPT_PROXYPORT set to '%d'"), ph->proxyport);
		break;
		case PM_OPT_XFERCOMMAND:
			if(ph->xfercommand) {
				FREE(ph->xfercommand);
			}
#if defined(__APPLE__) || defined(__OpenBSD__)
			ph->xfercommand = strdup((char*)data);
#else
			ph->xfercommand = strndup((char*)data, PATH_MAX);
#endif
			_pacman_log(PM_LOG_FLOW2, _("PM_OPT_XFERCOMMAND set to '%s'"), ph->xfercommand);
		break;
		case PM_OPT_NOPASSIVEFTP:
			ph->nopassiveftp = (unsigned short)data;
			_pacman_log(PM_LOG_FLOW2, _("PM_OPT_NOPASSIVEFTP set to '%d'"), ph->nopassiveftp);
		break;
		case PM_OPT_CHOMP:
			ph->chomp = (unsigned short)data;
			_pacman_log(PM_LOG_FLOW2, _("PM_OPT_CHOMP set to '%d'"), ph->chomp);
		break;
		case PM_OPT_MAXTRIES:
			ph->maxtries = (unsigned short)data;
			_pacman_log(PM_LOG_FLOW2, _("PM_OPT_MAXTRIES set to '%d'"), ph->maxtries);
		break;
		default:
			RET_ERR(PM_ERR_WRONG_ARGS, -1);
	}

	return(0);
}

int _pacman_handle_get_option(pmhandle_t *ph, unsigned char val, long *data)
{
	/* Sanity checks */
	ASSERT(ph != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	switch(val) {
		case PM_OPT_ROOT:      *data = (long)ph->root; break;
		case PM_OPT_DBPATH:    *data = (long)ph->dbpath; break;
		case PM_OPT_CACHEDIR:  *data = (long)ph->cachedir; break;
		case PM_OPT_HOOKSDIR:  *data = (long)ph->hooksdir; break;
		case PM_OPT_LOCALDB:   *data = (long)ph->db_local; break;
		case PM_OPT_SYNCDB:    *data = (long)ph->dbs_sync; break;
		case PM_OPT_LOGFILE:   *data = (long)ph->logfile; break;
		case PM_OPT_NOUPGRADE: *data = (long)ph->noupgrade; break;
		case PM_OPT_NOEXTRACT: *data = (long)ph->noextract; break;
		case PM_OPT_IGNOREPKG: *data = (long)ph->ignorepkg; break;
		case PM_OPT_HOLDPKG:   *data = (long)ph->holdpkg; break;
		case PM_OPT_NEEDLES:   *data = (long)ph->needles; break;
		case PM_OPT_USESYSLOG: *data = ph->usesyslog; break;
		case PM_OPT_LOGCB:     *data = (long)pm_logcb; break;
		case PM_OPT_DLCB:     *data = (long)pm_dlcb; break;
		case PM_OPT_UPGRADEDELAY: *data = (long)ph->upgradedelay; break;
		case PM_OPT_OLDDELAY:  *data = (long)ph->olddelay; break;
		case PM_OPT_LOGMASK:   *data = pm_logmask; break;
		case PM_OPT_DLFNM:     *data = (long)pm_dlfnm; break;
		case PM_OPT_DLOFFSET:  *data = (long)pm_dloffset; break;
		case PM_OPT_DLT0:      *data = (long)pm_dlt0; break;
		case PM_OPT_DLT:       *data = (long)pm_dlt; break;
		case PM_OPT_DLRATE:    *data = (long)pm_dlrate; break;
		case PM_OPT_DLXFERED1: *data = (long)pm_dlxfered1; break;
		case PM_OPT_DLETA_H:   *data = (long)pm_dleta_h; break;
		case PM_OPT_DLETA_M:   *data = (long)pm_dleta_m; break;
		case PM_OPT_DLETA_S:   *data = (long)pm_dleta_s; break;
		case PM_OPT_DLREMAIN:  *data = (long)ph->dlremain; break;
		case PM_OPT_DLHOWMANY: *data = (long)ph->dlhowmany; break;
		case PM_OPT_PROXYHOST: *data = (long)ph->proxyhost; break;
		case PM_OPT_PROXYPORT: *data = ph->proxyport; break;
		case PM_OPT_XFERCOMMAND: *data = (long)ph->xfercommand; break;
		case PM_OPT_NOPASSIVEFTP: *data = ph->nopassiveftp; break;
		case PM_OPT_CHOMP: *data = ph->chomp; break;
		case PM_OPT_MAXTRIES: *data = ph->maxtries; break;
		default:
			RET_ERR(PM_ERR_WRONG_ARGS, -1);
		break;
	}

	return(0);
}

pmdb_t *_pacman_handle_get_db_sync(pmhandle_t *handle, const char *name) {
	pmlist_t *i;

	f_foreach (i, handle->dbs_sync) {
		pmdb_t *db = i->data;

		if (strcmp(db->treename, name) == 0) {
			return db;
		}
	}
	return NULL;
}

int _pacman_handle_lock_acquire (pmhandle_t *handle) {
	char path[PATH_MAX];

	snprintf (path, PATH_MAX, "%s/%s", handle->root, PM_LOCK);
	handle->lckfd = _pacman_lckmk (path);
	if (handle->lckfd == -1) {
		RET_ERR(PM_ERR_HANDLE_LOCK, -1);
	}
	return 0;
}

int _pacman_handle_lock_release (pmhandle_t *handle) {
	char path[PATH_MAX];

	if (handle->lckfd == -1) {
		return 0;
	}
	close (handle->lckfd);
	handle->lckfd = -1;

	snprintf(path, PATH_MAX, "%s/%s", handle->root, PM_LOCK);
	if (_pacman_lckrm (path)) {
		_pacman_log (PM_LOG_WARNING, _("could not remove lock file %s"), path);
		pacman_logaction (_("warning: could not remove lock file %s"), path);
		return -1;
	}
	return 0;
}
/* vim: set ts=2 sw=2 noet: */
