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

#include "util/fmatcher.h"
#include "util/stringlist.h"

#include <regex.h>

class FStrMatcher
	: public FMatcher<const char *>
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

	virtual bool match(const char *str) const override;

protected:
	int m_flags;
	char *m_str;
	regex_t m_regex;
};

int f_stringlist_any_match(const FStringList *list, const FStrMatcher *matcher);

#endif /* F_STR_H */

/* vim: set ts=2 sw=2 noet: */
