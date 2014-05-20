/*
 *  ffunctional.h
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
#ifndef FFUNCTIONAL_H
#define FFUNCTIONAL_H

#include <functional>

namespace flib
{

template <typename>
class FCallable;

template <typename R, typename... Args>
struct FCallable<R(Args...)>
{
	typedef R function_type(Args...);

	virtual ~FCallable()
	{ }

	virtual R operator () (Args...) const = 0;
};

template <typename R, typename... Args>
struct FCallable<R(Args..., ...)>
{
	typedef R function_type(Args..., ...);

	virtual ~FCallable()
	{ }

	virtual R operator () (Args..., ...) const = 0;
};

}

#endif /* FFUNCTIONAL_H */

/* vim: set ts=2 sw=2 noet: */
