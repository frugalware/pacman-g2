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
#include <memory> // For std::shared_ptr
#include <typeinfo>
#include <type_traits>

#include "util.h" // For ASSERT

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

	virtual R operator()(Args...) const = 0;
};

template <typename R, typename... Args>
struct FCallable<R(Args..., ...)>
{
	typedef R function_type(Args..., ...);

	virtual ~FCallable()
	{ }

	virtual R operator()(Args..., ...) const = 0;
};

namespace detail
{

template <typename>
struct FAbstractFunctionImpl;

template <typename R, typename... Args>
struct FAbstractFunctionImpl<R(Args...)>
	: flib::FCallable<void(R*, Args...)>
{
	virtual const std::type_info &target_type() const = 0;
	virtual void *target() const = 0;

	virtual const std::type_info &target_method_type() const
	{
		return typeid(void);
	}
};

template <typename, typename, typename>
struct FFunctionImpl;

template <typename R, typename... Args, typename Target, typename FunctionR, typename... FunctionArgs>
struct FFunctionImpl<R(Args...), Target, FunctionR(FunctionArgs...)>
	: flib::detail::FAbstractFunctionImpl<R(Args...)>
{
	Target m_target;

	FFunctionImpl(Target target)
		: m_target(target)
	{ }

	const std::type_info &target_type() const
	{
		return typeid(Target);
	}

	virtual void *target() const
	{
		return (void *)(uintptr_t)m_target;
	}

	virtual void operator()(R *ret, Args... args) const
	{
		adapted(ret, args...);
	}

	template <typename T = R>
	inline void adapted(typename std::enable_if<!std::is_void<T>::value, FunctionR *>::type ret, FunctionArgs... args, ...) const
	{
		*ret = m_target(args...);
	}

	template <typename T = R>
	inline void adapted(typename std::enable_if<std::is_void<T>::value, void *>::type ret, FunctionArgs... args, ...) const
	{
		m_target(args...);
	}
};

template <typename, typename, typename>
struct FAbstractMethodImpl;

template <typename R, typename... Args, typename MethodSignature, typename Target>
struct FAbstractMethodImpl<R(Args...), MethodSignature, Target>
	: FAbstractFunctionImpl<R(Args...)>
{
	MethodSignature m_method;

	FAbstractMethodImpl(MethodSignature method)
		: m_method(method)
	{ }

	virtual const std::type_info &target_method_type() const
	{
		return typeid(MethodSignature);
	}
};

template <typename, typename, typename, typename, typename>
struct FMethodImpl;

template <typename R, typename... Args, typename MethodSignature, typename Target, typename Pointer, typename MethodR, typename... MethodArgs>
struct FMethodImpl<R(Args...), MethodSignature, Target, Pointer, MethodR(MethodArgs...)>
	: flib::detail::FAbstractMethodImpl<R(Args...), MethodSignature, Target>
{
	typedef flib::detail::FAbstractMethodImpl<R(Args...), MethodSignature, Target> super_type;

	Pointer m_target;

	FMethodImpl(Pointer target, MethodSignature method)
		: super_type(method)
		, m_target(target)
	{ }

  const std::type_info &target_type() const
	{
		return typeid(Target);
	}

	virtual void *target() const
	{
		return &(*m_target);
	}

	virtual void operator()(R *ret, Args... args) const
	{
		adapted(ret, args...);
	}

	template <typename T = R>
	inline void adapted(typename std::enable_if<!std::is_void<T>::value, MethodR *>::type ret, MethodArgs... args, ...) const
	{
		*ret = (m_target->*(super_type::m_method))(args...);
	}

	template <typename T = R>
	inline void adapted(typename std::enable_if<std::is_void<T>::value, void *>::type ret, MethodArgs... args, ...) const
	{
		((*m_target).*(super_type::m_method))(args...);
	}
};

}

template <typename>
class FFunction;

template <typename R, typename... Args>
class FFunction<R(Args...)>
{
	typedef std::shared_ptr<flib::detail::FAbstractFunctionImpl<R(Args...)>> shared_ptr;

	shared_ptr d;

	template <typename Pointer, typename MethodSignature, typename Target, typename VirtualSignature>
	void _connect(Pointer pointer, MethodSignature method)
	{
		ASSERT(pointer != nullptr, return;);
		ASSERT(method != nullptr, return;);

		d = shared_ptr(new flib::detail::FMethodImpl<R(Args...), MethodSignature, Target, Pointer, VirtualSignature>(pointer, method));
	}

public:
	FFunction()
	{ }

	FFunction( std::nullptr_t )
	{ }

	template <typename Target>
	FFunction(Target *target)
		: FFunction(target, &Target::operator())
	{ }

	template <typename FunctionR, typename... FunctionArgs>
	FFunction(FunctionR (*function)(FunctionArgs...))
	{
		ASSERT(function != nullptr, return;);

		d = shared_ptr(new flib::detail::FFunctionImpl<R(Args...), FunctionR (*)(FunctionArgs...), FunctionR(FunctionArgs...)>(function));
	}

	template <typename Pointer, typename MethodR, typename MethodTarget, typename... MethodArgs>
	FFunction(Pointer pointer, MethodR (MethodTarget::*method)(MethodArgs...))
		: FFunction(pointer, method, (MethodTarget *)nullptr)
	{ }

	template <typename Pointer, typename MethodR, typename MethodTarget, typename... MethodArgs, typename Target>
	FFunction(Pointer pointer, MethodR (MethodTarget::*method)(MethodArgs...), Target *)
	{
		_connect<Pointer, MethodR (MethodTarget::*)(MethodArgs...), Target *, MethodR(MethodArgs...)>(pointer, method);
	}

	template <typename Pointer, typename MethodR, typename MethodTarget, typename... MethodArgs>
	FFunction(Pointer pointer, MethodR (MethodTarget::*method)(MethodArgs...) const)
		: FFunction(pointer, method, (const MethodTarget *)nullptr)
	{ }

	template <typename Pointer, typename MethodR, typename MethodTarget, typename... MethodArgs, typename Target>
	FFunction(Pointer pointer, MethodR (MethodTarget::*method)(MethodArgs...) const, const Target *)
	{
		_connect<Pointer, MethodR (MethodTarget::*)(MethodArgs...) const, const Target *, MethodR(MethodArgs...)>(pointer, method);
	}

	template <typename T = R>
	typename std::enable_if<!std::is_void<T>::value, T>::type operator()(Args... args) const
	{
		T ret;
		d->operator()(&ret, args...);
		return ret;
	}

	template <typename T = R>
	typename std::enable_if<std::is_void<T>::value, T>::type operator()(Args... args) const
	{
		d->operator()(nullptr, args...);
	}

	inline operator bool() const
	{
		return (bool)d;
	}

	inline void swap(flib::FFunction<R(Args...)> &other)
	{
		d.swap(other.d);
	}

	inline const std::type_info &target_type() const
	{
		return d != nullptr ? d->target_type() : typeid(void); 
	}
#if 0
	template <typename T>
	inline T *target()
	{
		T *ret = nullptr;

		if(d != nullptr) {
			dynamic_cast<flib::detail::FAbstractFunctionImpl<R (Args...)> &>(*d);
		}
		return nullptr;
	}

	template <typename T>
	inline const T *target() const
	{
		return d != nullptr ? d->target() : nullptr;
	}
#endif
	template <typename T>
	inline const std::type_info &target_method_type() const
	{
		return d != nullptr ? d->target_method_type() : typeid(void);
	}
};

}

#endif /* FFUNCTIONAL_H */

/* vim: set ts=2 sw=2 noet: */
