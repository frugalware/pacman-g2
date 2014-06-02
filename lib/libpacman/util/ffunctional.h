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

#include "util/ftype_traits.h"

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
struct FAbstractFunctionImplBase;

template <typename R, typename... Args>
struct FAbstractFunctionImplBase<R(Args...)>
	: flib::FCallable<void(R *, Args...)>
{
	virtual const std::type_info &target_type() const = 0;

	virtual const std::type_info &method_type() const
	{
		return typeid(void);
	}
};

template <typename, typename>
struct FAbstractFunctionImpl;

template <typename R, typename... Args, typename Target>
struct FAbstractFunctionImpl<R(Args...), Target>
	: flib::detail::FAbstractFunctionImplBase<R(Args...)>
{
	const std::type_info &target_type() const
	{
		return typeid(Target);
	}

	virtual Target target() const = 0;
};

template <typename, typename, typename>
struct FFunctionImpl;

template <typename R, typename... Args, typename Target, typename FunctionR, typename... FunctionArgs>
struct FFunctionImpl<R(Args...), Target, FunctionR(FunctionArgs...)>
	: flib::detail::FAbstractFunctionImpl<R(Args...), Target>
{
	Target m_target;

	FFunctionImpl(Target target)
		: m_target(target)
	{ }

	virtual void operator()(R *ret, Args... args) const override
	{
		adapted(ret, args...);
	}

	template <typename T = R>
	inline void adapted(typename std::enable_if<!std::is_void<T>::value, T *>::type ret, FunctionArgs... args, ...) const
	{
		*ret = m_target(args...);
	}

	template <typename T = R>
	inline void adapted(typename std::enable_if<std::is_void<T>::value, T *>::type ret, FunctionArgs... args, ...) const
	{
		m_target(args...);
	}

	virtual Target target() const override
	{
		return m_target;
	}
};

template <typename, typename, typename>
struct FAbstractCallbackImpl;

template <typename R, typename... Args, typename Target, typename CallbackR, typename CallbackData, typename... CallbackArgs>
struct FAbstractCallbackImpl<R(Args...), Target, CallbackR(CallbackData, CallbackArgs...)>
  : flib::detail::FFunctionImpl<R(Args...), Target, CallbackR(CallbackArgs...)>
{
	virtual CallbackData callback_data() const = 0;
};

template <typename, typename, typename, typename>
struct FCallbackImpl;

template <typename R, typename... Args, typename Target, typename CallbackR, typename CallbackData, typename... CallbackArgs, typename CallbackDataHolder>
struct FCallbackImpl<R(Args...), Target, CallbackR(CallbackData, CallbackArgs...), CallbackDataHolder>
	: flib::detail::FFunctionImpl<R(Args...), Target, CallbackR(CallbackArgs...)>
{
	CallbackDataHolder m_data;

	FCallbackImpl(Target target, CallbackDataHolder data)
		: flib::detail::FFunctionImpl<R(Args...), Target, CallbackR(CallbackArgs...)>(target)
		, m_data(data)
	{ }

	virtual void operator()(R *ret, Args... args) const override
	{
		adapted(ret, args...);
	}

	template <typename T = R>
	inline void adapted(typename std::enable_if<!std::is_void<T>::value, T *>::type ret, CallbackArgs... args, ...) const
	{
		*ret = m_target(m_data, args...);
	}

	template <typename T = R>
	inline void adapted(typename std::enable_if<std::is_void<T>::value, T *>::type ret, CallbackArgs... args, ...) const
	{
		m_target(m_data, args...);
	}

	virtual CallbackData callback_data() const override
	{
		return m_data;
	}
};

template <typename, typename, typename>
struct FAbstractMethodImpl;

template <typename R, typename... Args, typename MethodSignature, typename Target>
struct FAbstractMethodImpl<R(Args...), MethodSignature, Target>
	: FAbstractFunctionImpl<R(Args...), Target>
{
	MethodSignature m_method;

	FAbstractMethodImpl(MethodSignature method)
		: m_method(method)
	{ }

	virtual const std::type_info &method_type() const override
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

	virtual void operator()(R *ret, Args... args) const override
	{
		adapted(ret, args...);
	}

	template <typename T = R>
	inline void adapted(typename std::enable_if<!std::is_void<T>::value, T *>::type ret, MethodArgs... args, ...) const
	{
		*ret = (m_target->*(super_type::m_method))(args...);
	}

	template <typename T = R>
	inline void adapted(typename std::enable_if<std::is_void<T>::value, T *>::type ret, MethodArgs... args, ...) const
	{
		((*m_target).*(super_type::m_method))(args...);
	}

	virtual Target target() const override
	{
		return &(*m_target);
	}
};

template <typename>
struct FSignature;

template <typename FunctionR, typename... FunctionArgs>
struct FSignature<FunctionR (*)(FunctionArgs...)>
{
	typedef FunctionR return_type;
	typedef FunctionR(function_type)(FunctionArgs...);
	typedef function_type *pfunction_type;
};

template <typename MethodR, typename MethodTarget, typename... MethodArgs>
struct FSignature<MethodR (MethodTarget::*)(MethodArgs...)>
{
	typedef MethodR return_type;
	typedef MethodR(function_type)(MethodArgs...);
	typedef MethodTarget target_type;
	typedef target_type *ptarget_type;
};

template <typename MethodR, typename MethodTarget, typename... MethodArgs>
struct FSignature<MethodR (MethodTarget::*)(MethodArgs...) const>
{
	typedef MethodR return_type;
	typedef MethodR(function_type)(MethodArgs...);
	typedef const MethodTarget target_type;
	typedef target_type *ptarget_type;
};

}

template <typename>
class FFunction;

template <typename R, typename... Args>
class FFunction<R(Args...)>
{
	typedef std::shared_ptr<flib::detail::FAbstractFunctionImplBase<R(Args...)>> shared_ptr;

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

	template <typename Pointer, typename MethodSignature>
	FFunction(Pointer pointer, MethodSignature method)
		: FFunction(pointer, method, (typename flib::detail::FSignature<MethodSignature>::ptarget_type) nullptr)
	{ }

	template <typename Pointer, typename MethodSignature, typename Target>
	FFunction(Pointer pointer, MethodSignature method, Target *)
	{
		_connect<Pointer, MethodSignature, Target *, typename flib::detail::FSignature<MethodSignature>::function_type>(pointer, method);
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

	template <typename T>
	inline typename std::enable_if<!flib::is_const_pointer<T>::value, T>::type target() const
	{
		auto imp = dynamic_cast<flib::detail::FAbstractFunctionImpl<R(Args...), T> *>(d.get());

		return imp != nullptr ? imp->target() : nullptr;
	}

	template <typename T>
	inline typename std::enable_if<flib::is_const_pointer<T>::value, T>::type target() const
	{
		auto imp = dynamic_cast<flib::detail::FAbstractFunctionImpl<R(Args...), T> *>(d.get());

		return imp != nullptr ? imp->target() : target<typename flib::is_const_pointer<T>::pointer_type>();
	}

	inline const std::type_info &method_type() const
	{
		return d != nullptr ? d->method_type() : typeid(void);
	}
};

}

#endif /* FFUNCTIONAL_H */

/* vim: set ts=2 sw=2 noet: */
