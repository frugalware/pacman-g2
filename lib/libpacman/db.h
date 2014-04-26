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

typedef struct __pmdb_ops_t pmdb_ops_t;

struct __pmdb_ops_t {
	pmlist_t *(*test)(pmdb_t *db);
	int (*open)(pmdb_t *db, int flags, time_t *timestamp);
	int (*close)(pmdb_t *db);

	int (*gettimestamp)(pmdb_t *db, time_t *timestamp);

	/* Package iterator */
	int (*rewind)(pmdb_t *db);
	pmpkg_t *(*readpkg)(pmdb_t *db, unsigned int inforeq);
	pmpkg_t *(*scan)(pmdb_t *db, const char *target, unsigned int inforeq);

	int (*read)(pmdb_t *db, pmpkg_t *info, unsigned int inforeq);
	int (*write)(pmdb_t *db, pmpkg_t *info, unsigned int inforeq); /* Optional */
	int (*remove)(pmdb_t *db, pmpkg_t *info); /* Optional */
};

/* Database */
struct __pmdb_t {
	__pmdb_t(pmhandle_t *handle, const char *treename);
	~__pmdb_t();

	/* Prototypes for backends functions */
	pmlist_t *test(); /* FIXME: constify */

	int open(int flags);
	int close();

	int gettimestamp(time_t *timestamp);
	int settimestamp(const time_t *timestamp);

	int rewind();
	pmpkg_t *readpkg(unsigned int inforeq);
	pmpkg_t *scan(const char *target, unsigned int inforeq);

	int read(pmpkg_t *info, unsigned int inforeq);
	int write(pmpkg_t *info, unsigned int inforeq);
	int remove(pmpkg_t *info);

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

int _pacman_db_cmp(const void *db1, const void *db2);

pmdb_t *_pacman_db_register(const char *treename, pacman_cb_db_register callback);

namespace libpacman
{

class Database
	: public libpacman::Object
{
public:
	virtual ~Database();
};

}

#endif /* _PACMAN_DB_H */

/* vim: set ts=2 sw=2 noet: */
