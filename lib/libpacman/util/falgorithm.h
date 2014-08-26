/*
 *  falgorithm.h
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
#ifndef F_ALGORITM_H
#define F_ALGORITM_H

#include <algorithm>

namespace flib
{
	template <class InputIterable, class T>
	bool contains(const InputIterable &iterable, const T &val)
	{
		return contains(iterable.begin(), iterable.end(), val);
	}

	template <class InputIterator, class T>
	InputIterator contains(InputIterator first, InputIterator last, const T &val)
	{
		return find(first, last, val) != last;
	}

	template <class InputIterable, class T>
	auto find(InputIterable iterable, const T &val)
	{
		return find(iterable.begin(), iterable.end(), val);
	}

	template <class InputIterator, class T>
	InputIterator find(InputIterator first, InputIterator last, const T &val)
	{
		for(; first != last && !(*first == val); ++first) {
			/* Nothing to do but iterate */
		}
		return first;
	}

	template <class T, class U>
	bool match(const T &val1, const U &val2)
	{
		return val1 == val2;
	}

	template <class InputIterable, class T>
	bool match_all(const InputIterable &iterable, const T &val)
	{
		return match_all(iterable.begin(), iterable.end(), val);
	}

	template <class InputIterator, class T>
	bool match_all(InputIterator first, InputIterator last, const T &val)
	{
		for(; first != last; ++first) {
			if(!match(*first, val)) {
				return false;
			}
		}
		return true;
	}

	template <class InputIterable, class T>
	bool match_any(const InputIterable &iterable, const T &val)
	{
		return match_any(iterable.begin(), iterable.end(), val);
	}

	template <class InputIterator, class T>
	bool match_any(InputIterator first, InputIterator last, const T &val)
	{
		for(; first != last; ++first) {
			if(match(*first, val)) {
				return true;
			}
		}
		return false;
	}
}

#endif /* F_ALGORITM_H */

/* vim: set ts=2 sw=2 noet: */
