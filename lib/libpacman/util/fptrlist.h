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

#ifndef __cplusplus

typedef struct FCList FCList;
typedef struct FCListItem FCListItem;

#ifndef F_NOCOMPAT
typedef struct FCListItem FPtrList;
#else
typedef struct FCList FPtrList;
#endif
typedef struct FCListItem FPtrListItem;

#else

#ifndef F_NOCOMPAT
typedef class FCListItem FPtrList;
#else
typedef class FCList FPtrList;
#endif
typedef class FCListItem FPtrListItem;

namespace flib
{
	template <typename Iterable>
	struct iterable_traits
	{
		typedef Iterable iterable;
//		typedef typename iterable::difference_type difference_type;
		typedef typename iterable::pointer pointer;
		typedef typename iterable::reference reference;
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

	template <typename Iterable>
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
			m_iterable = iterable_traits<Iterable>::next(m_iterable);
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
			m_iterable = iterable_traits<Iterable>::previous(m_iterable);
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

	template <typename Iterable>
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
	typedef void *value_type;
	typedef value_type *pointer;
	typedef value_type &reference;

	FCListItem()
		: FCListItem(NULL, NULL)
	{ }

	FCListItem(FCListItem *previous, FCListItem *next)
		: m_next(next), m_previous(previous)
	{ }

	virtual ~FCListItem()
	{ } // FIXME: Make pure virtual

	virtual void *c_data() const
	{ return m_data; } // FIXME: Make pure virtual

	FCListItem *next() const
	{
		return m_next;
	}

	FCListItem *previous() const
	{
		return m_previous;
	}

	void *&operator * ()
	{
		return m_data;
	}

	const void *operator * () const
	{
		return m_data;
	}

#ifndef F_NOCOMPAT
//	Migration code
	typedef FCListItem *iterator;
	typedef const iterator const_iterator;

	iterator begin()
	{
		return this;
	}

	const_iterator begin() const
	{
		return cbegin();
	}

	const_iterator cbegin() const
	{
		return const_cast<const_iterator>(this);
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
#endif

//protected: // Disable for now
	bool insert_after(FCListItem *previous);	

	FCListItem *m_next;
	FCListItem *m_previous;
	void *m_data; // Enabled for now
};

class FCList
	: private FCListItem
{
public:
	/* std::list compatibility */
	typedef FCListItem *iterable;
	typedef flib::const_iterator<FCListItem *> const_iterator;
	typedef flib::iterator<FCListItem *> iterator;
	typedef size_t size_type;

	FCList()
		: FCListItem(this, this)
	{ }

	~FCList()
	{
		clear();
	}

	void clear()
	{ }

	size_type size() const
	{
		size_type size = 0;
		for(auto dummy: *this) ++size;
		return size;
	}

	bool empty() const
	{
		return next() == this;
	}

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

public:
	/* extensions */
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
	
protected:
	virtual bool add(FCListItem *item);

protected:
	FCList(const FCList &o);

	FCList &operator = (const FCList &o);
};

extern "C" {
#endif

#define _FREELIST(p, f) do { if(p) { FVisitor visitor = { (FVisitorFunc)f, NULL }; f_ptrlist_delete(p, &visitor); p = NULL; } } while(0)

#define FREELIST(p) _FREELIST(p, free)
#define FREELISTPTR(p) _FREELIST(p, NULL)

/* Sort comparison callback function declaration */
typedef int (*_pacman_fn_cmp)(const void *, const void *);

FPtrList *f_ptrlist_add_sorted(FPtrList *list, void *data, _pacman_fn_cmp fn);
FPtrList *_pacman_list_remove(FPtrList *haystack, void *needle, _pacman_fn_cmp fn, void **data);
FPtrList *_pacman_list_reverse(FPtrList *list);

typedef int (*FPtrListItemComparatorFunc)(const FPtrListItem *item, const void *comparator_data);
typedef void (*FPtrListItemVisitorFunc)(FPtrListItem *item, void *visitor_data);

FPtrList *f_list_new(void);

FPtrListItem *f_ptrlistitem_new(void *data);
int f_ptrlistitem_delete(FPtrListItem *self, FVisitor *visitor);

void *f_ptrlistitem_data(const FPtrListItem *self);
int f_ptrlistitem_insert_after(FPtrListItem *self, FPtrListItem *previous);
FPtrListItem *f_ptrlistitem_next(FPtrListItem *self);
FPtrListItem *f_ptrlistitem_previous(FPtrListItem *self);

int f_ptrlistitem_ptrcmp(const FPtrListItem *item, const void *ptr);

FPtrList *f_ptrlist_new(void);
int f_ptrlist_delete(FPtrList *list, FVisitor *visitor);

int f_ptrlist_init(FPtrList *self);
int f_ptrlist_fini(FPtrList *self, FVisitor *visitor);

#define f_ptrlist_add f_ptrlist_append
#ifndef F_NOCOMPAT
FPtrList *f_ptrlist_append(FPtrList *list, void *data);
#else
int f_ptrlist_append(FPtrList *list, void *data);
#endif
int f_ptrlist_clear(FPtrList *list, FVisitor *visitor);
bool f_ptrlist_contains(const FPtrList *list, FPtrListItemComparatorFunc comparator, const void *comparator_data);
bool f_ptrlist_contains_ptr(const FPtrList *list, const void *ptr);
int f_ptrlist_count(const FPtrList *self);
bool f_ptrlist_empty(const FPtrList *self);
FPtrListItem *f_ptrlist_end(FPtrList *self);
const FPtrListItem *f_ptrlist_end_const(const FPtrList *self);
FPtrListItem *f_ptrlist_find(FPtrList *self, FPtrListItemComparatorFunc comparator, const void *comparator_data);
const FPtrListItem *f_ptrlist_find_const(const FPtrList *self, FPtrListItemComparatorFunc comparator, const void *comparator_data);
FPtrListItem *f_ptrlist_first(FPtrList *self);
const FPtrListItem *f_ptrlist_first_const(const FPtrList *self);
FPtrListItem *f_ptrlist_last(FPtrList *self);
const FPtrListItem *f_ptrlist_last_const(const FPtrList *self);
FPtrListItem *f_ptrlist_rend(FPtrList *self);
const FPtrListItem *f_ptrlist_rend_const(const FPtrList *self);

#ifdef __cplusplus
}
#endif
#endif /* F_PTRLIST_H */

/* vim: set ts=2 sw=2 noet: */
