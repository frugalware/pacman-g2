/*
 * pacman_p.h
 *
 *  Copyright (c) 2014 by Michel Hermier <hermier@frugalware.org>
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
#ifndef _PACMAN_P_H
#define _PACMAN_P_H

#include "pacman.h"

#include "util/fptrlist.h" 

#define DEFINE_CAST(c_type, cxx_type)         \
static inline c_type *c_cast(cxx_type *obj)   \
{ return (c_type *)obj; }                     \
																							\
static inline c_type *c_cast(cxx_type &obj)   \
{ return (c_type *)&obj; }                    \
                                              \
static inline cxx_type *cxx_cast(c_type *obj) \
{ return (cxx_type *)obj; }

namespace libpacman {

class Database;
class Group;
class Handle;
class Package;

}

DEFINE_CAST(struct __pmdb_t, libpacman::Database)
DEFINE_CAST(struct __pmgrp_t, libpacman::Group)
DEFINE_CAST(struct __pmhandle_t, libpacman::Handle)
DEFINE_CAST(struct __pmlist_t, FPtrList)
//DEFINE_CAST(struct __pmlist_iterator_t, FPtrListItem)
DEFINE_CAST(struct __pmpkg_t, libpacman::Package)
//DEFINE_CAST(struct __pmtrans_t, libpacman::Transaction)

template <typename T>
static inline __pmlist_t *c_cast(FList<T> &obj)
{ return (__pmlist_t *)&obj; }

template <typename T>
static inline __pmlist_t *c_cast(FList<T> *obj)
{ return (__pmlist_t *)obj; }

#undef DEFINE_CAST

#endif /* _PACMAN_P_H */

/* vim: set ts=2 sw=2 noet: */
