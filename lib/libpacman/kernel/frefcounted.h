/*
 *  frefcounted.h
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
#ifndef FREFCOUNTED_H
#define FREFCOUNTED_H

#include "kernel/fsignal.h"

#include <assert.h>

namespace flib
{
	class refcounted
	{
	public:
		flib::FSignal<void(flib::refcounted *)> about_to_destroy;

	public:
		refcounted();
	protected:
		virtual ~refcounted();

	public:
		void acquire() const;
		static inline void acquire(flib::refcounted *refcounted)
		{
			if(refcounted != nullptr) {
				refcounted->acquire();
			}
		}

		void release() const;
		static inline void release(flib::refcounted *refcounted)
		{
			if(refcounted != nullptr) {
				refcounted->release();
			}
		}

	private:
		void operator delete[](void *ptr);
		void *operator new[](std::size_t size);

		refcounted(const flib::refcounted &other);
		flib::refcounted &operator =(const flib::refcounted &other);

		mutable unsigned m_reference_counter;
	};

	template <class T>
	class refcounted_ptr
	{
	public:
		typedef T element_type;

	protected:
		refcounted_ptr()
			: m_refcounted_ptr(nullptr)
		{ }

		/* Manipulators */
		template <class Y>
		void reset(Y *refcounted_ptr)
		{
			m_refcounted_ptr = refcounted_ptr;
		}

	public:
		/* Accessors */
		T *get() const
		{
			return m_refcounted_ptr;
		}

		T &operator * () const
		{
			return *operator -> ();
		}

		T *operator -> () const
		{
			assert(m_refcounted_ptr != nullptr);
			return get();
		}

		explicit operator T * () const
		{
			return get();
		}

		operator bool () const
		{
			return m_refcounted_ptr != nullptr;
		}

	protected:
		void swap(refcounted_ptr<T> &o)
		{
			std::swap(m_refcounted_ptr, o.m_refcounted_ptr);
		}

	private:
		refcounted_ptr(const refcounted_ptr &o);
		refcounted_ptr &operator = (const refcounted_ptr &o);

		T *m_refcounted_ptr;
	};

	template <class T, class U>
	bool operator == (const refcounted_ptr<T> &lhs, const refcounted_ptr<U> &rhs)
	{
		return lhs.get() == rhs.get();
	}

	template <class T>
	bool operator == (const refcounted_ptr<T> &lhs, std::nullptr_t rhs)
	{
		return lhs.get() == rhs;
	}

	template <class T, class U>
	bool operator != (const refcounted_ptr<T> &lhs, const refcounted_ptr<U> &rhs)
	{ 
		return lhs.get() != rhs.get();
	}

	template <class T>
	bool operator != (const refcounted_ptr<T> &lhs, std::nullptr_t rhs)
	{
		return lhs.get() != rhs;
	}
} // namespace flib

namespace std
{
	template <class T>
	bool operator == (nullptr_t lhs, const flib::refcounted_ptr<T> &rhs)
	{
		return lhs == rhs.get();
	}

	template <class T>
	bool operator != (nullptr_t lhs, const flib::refcounted_ptr<T> &rhs)
	{
		return lhs != rhs.get();
	}
} // namespace std

namespace flib {
	template <class T>
	class refcounted_shared_ptr
		: public refcounted_ptr<T>
	{
	public:
		typedef refcounted_ptr<T> super_type;

		constexpr refcounted_shared_ptr()
			: refcounted_shared_ptr(nullptr)
		{ }

		constexpr refcounted_shared_ptr(std::nullptr_t)
		{ }

		template <class Y>
		explicit refcounted_shared_ptr(Y *refcounted_ptr)
			: refcounted_shared_ptr()
		{
			reset(refcounted_ptr);
		}

		template <class Y>
		refcounted_shared_ptr(const refcounted_ptr<Y> &o)
			: refcounted_shared_ptr(o.get())
		{ }

		refcounted_shared_ptr(const refcounted_ptr<T> &o)
			: refcounted_shared_ptr(o.get())
		{ }

		template <class Y>
		refcounted_shared_ptr(const refcounted_shared_ptr<Y> &o)
			: refcounted_shared_ptr(static_cast<const refcounted_ptr<Y> &>(o))
		{ }

		refcounted_shared_ptr(const refcounted_shared_ptr &o)
			: refcounted_shared_ptr(static_cast<const refcounted_ptr<T> &>(o))
		{ }

		refcounted_shared_ptr(refcounted_shared_ptr &&o)
			: refcounted_shared_ptr()
		{
			swap(o);
		}

		~refcounted_shared_ptr()
		{
			reset();
		}

		template <class Y>
		refcounted_shared_ptr &operator = (const refcounted_ptr<Y> &o)
		{
			reset(o.get());
			return *this;
		}

		refcounted_shared_ptr &operator = (const refcounted_ptr<T> &o)
		{
			reset(o.get());
			return *this;
		}

		template <class Y>
		refcounted_shared_ptr &operator = (const refcounted_shared_ptr<Y> &o)
		{
			return operator = (static_cast<const refcounted_ptr<Y> &>(o));
		}

		refcounted_shared_ptr &operator = (const refcounted_shared_ptr<T> &o)
		{
			return operator = (static_cast<const refcounted_ptr<T> &>(o));
		}

		refcounted_shared_ptr &operator = (refcounted_shared_ptr &&o)
		{
			swap(o);
			return *this;
		}

		/* Manipulators */
		void reset()
		{
			reset<T>(nullptr);
		}

		template <class Y>
		void reset(Y *ptr)
		{
			if(super_type::get() != nullptr) {
				super_type::get()->release();
			}
			if(ptr != nullptr) {
				ptr->acquire();
			}
			super_type::reset(ptr);
		}

		void swap(refcounted_shared_ptr<T> &o)
		{
			super_type::swap(o);
		}
	};
} // namespace flib

#endif /* FREFCOUNTED_H */

/* vim: set ts=2 sw=2 noet: */
