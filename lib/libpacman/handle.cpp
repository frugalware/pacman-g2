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

#include "db/localdb.h"
#include "db/syncdb.h"
#include "error.h"
#include "trans.h"
#include "pacman.h"
#include "pacman_p.h"
#include "server.h"
#include "util.h"

#include "util/log.h"
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
	free(root);
	free(dbpath);
	free(cachedir);
	free(hooksdir);
	free(language);
	free(logfile);
	free(proxyhost);
	free(xfercommand);
	dbs_sync.clear(/* free */);
}

int Handle::lock()
{
	char lckpath[PATH_MAX];

	snprintf(lckpath, PATH_MAX, "%s/%s", root, PM_LOCK);
	return (filelock = f_filelock_acquire(lckpath, F_FILELOCK_CREATE_HOLD_DIR | F_FILELOCK_EXCLUSIVE | F_FILELOCK_UNLINK_ON_CLOSE)) != NULL ? 0: -1;
}

int Handle::unlock()
{
	int ret = 0;

	if(filelock != NULL) {
		ret = f_filelock_release(filelock);
		filelock = NULL;
	}
	return ret;
}

Database *Handle::createDatabase(const char *treename, pacman_cb_db_register callback)
{
	Database *db;

	if((db = getDatabase(treename)) != NULL) {
		_pacman_log(PM_LOG_WARNING, _("attempt to re-register the '%s' database, using existing\n"), db->treename());
		return db;
	}

	_pacman_log(PM_LOG_FLOW1, _("registering database '%s'"), treename);

	db = strcmp(treename, "local") == 0 ? (Database *)new LocalDatabase(this, treename) : new SyncDatabase(this, treename);
	if(db == NULL) {
		RET_ERR(PM_ERR_DB_CREATE, NULL);
	}

	_pacman_log(PM_LOG_DEBUG, _("opening database '%s'"), db->treename());
	if(db->open(0) == -1) {
		delete db;
		RET_ERR(PM_ERR_DB_OPEN, NULL);
	}

	/* Only call callback on NEW registration. */
	if(callback) callback(treename, c_cast(db));

	if(strcmp(treename, "local") == 0) {
		db_local = db;
	} else {
		dbs_sync.add(db);
	}
	return(db);
}

Database *Handle::getDatabase(const char *treename)
{
	if(strcmp(treename, "local") == 0) {
		return db_local;
	}

	for(auto i = dbs_sync.begin(), end = dbs_sync.end(); i != end; ++i) {
			Database *sdb = (Database *)*i;
			if(strcmp(treename, sdb->treename()) == 0) {
				return sdb;
		}
	}
	return NULL;
}

/* vim: set ts=2 sw=2 noet: */

