/*
 *  object.c
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

#include "config.h"

#include "kernel/fobject.h"

#include "util.h"

#include <fstdlib.h>

using namespace flib;

void FObject::operator delete(void *ptr)
{
	free(ptr);
}

void *FObject::operator new(std::size_t size)
{
	return f_zalloc(size);
}

FObject::FObject()
	: m_reference_counter(1)
{ }

FObject::~FObject()
{
}

void FObject::acquire() const
{
	++m_reference_counter;
}

void FObject::release() const
{
	if(--m_reference_counter == 0) {
		aboutToDestroy(const_cast<FObject *>(this));
		delete this;
	}
}

int FObject::get(unsigned val, unsigned long *data) const
{
	return -1;
}

int FObject::set(unsigned val, unsigned long data)
{
	return -1;
}

/* vim: set ts=2 sw=2 noet: */
