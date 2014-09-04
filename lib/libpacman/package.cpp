/*
 *  package.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
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

#include "config.h"

/* pacman-g2 */
#include "package.h"

#include "db/localdb_files.h"
#include "util.h"
#include "error.h"
#include "db.h"
#include "handle.h"
#include "cache.h"
#include "pacman.h"

#include "io/archive.h"
#include "util/log.h"
#include "fstdlib.h"
#include "fstring.h"

#include <sys/utsname.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>

using namespace libpacman;

Package::Package(Database *database)
	: m_database(database), m_reason(PM_PKG_REASON_EXPLICIT)
{
}

Package::Package(const char *name, const char *version)
	: m_reason(PM_PKG_REASON_EXPLICIT)
{
	if(!_pacman_strempty(name)) {
		flags |= PM_PACKAGE_FLAG_NAME;
		STRNCPY(m_name, name, PKG_NAME_LEN);
	}
	if(!_pacman_strempty(version)) {
		flags |= PM_PACKAGE_FLAG_VERSION;
		STRNCPY(m_version, version, PKG_VERSION_LEN);
	}
}

Package::~Package()
{
	FREELIST(license);
	free(m_path);
}

Database *Package::database() const
{
	return m_database;
}

bool Package::set_filename(const char *filename, int witharch)
{
	if(splitname(filename, m_name, m_version, witharch)) {
		flags |= PM_PACKAGE_FLAG_NAME | PM_PACKAGE_FLAG_VERSION;
		return true;
	}
	return false;
}

int _pacman_pkg_delete(Package *self)
{
	self->release();
	return 0;
}

/* Helper function for comparing packages
 */
int _pacman_pkg_cmp(const void *p1, const void *p2)
{
	Package *pkg1 = (Package *)p1;
	Package *pkg2 = (Package *)p2;

	return pkg1 == pkg2 ? 0: strcmp(pkg1->name(), pkg2->name());
}

bool Package::is_valid(const pmtrans_t *trans, const char *pkgfile) const
{
	struct utsname name;

	if(_pacman_strempty(m_name)) {
		_pacman_log(PM_LOG_ERROR, _("missing package name in %s"), pkgfile);
		goto pkg_error;
	}
	if(_pacman_strempty(m_version)) {
		_pacman_log(PM_LOG_ERROR, _("missing package version in %s"), pkgfile);
		goto pkg_error;
	}
	if(strchr(m_version, '-') != strrchr(m_version, '-')) {
		_pacman_log(PM_LOG_ERROR, _("version contains additional hyphens in %s"), pkgfile);
		goto invalid_name_error;
	}
	if (trans != NULL && !(trans->flags & PM_TRANS_FLAG_NOARCH)) {
		if(_pacman_strempty(arch)) {
			_pacman_log(PM_LOG_ERROR, _("missing package architecture in %s"), pkgfile);
			goto pkg_error;
		}

		uname (&name);
		if(strncmp(name.machine, arch, strlen(arch))) {
			_pacman_log(PM_LOG_ERROR, _("wrong package architecture in %s"), pkgfile);
			goto arch_error;
		}
	}
	return true;

invalid_name_error:
	pm_errno = PM_ERR_PKG_INVALID_NAME;
	return false;

arch_error:
	pm_errno = PM_ERR_WRONG_ARCH;
	return false;

pkg_error:
	pm_errno = PM_ERR_PKG_INVALID;
	return false;
}

/* Test for existence of a package in a FPtrList*
 * of Package*
 */
Package *_pacman_pkg_isin(const char *needle, FPtrList *haystack)
{
	if(needle == NULL || haystack == NULL) {
		return(NULL);
	}

	for(auto lp = haystack->begin(), end = haystack->end(); lp != end; lp = lp->next()) {
		Package *info = f_ptrlistitem_data(lp);

		if(info && !strcmp(info->name(), needle)) {
			return info;
		}
	}
	return(NULL);
}

bool Package::splitname(const char *target, char *name, char *version, int witharch)
{
	char *tmp;
	char *p, *q;

	if ((tmp = _pacman_basename(target)) == NULL) {
		return false;
	}
	/* trim file extension (if any) */
	if((p = strstr(tmp, PM_EXT_PKG))) {
		*p = 0;
	}
	if(witharch) {
		/* trim architecture */
		if((p = strrchr(tmp, '-'))) {
			*p = 0;
		}
	}

	p = tmp + strlen(tmp);

	for(q = --p; *q && *q != '-'; q--);
	if(*q != '-' || q == tmp) {
		return false;
	}
	for(p = --q; *p && *p != '-'; p--);
	if(*p != '-' || p == tmp) {
		return false;
	}
	if(version) {
		STRNCPY(version, p+1, PKG_VERSION_LEN);
	}
	*p = 0;

	if(name) {
		STRNCPY(name, tmp, PKG_NAME_LEN);
	}

	return true;
}

int Package::read(unsigned int flags)
{
	return -1;
}

