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

#include "pacman.h"

int _pacman_localdb_desc_fread(pmpkg_t *info, FILE *fp);
int _pacman_localdb_depends_fread(pmpkg_t *info, FILE *fp);
int _pacman_localdb_files_fread(pmpkg_t *info, FILE *fp);

#endif /* _PACMAN_LOCALDB_FILES_H */

/* vim: set ts=2 sw=2 noet: */
