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
		: public FList<T>
	{
	public:
		typedef FList<T> super_type;

		using FList<T>::FList;

		typename super_type::iterator add(const typename super_type::value_type &data)
		{
			typename super_type::iterator end = this->end();
			/* Find insertion point. */
			typename super_type::iterator next = FList<T>::find_if_not(
					[&] (const T &o) -> bool { return m_less(o, data); });

			// ensure we don't have an egality
			if(next == end || m_less(data, *next)) {
				typename super_type::iterable add = new FListItem<T>(data);
				add->insert_after(next.previous());
				return typename super_type::iterator(add);
			}
			return end;
		}

	private:
		Compare m_less;
	};
} // namespace flib

#endif /* F_SET_H */

/* vim: set ts=2 sw=2 noet: */
