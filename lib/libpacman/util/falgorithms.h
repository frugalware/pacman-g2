/*
 *  falgorithms.h
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
#ifndef FALGORITHMS_H
#define FALGORITHMS_H

#include <memory> // For std::unique_ptr
#include <vector> // For std::vector

#include "util.h" // for ASSERT

namespace flib
{

template <typename T>
T fdefault_constructor()
{
	return T();
}

template <typename T>
struct FAccumulator
{
	T value;

	FAccumulator()
		: value(fdefault_constructor<T>())
	{ }

	template <typename Any>
	const T &operator += (const Any &any)
	{
		return value += any;
	}
};

template <>
struct FAccumulator<void>
{
	template <typename Any>
	void operator += (const Any &any)
	{ }
};

template <typename>
class FCallable;

template <typename R, typename... Args>
struct FCallable<R(Args...)>
{
	virtual ~FCallable()
	{ }

	virtual R operator () (Args...) const = 0;
};

}

#endif /* FALGORITHMS_H */

/* vim: set ts=2 sw=2 noet: */
