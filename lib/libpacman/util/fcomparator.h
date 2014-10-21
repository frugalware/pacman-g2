/*
 *  fcomparator.h
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
#ifndef F_COMPARATOR_H
#define F_COMPARATOR_H

#include "fmatcher.h"

template <typename T>
class FComparator
{
public:
	FComparator()
	{ }

	virtual ~FComparator() override
	{ }

	virtual int compare(const T &o) const = 0;

	bool match(const T &o) const override
	{
		return compare(o) == 0;
	}

protected:
	FComparator(const FComparator<T> &o)
	{ }

	FComparator<T> &operator = (const FComparator<T> &o)
	{
		return *this;
	}
};

// FIXME: Add FComparatorList with compare_all

#endif /* F_COMPARATOR_H */

/* vim: set ts=2 sw=2 noet: */
