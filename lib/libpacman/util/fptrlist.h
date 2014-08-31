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
extern "C" {
#endif /* __cplusplus */

#define _FREELIST(p, f) do { if(p) { FVisitor visitor = { (FVisitorFunc)f, NULL }; f_ptrlist_delete(p, &visitor); p = NULL; } } while(0)

#define FREELIST(p) _FREELIST(p, free)
#define FREELISTPTR(p) _FREELIST(p, NULL)

/* Sort comparison callback function declaration */
typedef int (*_pacman_fn_cmp)(const void *, const void *);

FPtrList *f_ptrlist_add_sorted(FPtrList *list, void *data, _pacman_fn_cmp fn);
FPtrList *_pacman_list_remove(FPtrList *haystack, void *needle, _pacman_fn_cmp fn, void **data);
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
	friend FPtrList *_pacman_list_remove(FPtrList *haystack, void *needle, _pacman_fn_cmp fn, void **data);
	friend FPtrList *f_ptrlist_add(FPtrList *list, void *data);
	friend int f_ptrlistitem_insert_after(FPtrListIterator *self, FPtrListIterator *previous);

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

private:
	FCListItem *m_next;
	FCListItem *m_previous;
};

#ifndef F_NOCOMPAT
class FPtrList
	: protected FCListItem
{
public:
	friend FPtrList *f_ptrlist_add_sorted(FPtrList *list, void *data, _pacman_fn_cmp fn);
	friend FPtrList *_pacman_list_remove(FPtrList *haystack, void *needle, _pacman_fn_cmp fn, void **data);
	friend FPtrList *f_ptrlist_add(FPtrList *list, void *data);

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
		return const_cast<const_iterator>((const FCListItem * const)this);
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
		return this == NULL;
	}

	FPtrList *add(const void *data);
};
#endif

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
		for(auto dummy: *this) ++size;
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
#endif
#endif /* F_PTRLIST_H */

/* vim: set ts=2 sw=2 noet: */
