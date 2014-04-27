/*
 *  syncdb.h
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
#ifndef _PACMAN_SYNCDB_H
#define _PACMAN_SYNCDB_H

#include "db.h"
#include "package.h"

extern
const pmdb_ops_t _pacman_syncdb_ops;

int _pacman_syncdb_update(libpacman::Database *db, int force);

namespace libpacman
{

class SyncDatabase
	: public libpacman::Database
{
public:
	SyncDatabase(pmhandle_t *handle, const char *treename);
	virtual ~SyncDatabase();

	virtual int rewind();
	virtual pmpkg_t *readpkg(unsigned int inforeq);
	virtual pmpkg_t *scan(const char *target, unsigned int inforeq);
};

class SyncPackage
	: public libpacman::Package
{
public:
	virtual ~SyncPackage();
};

}

#endif /* _PACMAN_SYNCDB_H */

/* vim: set ts=2 sw=2 noet: */
