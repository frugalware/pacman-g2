/*
 *  sync.h
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005, 2006 by Miklos Vajna <vmiklos@frugalware.org>
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
#ifndef _PACMAN_SYNC_H
#define _PACMAN_SYNC_H

#include "db.h"
#include "package.h"
#include "trans.h"

struct __pmsyncpkg_t
	: public ::flib::FObject
{
	__pmsyncpkg_t(int type, libpacman::Package *spkg);
	~__pmsyncpkg_t();

	unsigned char type;
	const char *pkg_name;
	libpacman::Package *pkg_new;
	libpacman::Package *pkg_local;
	FPtrList m_replaces;
};

#endif /* _PACMAN_SYNC_H */

/* vim: set ts=2 sw=2 noet: */
