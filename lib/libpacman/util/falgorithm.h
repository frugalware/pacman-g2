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
	namespace detail {
		struct null_visitor
		{
			template <class... Args>
			void operator () (Args... args)
			{ }
		};

		struct false_function
		{
			template <class... Args>
			bool operator () (Args... args)
			{ return false; }
		};

		struct true_function
		{
			template <class... Args>
			bool operator () (Args... args)
			{ return true; }
		};
	}

	template <class InputIterator, class T>
	InputIterator contains(InputIterator first, InputIterator last, const T &val)
	{
		return find(first, last, val) != last;
	}

	template <class InputIterator>
	typename InputIterator::size_type count(InputIterator first, InputIterator last)
	{
		typename InputIterator::size_type size = 0;

		for(; first != last; ++first, ++size) {
			/* Nothing to do but iterate */
		}
		return size;
	}

	template <class InputIterator, class T>
	InputIterator find(InputIterator first, InputIterator last, const T &val)
	{
		return find_if(first, last,
				[&] (const typename InputIterator::value_type &o) -> bool
				{ return o == val; });
	}

	template <class InputIterator, class UnaryPredicate>
	InputIterator find_if(InputIterator first, InputIterator last, UnaryPredicate pred)
	{
		for(; first != last && !pred(*first); ++first) {
			/* Nothing to do but iterate */
		}
		return first;
	}

	template <class InputIterator, class UnaryPredicate>
	InputIterator find_if_not(InputIterator first, InputIterator last, UnaryPredicate pred)
	{
		for(; first != last && pred(*first); ++first) {
			/* Nothing to do but iterate */
		}
		return first;
	}

	template <class T, class U>
	bool match(const T &val1, const U &val2)
	{
		return val1 == val2;
	}

	template <class InputIterator, class T>
	bool all_match(InputIterator first, InputIterator last, const T &val)
	{
		return all_match_if(
				[&] (const typename InputIterator::reference o) -> bool
				{ return match(o, val); });
	}

	template <class InputIterator, class UnaryPredicate>
	bool all_match_if(InputIterator first, InputIterator last, UnaryPredicate pred)
	{
		for(; first != last; ++first) {
			if(!pred(*first)) {
				return false;
			}
		}
		return true;
	}

	template <class InputIterator, class T>
	bool any_match(InputIterator first, InputIterator last, const T &val)
	{
		return any_match_if(
				[&] (const typename InputIterator::reference o) -> bool
				{ return match(o, val); });
	}

	template <class InputIterator, class UnaryPredicate>
	bool any_match_if(InputIterator first, InputIterator last, UnaryPredicate pred)
	{
		for(; first != last; ++first) {
			if(pred(*first)) {
				return true;
			}
		}
		return false;
	}

	template <class InputOutputIterator, class T, class Function>
	typename InputOutputIterator::size_type
			replace(InputOutputIterator first, InputOutputIterator last, const T &val, const T &new_val, Function fn = detail::null_visitor())
	{
		return replace_if(first, last,
				[&] (const typename InputOutputIterator::reference o) -> typename InputOutputIterator::size_type
				{ return match(o, val); }, fn);
	}

	template <class InputOutputIterator, class UnaryPredicate, class T, class Function>
	typename InputOutputIterator::size_type
			replace_if(InputOutputIterator first, InputOutputIterator last, UnaryPredicate pred, const T &new_val, Function fn = detail::null_visitor())
	{
		typename InputOutputIterator::size_type size = 0;

		while((first = std::find_if(first, last, pred)) != last) {
			fn(*first);
			*first = new_val;
			++first;
			++size;
		}
		return size;
	}
}

#endif /* F_ALGORITM_H */

/* vim: set ts=2 sw=2 noet: */
