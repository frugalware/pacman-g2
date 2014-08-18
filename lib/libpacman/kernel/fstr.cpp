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

#include "fstr.h"

#include "fstring.h"

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
	if(((m_flags & EQUAL) && strcmp(str, m_str) == 0) ||
			((m_flags & SUBSTRING) && strstr(str, m_str) != NULL) ||
			((m_flags & REGEXP) && regexec(&m_regex, str, 0, 0, 0) == 0)) {
		return 1;
	}
	return 0;
}

int f_stringlist_any_match(const FStringList *list, const FStrMatcher *matcher)
{
	const FStringListItem *it;

	for(it = f_ptrlist_first_const(list); it != f_list_end_const(list); it = it->next) {
		if(matcher->match(f_stringlistitem_to_str(it)) != 0) {
			return 1;
		}
	}
	return 0;
}

/* vim: set ts=2 sw=2 noet: */

