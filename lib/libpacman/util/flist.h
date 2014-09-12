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

/* Sort comparison callback function declaration */
typedef int (*_pacman_fn_cmp)(const void *, const void *);

#ifdef __cplusplus

#include <algorithm>

namespace flib
{
	template <typename Iterable>
	struct iterable_traits
	{
		typedef Iterable iterable;
//		typedef typename iterable::difference_type difference_type;
		typedef typename iterable::pointer pointer;
		typedef typename iterable::reference reference;
		typedef typename iterable::size_type size_type;
		typedef typename iterable::value_type value_type;

		static iterable next(const iterable &i)
		{
			return i.next();
		}

		static iterable previous(const iterable &i)
		{ 
			return i.previous();
		}

		static reference reference_of(iterable i)
		{
			return *i;
		}

		static pointer pointer_of(iterable i)
		{
			return i.operator -> ();
		}

		static value_type value_of(const iterable i)
		{
			return *i;
		}
	};

	template <typename Iterable>
	struct iterable_traits<Iterable *>
	{
		typedef Iterable *iterable;
//		typedef typename Iterable::difference_type difference_type;
		typedef typename Iterable::pointer pointer;
		typedef typename Iterable::reference reference;
		typedef typename Iterable::size_type size_type;
		typedef typename Iterable::value_type value_type;

		static iterable next(const iterable &i)
		{
			return i->next();
		}

		static iterable previous(const iterable &i)
		{ 
			return i->previous();
		}

		static reference reference_of(iterable i)
		{
			return i->operator * ();
		}

		static pointer pointer_of(iterable i)
		{
			return i->operator -> ();
		}

		static value_type value_of(const iterable i)
		{
			return i->operator * ();
		}
	};

	template <typename Iterable, bool Reverse = false>
	struct const_iterator
	{
	public:
//		typedef typename iterable_traits<Iterable>::difference_type difference_type;
		typedef typename iterable_traits<Iterable>::iterable iterable;
		typedef typename iterable_traits<Iterable>::pointer pointer;
		typedef typename iterable_traits<Iterable>::value_type value_type;

		explicit const_iterator(iterable i = iterable())
			: m_iterable(i)
		{ }

		const_iterator(const const_iterator &o)
			: m_iterable(o.m_iterable)
		{ }

		~const_iterator()
		{ }

		operator iterable ()
		{
			return m_iterable;
		}

		const_iterator &operator = (const const_iterator &o)
		{
			m_iterable = o.m_iterable;
			return *this;
		}

		bool operator == (const const_iterator &o) const
		{
			return m_iterable == o.m_iterable;
		}

		bool operator != (const const_iterator &o) const
		{
			return !operator == (o);
		}

		const_iterator &operator ++ ()
		{
			m_iterable = _next();
			return *this;
		}

		const_iterator operator ++ (int)
		{
			const_iterator tmp(*this);
			operator ++ ();
			return tmp;
		}

		const_iterator &operator -- ()
		{
			m_iterable = _previous();
			return *this;
		}

		const_iterator operator -- (int)
		{
			const_iterator tmp(*this);
			operator -- ();
			return tmp;
		}

		value_type operator * () const
		{
			return iterable_traits<Iterable>::value_of(m_iterable);
		}

		const pointer operator -> () const
		{
			return iterable_traits<Iterable>::pointer_of(m_iterable);
		}

		const_iterator next() const
		{
			return const_iterator(_next());
		}

		const_iterator previous() const
		{
			return const_iterator(_previous());
		}

		/* FIXME: temporary */
		template <typename T>
		T *data() const
		{
			return (T *)operator *();
		}

	protected:
		iterable _next() const
		{
			return !Reverse ? iterable_traits<Iterable>::next(m_iterable) : iterable_traits<Iterable>::previous(m_iterable);
		}

		iterable _previous() const
		{
			return !Reverse ? iterable_traits<Iterable>::previous(m_iterable) : iterable_traits<Iterable>::next(m_iterable);
		}

