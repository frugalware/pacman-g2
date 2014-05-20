/*
 *  object.h
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
#ifndef FOBJECT_H
#define FOBJECT_H

#include "kernel/fsignal.h"

#include <cstddef>

namespace flib
{

class FObject
{
public:
	void operator delete(void *ptr);
	void *operator new(std::size_t size);

	FObject();
protected:
	virtual ~FObject();

public:
	flib::FSignal<void(FObject *)> aboutToDestroy;

	void acquire() const;
	void release() const;

	virtual int get(unsigned val, unsigned long *data) const;
	virtual int set(unsigned val, unsigned long data);

private:
	void operator delete[](void *ptr);
	void *operator new[](std::size_t size);

	FObject(const flib::FObject &other);
	flib::FObject &operator =(const flib::FObject &other);

	mutable unsigned m_reference_counter;
};

static inline void fAcquire(flib::FObject *object)
{
	if(object != nullptr) {
		object->acquire();
	}
}

static inline void fRelease(flib::FObject *object)
{
	if(object != nullptr) {
		object->release();
	}
}

}

#endif /* FOBJECT_H */

/* vim: set ts=2 sw=2 noet: */
