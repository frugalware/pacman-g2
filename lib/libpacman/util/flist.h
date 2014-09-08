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

#ifndef __cplusplus
typedef struct FPtrList FPtrList;
typedef struct FPtrListItem FPtrListIterator;
#else /* __cplusplus */
typedef class FPtrList FPtrList;
typedef class FPtrListItem FPtrListIterator;
#endif /* __cplusplus */

#ifdef __cplusplus
#include <algorithm>

extern "C" {
#endif /* __cplusplus */

#define _FREELIST(p, f) do { if(p) { FVisitor visitor = { (FVisitorFunc)f, NULL }; f_ptrlist_delete(p, &visitor); p = NULL; } } while(0)

#define FREELIST(p) _FREELIST(p, free)
#define FREELISTPTR(p) _FREELIST(p, NULL)

/* Sort comparison callback function declaration */
typedef int (*_pacman_fn_cmp)(const void *, const void *);

FPtrList *f_ptrlist_add_sorted(FPtrList *list, void *data, _pacman_fn_cmp fn);
bool _pacman_list_remove(FPtrList *haystack, void *needle, _pacman_fn_cmp fn, void **data);
FPtrList *_pacman_list_reverse(FPtrList *list);

typedef void (*FPtrListIteratorVisitorFunc)(FPtrListIterator *item, void *visitor_data);

void *f_ptrlistitem_data(const FPtrListIterator *self);
int f_ptrlistitem_insert_after(FPtrListIterator *self, FPtrListIterator *previous);
FPtrListIterator *f_ptrlistitem_next(FPtrListIterator *self);
size_t f_ptrlistiterator_count(const FPtrListIterator *self, const FPtrListIterator *to);

FPtrList *f_ptrlist_new(void);
int f_ptrlist_delete(FPtrList *list, FVisitor *visitor);

FPtrList *f_ptrlist_add(FPtrList *list, void *data);
int f_ptrlist_clear(FPtrList *list, FVisitor *visitor);
size_t f_ptrlist_count(const FPtrList *self);
FPtrListIterator *f_ptrlist_end(FPtrList *self);
const FPtrListIterator *f_ptrlist_end_const(const FPtrList *self);
FPtrListIterator *f_ptrlist_first(FPtrList *self);
const FPtrListIterator *f_ptrlist_first_const(const FPtrList *self);
FPtrListIterator *f_ptrlist_last(FPtrList *self);
const FPtrListIterator *f_ptrlist_last_const(const FPtrList *self);
FPtrListIterator *f_ptrlist_rend(FPtrList *self);
const FPtrListIterator *f_ptrlist_rend_const(const FPtrList *self);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus
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

		explicit const_iterator(iterable i)
			: m_iterable(i)
		{ }

		const_iterator(const const_iterator &o)
			: m_iterable(o.m_iterable)
		{ }

		~const_iterator()
		{ }

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

		explicit iterator(iterable i)
			: m_iterable(i)
		{ }

		iterator(const iterator &o)
			: m_iterable(o.m_iterable)
		{ }

		~iterator()
		{ }

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

		iterable m_iterable;
	};
}

class FCListItem
{
public:
	friend struct flib::iterable_traits<FCListItem *>;

	friend FPtrList *f_ptrlist_add_sorted(FPtrList *list, void *data, _pacman_fn_cmp fn);
	friend bool _pacman_list_remove(FPtrList *haystack, void *needle, _pacman_fn_cmp fn, void **data);
	friend FPtrList *f_ptrlist_add(FPtrList *list, void *data);
	friend int f_ptrlistitem_insert_after(FPtrListIterator *self, FPtrListIterator *previous);
	friend class FPtrList;

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
#endif

//protected: // Disable for now
	bool insert_after(FCListItem *previous);	

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
			return iterable_traits<FCListItem *>::next(i);
		}

		static iterable previous(const iterable &i)
		{
			return iterable_traits<FCListItem *>::previous(i);
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
	: private FCListItem
{
public:
	typedef FListItem<T> *iterable;
	/* std::list compatibility */
	typedef flib::iterator<iterable> iterator;
	typedef flib::iterator<iterable, true> reverse_iterator;
	typedef flib::const_iterator<iterable> const_iterator;
	typedef flib::const_iterator<iterable, true> const_reverse_iterator;
	typedef size_t size_type;

	FList()
		: FCListItem(this, this)
	{ }

	virtual ~FList() override
	{
//		clear();
	}

	/* Iterators */
	iterator begin()
	{
		return iterator(m_next);
	}

	const_iterator begin() const
	{
		return cbegin();
	}

	const_iterator cbegin() const
	{
		return const_iterator(m_next);
	}

	reverse_iterator rbegin()
	{
		return reverse_iterator(m_previous);
	}

	const_reverse_iterator rbegin() const
	{
		return crbegin();
	}

	const_reverse_iterator crbegin() const
	{
		return const_reverse_iterator(m_previous);
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
		return const_iterator(const_cast<FList<T> *>(this));
	}

	reverse_iterator rend()
	{
		return reverse_iterator(this);
	}

	const_reverse_iterator rend() const
	{
		return crend();
	}

	const_reverse_iterator crend() const
	{
		return const_reverse_iterator(const_cast<FList<T> *>(this));
	}

	/* Capacity */
	bool empty() const
	{
		return m_next == m_previous;
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
#if 0
	void clear()
	{ }
#endif

public:
	/* extensions */
	iterator first()
	{
		return iterator(m_next);
	}

	const_iterator first() const
	{
		return const_iterator(m_next);
	}

	iterator last()
	{
		return iterator(m_previous);
	}

	const_iterator last() const
	{
		return iterator(m_previous);
	}
#if 0
	virtual FList<T> &add(const reference val); // Make default implementation to happend
	{
		return *this;
	}
#endif

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
