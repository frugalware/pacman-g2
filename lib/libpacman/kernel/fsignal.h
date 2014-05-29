/*
 *  fsignal.h
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
#ifndef FSIGNAL_H
#define FSIGNAL_H

#include "util/falgorithms.h"
#include "util/ffunctional.h"

#include <vector> // For std::vector

#include "util.h" // for ASSERT

namespace flib
{

namespace detail
{

template <typename>
class FSignalBase;

template <typename R, typename... Args>
class FSignalBase<R(Args...)>
{
protected:
	typedef std::vector<flib::FFunction<R(Args...)>> connections_container;
	typedef typename connections_container::value_type value_type;
	typedef typename connections_container::const_reference const_reference;

	connections_container m_connections;

public:
	bool connect(const flib::FFunction<R(Args...)>& function)
	{
		ASSERT(function, return false);

		m_connections.push_back(function);
		return true;
	}

	template <typename Pointer, typename MethodSignature>
	bool connect(Pointer pointer, MethodSignature method)
	{
		return connect(flib::FFunction<R(Args...)>(pointer, method));
	}

	template <typename Pointer, typename MethodSignature, typename Receiver>
	bool connect(Pointer *pointer, MethodSignature method, const Receiver *)
	{
		return connect(flib::FFunction<R(Args...)>(pointer, method, (const Receiver *)nullptr));
	}
};

}

template <typename>
class FSignal;

template <typename R, typename... Args>
class FSignal<R(Args...)>
	: public flib::detail::FSignalBase<R(Args...)>
{
	typedef flib::detail::FSignalBase<R(Args...)> super_type;
	typedef typename super_type::const_reference const_reference;

public:
	template <typename Accumulator = flib::FAccumulator<void>>
	Accumulator operator()(Args... args) const
	{
		Accumulator accumulator = flib::fdefault_constructor<Accumulator>();

		for(const_reference connection : super_type::m_connections) {
			accumulator += connection(args...);
		}
		return accumulator;
	}

	template <typename Accumulator = flib::FAccumulator<void>>
	inline Accumulator emit(Args... args) const
	{
		return operator()<Accumulator> (args...);
	}
};

template <typename... Args>
class FSignal<void(Args...)>
	: public flib::detail::FSignalBase<void(Args...)>
{
	typedef flib::detail::FSignalBase<void(Args...)> super_type;
	typedef typename super_type::const_reference const_reference;

public:
	void operator()(Args... args) const
	{
		for(const_reference connection : super_type::m_connections) {
			connection(args...);
		}
	}

	inline void emit(Args... args) const
	{
		operator()(args...);
	}
};

}

#endif /* FSIGNAL_H */

/* vim: set ts=2 sw=2 noet: */
