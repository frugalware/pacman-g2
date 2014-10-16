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

namespace flib
{

	class refcounted
	{
	public:
		flib::FSignal<void(flib::refcounted *)> aboutToDestroy;

	public:
		refcounted();
	protected:
		virtual ~refcounted();

	public:
		void acquire() const;
		void release() const;

	private:
		void operator delete[](void *ptr);
		void *operator new[](std::size_t size);

		refcounted(const flib::refcounted &other);
		flib::refcounted &operator =(const flib::refcounted &other);

		mutable unsigned m_reference_counter;
	};

	static inline void acquire(flib::refcounted *refcounted)
	{
		if(refcounted != nullptr) {
			refcounted->acquire();
		}
	}

	static inline void release(flib::refcounted *refcounted)
	{
		if(refcounted != nullptr) {
			refcounted->release();
		}
	}
}

#endif /* FREFCOUNTED_H */

/* vim: set ts=2 sw=2 noet: */
