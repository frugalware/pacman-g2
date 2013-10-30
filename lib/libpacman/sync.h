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

struct __pmsyncpkg_t {
	unsigned char type;
	pmpkg_t *pkg;
	void *data;
};

#define FREESYNC(p) do { if(p) { _pacman_sync_free(p); p = NULL; } } while(0)

pmsyncpkg_t *_pacman_sync_new(int type, pmpkg_t *spkg, void *data);
void _pacman_sync_free(void *data);
pmsyncpkg_t *find_pkginsync(char *needle, pmlist_t *haystack);

const pmtrans_ops_t _pacman_sync_pmtrans_opts;

#endif /* _PACMAN_SYNC_H */

/* vim: set ts=2 sw=2 noet: */
