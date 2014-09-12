/*
 *  localdb.h
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
#ifndef _PACMAN_LOCALDB_H
#define _PACMAN_LOCALDB_H

#include "db.h"
#include "package.h"

#include <dirent.h>

namespace libpacman
{

class LocalDatabase;

class LocalPackage
	  : public libpacman::Package
{
public:
	LocalPackage(LocalDatabase *database = 0);
	virtual ~LocalPackage();

	virtual LocalDatabase *database() const;

	virtual int read(unsigned int flags);
	virtual int remove();
};

class LocalDatabase
	: public libpacman::Database
{
public:
	LocalDatabase(libpacman::Handle *handle, const char *treename);
	virtual ~LocalDatabase();

	virtual FStringList test() const;

	virtual int close();

	virtual int rewind();
	virtual libpacman::Package *readpkg(unsigned int inforeq);
	virtual libpacman::Package *scan(const char *target, unsigned int inforeq);

	virtual int write(libpacman::Package *info, unsigned int inforeq);

	virtual FList<libpacman::Package *> getowners(const char *filename);

protected:
	virtual int open(int flags, libpacman::Timestamp *timestamp);

private:
	DIR *m_dir;
};

}

int _pacman_localpackage_remove(libpacman::Package *pkg, pmtrans_t *trans, int howmany, int remain);

#endif /* _PACMAN_LOCALDB_H */

/* vim: set ts=2 sw=2 noet: */
