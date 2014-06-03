/*
 *  trans.h
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005 by Christian Hamar <krics@linuxforum.hu>
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
#ifndef _PACMAN_TRANS_H
#define _PACMAN_TRANS_H

typedef struct __pmtrans_t pmtrans_t;

#include "handle.h"

#include "kernel/fobject.h"
#include "util/stringlist.h"

namespace libpacman {

class Handle;
class Package;

}

enum {
	STATE_IDLE = 0,
	STATE_INITIALIZED,
	STATE_PREPARED,
	STATE_DOWNLOADING,
	STATE_COMMITING,
	STATE_COMMITED,
	STATE_INTERRUPTED,
	STATE_MAX
};

typedef struct __pmtrans_cbs_t {
	pacman_trans_cb_event event;
	pacman_trans_cb_conv conv;
	pacman_trans_cb_progress progress;
} pmtrans_cbs_t;

struct __pmtrans_t
	: public ::flib::FObject
{
	__pmtrans_t(pmtranstype_t type, unsigned int flags, pmtrans_cbs_t cbs);
	~__pmtrans_t();

	pmsyncpkg_t *find(const char *pkgname) const;

	int prepare(pmlist_t **data);
	int commit(pmlist_t **data);

	pmsyncpkg_t *add(pmsyncpkg_t *syncpkg, int flags);
	int add(libpacman::Package *pkg, pmtranstype_t type, int flags);
	int add(const char *target, pmtranstype_t type, int flags);

	int (*set_state)(pmtrans_t *trans, int new_state);
	libpacman::Handle *handle;
	pmtranstype_t m_type;
	unsigned int flags;
	unsigned char state;
	FStringList *targets;
	pmlist_t *packages;    /* pmlist_t of (pmpkg_t *) or (pmsyncpkg_t *) */
	FPtrList *syncpkgs; /* FPtrList of (pmsyncpkg_t *) */
	FStringList *skiplist;
	FStringList *triggers;
	pmtrans_cbs_t cbs;

private:
};

#define EVENT(t, e, d1, d2) \
	_pacman_trans_event((t), (e), (d1), (d2))
#define QUESTION(_t, q, d1, d2, d3, r) \
do { \
	pmtrans_t *t = (_t); \
	if(t && t->cbs.conv) { \
		t->cbs.conv((q), (d1), (d2), (d3), (r)); \
	} \
} while(0)
#define PROGRESS(_t, e, p, per, h, r) \
do { \
	pmtrans_t *t = (_t); \
	if(t && t->cbs.progress) { \
		t->cbs.progress((e), (p), (per), (h), (r)); \
	} \
} while(0)

int _pacman_trans_event(pmtrans_t *trans, unsigned char, void *, void *);

int _pacman_trans_sysupgrade(pmtrans_t *trans);

#endif /* _PACMAN_TRANS_H */

/* vim: set ts=2 sw=2 noet: */
