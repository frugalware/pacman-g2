/*
 *  handle.h
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
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
#ifndef _PACMAN_HANDLE_H
#define _PACMAN_HANDLE_H

#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "pacman.h"

#include "io/ffilelock.h"
#include "db.h"
#include "object.h"
#include "trans.h"

namespace libpacman {

class Database;

}

typedef enum __pmaccess_t {
	PM_ACCESS_RO,
	PM_ACCESS_RW
} pmaccess_t;

namespace libpacman {

class Handle
	: public libpacman::Object
{
public:
	Handle();
	virtual ~Handle();

	pmaccess_t access;
	uid_t uid;
	libpacman::Database *db_local;
	pmlist_t *dbs_sync; /* List of (libpacman::Database *) */
	FILE *logfd;
	FFileLock *filelock;
	pmtrans_t *trans;
	/* parameters */
	char *root;
	char *dbpath;
	char *cachedir;
	char *logfile;
	char *hooksdir;
	char *triggersdir;
	pmlist_t *noupgrade; /* List of strings */
	pmlist_t *noextract; /* List of strings */
	pmlist_t *ignorepkg; /* List of strings */
	pmlist_t *holdpkg; /* List of strings */
	unsigned char usesyslog;
	time_t upgradedelay;
	time_t olddelay;
	/* servers */
	char *proxyhost;
	unsigned short proxyport;
	char *xfercommand;
	unsigned short nopassiveftp;
	unsigned short chomp; /* if eye-candy features should be enabled or not */
	unsigned short maxtries; /* for downloading */
	pmlist_t *needles; /* for searching */
	char *language;
	int *dlremain;
	int *dlhowmany;
	int sysupgrade;
};

}

extern libpacman::Handle *handle;

int _pacman_handle_lock(libpacman::Handle *handle);
int _pacman_handle_unlock(libpacman::Handle *handle);

#endif /* _PACMAN_HANDLE_H */

/* vim: set ts=2 sw=2 noet: */
