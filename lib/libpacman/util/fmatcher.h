/*
 *  fmatcher.h
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
#ifndef F_MATCHER_H
#define F_MATCHER_H

#include "fstdlib.h"
#include "util.h"

template <typename T>
class FMatcher
{
public:
	FMatcher()
	{ }

	virtual ~FMatcher()
	{ }

	virtual bool match(T data) const = 0;

protected:
	FMatcher(const FMatcher<T> &o)
	{ }

	FMatcher<T> &operator = (const FMatcher<T> &o)
	{
		return *this;
	}
};

namespace flib
{
	template <class T, class U>
	bool match(const T &val, const FMatcher<U> &matcher)
	{
		return flib::match(matcher, val);
	}

	template <class T, class U>
	bool match(const FMatcher<T> &matcher, const U &val)
	{
		return matcher.match(val);
	}
}

#endif /* F_MATCHER_H */

/* vim: set ts=2 sw=2 noet: */
