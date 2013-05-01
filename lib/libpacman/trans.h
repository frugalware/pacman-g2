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
typedef struct __pmsyncpkg_t pmtranspkg_t;

#include "handle.h"

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

typedef struct __pmtrans_ops_t {
	int (*commit)(pmtrans_t *trans, pmlist_t **data);
} pmtrans_ops_t;

typedef struct __pmtrans_cbs_t {
	pacman_trans_cb_event event;
	pacman_trans_cb_conv conv;
	pacman_trans_cb_progress progress;
} pmtrans_cbs_t;

struct __pmtrans_t {
	struct pmobject base;

	const pmtrans_ops_t *ops;
	int (*set_state)(pmtrans_t *trans, int new_state);
	pmhandle_t *handle;
	pmtranstype_t type;
	unsigned int flags;
	unsigned char state;
	pmlist_t *targets;     /* pmlist_t of (char *) */
	pmlist_t *_packages;   /* pmlist_t of (pmpkg_t *) (TO BE REMOVED) */
	pmlist_t *packages;    /* pmlist_t of (pmsyncpkg_t *) */
	pmlist_t *skiplist;    /* pmlist_t of (char *) */
	pmtrans_cbs_t cbs;
};

#define FREETRANS(p) \
do { \
	if(p) { \
		_pacman_trans_free(p); \
		p = NULL; \
	} \
} while (0)
#define EVENT(_t, e, d1, d2) \
do { \
	pmtrans_t *t = (_t); \
	if(t && t->cbs.event) { \
		t->cbs.event((e), (d1), (d2)); \
	} \
} while(0)
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

pmtrans_t *_pacman_trans_new(void);
void _pacman_trans_free(pmtrans_t *trans);

int _pacman_trans_set_state(pmtrans_t *trans, int new_state);
int _pacman_trans_addtarget(pmtrans_t *trans, const char *target, pmtranstype_t type, unsigned int flags);
int _pacman_trans_prepare(pmtrans_t *trans, pmlist_t **data);
int _pacman_trans_commit(pmtrans_t *trans, pmlist_t **data);

int _pacman_trans_sysupgrade(pmtrans_t *trans);

/* FIXME: Make private when unification is done */
/* RENAMEME: struct __pmtrans_pkg  */
typedef struct __pmsyncpkg_t {
	pmtranstype_t type;
	pmpkg_t *pkg_new;
	pmpkg_t *pkg_local;
	pmlist_t *replaces; /* list of (pmpkg_t *) */
	int flags;
} pmsyncpkg_t;

pmsyncpkg_t *__pacman_trans_pkg_new (int type, pmpkg_t *spkg);
void __pacman_trans_pkg_delete (pmsyncpkg_t *pkg);

const char *__pacman_transpkg_name (pmtranspkg_t *transpkg);
int __pacman_transpkg_cmp (pmtranspkg_t *transpkg1, pmtranspkg_t *transpkg2);
int __pacman_transpkg_detect_name (pmtranspkg_t *transpkg, const char *package);

/* Implementation details */
int __pacman_trans_init(pmtrans_t *trans, pmtranstype_t type, unsigned int flags, pmtrans_cbs_t cbs);
/* void __pacman_trans_fini(pmtrans_t *trans); */

pmsyncpkg_t *__pacman_trans_get_trans_pkg(pmtrans_t *trans, const char *package);

#endif /* _PACMAN_TRANS_H */

/* vim: set ts=2 sw=2 noet: */
