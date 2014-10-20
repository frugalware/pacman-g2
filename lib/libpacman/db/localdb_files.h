/*
 *  localdb_files.h
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
#ifndef _PACMAN_LOCALDB_FILES_H
#define _PACMAN_LOCALDB_FILES_H

#include <limits.h>

#include "package.h"

#define PM_LOCALPACKAGE_FLAGS_NONE     0

#define PM_LOCALPACKAGE_FLAGS_DESC ( \
	PM_PACKAGE_FLAG_ARCH | \
	PM_PACKAGE_FLAG_BUILDDATE | \
	PM_PACKAGE_FLAG_BUILDTYPE | \
	PM_PACKAGE_FLAG_DESCRIPTION | \
	PM_PACKAGE_FLAG_FORCE | \
	PM_PACKAGE_FLAG_GROUPS | \
	/*PM_PACKAGE_FLAG_HASH |*/ \
	/*PM_PACKAGE_FLAG_HASH_MD5SUM |*/ \
	/*PM_PACKAGE_FLAG_HASH_SHA1SUM |*/ \
	/*PM_PACKAGE_FLAG_LOCALISED_DESCRIPTION |*/ \
	PM_PACKAGE_FLAG_INSTALLDATE | \
	PM_PACKAGE_FLAG_LICENSE | \
	PM_PACKAGE_FLAG_NAME | \
	PM_PACKAGE_FLAG_PACKAGER | \
	PM_PACKAGE_FLAG_REASON | \
	/*PM_PACKAGE_FLAG_REMOVES |*/ \
	PM_PACKAGE_FLAG_REPLACES | \
	/*PM_PACKAGE_FLAG_SIZE |*/ \
	PM_PACKAGE_FLAG_STICKY | \
	PM_PACKAGE_FLAG_URL | \
	PM_PACKAGE_FLAG_TRIGGERS | \
	/*PM_PACKAGE_FLAG_USIZE |*/ \
	PM_PACKAGE_FLAG_VERSION | \
	0 )

#define PM_LOCALPACKAGE_FLAGS_DEPENDS ( \
	PM_PACKAGE_FLAG_CONFLICTS | \
	PM_PACKAGE_FLAG_DEPENDS | \
	PM_PACKAGE_FLAG_PROVIDES | \
	PM_PACKAGE_FLAG_REQUIREDBY | \
	0 )

#define PM_LOCALPACKAGE_FLAGS_FILES ( \
	PM_PACKAGE_FLAG_BACKUP | \
	PM_PACKAGE_FLAG_FILES | \
	0 )
	
#define PM_LOCALPACKAGE_FLAGS_SCRIPLET ( \
	PM_PACKAGE_FLAG_SCRIPTLET  | \
	0 )

#define PM_LOCALPACKAGE_FLAGS_ALL ( \
	PM_LOCALPACKAGE_FLAGS_DESC | \
	PM_LOCALPACKAGE_FLAGS_DEPENDS | \
	PM_LOCALPACKAGE_FLAGS_FILES | \
	PM_LOCALPACKAGE_FLAGS_SCRIPLET | \
	0 )

#define INFRQ_DESC PM_LOCALPACKAGE_FLAGS_DESC
#define INFRQ_DEPENDS PM_LOCALPACKAGE_FLAGS_DEPENDS
#define INFRQ_FILES PM_LOCALPACKAGE_FLAGS_FILES
#define INFRQ_SCRIPLET PM_LOCALPACKAGE_FLAGS_SCRIPLET
#define INFRQ_ALL PM_LOCALPACKAGE_FLAGS_ALL 

int _pacman_localdb_desc_fread(libpacman::package_ptr info, FILE *fp);
int _pacman_localdb_depends_fread(libpacman::package_ptr info, FILE *fp);
int _pacman_localdb_files_fread(libpacman::package_ptr info, FILE *fp);

#endif /* _PACMAN_LOCALDB_FILES_H */

/* vim: set ts=2 sw=2 noet: */
