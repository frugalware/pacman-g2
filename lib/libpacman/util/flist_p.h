/*
 *  flist_p.h
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2013-2014 by Michel Hermier <hermier@frugalware.org>
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
#ifndef F_LIST_P_H
#define F_LIST_P_H

#include "util/flist.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef F_NOCOMPAT
/* Chained list struct */
struct __pmlist_t {
	struct __pmlist_t *prev;
	struct __pmlist_t *next;
	void *data;
};
#else
struct FListItem {
	FListItem *next;
	FListItem *previous;
};

struct FList {
	FListItem as_FListItem;
};
#endif

#ifdef __cplusplus
}
#endif

#endif /* F_LIST_P_H */

/* vim: set ts=2 sw=2 noet: */