int Package::write(unsigned int flags)
{
	int ret;

	ASSERT(m_database != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	if((ret = m_database->write(this, flags)) != 0) {
		_pacman_log(PM_LOG_ERROR, _("could not update requiredby for database entry %s-%s"),
			name(), version());
	}
	return ret;
}

int Package::remove()
{
	return -1;
}

int Package::filename(char *str, size_t size) const
{
	return snprintf(str, size, "%s-%s-%s%s",
			m_name, m_version, arch, PM_EXT_PKG);
}

/* Look for a filename in a Package.backup list.  If we find it,
 * then we return the md5 or sha1 hash (parsed from the same line)
 */
char *Package::fileneedbackup(const char *file) const
{
	ASSERT(!_pacman_strempty(file), RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	/* run through the backup list and parse out the md5 or sha1 hash for our file */
	for(auto lp = m_backup.begin(), end = m_backup.end(); lp != end; lp = lp->next()) {
		char *str = strdup(f_stringlistitem_to_str(lp));
		char *ptr;

		/* tab delimiter */
		ptr = strchr(str, '\t');
		if(ptr == NULL) {
			free(str);
			continue;
		}
		*ptr = '\0';
		ptr++;
		/* now str points to the filename and ptr points to the md5 or sha1 hash */
		if(!strcmp(file, str)) {
			char *hash = strdup(ptr);
			free(str);
			return hash;
		}
		free(str);
	}

	return(NULL);
}

#define LIBPACMAN_PACKAGE_PROPERTY(type, name, flag)                           \
type Package::name()                                                           \
{                                                                              \
	if((flags & PM_PACKAGE_FLAG_##flag) == 0) {                                  \
		read(PM_PACKAGE_FLAG_##flag);                                              \
	}                                                                            \
	return m_##name;                                                             \
}

#include "package_properties.h"

#undef LIBPACMAN_PACKAGE_PROPERTY

const char *Package::path() const
{
	return m_path;
}

const FStringList &Package::provides() const
{
	return m_provides;
}

bool Package::provides(const char *pkgname)
{
	return _pacman_list_is_strin(pkgname, &provides());
}

typedef struct FPackageStrMatcher FPackageStrMatcher;

struct FPackageStrMatcher
{
	int flags;
	const FStrMatcher *strmatcher;
};

static
int _pacman_strmatcher_match(const FStrMatcher *strmatcher, Package *pkg, int flags) {
	ASSERT(pkg != NULL, RET_ERR(PM_ERR_WRONG_ARGS, 0));
	
	/* FIXME: Make const when const accessors are available */
	if(((flags & PM_PACKAGE_FLAG_NAME) && strmatcher->match(pkg->name())) ||
			((flags & PM_PACKAGE_FLAG_VERSION) && strmatcher->match(pkg->version())) ||
			((flags & PM_PACKAGE_FLAG_DESCRIPTION) && strmatcher->match(pkg->description())) ||
			((flags & PM_PACKAGE_FLAG_BUILDDATE) && strmatcher->match(pkg->builddate)) ||
			((flags & PM_PACKAGE_FLAG_BUILDTYPE) && strmatcher->match(pkg->buildtype)) ||
			((flags & PM_PACKAGE_FLAG_INSTALLDATE) && strmatcher->match(pkg->installdate)) ||
			((flags & PM_PACKAGE_FLAG_PACKAGER) && strmatcher->match(pkg->packager)) ||
//			((flags & PM_PACKAGE_FLAG_HASH) && ) ||
			((flags & PM_PACKAGE_FLAG_ARCH) && strmatcher->match(pkg->arch)) ||
			((flags & PM_PACKAGE_FLAG_LOCALISED_DESCRIPTION) && f_stringlist_any_match(&pkg->desc_localized, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_LICENSE) && f_stringlist_any_match(pkg->license, strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_REPLACES) && f_stringlist_any_match(&pkg->replaces(), strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_GROUPS) && f_stringlist_any_match(&pkg->groups(), strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_FILES) && f_stringlist_any_match(&pkg->files(), strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_BACKUP) && f_stringlist_any_match(&pkg->backup(), strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_DEPENDS) && f_stringlist_any_match(&pkg->depends(), strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_REMOVES) && f_stringlist_any_match(&pkg->removes(), strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_REQUIREDBY) && f_stringlist_any_match(&pkg->requiredby(), strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_CONFLICTS) && f_stringlist_any_match(&pkg->conflicts(), strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_PROVIDES) && f_stringlist_any_match(&pkg->provides(), strmatcher)) ||
			((flags & PM_PACKAGE_FLAG_TRIGGERS) && f_stringlist_any_match(&pkg->triggers(), strmatcher))) {
		return 1;
	}
	return 0;
}

static
int PackageMatcher_match_cb(const void *ptr, const void *matcher_data)
{
	return ((const PackageMatcher *)matcher_data)->match((const Package *)ptr);
}

PackageMatcher::PackageMatcher(const char *pattern, int flags, int strmatcher_flags)
	: m_strmatcher(&m_strmatcher_internal), m_flags(flags), m_strmatcher_internal(pattern, strmatcher_flags)
{ }

PackageMatcher::PackageMatcher(const FStrMatcher *strmatcher, int flags)
	: m_strmatcher(strmatcher), m_flags(flags)
{ }

PackageMatcher::~PackageMatcher()
{ }

bool PackageMatcher::match(const libpacman::Package *package) const
{
	return match(package, ~0);
}

bool PackageMatcher::match(const Package *package, int mask = ~0) const
{
	return _pacman_strmatcher_match(m_strmatcher, package, m_flags & mask);
}

/* vim: set ts=2 sw=2 noet: */
