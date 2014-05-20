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

#include <memory> // For std::unique_ptr
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
	std::vector<std::unique_ptr<flib::FCallable<R(Args...)>>> m_connections;
	typedef typename std::vector<std::unique_ptr<flib::FCallable<R(Args...)>>>::value_type value_type;
	typedef typename std::vector<std::unique_ptr<flib::FCallable<R(Args...)>>>::const_reference const_reference;

	bool connect(FCallable<R(Args...)> *connection)
	{
		ASSERT(connection != nullptr, return false);
		m_connections.push_back(value_type(connection));
		return true;
	}
};

}

template <typename>
class FSignal;

template <typename R, typename... Args>
class FSignal<R(Args...)>
	: protected flib::detail::FSignalBase<R(Args...)>
{
	typedef flib::detail::FSignalBase<R(Args...)> super_type;
	typedef typename super_type::const_reference const_reference;

	template <typename FunctionR, typename... FunctionArgs>
	struct FFunctionConnection
		: flib::FCallable<R(Args...)>
	{
		typedef FunctionR (*function_type)(FunctionArgs..., ...);

		function_type m_function;

		FFunctionConnection(FunctionR (*function)(FunctionArgs...))
			: m_function((function_type)function)
		{ }

		R operator () (Args... args) const
		{
			return m_function(args...);
		}
	};

	template <typename Receiver, typename MethodR, typename... MethodArgs>
	struct FMethodConnection
		: flib::FCallable<R(Args...)>
	{
		typedef MethodR (Receiver::*method_type)(MethodArgs..., ...);

		Receiver *m_receiver;
		method_type m_method;

		FMethodConnection(Receiver *receiver, MethodR (Receiver::*method)(MethodArgs...))
			: m_receiver(receiver), m_method((method_type)method)
		{ }

		R operator () (Args... args) const
		{
			return (m_receiver->*m_method)(args...);
		}
	};

public:
	template <typename FunctionR, typename... FunctionArgs>
	bool connect(FunctionR (*function)(FunctionArgs...))
	{
		ASSERT(function != nullptr, return false);

		return super_type::connect(new FFunctionConnection<FunctionR, FunctionArgs...>(function));
	}

	template <typename Receiver, typename MethodR, typename... MethodArgs>
	bool connect(Receiver *receiver, MethodR (Receiver::*method)(MethodArgs...))
	{
		ASSERT(receiver != nullptr, return false);
		ASSERT(receiver != nullptr, return false);

		return super_type::connect(new FMethodConnection<Receiver, MethodR, MethodArgs...>(receiver, method));
	}

	template <typename Acc = flib::FAccumulator<void>>
	Acc operator () (Args... args) const
	{
		Acc accumulator = flib::fdefault_constructor<Acc>();

		for(const_reference callable : super_type::m_connections) {
			accumulator += (*callable)(args...);
		}
		return accumulator;
	}

	template <typename Acc = flib::FAccumulator<void>>
	inline Acc emit(Args... args) const
	{
		return operator () <Acc> (args...);
	}
};

template <typename... Args>
class FSignal<void(Args...)>
	: protected flib::detail::FSignalBase<void(Args...)>
{
	typedef flib::detail::FSignalBase<void(Args...)> super_type;
	typedef typename super_type::const_reference const_reference;

	template <typename FunctionR, typename... FunctionArgs>
	struct FFunctionConnection
		: flib::FCallable<void(Args...)>
	{
		typedef void (*function_type)(FunctionArgs..., ...);

		function_type m_function;

		FFunctionConnection(FunctionR (*function)(FunctionArgs...))
			: m_function((function_type)function)
		{ }

		void operator () (Args... args) const
		{
			m_function(args...);
		}
	};

	template <typename Receiver, typename MethodR, typename... MethodArgs>
	struct FMethodConnection
		: flib::FCallable<void(Args...)>
	{
		typedef void (Receiver::*method_type)(MethodArgs..., ...);

		Receiver *m_receiver;
		method_type m_method;

		FMethodConnection(Receiver *receiver, MethodR (Receiver::*method)(MethodArgs...))
			: m_receiver(receiver), m_method((method_type)method)
		{ }

		void operator () (Args... args) const
		{
			(m_receiver->*m_method)(args...);
		}
	};

public:
	template <typename FunctionR, typename... FunctionArgs>
	bool connect(FunctionR (*function)(FunctionArgs...))
	{
		ASSERT(function != nullptr, return false);

		return super_type::connect(new FFunctionConnection<FunctionR, FunctionArgs...>(function));
	}

	template <typename Receiver, typename MethodR, typename... MethodArgs>
	bool connect(Receiver *receiver, MethodR (Receiver::*method)(MethodArgs...))
	{
		ASSERT(receiver != nullptr, return false);
		ASSERT(receiver != nullptr, return false);

		return super_type::connect(new FMethodConnection<Receiver, MethodR, MethodArgs...>(receiver, method));
	}
				  
	void operator () (Args... args) const
	{
		for(const_reference callable : super_type::m_connections) {
			(*callable)(args...);
		}
	}

	inline void emit(Args... args) const
	{
		operator () (args...);
	}
};

}

#endif /* FSIGNAL_H */

/* vim: set ts=2 sw=2 noet: */