		iterable m_iterable;
	};

	template <typename Iterable, bool Reverse = false>
	struct iterator
	{
	public:
//		typedef typename iterable_traits<Iterable>::difference_type difference_type;
		typedef typename iterable_traits<Iterable>::iterable iterable;
		typedef typename iterable_traits<Iterable>::pointer pointer;
		typedef typename iterable_traits<Iterable>::reference reference;
		typedef typename iterable_traits<Iterable>::value_type value_type;

		explicit iterator(iterable i = iterable())
			: m_iterable(i)
		{ }

		iterator(const iterator &o)
			: m_iterable(o.m_iterable)
		{ }

		~iterator()
		{ }

		operator iterable ()
		{
			return m_iterable;
		}

		iterator &operator = (const iterator &o)
		{
			m_iterable = o.m_iterable;
			return *this;
		}

		bool operator == (const iterator &o) const
		{
			return m_iterable == o.m_iterable;
		}

		bool operator != (const iterator &o) const
		{
			return !operator == (o);
		}

		iterator &operator ++ ()
		{
			m_iterable = _next();
			return *this;
		}

		iterator operator ++ (int)
		{
			iterator tmp(*this);
			operator ++ ();
			return tmp;
		}

		iterator &operator -- ()
		{
			m_iterable = _previous();
			return *this;
		}

		iterator operator -- (int)
		{
			iterator tmp(*this);
			operator -- ();
			return tmp;
		}

		reference operator * ()
		{
			return iterable_traits<Iterable>::reference_of(m_iterable);
		}

		value_type operator * () const
		{
			return iterable_traits<Iterable>::value_of(m_iterable);
		}

		pointer operator -> ()
		{
			return iterable_traits<Iterable>::pointer_of(m_iterable);
		}

		const pointer operator -> () const
		{
			return iterable_traits<Iterable>::pointer_of(m_iterable);
		}

		iterator next() const
		{
			return iterator(_next());
		}

		iterator previous() const
		{
			return iterator(_previous());
		}

		/* FIXME: temporary */
		template <typename T>
		T *data() const
		{
			return (T *)operator *();
		}

	protected:
		iterable _next() const
		{
			return !Reverse ? iterable_traits<Iterable>::next(m_iterable) : iterable_traits<Iterable>::previous(m_iterable);
		}

		iterable _previous() const
		{
			return !Reverse ? iterable_traits<Iterable>::previous(m_iterable) : iterable_traits<Iterable>::next(m_iterable);
		}

	public: /* FIXME: Make protected/private */
		iterable m_iterable;
	};
}

class FCListItem
{
public:
	friend struct flib::iterable_traits<FCListItem *>;

	typedef void *value_type;
	typedef value_type *pointer;
	typedef size_t size_type;
	typedef value_type &reference;

	FCListItem()
		: FCListItem(NULL, NULL)
	{ }

	FCListItem(FCListItem *previous, FCListItem *next)
		: m_next(next), m_previous(previous)
	{ }

	virtual ~FCListItem()
	{ } // FIXME: Make pure virtual

#if 0
	virtual void *c_data() const
	{ return m_data; } // FIXME: Make pure virtual
#endif

	bool insert_after(FCListItem *previous)
	{
		FCListItem *next;

		ASSERT(previous != NULL, RET_ERR(PM_ERR_WRONG_ARGS, false));

		next = previous->m_next;
		previous->m_next = this;
		m_next = next;
		m_previous = previous;
#ifndef F_NOCOMPAT
		if (next != NULL)
#endif
		next->m_previous = this;
		return true;
	}

	FCListItem *next() const
	{
		return m_next;
	}

	FCListItem *previous() const
	{
		return m_previous;
	}

