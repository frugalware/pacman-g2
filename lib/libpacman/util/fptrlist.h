/*
 *  fptrlist.h
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2013-2014 by Michel Hermier <hermier@frugalware.org>
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
#ifndef F_PTRLIST_H
#define F_PTRLIST_H

#include "pacman.h"

#include "stdbool.h"

#include "util/fcallback.h"
#include "util/flist.h"

#ifdef __cplusplus
class FPtrList
	: protected FCListItem
{
public:
	friend FPtrList *f_ptrlist_add_sorted(FPtrList *list, void *data, _pacman_fn_cmp fn);
	friend bool _pacman_list_remove(FPtrList *haystack, void *needle, _pacman_fn_cmp fn, void **data);
	friend FPtrList *f_ptrlist_add(FPtrList *list, void *data);

	typedef FCListItem *iterator;
	typedef const iterator const_iterator;

	FPtrList()
	{ }

	FPtrList(FPtrList &&o)
		: FPtrList()
	{
		swap(o);
	}

	FPtrList &operator = (FPtrList &&o)
	{
		swap(o);
		return *this;
	}

	iterator begin()
	{
		ASSERT(this != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
		return m_data != NULL ? this : end();
	}

	const_iterator begin() const
	{
		return cbegin();
	}

	const_iterator cbegin() const
	{
		ASSERT(this != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
		return m_data != NULL ? const_cast<const_iterator>((const FCListItem * const)this) : NULL;
	}

	iterator end()
	{
		return NULL;
	}

	const_iterator end() const
	{
		return NULL;
	}

	const_iterator cend() const
	{
		return NULL;
	}

	iterator last()
	{
		return const_cast<iterator>(clast());
	}

	const_iterator last() const
	{
		auto it = cbegin(), end = cend();

		if (it != end)
		{
			while (it->next() != end) {
				it = it->next();
			}
		}
		return it;
	}

	const_iterator clast() const
	{
		return last();
	}

	iterator rbegin()
	{
		return last();
	}

	const_iterator rbegin() const
	{
		return last();
	}

	const_iterator crbegin() const
	{
		return last();
	}

	iterator rend()
	{
		return NULL;
	}

	const_iterator rend() const
	{
		return NULL;
	}

	const_iterator crend() const
	{
		return NULL;
	}

	bool empty() const
	{
		ASSERT(this != NULL, RET_ERR(PM_ERR_WRONG_ARGS, true));
		return this->m_data == NULL;
	}

	FPtrList &add(const void *data);

	void clear()
	{
		// FIXME: lets leak for now
		ASSERT(this != NULL, pm_errno = PM_ERR_WRONG_ARGS; return);
		m_next = m_previous = m_data = NULL;
	}

	void swap(FPtrList &o) {
		std::swap(m_data, o.m_data);
		std::swap(m_next, o.m_next);
		std::swap(m_previous, o.m_previous);
		if (m_next != NULL) {
			m_next->m_previous = this;
		}
		if (m_previous != NULL) {
			m_previous->m_next = this;
		}
		if (o.m_next != NULL) {
			o.m_next->m_previous = &o;
		} 
		if (o.m_previous != NULL) {
			o.m_previous->m_next = &o;
		}
	}
private:
	FPtrList(const FPtrList &o);
	FPtrList &operator = (const FPtrList &o);
};
#endif
#endif /* F_PTRLIST_H */

/* vim: set ts=2 sw=2 noet: */
