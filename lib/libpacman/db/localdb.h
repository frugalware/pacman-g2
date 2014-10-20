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
	: public libpacman::package
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
	virtual ~LocalDatabase() override;

	virtual FStringList test() const override;

	virtual int close() override;

	virtual int rewind();
	virtual libpacman::package_ptr readpkg(unsigned int inforeq) override;
	virtual libpacman::package_ptr scan(const char *target, unsigned int inforeq) override;

	virtual int write(libpacman::package_ptr info, unsigned int inforeq) override;

	virtual libpacman::package_list getowners(const char *filename) override;

protected:
	virtual int open(int flags, libpacman::Timestamp *timestamp) override;

private:
	DIR *m_dir;
};

}

int _pacman_localpackage_remove(libpacman::package_ptr pkg, pmtrans_t *trans, int howmany, int remain);

#endif /* _PACMAN_LOCALDB_H */

/* vim: set ts=2 sw=2 noet: */
