/*
 *  fstddef.h
 *
 *  Copyright (c) 2013 by Michel Hermier <hermier@frugalware.org>
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
#ifndef F_STDDEF_H
#define F_STDDEF_H

#include <stddef.h>

#ifdef __GNUC__
#define f_typecmp(type1, type2) \
	__builtin_types_compatible_p (type1, type2)

#define f_typeof(expr) \
	__typeof__ (expr)

#define f_static_if(cond, expr_if_true, expr_if_false) \
	__builtin_choose_expr (cond, expr_if_true, expr_if_false)
#else
#warning Unsupported compiler
#endif

#define f_containerof(ptr, type, member) \
	((type *) ((char *)f_identity_cast (f_typeof (&((type *)0)->member), (ptr)) - offsetof (type, member)))

#define f_identity_cast(type, expr) \
	f_static_if (f_typecmp (type, f_typeof (expr)), \
			(expr), (void)0)

#define f_static_assert(cond) \
	(f_static_if (cond, 0, (void)0) == 0)

#define f_type_assert(expr, type) \
	f_static_assert (f_typecmp (f_typeof (expr), type))

#endif /* F_STDDEF_H */

/* vim: set ts=2 sw=2 noet: */
