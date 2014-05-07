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

#include "handle.h"

#include "error.h"
#include "trans.h"
#include "pacman.h"
#include "server.h"
#include "util.h"

#include "util/list.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "fstdlib.h"
#include "fstring.h"

#include <sys/types.h>
#include <limits.h>
#include <locale.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

using namespace libpacman;

Handle::Handle()
{
	maxtries = 1;

#ifndef CYGWIN
	/* see if we're root or not */
	uid = geteuid();
#ifdef FAKEROOT_PROOF
	if(!uid && getenv("FAKEROOTKEY")) {
		/* fakeroot doesn't count, we're non-root */
		uid = 99;
	}
#endif

	/* see if we're root or not (fakeroot does not count) */
#ifdef FAKEROOT_PROOF
	if(uid == 0 && !getenv("FAKEROOTKEY")) {
#else
	if(uid == 0) {
#endif
		access = PM_ACCESS_RW;
	} else {
		access = PM_ACCESS_RO;
	}
#else
	access = PM_ACCESS_RW;
#endif

	dbpath = strdup(PM_DBPATH);
	cachedir = strdup(PM_CACHEDIR);
	hooksdir = strdup(PM_HOOKSDIR);
	triggersdir = strdup(PM_TRIGGERSDIR);

	language = strdup(setlocale(LC_ALL, NULL));
}

Handle::~Handle()
{
	/* close logfiles */
	if(logfd) {
		fclose(logfd);
		logfd = NULL;
	}
	if(usesyslog) {
		usesyslog = 0;
		closelog();
	}

	/* free memory */
	delete trans;
	FREE(root);
	FREE(dbpath);
	FREE(cachedir);
	FREE(hooksdir);
	FREE(language);
	FREE(logfile);
	FREE(proxyhost);
	FREE(xfercommand);
	FREELIST(dbs_sync);
	FREELIST(noupgrade);
	FREELIST(noextract);
	FREELIST(ignorepkg);
	FREELIST(holdpkg);
	FREELIST(needles);
}

int _pacman_handle_lock(Handle *handle)
{
	char lckpath[PATH_MAX];

	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	snprintf(lckpath, PATH_MAX, "%s/%s", handle->root, PM_LOCK);
	return (handle->filelock = f_filelock_aquire(lckpath, F_FILELOCK_CREATE_HOLD_DIR | F_FILELOCK_EXCLUSIVE | F_FILELOCK_UNLINK_ON_CLOSE)) != NULL ? 0: -1;
}

int _pacman_handle_unlock(Handle *handle)
{
	int ret = 0;

	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	if(handle->filelock != NULL) {
		ret = f_filelock_release(handle->filelock);
		handle->filelock = NULL;
	}
	return ret;
}

/* vim: set ts=2 sw=2 noet: */
