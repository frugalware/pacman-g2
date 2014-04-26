/*
 *  db.h
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2006 by Miklos Vajna <vmiklos@frugalware.org>
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
#ifndef _PACMAN_DB_H
#define _PACMAN_DB_H

#include <limits.h>
#include <time.h>

#include "handle.h"
#include "object.h"

/* Database entries */
#define INFRQ_NONE     0x00
#define INFRQ_DESC     0x01
#define INFRQ_DEPENDS  0x02
#define INFRQ_FILES    0x04
#define INFRQ_SCRIPLET 0x08
#define INFRQ_ALL      0xFF

namespace libpacman {

class Database;

}

typedef struct __pmdb_ops_t pmdb_ops_t;

struct __pmdb_ops_t {
	pmlist_t *(*test)(libpacman::Database *db);
	int (*open)(libpacman::Database *db, int flags, time_t *timestamp);
	int (*close)(libpacman::Database *db);

	int (*gettimestamp)(libpacman::Database *db, time_t *timestamp);

	/* Package iterator */
	int (*rewind)(libpacman::Database *db);
	pmpkg_t *(*readpkg)(libpacman::Database *db, unsigned int inforeq);
	pmpkg_t *(*scan)(libpacman::Database *db, const char *target, unsigned int inforeq);

	int (*read)(libpacman::Database *db, pmpkg_t *info, unsigned int inforeq);
	int (*write)(libpacman::Database *db, pmpkg_t *info, unsigned int inforeq); /* Optional */
	int (*remove)(libpacman::Database *db, pmpkg_t *info); /* Optional */
};

namespace libpacman {

class Database
	: public libpacman::Object
{
	public:
	Database(pmhandle_t *handle, const char *treename);
	virtual ~Database();

	/* Prototypes for backends functions */
	virtual pmlist_t *test(); /* FIXME: constify */

	virtual int open(int flags);
	virtual int close();

	virtual int gettimestamp(time_t *timestamp);
	virtual int settimestamp(const time_t *timestamp);

	virtual int rewind();
	virtual pmpkg_t *readpkg(unsigned int inforeq);
	virtual pmpkg_t *scan(const char *target, unsigned int inforeq);

	virtual int read(pmpkg_t *info, unsigned int inforeq);
	virtual int write(pmpkg_t *info, unsigned int inforeq);
	virtual int remove(pmpkg_t *info);

	// Cache operations
	pmlist_t *search(pmlist_t *needles);

	const pmdb_ops_t *ops;
	char *path;
	char treename[PATH_MAX];
	void *handle;
	time_t cache_timestamp;
	pmlist_t *pkgcache;
	pmlist_t *grpcache;
	pmlist_t *servers;
};

}

int _pacman_db_cmp(const void *db1, const void *db2);

libpacman::Database *_pacman_db_register(const char *treename, pacman_cb_db_register callback);

#endif /* _PACMAN_DB_H */

/* vim: set ts=2 sw=2 noet: */
