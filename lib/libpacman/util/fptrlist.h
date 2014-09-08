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
class FPtrListItem
	: public FListItem<void *>
{
public:
	FPtrListItem(void *data = nullptr)
		: FListItem<void *>(data)
	{ }

	FPtrListItem *next() const
	{
		return (FPtrListItem *)flib::iterable_traits<FCListItem *>::next(this);
	}

	FPtrListItem *previous() const
	{
		return (FPtrListItem *)flib::iterable_traits<FCListItem *>::previous(this);
	}
};

class FPtrList
	: protected FCListItem
{
public:
	friend FPtrList *f_ptrlist_add_sorted(FPtrList *list, void *data, _pacman_fn_cmp fn);

	typedef FPtrListItem *iterator;
	typedef const iterator const_iterator;

	FPtrList()
		: FCListItem(this, this)
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
		return iterator(next());
	}

	const_iterator begin() const
	{
		return cbegin();
	}

	const_iterator cbegin() const
	{
		ASSERT(this != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
		return const_iterator(next());
	}

	iterator end()
	{
		return iterator(this);
	}

	const_iterator end() const
	{
		return cend();
	}

	const_iterator cend() const
	{
		return const_iterator(this);
	}

	iterator last()
	{
		return iterator(previous());
	}

	const_iterator last() const
	{
		return clast();
	}

	const_iterator clast() const
	{
		return const_iterator(previous());
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
		return iterator(this);
	}

	const_iterator rend() const
	{
		return crend();
	}

	const_iterator crend() const
	{
		return const_iterator(this);
	}

	bool empty() const
	{
		ASSERT(this != NULL, RET_ERR(PM_ERR_WRONG_ARGS, true));
		return begin() == end();
	}

	FPtrList &add(void *data);

	void clear()
	{
		// FIXME: lets leak for now
		ASSERT(this != NULL, pm_errno = PM_ERR_WRONG_ARGS; return);
		m_next = m_previous = this;
	}

	void swap(FPtrList &o) {
		FCListItem::swap(o);
	}

private:
	FPtrList(const FPtrList &o);
	FPtrList &operator = (const FPtrList &o);
};
#endif
#endif /* F_PTRLIST_H */

/* vim: set ts=2 sw=2 noet: */
