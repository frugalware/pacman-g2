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

#include "pacman.h"

#include "trans.h"

#include "kernel/fobject.h"
#include "kernel/fstr.h"
#include "util/fset.h"
#include "util/fstringlist.h"

typedef struct __pmdepend_t pmdepend_t;

namespace libpacman {

	class Database;
	class package_node;

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
class package
	: public ::flib::FObject
{
#define LIBPACMAN_PACKAGE_PROPERTY(type, name, flag)                           \
public:                                                                        \
	type name();

#include "package_properties.h"

#undef LIBPACMAN_PACKAGE_PROPERTY

public:
	package(libpacman::Database *database = 0);
	package(libpacman::package_node *package_node);
	package(const char *name, const char *version);
protected:
	virtual ~package();

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

	const FStringList &provides() const;
	bool provides(const char *pkgname);

	const FStringList &replaces() const;
	bool replaces(const char *pkgname);

	bool match(const pmdepend_t &depend);

private:
	package(const package &other);
	package &operator =(const package &other);

public:
	libpacman::Database *m_database;

	unsigned int flags;
	package_node *m_package_node;
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
	FStringList desc_localized;
	FStringList license;
	FStringList m_replaces;
	FStringList m_groups;
	FStringList m_files;
	FStringList m_backup;
	FStringList m_depends;
	FStringList m_removes;
	FStringList m_requiredby;
	FStringList m_conflicts;
	FStringList m_provides;
	FStringList m_triggers;
	char *m_path;
};

	typedef flib::refcounted_ptr<package> package_ptr;
	bool operator < (const package_ptr &pkg1, const package_ptr &pkg2);

	typedef FList<package_ptr> package_list;
	typedef flib::set<libpacman::package_ptr> package_set;

class PackageMatcher
{
public:
	PackageMatcher(const char *str, int flags = PM_PACKAGE_FLAG_NAME, int strmatcher_flags = FStrMatcher::EQUAL);
	PackageMatcher(const FStrMatcher *strmatcher, int flags);
	~PackageMatcher();

	bool match(const libpacman::package_ptr package, int mask = ~0) const;

private:
	const FStrMatcher *m_strmatcher;
	int m_flags;
	FStrMatcher m_strmatcher_internal;
};

	class package_node
		: public flib::refcounted
	{
	public:
		package_node(const char *name);
		~package_node();
		bool operator < (const package_node &o) const;

		const char *name() const;

	private:
		char m_name[PKG_NAME_LEN];
		libpacman::package_set m_packages;
	};

	typedef flib::refcounted_ptr<libpacman::package_node> package_node_ptr;
	bool operator < (const package_node_ptr &pn1, const package_node_ptr &pn2);

	typedef flib::set<libpacman::package_node_ptr> package_node_set;

	class package_graph
		: package_node_set
	{
	public:
		using package_node_set::package_node_set;
	};
} // namespace libpacman

const libpacman::package_ptr _pacman_pkg_isin(const char *needle, const libpacman::package_list &haystack);

#endif /* _PACMAN_PACKAGE_H */

/* vim: set ts=2 sw=2 noet: */
