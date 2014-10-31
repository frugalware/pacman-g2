/*
 *  fstr.h
 *
 *  Copyright (c) 2014 by Michel Hermier <hermier@frugalware.org>
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
#ifndef F_STR_H
#define F_STR_H

#include "fstring.h"

#include "util/flist.h"

#include <cstddef>
#include <regex.h>

namespace flib {
	class str
	{
	public:
		typedef size_t size_type;

		static str create(char *s);
		static str create(const char *s);
		static str format(const char *fmt, ...);
		static str vformat(const char *fmt, va_list ap);

		constexpr str()
			: str(nullptr)
		{ }

		constexpr str(std::nullptr_t n)
			: m_str(n)
		{ }

		str(const char *s);
		str(const str &o);
		str(str &&o);
		~str();

		str &operator = (const str &o);
		str &operator = (str &&o);

		explicit operator const char * ()
		{
			return m_str;
		}

		/* Element access */
		const char *c_str() const
		{
			return data();
		}

		const char *data() const;

		/* Capacity */
		bool empty() const
		{
			return m_str == nullptr || m_str[0] == '\0';
		}

		size_type length() const
		{
			return size();
		}

		size_type size() const;

		/* Operations */
		int compare(const str &str) const;
		void reset(const char *s = nullptr);
		void swap(str &o);

	private:
		char *m_str;
	};

	bool operator == (const str &lhs, const str &rhs);
	bool operator == (const str &lhs, std::nullptr_t rhs);
	bool operator != (const str &lhs, const str &rhs);
	bool operator != (const str &lhs, std::nullptr_t rhs);
	bool operator < (const str &lhs, const str &rhs);
} // namespace flib

namespace std {
	bool operator == (nullptr_t lhs, const flib::str &rhs);
	bool operator != (nullptr_t lhs, const flib::str &rhs);
} // namespace std

class FStringList;

class FStrMatcher final
{
public:
	enum {
		EQUAL						= (1<<0),
		IGNORE_CASE			= (1<<1),
		REGEXP					= (1<<2),
		SUBSTRING				= (1<<3),

		ALL							= (EQUAL | REGEXP | SUBSTRING),
		ALL_IGNORE_CASE	= (IGNORE_CASE | ALL),
	};

	FStrMatcher();
	FStrMatcher(const char *pattern, int flags = EQUAL/*, owner*/);

	~FStrMatcher();

	bool operator () (const char *s) const
	{
		return match(s);
	}

	bool operator () (const flib::str &s) const
	{
		return match(s);
	}

	bool match(const char *s) const;
	bool match(const flib::str &s) const;

protected:
	int m_flags;
	char *m_str;
	regex_t m_regex;
};

int f_stringlist_any_match(const FStringList *list, const FStrMatcher *matcher);

#endif /* F_STR_H */

/* vim: set ts=2 sw=2 noet: */
