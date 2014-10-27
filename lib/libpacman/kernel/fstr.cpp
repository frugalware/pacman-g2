/*
 *  fstr.cpp
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

#include "kernel/fstr.h"

#include "fstring.h"
#include "util/fstringlist.h"

using namespace flib;

str::str(const char *s)
	: str()
{
	reset(s);
}

str::str(const str &o)
	: str(o.m_str)
{ }

str::str(str &&o)
	: str()
{
	swap(o);
}

str::~str()
{
	reset();
}

str &str::operator = (const str &o)
{
	reset(o.m_str);
	return *this;
}

str &str::operator = (str &&o)
{
	swap(o);
	return *this;
}

str str::create(char *s)
{
	str ret;
	ret.m_str = s;
	return ret;
}

str str::create(const char *s)
{
	return str(s);
}

str str::format(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	str ret = vformat(fmt, ap);
	va_end(ap);
	return ret;
}

str str::vformat(const char *fmt, va_list ap)
{
	char *dest;

	vasprintf(&dest, fmt, ap);
	return create(dest);
}

const char *str::data() const
{
	if (empty()) {
		return "";
	}
	return m_str;
}

str::size_type str::size() const
{
	if(!empty()) {
		return strlen(m_str);
	}
	return 0;
}

int str::compare(const str &s) const
{
	return strcmp(c_str(), s.c_str());
}

void str::reset(const char *s)
{
	if(m_str != nullptr) {
		free(m_str);
	}
	m_str = f_strdup(s);
}

void str::swap(str &o)
{
	std::swap(m_str, o.m_str);
}

bool flib::operator == (const str &lhs, const str &rhs)
{
	return lhs.compare(rhs) == 0;
}

bool flib::operator == (const str &lhs, std::nullptr_t rhs)
{
	return lhs.compare(str(rhs)) == 0;
}

bool flib::operator != (const str &lhs, const str &rhs)
{
	return !(lhs == rhs);
}

bool flib::operator != (const str &lhs, std::nullptr_t rhs)
{
	return !(lhs == rhs);
}

bool flib::operator < (const str &lhs, const str &rhs)
{
	return lhs.compare(rhs) < 0;
}

bool std::operator == (nullptr_t lhs, const flib::str &rhs)
{
	return rhs == lhs;
}

bool std::operator != (nullptr_t lhs, const flib::str &rhs)
{
	return rhs != lhs;
}

FStrMatcher::FStrMatcher()
	: m_flags(0), m_str(NULL)
{ }

FStrMatcher::FStrMatcher(const char *pattern, int flags)
{
	if(flags & (EQUAL | SUBSTRING)) {
		if((m_str = strdup(pattern)) != NULL) {
			if(flags & IGNORE_CASE) {
				f_strtoupper(m_str);
			}
		} else {
			flags = 0; /* strdup fails, the matcher is invalid */
		}
	}
	if(flags & REGEXP) {
		int regex_flags = REG_EXTENDED | REG_NOSUB;
		
		regex_flags |= flags & IGNORE_CASE ? REG_ICASE : 0;
		if(regcomp(&m_regex, pattern, regex_flags) != 0) {
			flags |= ~REGEXP;
		}
	}
	m_flags = flags;
#if 0
	if((flags & ~IGNORE_CASE) == 0) {
		f_strmatcher_fini(strmatcher);
		RET_ERR(PM_ERR_WRONG_ARGS, -1);
	}
#endif
}

FStrMatcher::~FStrMatcher()
{
	free(m_str);
	if(m_flags & REGEXP) {
		regfree(&m_regex);
	}
}

bool FStrMatcher::match(const char *str) const
{
	if(m_flags & IGNORE_CASE) {
		char *_str = alloca(strlen(str) + sizeof(char));
		f_str_toupper(_str, str);
		str = _str;
	}
	if(((m_flags & EQUAL) && f_streq(str, m_str)) ||
			((m_flags & SUBSTRING) && strstr(str, m_str) != NULL) ||
			((m_flags & REGEXP) && regexec(&m_regex, str, 0, 0, 0) == 0)) {
		return 1;
	}
	return 0;
}

int f_stringlist_any_match(const FStringList *list, const FStrMatcher *matcher)
{
#ifndef F_NOCOMPAT
	for(auto it = list->begin(), end = list->end(); it != end; ++it) {
		if(matcher->match((const char *)*it) != 0) {
			return 1;
		}
	}
	return 0;
#else
	for(auto str: *list) {
		if(matcher->match((const char *)str) != 0) {
			return 1;
		}
	}
	return 0;
#endif
}

/* vim: set ts=2 sw=2 noet: */

