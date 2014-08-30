/*
 *  deps.h
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
#ifndef _PACMAN_DEPS_H
#define _PACMAN_DEPS_H

#include "db.h"
#include "sync.h"

typedef struct __pmdepend_t {
	unsigned char mod;
	char name[PKG_NAME_LEN];
	char version[PKG_VERSION_LEN];
} pmdepend_t;

struct __pmdepmissing_t {
	__pmdepmissing_t(const char *target, unsigned char type, unsigned char depmod,
			const char *depname, const char *depversion);

	char target[PKG_NAME_LEN];
	unsigned char type;
	pmdepend_t depend;
};

typedef struct __pmgraph_t {
	int state; /* 0: untouched, -1: entered, other: leaving time */
	void *data;
	struct __pmgraph_t *parent; /* where did we come from? */
	FPtrList *children;
	FPtrListIterator *childptr; /* points to a child in children list */
} pmgraph_t;

FPtrList *_pacman_depmisslist_add(FPtrList *misslist, pmdepmissing_t *miss);

FPtrList *_pacman_sortbydeps(FPtrList *targets, int mode);
FPtrList *_pacman_checkdeps(pmtrans_t *trans, unsigned char op, FPtrList *packages);
int _pacman_splitdep(char *depstr, pmdepend_t *depend);
FPtrList *_pacman_removedeps(libpacman::Database *db, FPtrList *targs);
int _pacman_resolvedeps(pmtrans_t *trans, libpacman::Package *syncpkg, FPtrList *list,
                FPtrList *trail, FPtrList **data);
int _pacman_depcmp(libpacman::Package *pkg, pmdepend_t *dep);

#endif /* _PACMAN_DEPS_H */

/* vim: set ts=2 sw=2 noet: */
