/*
 *  fset.h
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
#ifndef F_SET_H
#define F_SET_H

#include "util/flist.h"
#include "util.h"

#include <functional>

namespace flib
{
	template <class T>
	struct keyed_value_traits
	{
		typedef T keyed_value_type;
		typedef T key_type;
		typedef T value_type;

		static const key_type &key_of(const keyed_value_type &o)
		{
			return o;
		}

		static value_type &value_of(keyed_value_type &o)
		{
			return o;
		}

		static const value_type &value_of(const keyed_value_type &o)
		{
			return o;
		}
	};

	template <class T, class Compare = std::less<typename flib::keyed_value_traits<T>::key_type>>
	class set
		: private flib::list<T>
	{
	private:
		typedef flib::list<T> super_type;
		typedef flib::keyed_value_traits<T> keyed_value_traits;

	public:
		using typename super_type::iterator;
		using typename super_type::const_iterator;

		using super_type::list;

		/* Iterators */
		using super_type::begin;
		using super_type::end;
		using super_type::rbegin;
		using super_type::rend;

		/* Capacity */
		using super_type::empty;
		using super_type::size;

		/* Element access */
		using super_type::clear;
		using super_type::contains;
		using super_type::remove;

		using super_type::any_match_if;

	public:
		typedef typename keyed_value_traits::keyed_value_type keyed_value_type;
		typedef typename keyed_value_traits::key_type key_type;
		typedef typename keyed_value_traits::value_type value_type;

		typedef Compare key_compare;
		typedef Compare value_compare;

		explicit operator const flib::list<T> &() const
		{
			return *this;
		}

		virtual iterator add(const keyed_value_type &keyed_value) override
		{
			iterator end = this->end();
			/* Find insertion point. */
			iterator next = find_insertion_point(keyed_value_traits::key_of(keyed_value));

			// ensure we don't have an egality
			if(next == end
					|| m_compare(keyed_value_traits::key_of(keyed_value), keyed_value_traits::key_of(*next))) {
				typename super_type::data_holder add = new FListItem<T>(keyed_value);
				add->insert_after(next.previous());
				return iterator(add);
			}
			return end;
		}

		iterator find(const key_type &key)
		{
			iterator end = this->end();
			iterator it = find_insertion_point(key);

			// ensure we have an egality
			if(it == end || !m_compare(key, *it)) {
				return end;
			}
			return it;
		}

		const_iterator find(const key_type &key) const
		{
			const_iterator end = this->end();
			const_iterator it = find_insertion_point(key);

			// ensure we have an egality
			if(it == end || !m_compare(key, *it)) {
				return end;
			}
			return it;
		}

		/* Observers */
		key_compare key_comp() const
		{
			return m_compare;
		}

		value_compare value_comp() const
		{
			return m_compare;
		}

	private:
		/* Return the first iterator where value does not satisfy Compare */
		iterator find_insertion_point(const key_type &key)
		{
			return super_type::find_if_not(
					[&] (const keyed_value_type &keyed_value) -> bool
					{ return m_compare(keyed_value_traits::key_of(keyed_value), key); });
		}

		Compare m_compare;
	};
} // namespace flib

#endif /* F_SET_H */

/* vim: set ts=2 sw=2 noet: */
