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
	template <class T, class Compare = std::less<T>>
	class set
		: public flib::list<T>
	{
	public:
		typedef flib::list<T> super_type;

		using typename super_type::iterable;
		using typename super_type::iterator;
		using typename super_type::value_type;

		using super_type::list;

		typedef Compare key_compare;
		typedef Compare value_compare;

		virtual iterator add(const value_type &data) override
		{
			iterator end = this->end();
			/* Find insertion point. */
			iterator next = find_insertion_point(data);

			// ensure we don't have an egality
			if(next == end || m_compare(data, *next)) {
				iterable add = new FListItem<T>(data);
				add->insert_after(next.previous());
				return iterator(add);
			}
			return end;
		}

		iterator find(const value_type &data)
		{
			iterator end = this->end();
			iterator it = find_insertion_point(data);

			// ensure we have an egality
			if(it == end || !m_compare(data, *it)) {
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
		iterator find_insertion_point(const value_type &data)
		{
			return super_type::find_if_not([&] (const T &o) -> bool { return m_compare(o, data); });
		}

		Compare m_compare;
	};
} // namespace flib

#endif /* F_SET_H */

/* vim: set ts=2 sw=2 noet: */