	void swap(FCListItem &o)
	{
		std::swap(m_next, o.m_next);
		std::swap(m_previous, o.m_previous);
		if(m_next != &o) {
			m_next->m_previous = this;
			m_previous->m_next = this;
		} else {
			m_next = m_previous = this;
		}
		if(o.m_next != this) {
			o.m_next->m_previous = &o;
			o.m_previous->m_next = &o;
		} else {
			o.m_next = o.m_previous = &o;
		}
	}

	void remove()
	{
		m_next->m_previous = m_previous;
		m_previous->m_next = m_next;
		m_next = m_previous = NULL;
	}

protected:
	FCListItem *m_next;
	FCListItem *m_previous;
};

namespace flib {
	template <>
	struct iterable_traits<FCListItem *>
	{
		typedef FCListItem *iterable;
//		typedef typename FCListItem::difference_type difference_type;
		typedef typename FCListItem::pointer pointer;
		typedef typename FCListItem::reference reference;
		typedef typename FCListItem::size_type size_type;
		typedef typename FCListItem::value_type value_type;

		static iterable next(const FCListItem * const i)
		{
			ASSERT(i != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
			return i->m_next;
		}

		static iterable previous(const FCListItem * const i)
		{
			ASSERT(i != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
			return i->m_previous;
		}
#if 0
		static reference reference_of(iterable i)
		{
			return i->operator * ();
		}

		static pointer pointer_of(iterable i)
		{
			return i->operator -> ();
		}

		static value_type value_of(const iterable i)
		{
			return i->operator * ();
		}
#endif
	};
}

template <typename T>
class FListItem
	: public FCListItem
{
public:
	friend struct flib::iterable_traits<FListItem *>;

	typedef T value_type;
	typedef value_type *pointer;
	typedef size_t size_type;
	typedef value_type &reference;

	explicit FListItem(const T &data = T())
		: m_data(data)
	{ }

	FListItem(T &&data)
		: m_data(std::move(data))
	{ }

	virtual ~FListItem()
	{ }

#if 0
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
#endif
	reference operator * ()
	{
		return m_data;
	}

	const value_type operator * () const
	{
		return m_data;
	}

	pointer operator -> ()
	{
		return &m_data;
	}

	const pointer operator -> () const
	{
		return &m_data;
	}

	FListItem *next() const
	{
		return static_cast<FListItem *>(m_next);
	}

	FListItem *previous() const
	{
		return static_cast<FListItem *>(m_previous);
	}

	void swap_data(T &o)
	{
		std::swap(m_data, o);
	}

	void swap_data(T *o)
	{
		std::swap(m_data, *o);
	}

//protected:
	T m_data;

private:
	FListItem(const FListItem<T> &o);

	FListItem<T> &operator = (const FListItem<T> &o);
};

namespace flib {
	template <typename T>
	struct iterable_traits<FListItem<T> *>
	{
		typedef FListItem<T> *iterable;
//		typedef typename FListItem<T>::difference_type difference_type;
		typedef typename FListItem<T>::pointer pointer;
		typedef typename FListItem<T>::reference reference;
		typedef typename FListItem<T>::size_type size_type;
		typedef typename FListItem<T>::value_type value_type;

		static iterable next(const iterable &i)
		{
			return static_cast<iterable>(iterable_traits<FCListItem *>::next(i));
		}

		static iterable previous(const iterable &i)
		{
			return static_cast<iterable>(iterable_traits<FCListItem *>::previous(i));
		}

		static reference reference_of(iterable i)
		{
			return i->operator * ();
		}

		static pointer pointer_of(iterable i)
		{
			return i->operator -> ();
		}

