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

/* Chained list struct */
struct __pmlist_t {
	struct __pmlist_t *m_previous;
	struct __pmlist_t *next;
	void *m_data;
};

#ifdef __cplusplus

#ifdef F_NOCOMPAT
class FListItemBase
	: public __pmlist_t
{
public:
	ListItem()
		: ListItem(NULL, NULL)
	{ }

	ListItem(FListItemBase *previous, FListItemBase *next)
		: m_next(next), m_previous(previous)
	{ }

	virtual ~FListItemBase() = 0;

	ListItemBase *next()
	{
		return m_next;
	}

	ListItemBase *next() const
	{
		return m_next;
	}

	ListItemBase *previous()
	{
		return m_previous;
	}

	const ListItemBase *previous() const
	{
		return m_previous;
	}

protected:
	virtual void *c_data() const = 0;

	bool insert_after(FListItemBase *previous);	

	ListItemBase *m_next;
	ListItemBase *m_previous;
};

template <typename T>
class FListItem
	: public FListItemBase
{
	ListItem()
	{ }

	explicit ListItem(T &data)
		: m_data(data)
	{ }

	virtual ~ListItem()
	{ }

	virtual void *c_data() const override
	{
		return &m_data;
	}

	T *c_data() const
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

	ListItem<T> *next()
	{
		return FListItemBase::next();
	}

	ListItem<T> *next() const
	{
		return FListItemBase::next();
	}

	ListItem<T> *previous()
	{
		return FListItemBase::previous();
	}

	const ListItem<T> *previous() const
	{
		return FListItemBase::previous();
	}

protected:
	T m_data;

private:
	ListItem(const ListItem<T> &o);

	ListItem<T> &operator = (const ListItem<T> &o);
};

class FListBase
	: private FListItemBase
{
public:
	/* std::list compatibility */
	typedef ListItem<T> *iterator;
	typedef const List<T>::iterator const_iterator;

	FListBase()
		: FListBase(this, this)
	{ }

	~FListBase()
	{
		clear();
	}
	
	size_t count() const;

	FList::iterator begin()
	{
		return first();
	}

	FList::const_iterator begin() const
	{
		return first();
	}

	FList::const_iterator cbegin() const
	{
		return first();
	}

	List::iterator end()
	{
		return last();
	}

	List::const_iterator end() const
	{
		return last();
	}

	List::const_iterator cend() const
	{
		return last();
	}

	ListItemBase *first()
	{
		return FListItemBase::next();
	}

	const ListItemBase *first() const
	{
		return FListItemBase::next();
	}

	ListItemBase *last()
	{
		return FListItemBase::previous();
	}

	const ListItemBase *last() const
	{
		return FListItemBase::previous();
	}
	
protected:
	virtual bool add(FListItem *item);

protected:
	ListBase(const ListItem<T> &o);

	ListBase &operator = (const ListBase &o);

};

template <typename T>
class List
	: private FListBase
{
public:
	/* std::list compatibility */
	typedef ListItem<T> *iterator;
	typedef const List<T>::iterator const_iterator;

	List()
	{ }

	~List()
	{
		clear();
	}

	void clear();
	bool empty() const;

	FList::iterator begin()
	{
		return first();
	}

	FList::const_iterator begin() const
	{
		return first();
	}

	FList::const_iterator cbegin() const
	{
		return first();
	}

	List::iterator end()
	{
		return last();
	}

	List::const_iterator end() const
	{
		return last();
	}

	List::const_iterator cend() const
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

	ListItem<T> *first()
	{
		return FListBase::first();
	}

	const ListItem<T> *first() const
	{
		return FListBase::first();
	}

	ListItem<T> *last()
	{
		return FListBase::last();
	}

	const ListItem<T> *last() const
	{
		return FListBase::last();
	}

private:
	List(const List &);

	List &operator = (const List &);

	virtual void *c_data() const override
	{
		RET_ERR(PM_ERR_WRONG_ARGS, NULL);
	}
};
#endif /* F_NOCOMPAT */
#endif /* __cplusplus */

#endif /* F_LIST_H */

/* vim: set ts=2 sw=2 noet: */
