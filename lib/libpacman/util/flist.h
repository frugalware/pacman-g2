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

#include "util.h"

#include <stddef.h>

#ifdef __cplusplus

#include "util/falgorithm.h"

namespace flib
{
	template <typename Iterator>
	struct iterator_traits
	{
		typedef Iterator iterator;
		typedef typename iterator::difference_type difference_type;
		typedef typename iterator::pointer pointer;
		typedef typename iterator::reference reference;
		typedef typename iterator::size_type size_type;
		typedef typename iterator::value_type value_type;

		static iterator next(const iterator &i)
		{
			return i.next();
		}

		static iterator previous(const iterator &i)
		{ 
			return i.previous();
		}

		static reference reference_of(iterator i)
		{
			return *i;
		}

		static pointer pointer_of(iterator i)
		{
			return i.operator -> ();
		}

		static value_type value_of(const iterator i)
		{
			return *i;
		}
	};

	template <typename Iterable, bool Reverse = false>
	struct const_iterator_wrapper
	{
	public:
//		typedef typename iterator_traits<Iterable>::difference_type difference_type;
		typedef typename iterator_traits<Iterable>::iterable iterable;
		typedef typename iterator_traits<Iterable>::pointer pointer;
		typedef typename iterator_traits<Iterable>::size_type size_type;
		typedef typename iterator_traits<Iterable>::value_type value_type;

		explicit const_iterator_wrapper(iterable i = iterable())
			: m_iterable(i)
		{ }

		const_iterator_wrapper(const const_iterator_wrapper &o)
			: m_iterable(o.m_iterable)
		{ }

		~const_iterator_wrapper()
		{ }

		operator iterable ()
		{
			return m_iterable;
		}

		const_iterator_wrapper &operator = (const const_iterator_wrapper &o)
		{
			m_iterable = o.m_iterable;
			return *this;
		}

		bool operator == (const const_iterator_wrapper &o) const
		{
			return m_iterable == o.m_iterable;
		}

		bool operator != (const const_iterator_wrapper &o) const
		{
			return !operator == (o);
		}

		const_iterator_wrapper &operator ++ ()
		{
			m_iterable = _next();
			return *this;
		}

		const_iterator_wrapper operator ++ (int)
		{
			const_iterator_wrapper tmp(*this);
			operator ++ ();
			return tmp;
		}

		const_iterator_wrapper &operator -- ()
		{
			m_iterable = _previous();
			return *this;
		}

		const_iterator_wrapper operator -- (int)
		{
			const_iterator_wrapper tmp(*this);
			operator -- ();
			return tmp;
		}

		value_type operator * () const
		{
			return iterator_traits<Iterable>::value_of(m_iterable);
		}

		const pointer operator -> () const
		{
			return iterator_traits<Iterable>::pointer_of(m_iterable);
		}

		const_iterator_wrapper next() const
		{
			return const_iterator_wrapper(_next());
		}

		const_iterator_wrapper previous() const
		{
			return const_iterator_wrapper(_previous());
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
			return !Reverse ? iterator_traits<Iterable>::next(m_iterable) : iterator_traits<Iterable>::previous(m_iterable);
		}

		iterable _previous() const
		{
			return !Reverse ? iterator_traits<Iterable>::previous(m_iterable) : iterator_traits<Iterable>::next(m_iterable);
		}

		iterable m_iterable;
	};

	template <typename Iterable, bool Reverse = false>
	struct iterator_wrapper
	{
	public:
//		typedef typename iterator_traits<Iterable>::difference_type difference_type;
		typedef typename iterator_traits<Iterable>::iterable iterable;
		typedef typename iterator_traits<Iterable>::pointer pointer;
		typedef typename iterator_traits<Iterable>::reference reference;
		typedef typename iterator_traits<Iterable>::size_type size_type;
		typedef typename iterator_traits<Iterable>::value_type value_type;

