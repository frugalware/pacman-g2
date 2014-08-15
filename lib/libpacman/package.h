/*
 *  package.h
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2006 by David Kimpe <dnaku@frugalware.org>
 *  Copyright (c) 2005, 2006 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2005, 2006. 2007 by Miklos Vajna <vmiklos@frugalware.org>
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
#ifndef _PACMAN_PACKAGE_H
#define _PACMAN_PACKAGE_H

#include <time.h>

#include "util/stringlist.h"

#include "pacman.h"

#include "trans.h"

#include "kernel/fobject.h"

namespace libpacman {

	class Database;

}

enum {
	PKG_FROM_CACHE = 1,
	PKG_FROM_FILE
};

#define PKG_NAME_LEN     256
#define PKG_VERSION_LEN  64
#define PKG_FULLNAME_LEN (PKG_NAME_LEN-1)+1+(PKG_VERSION_LEN-1)+1
#define PKG_DESC_LEN     512
#define PKG_URL_LEN      256
#define PKG_DATE_LEN     32
#define PKG_TYPE_LEN     32
#define PKG_PACKAGER_LEN 64
#define PKG_MD5SUM_LEN   33
#define PKG_SHA1SUM_LEN  41
#define PKG_ARCH_LEN     32

#define PM_PACKAGE_FLAG_NAME                  (1<< 0)
#define PM_PACKAGE_FLAG_VERSION               (1<< 1)
#define PM_PACKAGE_FLAG_DESCRIPTION           (1<< 2)
#define PM_PACKAGE_FLAG_URL                   (1<< 3)
#define PM_PACKAGE_FLAG_BUILDDATE             (1<< 4)
#define PM_PACKAGE_FLAG_BUILDTYPE             (1<< 5)
#define PM_PACKAGE_FLAG_INSTALLDATE           (1<< 6)
//#define PM_PACKAGE_FLAG_HASH                (1<< 7)
#define PM_PACKAGE_FLAG_ARCH                  (1<< 8)
#define PM_PACKAGE_FLAG_LOCALISED_DESCRIPTION (1<< 9)
#define PM_PACKAGE_FLAG_LICENSE               (1<<10)
#define PM_PACKAGE_FLAG_REPLACES              (1<<11)
#define PM_PACKAGE_FLAG_GROUPS                (1<<12)
#define PM_PACKAGE_FLAG_FILES                 (1<<13)
#define PM_PACKAGE_FLAG_BACKUP                (1<<14)
#define PM_PACKAGE_FLAG_DEPENDS               (1<<15)
#define PM_PACKAGE_FLAG_REMOVES               (1<<16)
#define PM_PACKAGE_FLAG_REQUIREDBY            (1<<17)
#define PM_PACKAGE_FLAG_CONFLICTS             (1<<18)
#define PM_PACKAGE_FLAG_PROVIDES              (1<<19)
#define PM_PACKAGE_FLAG_TRIGGERS              (1<<20)
#define PM_PACKAGE_FLAG_PACKAGER              (1<<21)
#define PM_PACKAGE_FLAG_FORCE                 (1<<22)
#define PM_PACKAGE_FLAG_REASON                (1<<23)
#define PM_PACKAGE_FLAG_SCRIPTLET             (1<<24)
#define PM_PACKAGE_FLAG_STICKY                (1<<25)

namespace libpacman {

/* IMPROVEMENT: Add a dirty_flags to know what flags needs to be written */
class Package
	: public ::flib::FObject
{
#define LIBPACMAN_PACKAGE_PROPERTY(type, name, flag)                           \
public:                                                                        \
	type name();

#include "package_properties.h"

#undef LIBPACMAN_PACKAGE_PROPERTY

public:
	Package(libpacman::Database *database = 0);
	Package(const char *name, const char *version);
protected:
	virtual ~Package();

public:
	libpacman::Database *database() const;

	virtual bool is_valid(const pmtrans_t *trans, const char *pkgfile) const;

	virtual int read(unsigned int flags);
	virtual int write(unsigned int flags); /* Optional */
	virtual int remove(); /* Optional */

	int filename(char *str, size_t size) const;
	char *fileneedbackup(const char *file) const;

	bool set_filename(const char *filename, int witharch);
	static bool splitname(const char *target, char *name, char *version, int witharch);

	const char *path() const;

	FStringList *provides() const;
	bool provides(const char *pkgname);

private:
	Package(const libpacman::Package &other);
	libpacman::Package &operator =(const libpacman::Package &other);

public:
	libpacman::Database *m_database;

	unsigned int flags;
	char m_name[PKG_NAME_LEN];
	char m_version[PKG_VERSION_LEN];
	char m_description[PKG_DESC_LEN];
	char m_url[PKG_URL_LEN];
	char builddate[PKG_DATE_LEN];
	char buildtype[PKG_TYPE_LEN];
	char installdate[PKG_DATE_LEN];
	char packager[PKG_PACKAGER_LEN];
	char md5sum[PKG_MD5SUM_LEN];
	char sha1sum[PKG_SHA1SUM_LEN];
	char arch[PKG_ARCH_LEN];
	unsigned long size;
	unsigned long usize;
	unsigned char scriptlet;
	unsigned char m_force;
	unsigned char m_stick;
	time_t date;
	unsigned char m_reason;
	FStringList *desc_localized;
	FStringList *license;
	FStringList *m_replaces;
	FStringList *m_groups;
	FStringList *m_files;
	FStringList *m_backup;
	FStringList *m_depends;
	FStringList *m_removes;
	FStringList *m_requiredby;
	FStringList *m_conflicts;
	FStringList *m_provides;
	FStringList *m_triggers;
	char *m_path;
};

}

#define FREELISTPKGS(p) _FREELIST(p, _pacman_pkg_delete)

int _pacman_pkg_delete(libpacman::Package *self);

int _pacman_pkg_cmp(const void *p1, const void *p2);
libpacman::Package *_pacman_pkg_isin(const char *needle, pmlist_t *haystack);

int _pacman_packagestrmatcher_init(FMatcher *matcher, const FStrMatcher *strmatcher, int flags);
int _pacman_packagestrmatcher_fini(FMatcher *matcher);

int _pacman_packagestrmatcher_set_flags(FMatcher *matcher, int flags);

#endif /* _PACMAN_PACKAGE_H */

/* vim: set ts=2 sw=2 noet: */
