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

#ifndef __cplusplus
typedef struct FPtrList FPtrList;
typedef struct FPtrListIterator FPtrListIterator;
#else /* __cplusplus */
typedef class FPtrList FPtrList;
typedef class FListItem<void *> FPtrListItem;
typedef FPtrListItem FPtrListIterator;

extern "C" {
#endif /* __cplusplus */

#define _FREELIST(p, f) do { if(p) { FVisitor visitor = { (FVisitorFunc)f, NULL }; f_ptrlist_delete(p, &visitor); p = NULL; } } while(0)

#define FREELIST(p) _FREELIST(p, free)

FPtrList *f_ptrlist_add_sorted(FPtrList *list, void *data, _pacman_fn_cmp fn);
bool _pacman_list_remove(FPtrList *haystack, void *needle, _pacman_fn_cmp fn, void **data);
FPtrList *_pacman_list_reverse(FPtrList *list);

typedef void (*FPtrListIteratorVisitorFunc)(FPtrListIterator *item, void *visitor_data);

void *f_ptrlistitem_data(const FPtrListIterator *self);
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

class FPtrList
	: protected FCListItem
{
public:
	friend FPtrList *f_ptrlist_add_sorted(FPtrList *list, void *data, _pacman_fn_cmp fn);

	typedef FPtrListItem *iterable;

	class iterator
	{
	public:
		iterator(iterable it = iterable())
			: m_iterable(it)
		{ }

		bool operator == (const iterator &o)
		{
			return m_iterable == o.m_iterable;
		}

		bool operator != (const iterator &o)
		{ 
			return !operator == (o);
		}

		iterable operator -> () const
		{
			return m_iterable;
		}

		void *operator * () const
		{
			return m_iterable->m_data;
		}

		operator iterable ()
		{
			return m_iterable;
		}

		iterator next() const
		{
			return m_iterable->next();
		}

		iterator previous() const
		{
			return m_iterable->previous();
		}

		iterable m_iterable;
	};

	typedef iterator const_iterator;
	typedef iterator reverse_iterator;
	typedef iterator const_reverse_iterator;

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
		return iterator(_next());
	}

	const_iterator begin() const
	{
		return cbegin();
	}

	const_iterator cbegin() const
	{
		ASSERT(this != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
		return const_iterator(_next());
	}

	iterator end()
	{
		return iterator(_self());
	}

	const_iterator end() const
	{
		return cend();
	}

	const_iterator cend() const
	{
		return const_iterator(_self());
	}

	iterator last()
	{
		return iterator(_previous());
	}

	const_iterator last() const
	{
		return clast();
	}

	const_iterator clast() const
	{
		return const_iterator(_previous());
	}

	reverse_iterator rbegin()
	{
		return reverse_iterator(_previous());
	}

	const_reverse_iterator rbegin() const
	{
		return crbegin();
	}

	const_reverse_iterator crbegin() const
	{
		return const_reverse_iterator(_previous());
	}

	reverse_iterator rend()
	{
		return reverse_iterator(_self());
	}

	const_reverse_iterator rend() const
	{
		return crend();
	}

	const_reverse_iterator crend() const
	{
		return const_reverse_iterator(_self());
	}

	bool empty() const
	{
		ASSERT(this != NULL, RET_ERR(PM_ERR_WRONG_ARGS, true));
		return begin() == end();
	}

	FPtrList &add(void *data);
	bool remove(void *ptr, _pacman_fn_cmp fn, void **data);

	void clear()
	{
		// FIXME: lets leak for now
		ASSERT(this != NULL, pm_errno = PM_ERR_WRONG_ARGS; return);
		m_next = m_previous = this;
	}

	void swap(FPtrList &o) {
		FCListItem::swap(o);
	}

protected:
	iterable _next() const
	{
		return static_cast<iterable>(m_next);
	}

	iterable _previous() const
	{
		return static_cast<iterable>(m_previous);
	}

	iterable _self() const
	{
		return static_cast<iterable>((FCListItem *)this);
	}

private:
	FPtrList(const FPtrList &o);
	FPtrList &operator = (const FPtrList &o);
};
#endif /* __cplusplus */
#endif /* F_PTRLIST_H */

/* vim: set ts=2 sw=2 noet: */