		explicit iterator_wrapper(iterable i = iterable())
			: m_iterable(i)
		{ }

		iterator_wrapper(const iterator_wrapper &o)
			: m_iterable(o.m_iterable)
		{ }

		~iterator_wrapper()
		{ }

		operator iterable ()
		{
			return m_iterable;
		}

		iterator_wrapper &operator = (const iterator_wrapper &o)
		{
			m_iterable = o.m_iterable;
			return *this;
		}

		bool operator == (const iterator_wrapper &o) const
		{
			return m_iterable == o.m_iterable;
		}

		bool operator != (const iterator_wrapper &o) const
		{
			return !operator == (o);
		}

		iterator_wrapper &operator ++ ()
		{
			m_iterable = _next();
			return *this;
		}

		iterator_wrapper operator ++ (int)
		{
			iterator_wrapper tmp(*this);
			operator ++ ();
			return tmp;
		}

		iterator_wrapper &operator -- ()
		{
			m_iterable = _previous();
			return *this;
		}

		iterator_wrapper operator -- (int)
		{
			iterator_wrapper tmp(*this);
			operator -- ();
			return tmp;
		}

		reference operator * ()
		{
			return iterator_traits<Iterable>::reference_of(m_iterable);
		}

		value_type operator * () const
		{
			return iterator_traits<Iterable>::value_of(m_iterable);
		}

		pointer operator -> ()
		{
			return iterator_traits<Iterable>::pointer_of(m_iterable);
		}

		const pointer operator -> () const
		{
			return iterator_traits<Iterable>::pointer_of(m_iterable);
		}

		iterator_wrapper next() const
		{
			return iterator_wrapper(_next());
		}

		iterator_wrapper previous() const
		{
			return iterator_wrapper(_previous());
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
			return !Reverse ? iterator_traits<Iterable>::next(m_iterable) : iterator_traits<Iterable>::previous(m_iterable);
		}

		iterable _previous() const
		{
			return !Reverse ? iterator_traits<Iterable>::previous(m_iterable) : iterator_traits<Iterable>::next(m_iterable);
		}

	public: /* FIXME: Make protected/private */
		iterable m_iterable;
	};

	class FCListItem
	{
	public:
		friend struct flib::iterator_traits<FCListItem *>;

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

		void reverse()
		{
			std::swap(m_next, m_previous);
		}

	protected:
		FCListItem *m_next;
		FCListItem *m_previous;
	};

	template <>
	struct iterator_traits<FCListItem *>
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

	template <typename T>
	class FListItem
		: public flib::FCListItem
	{
	public:
		friend struct flib::iterator_traits<FListItem *>;

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

//	protected:
		T m_data;

	private:
		FListItem(const FListItem<T> &o);

		FListItem<T> &operator = (const FListItem<T> &o);
	};

	template <typename T>
	struct iterator_traits<FListItem<T> *>
	{
		typedef FListItem<T> *iterable;
//		typedef typename FListItem<T>::difference_type difference_type;
		typedef typename FListItem<T>::pointer pointer;
		typedef typename FListItem<T>::reference reference;
		typedef typename FListItem<T>::size_type size_type;
		typedef typename FListItem<T>::value_type value_type;

		static iterable next(const iterable &i)
		{
			return static_cast<iterable>(iterator_traits<FCListItem *>::next(i));
		}

