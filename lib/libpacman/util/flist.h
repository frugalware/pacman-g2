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
typedef struct FCList FCList;
typedef struct FCListItem FCListItem;
#ifndef F_NOCOMPAT
typedef struct FPtrList FPtrList;
#else /* F_NOCOMPAT */
typedef struct FCList FPtrList;
#endif /* F_NOCOMPAT */
typedef struct FCListItem FPtrListIterator;
#else /* __cplusplus */
#ifndef F_NOCOMPAT
typedef class FPtrList FPtrList;
#else /* F_NOCOMPAT */
typedef class FCList FPtrList;
#endif /* F_NOCOMPAT */
typedef class FCListItem FPtrListIterator;
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
			m_iterable = !Reverse ? iterable_traits<Iterable>::next(m_iterable) : iterable_traits<Iterable>::previous(m_iterable);
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
			m_iterable = !Reverse ? iterable_traits<Iterable>::previous(m_iterable) : iterable_traits<Iterable>::next(m_iterable);
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

	protected:
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
			m_iterable = iterable_traits<Iterable>::next(m_iterable);
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
			m_iterable = iterable_traits<Iterable>::previous(m_iterable);
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

	protected:
		iterable m_iterable;
	};
}

class FCListItem
{
public:
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
		: m_next(next), m_previous(previous), m_data(NULL)
	{ }

	virtual ~FCListItem()
	{ } // FIXME: Make pure virtual

	virtual void *c_data() const
	{ return m_data; } // FIXME: Make pure virtual

	FCListItem *next() const
	{
		ASSERT(this != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
		return m_next;
	}

	FCListItem *previous() const
	{
		ASSERT(this != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
		return m_previous;
	}

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

//protected: // Disable for now
	bool insert_after(FCListItem *previous);	

	void *m_data; // Enabled for now

protected:
	FCListItem *m_next;
	FCListItem *m_previous;
};

class FCList
	: private FCListItem
{
public:
	/* std::list compatibility */
	typedef FCListItem *iterable;
	typedef flib::iterator<iterable> iterator;
	typedef flib::iterator<iterable, true> reverse_iterator;
	typedef flib::const_iterator<iterable> const_iterator;
	typedef flib::const_iterator<iterable, true> const_reverse_iterator;
	typedef size_t size_type;

	FCList()
		: FCListItem(this, this)
	{ }

	virtual ~FCList() override
	{
		clear();
	}

	/* Iterators */
	iterator begin()
	{
		return first();
	}

	const_iterator begin() const
	{
		return cbegin();
	}

	const_iterator cbegin() const
	{
		return first();
	}

	reverse_iterator rbegin()
	{
		return reverse_iterator(FCListItem::previous());
	}

	const_reverse_iterator rbegin() const
	{
		return crbegin();
	}

	const_reverse_iterator crbegin() const
	{
		return const_reverse_iterator(FCListItem::previous());
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
		return const_iterator(const_cast<FCList *>(this));
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
		return const_reverse_iterator(const_cast<FCList *>(this));
	}

	/* Capacity */
	bool empty() const
	{
		return next() == this;
	}

	size_type size() const
	{
		size_type size = 0;
		for(auto unused: *this) {
			(void) unused;
			++size;
		}
		return size;
	}

	/* Element access */
	
	/* Modifiers */
	void clear()
	{ }

public:
	/* Extensions */
	/* Element access */
	iterator first()
	{
		return iterator(FCListItem::next());
	}

	const_iterator first() const
	{
		return const_iterator(FCListItem::next());
	}

	iterator last()
	{
		return iterator(FCListItem::previous());
	}

	const_iterator last() const
	{
		return const_iterator(FCListItem::previous());
	}
	
	/* Modifiers */
	virtual bool add(const reference val); // Make default implementation to happend

protected:
	FCList(const FCList &o);

	FCList &operator = (const FCList &o);
};

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