		static value_type value_of(const iterable i)
		{
			return i->operator * ();
		}
	};
}

template <typename T>
class FList
	: protected FCListItem
{
public:
	typedef FListItem<T> *iterable;
	/* std::list compatibility */
	typedef T value_type;
	typedef flib::iterator<iterable> iterator;
	typedef flib::iterator<iterable, true> reverse_iterator;
	typedef flib::const_iterator<iterable> const_iterator;
	typedef flib::const_iterator<iterable, true> const_reverse_iterator;
	typedef size_t size_type;

	FList()
		: FCListItem(this, this)
	{ }

	FList(FList &&o)
		: FList()
	{
		swap(o);
	}

	FList &operator = (FList &&o)
	{
		swap(o);
		return *this;
	}

	virtual ~FList() override
	{
		clear();
	}

	/* Iterators */
	iterator begin()
	{
		return iterator(c_first());
	}

	const_iterator begin() const
	{
		return cbegin();
	}

	const_iterator cbegin() const
	{
		return const_iterator(c_first());
	}

	reverse_iterator rbegin()
	{
		return reverse_iterator(c_last());
	}

	const_reverse_iterator rbegin() const
	{
		return crbegin();
	}

	const_reverse_iterator crbegin() const
	{
		return const_reverse_iterator(c_last());
	}

	iterator end()
	{
		return iterator(c_end());
	}

	const_iterator end() const
	{
		return cend();
	}

	const_iterator cend() const
	{
		return const_iterator(c_end());
	}

	reverse_iterator rend()
	{
		return reverse_iterator(c_end());
	}

	const_reverse_iterator rend() const
	{
		return crend();
	}

	const_reverse_iterator crend() const
	{
		return const_reverse_iterator(c_end());
	}

	/* Capacity */
	bool empty() const
	{
		return begin() == end();
	}

	size_type size() const
	{
		size_type size = 0;
		for(auto it = cbegin(), end = cend(); it != end; ++it, ++size) {
			/* Nothing to do */
		}
		return size;
	}

	/* Element access */
	
	/* Modifiers */
	void clear()
	{
		// FIXME: lets leak for now
		m_next = m_previous = this;
	}

public:
	/* extensions */
	iterator first()
	{
		return iterator(c_first());
	}

	const_iterator first() const
	{
		return const_iterator(c_first());
	}

	iterator last()
	{
		return iterator(c_last());
	}

	const_iterator last() const
	{
		return const_iterator(c_last());
	}

	FList &add(const value_type &val) // Make default implementation to happend
	{
		(new FListItem<T>(val))->insert_after(last());
		return *this;
	}

	template <typename Comparator>
	FList &add_sorted(const value_type &data, Comparator cmp)
	{
		iterable add = new FListItem<T>(data);

		/* Find insertion point. */
		iterable previous, end;
		for(previous = end = c_end(); previous->next() != end; previous = previous->next()) {
			if(cmp(data, previous->next()->m_data) <= 0) {
				break;
			}
		}

		/*  Insert node before insertion point. */
		add->insert_after(previous);
		return *this;
	}

	bool remove(void *ptr, _pacman_fn_cmp fn, value_type *data)
	{
		return remove(fn, ptr, data);
	}

	bool remove(_pacman_fn_cmp fn, void *ptr, value_type *data = nullptr)
	{
		for(auto i = c_first(), end = c_end(); i != end; i = i->next()) {
			if(fn(ptr, i->m_data) == 0) {
				/* we found a matching item */
				i->remove();
				if(data != nullptr) {
					*data = i->m_data;
				}
				delete i;
				return true;
			}
		}
		return false;
	}

	void swap(FList &o) {
		FCListItem::swap(o);
	}

public:
	iterable c_first() const
	{
		ASSERT(this != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
		return static_cast<iterable>(m_next);
	}

	iterable c_last() const
	{
		ASSERT(this != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
		return static_cast<iterable>(m_previous);
	}

	iterable c_end() const
	{
		ASSERT(this != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
		return static_cast<iterable>((FCListItem *)this);
	}

private:
	FList(const FList &);
	FList &operator = (const FList &);

#if 0
	virtual void *c_data() const override
	{
		RET_ERR(PM_ERR_WRONG_ARGS, NULL);
	}
#endif
};
#endif /* __cplusplus */

#endif /* F_LIST_H */

/* vim: set ts=2 sw=2 noet: */
