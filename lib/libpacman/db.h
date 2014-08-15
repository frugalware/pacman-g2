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
#include "package.h"
#include "timestamp.h"

#include "kernel/fobject.h"
#include "util/fptrlist.h"
#include "util/stringlist.h"
#include "fstring.h"

namespace libpacman {

class Handle;
class Package;

}

namespace libpacman {

class Database
	: public ::flib::FObject
{
public:
	virtual ~Database();

	/* Prototypes for backends functions */
	virtual pmlist_t *test() const;

	virtual int open(int flags = 0);
	virtual int close();

	virtual int gettimestamp(libpacman::Timestamp *timestamp);
	virtual int settimestamp(const libpacman::Timestamp *timestamp);

	/* Package iterator */
	virtual int rewind();
	virtual libpacman::Package *readpkg(unsigned int inforeq);
	virtual libpacman::Package *scan(const char *target, unsigned int inforeq) = 0;

	virtual int write(libpacman::Package *info, unsigned int inforeq);

	// Cache operations
	pmlist_t *filter(const FMatcher *packagestrmatcher);
	pmlist_t *filter(const FStrMatcher *strmatcher, int packagestrmatcher_flags);
	FPtrList *filter(const FStringList *needles, int packagestrmatcher_flags, int strmatcher_flags = F_STRMATCHER_EQUAL);
	FPtrList *filter(const char *target, int packagestrmatcher_flags, int strmatcher_flags = F_STRMATCHER_EQUAL);
	libpacman::Package *find(const FMatcher *packagestrmatcher);
	libpacman::Package *find(const FStrMatcher *strmatcher, int packagestrmatcher_flags);
	libpacman::Package *find(const char *target);
	libpacman::Package *find(const char *target, int packagestrmatcher_flags, int strmatcher_flags = F_STRMATCHER_EQUAL);
	FPtrList *whatPackagesProvide(const char *target);

	virtual pmlist_t *getowners(const char *filename); /* Make pure virtual */

	::libpacman::Handle *m_handle;
	char *path;
	char treename[PATH_MAX];
	libpacman::Timestamp cache_timestamp;
	pmlist_t *pkgcache;
	pmlist_t *grpcache;
	pmlist_t *servers;

protected:
	Database(libpacman::Handle *handle, const char *treename);

	virtual int open(int flags, libpacman::Timestamp *timestamp);

private:
};

}

#endif /* _PACMAN_DB_H */

/* vim: set ts=2 sw=2 noet: */