		static iterable previous(const iterable &i)
		{
			return static_cast<iterable>(iterator_traits<FCListItem *>::previous(i));
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

	template <typename T>
	class list
		: protected FCListItem
	{
	public:
		typedef flib::FListItem<T> *iterable;

		/* std::list compatibility */
		typedef T value_type;
		typedef flib::iterator_wrapper<iterable> iterator;
		typedef flib::iterator_wrapper<iterable, true> reverse_iterator;
		typedef flib::const_iterator_wrapper<iterable> const_iterator;
		typedef flib::const_iterator_wrapper<iterable, true> const_reverse_iterator;
		typedef size_t size_type;

		list()
			: FCListItem(this, this)
		{ }

		list(list &&o)
			: list()
		{
			swap(o);
		}

		list &operator = (list &&o)
		{
			swap(o);
			return *this;
		}

		virtual ~list() override
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
		bool contains(const value_type &val) const
		{
			return find(val) != end();
		}

		iterator find(const value_type &val)
		{
			return flib::find(begin(), end(), val);
		}

		const_iterator find(const value_type &val) const
		{ 
			return flib::find(begin(), end(), val);
		}

		template <class UnaryPredicate>
		iterator find_if(UnaryPredicate pred)
		{
			return flib::find_if(begin(), end(), pred);
		}

		template <class UnaryPredicate>
		const_iterator find_if(UnaryPredicate pred) const
		{
			return flib::find_if(begin(), end(), pred);
		}

		template <class UnaryPredicate>
		iterator find_if_not(UnaryPredicate pred)
		{
			return flib::find_if_not(begin(), end(), pred);
		}

		template <class UnaryPredicate>
		const_iterator find_if_not(UnaryPredicate pred) const
		{
			return flib::find_if_not(begin(), end(), pred);
		}

		/* Modifiers */
		template <class Function = ::flib::detail::null_visitor>
		void clear(Function fn = Function())
		{
			erase(begin(), end(), fn);
		}

		template <class Function = ::flib::detail::null_visitor>
		iterator erase(iterator position, Function fn = Function())
		{
			ASSERT(position != c_end(), RET_ERR(PM_ERR_WRONG_ARGS, position));

			iterable erased = (iterable)position;
			iterable next = erased->next();
			erased->remove();
			delete erased;
			return iterator(next);
		}

		template <class Function = ::flib::detail::null_visitor>
		iterator erase(iterator first, iterator last, Function fn = Function())
		{
			auto it = first;
			while(it != last) {
				it = erase(it);
			}
			return it;
		}

		template <class Function = ::flib::detail::null_visitor>
		size_type remove(const value_type &val, Function fn = Function())
		{
			return remove_if(
					[&] (const value_type &o) -> bool
					{ return o == val; }, fn );
		}

		template <class UnaryPredicate, class Function = ::flib::detail::null_visitor>
		size_type remove_if(UnaryPredicate pred, Function fn = Function())
		{
			size_type size = 0;
			iterator it = begin(), end = this->end();

			while((it = flib::find_if(it, end, pred)) != end) {
				fn(*it);
				it = erase(it);
				++size;
			}
			return size;
		}

		/* Observers */

		/* Operations */
		void reverse() {
			FCListItem *it = this;
			do {
				it->reverse();
			} while ((it = it->next()) != this);
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

		virtual iterator add(const value_type &val)
		{
			// Default implementation is append
			iterable newItem = new flib::FListItem<T>(val);
			newItem->insert_after(last());
			return iterator(newItem);
		}

		void swap(list &o) {
			FCListItem::swap(o);
		}

		template <class U>
		bool all_match(const U &val) const
		{
			return flib::all_match(begin(), end(), val);
		}

		template <class UnaryPredicate>
		bool all_match_if(UnaryPredicate pred) const
		{
			return flib::all_match_if(begin(), end(), pred);
		}

		template <class U>
		bool any_match(const U &val) const
		{
			return flib::any_match(begin(), end(), val());
		}

		template <class UnaryPredicate>
		bool any_match_if(UnaryPredicate pred)
		{
			return flib::any_match_if(begin(), end(), pred);
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
		list(const list &);
		list &operator = (const list &);

#if 0
		virtual void *c_data() const override
		{
			RET_ERR(PM_ERR_WRONG_ARGS, NULL);
		}
#endif
	};
} // namespace flib
#endif /* __cplusplus */

#endif /* F_LIST_H */

/* vim: set ts=2 sw=2 noet: */
