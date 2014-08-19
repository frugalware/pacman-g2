/*
 *  flist.h
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
#ifndef F_LIST_H
#define F_LIST_H

#include "pacman.h"

#include "util/fcallback.h"
#include "util/fptrlist.h"

#ifdef __cplusplus
template <typename T>
class FListItem
	: public FCListItem
{
	FListItem()
	{ }

	explicit FListItem(T &data)
		: m_data(data)
	{ }

	virtual ~FListItem()
	{ }

	virtual void *c_data() const override
	{
		return &m_data;
	}

	T &data()
	{
		return m_data;
	}

	const T &data() const
	{
		return m_data;
	}

	FListItem<T> *next()
	{
		return FCListItem::next();
	}

	const FListItem<T> *next() const
	{
		return FCListItem::next();
	}

	FListItem<T> *previous()
	{
		return FCListItem::previous();
	}

	const FListItem<T> *previous() const
	{
		return FCListItem::previous();
	}

protected:
	T m_data;

private:
	FListItem(const FListItem<T> &o);

	FListItem<T> &operator = (const FListItem<T> &o);
};

template <typename T>
class FList
	: private FCList
{
public:
	/* std::list compatibility */
	typedef FListItem<T> *iterator;
	typedef const iterator const_iterator;

	FList()
	{ }

	~FList()
	{
		clear();
	}

	void clear();
	bool empty() const;

	iterator begin()
	{
		return first();
	}

	const_iterator begin() const
	{
		return first();
	}

	const_iterator cbegin() const
	{
		return first();
	}

	iterator end()
	{
		return last();
	}

	const_iterator end() const
	{
		return last();
	}

	const_iterator cend() const
	{
		return last();
	}

#if 0
	typedef ListItem *reverse_iterator;
	typedef const List::iterator const_reverse_iterator;

	List::reverse_iterator rbegin();
	List::const_reverse_iterator rbegin() const;
	List::const_reverse_iterator crbegin() const;
	List::reverse_iterator rend();
	List::const_reverse_iterator rend() const;
	List::const_reverse_iterator crend() const;
#endif

public:
	/* extensions */
#if 0
	bool all_match(const FMatcher &matcher) const
	{
		const_iterator end = cend();

		for(const_iterator it = beging(); it != end; it = it->next()) {
			if(matcher.match(it.data()) == false) {
				return false;
			}
		}
		return true;
	}

	bool any_match(const FList *list, const FMatcher &matcher) const
	{
		const_iterator end = cend();

		for(const_iterator it = begin(); it != end; it = it->next()) {
			if(matcher.match(it->data()) == true) {
				return true;
			}
		}
		return false;
	}

	ListItem<T> *find(const FComparator &comparator)
	{
		iterator end = this->end();

		for(iterator it = begin(); it != end; it = it->next()) {
			if(comparator->compare(it->data()) == 0) {
				return it;
			}
		}
		return NULL;
	}
#endif

	iterator first()
	{
		return FCList::first();
	}

	const_iterator first() const
	{
		return FCList::first();
	}

	iterator last()
	{
		return FCList::last();
	}

	const_iterator last() const
	{
		return FCList::last();
	}

private:
	FList(const FList &);

	FList &operator = (const FList &);

	virtual void *c_data() const override
	{
		RET_ERR(PM_ERR_WRONG_ARGS, NULL);
	}
};
#endif /* __cplusplus */

#endif /* F_LIST_H */

/* vim: set ts=2 sw=2 noet: */
