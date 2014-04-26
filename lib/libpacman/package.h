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

#include "object.h"
#include "trans.h"

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
#define PM_PACKAGE_FLAG_CONFLITS              (1<<18)
#define PM_PACKAGE_FLAG_PROVIDES              (1<<19)
#define PM_PACKAGE_FLAG_TRIGGERS              (1<<20)
#define PM_PACKAGE_FLAG_PACKAGER              (1<<21)
#define PM_PACKAGE_FLAG_FORCE                 (1<<22)
#define PM_PACKAGE_FLAG_REASON                (1<<23)
#define PM_PACKAGE_FLAG_SCRIPTLET             (1<<24)
#define PM_PACKAGE_FLAG_STICKY                (1<<25)

struct __pmpkg_operations_t {
	struct __pmobject_operations_t as_pmobject_operations_t;

	int (*read)(pmpkg_t *pkg, unsigned int flags);
	int (*write)(pmpkg_t *pkg, unsigned int flags); /* Optional */
	int (*remove)(pmpkg_t *pkg); /* Optional */
};

/* IMPROVEMENT: Add a dirty_flags to know what flags needs to be written */
struct __pmpkg_t {
	struct __pmobject_t as_pmobject_t;
	libpacman::Database *database;

	unsigned int flags;
	char name[PKG_NAME_LEN];
	char version[PKG_VERSION_LEN];
	char desc[PKG_DESC_LEN];
	char url[PKG_URL_LEN];
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
	unsigned char force;
	unsigned char stick;
	time_t date;
	unsigned char reason;
	FStringList *desc_localized;
	FStringList *license;
	FStringList *replaces;
	FStringList *groups;
	FStringList *files;
	FStringList *backup;
	FStringList *depends;
	FStringList *removes;
	FStringList *requiredby;
	FStringList *conflicts;
	FStringList *provides;
	FStringList *triggers;
	/* internal */
	unsigned char origin;
	void *data;
	unsigned char infolevel;
};

#define FREEPKG(p) \
do { \
	if(p && _pacman_pkg_delete(p) == 0) { \
		p = NULL; \
	} \
} while(0)

#define FREELISTPKGS(p) _FREELIST(p, _pacman_pkg_delete)

pmpkg_t *_pacman_pkg_alloc(size_t size, const struct __pmpkg_operations_t *package_operations);

pmpkg_t *_pacman_pkg_new(const char *name, const char *version);
pmpkg_t *_pacman_pkg_new_from_filename(const char *filename, int witharch);
pmpkg_t *_pacman_pkg_dup(pmpkg_t *pkg);
int _pacman_pkg_delete(pmpkg_t *self);

int _pacman_pkg_init(pmpkg_t *self, libpacman::Database *db);
int _pacman_pkg_fini(pmpkg_t *self);

int _pacman_pkg_cmp(const void *p1, const void *p2);
int _pacman_pkg_is_valid(const pmpkg_t *pkg, const pmtrans_t *trans, const char *pkgfile);
pmpkg_t *_pacman_pkg_isin(const char *needle, pmlist_t *haystack);
int _pacman_pkg_splitname(const char *target, char *name, char *version, int witharch);

int _pacman_pkg_read(pmpkg_t *pkg, unsigned int flags);
int _pacman_pkg_write(pmpkg_t *pkg, unsigned int flags);
int _pacman_pkg_remove(pmpkg_t *pkg);

void *_pacman_pkg_getinfo(pmpkg_t *pkg, unsigned char parm);
pmlist_t *_pacman_pkg_getowners(const char *filename);

int _pacman_pkg_filename(char *str, size_t size, const pmpkg_t *pkg);
char *_pacman_pkg_fileneedbackup(const pmpkg_t *pkg, const char *file);

int _pacman_packagestrmatcher_init(FMatcher *matcher, const FStrMatcher *strmatcher, int flags);
int _pacman_packagestrmatcher_fini(FMatcher *matcher);

int _pacman_packagestrmatcher_set_flags(FMatcher *matcher, int flags);

namespace libpacman
{

class Package
	: public libpacman::Object
{
public:
	~Package();

	virtual int read(pmpkg_t *pkg, unsigned int flags) = 0;
	virtual int write(pmpkg_t *pkg, unsigned int flags); /* Optional */
	virtual	int remove(pmpkg_t *pkg); /* Optional */

	struct __pmpkg_t *d;
};

}

#endif /* _PACMAN_PACKAGE_H */

/* vim: set ts=2 sw=2 noet: */
