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

typedef struct FCListItem FPtrList;
typedef struct FCListItem FPtrListItem;

#else

typedef class FCListItem FPtrList;
typedef class FCListItem FPtrListItem;

#include <memory> // for std::allocator, std::allocator_traits

namespace flib
{
	template <typename Iter, typename Alloc = std::allocator<Iter>>
	struct iterable_traits
		: std::allocator_traits<Alloc>
	{
		typedef std::allocator_traits<Alloc> super_type;
		typedef iterable_traits<Iter, Alloc> self_type;
		typedef Iter iterable;
		typedef typename super_type::pointer pointer;
		typedef typename super_type::const_pointer const_pointer;
		typedef typename iterable::reference reference;
		typedef typename iterable::const_reference const_reference;

		static pointer next(pointer p)
		{
			return p->next();
		}

		static const_pointer next(const_pointer p)
		{
			return p->next();
		}

		static pointer previous(pointer p)
		{ 
			return p->previous();
		}

		static const_pointer previous(const_pointer p)
		{
			return p->previous();
		}

		static reference dereference(pointer p)
		{
			return *p;
		}

		static const_reference dereference(const_pointer p)
		{
			return *p;
		}
	};

	template <typename T>
	struct const_iterator
	{
	public:
		typedef const_iterator<T> self_type;
		typedef self_type &self_reference_type;
		typedef const self_reference_type const_self_reference_type;
		typedef flib::iterable_traits<T> iterable_traits;
		typedef typename iterable_traits::const_pointer const_pointer;
		typedef typename iterable_traits::const_reference const_reference;

		explicit const_iterator(const_pointer iterable)
			: m_iterable(iterable)
		{ }

		const_iterator(const const_iterator &o)
			: m_iterable(o.m_iterable)
		{ }

		~const_iterator()
		{ }

		self_reference_type operator = (const_self_reference_type o)
		{
			m_iterable = o.m_iterable;
			return *this;
		}

		bool operator == (const_self_reference_type o)
		{
			return m_iterable == o.m_iterable;
		}

		bool operator != (const_self_reference_type o)
		{
			return !operator == (o.m_iterable);
		}

		self_reference_type operator ++ ()
		{
			m_iterable = iterable_traits::next(m_iterable);
			return *this;
		}

		self_type operator ++ (int)
		{
			self_type tmp(*this);
			operator ++ ();
			return tmp;
		}

		self_reference_type operator -- ()
		{
			m_iterable = iterable_traits::previous(m_iterable);
			return *this;
		}

		self_type operator -- (int)
		{
			self_type tmp(*this);
			operator -- ();
			return tmp;
		}

		const_reference operator * () const
		{
			return iterable_traits::dereference(m_iterable);
		}

	protected:
		const_pointer m_iterable;
	};

	template <typename T>
	struct iterator
	{
	public:
		typedef const_iterator<T> self_type;
		typedef self_type &self_reference_type;
		typedef const self_reference_type const_self_reference_type;
		typedef flib::iterable_traits<T> iterable_traits;
		typedef typename iterable_traits::pointer pointer;
		typedef typename iterable_traits::reference reference;
		typedef typename iterable_traits::const_reference const_reference;

		explicit iterator(pointer iterable)
			: m_iterable(iterable)
		{ }

		iterator(const_self_reference_type o)
			: m_iterable(o.m_iterable)
		{ }

		~iterator()
		{ }

		const_self_reference_type operator = (const_self_reference_type o)
		{
			m_iterable = o.m_iterable;
			return *this;
		}

		bool operator == (const_self_reference_type o)
		{
			return m_iterable == o.m_iterable;
		}

		bool operator != (const_self_reference_type o)
		{
			return !operator == (o.m_iterable);
		}

		self_reference_type operator ++ ()
		{
			m_iterable = iterable_traits::next(m_iterable);
			return *this;
		}

		self_type operator ++ (int)
		{
			self_type tmp(*this);
			operator ++ ();
			return tmp;
		}

		self_reference_type operator -- ()
		{ 
			m_iterable = iterable_traits::previous(m_iterable);
			return *this;
		}

		self_type operator -- (int)
		{
			self_type tmp(*this);
			operator -- ();
			return tmp;
		}

		reference operator * ()
		{
			return iterable_traits::dereference(m_iterable);
		}

		const_reference operator * () const
		{
			return iterable_traits::dereference(m_iterable);
		}

	protected:
		pointer m_iterable;
	};
}

class FCListItem
{
public:
	typedef void * &reference;
	typedef const reference const_reference;

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

	FCListItem *next()
	{
		return m_next;
	}

	FCListItem *next() const
	{
		return m_next;
	}

	FCListItem *previous()
	{
		return m_previous;
	}

	const FCListItem *previous() const
	{
		return m_previous;
	}

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
	typedef flib::iterator<FCListItem> iterator;
	typedef flib::const_iterator<FCListItem> const_iterator;

	FCList()
		: FCListItem(this, this)
	{ }

	~FCList()
	{
		clear();
	}

	void clear()
	{ }

	size_t count() const;

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
void f_ptrlist_foreach(const FPtrList *list, FPtrListItemVisitorFunc visitor, void *visitor_data);
FPtrListItem *f_ptrlist_last(FPtrList *self);
const FPtrListItem *f_ptrlist_last_const(const FPtrList *self);
FPtrListItem *f_ptrlist_rend(FPtrList *self);
const FPtrListItem *f_ptrlist_rend_const(const FPtrList *self);

#ifdef __cplusplus
}
#endif
#endif /* F_PTRLIST_H */

/* vim: set ts=2 sw=2 noet: */
