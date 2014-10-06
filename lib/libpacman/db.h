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

#include "group.h"
#include "handle.h"
#include "package.h"
#include "timestamp.h"

#include "kernel/fobject.h"
#include "util/fptrlist.h"
#include "util/fstringlist.h"
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

	virtual bool add_server(const char *url);
	const char *treename() const;

	/* Prototypes for backends functions */
	virtual FStringList test() const;

	virtual int open(int flags = 0);
	virtual int close();

	virtual int gettimestamp(libpacman::Timestamp *timestamp);
	virtual int settimestamp(const libpacman::Timestamp *timestamp);

	/* Package iterator */
	virtual int rewind();
	virtual libpacman::Package *readpkg(unsigned int inforeq);
	virtual libpacman::Package *scan(const char *target, unsigned int inforeq) = 0;

	virtual int write(libpacman::Package *info, unsigned int inforeq);

	/* Cache operations */
	FList<libpacman::Package *> filter(const libpacman::PackageMatcher &packagematcher);
	FList<libpacman::Package *> filter(const FStrMatcher *strmatcher, int packagestrmatcher_flags);
	FList<libpacman::Package *> filter(const FStringList &needles, int packagestrmatcher_flags, int strmatcher_flags = FStrMatcher::EQUAL);
	FList<libpacman::Package *> filter(const char *pattern, int packagestrmatcher_flags, int strmatcher_flags = FStrMatcher::EQUAL);
	libpacman::Package *find(const libpacman::PackageMatcher &packagematcher);
	libpacman::Package *find(const FStrMatcher *strmatcher, int packagestrmatcher_flags);
	libpacman::Package *find(const char *target,
			int packagestrmatcher_flags = PM_PACKAGE_FLAG_NAME,
			int strmatcher_flags = FStrMatcher::EQUAL);
	libpacman::package_set &get_packages();
	libpacman::group_set &get_groups();
	FList<libpacman::Package *> whatPackagesProvide(const char *target);

	virtual FList<libpacman::Package *> getowners(const char *filename); /* Make pure virtual */

	::libpacman::Handle *m_handle;
	char *path;
	libpacman::Timestamp cache_timestamp;
	libpacman::package_set pkgcache;
	libpacman::group_set grpcache;
	FPtrList servers;

protected:
	Database(libpacman::Handle *handle, const char *treename);

	virtual int open(int flags, libpacman::Timestamp *timestamp);

private:
	char m_treename[PATH_MAX];
};

}

#endif /* _PACMAN_DB_H */

/* vim: set ts=2 sw=2 noet: */
