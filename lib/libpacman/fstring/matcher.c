/*
 *  matcher.c
 *
 *  Copyright (c) 2013 by Michel Hermier <hermier@frugalware.org>
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

#include "config.h"

#include "fstring.h"

#include "fstdlib.h"

#include <regex.h>

typedef struct FStrMatcherData FStrMatcherData;

struct FStrMatcherData {
	int flags;
	char *str;
	regex_t regex;
};

static
int f_strmatcher_match(const char *str, const void *_data)
{
	const FStrMatcherData *data = _data;

	if(data->flags & F_STRMATCHER_IGNORE_CASE) {
		char *_str = alloca(strlen(str) + sizeof(char));
		strcpy(_str, str);
		f_strtoupper(_str);
		str = _str;
	}
	if(((data->flags & F_STRMATCHER_EQUAL) && strcmp(str, data->str) == 0) ||
			((data->flags & F_STRMATCHER_SUBSTRING) && strstr(str, data->str) != NULL) ||
			((data->flags & F_STRMATCHER_REGEXP) && regexec(&data->regex, str, 0, 0, 0) == 0))
		return 1;

	return 0;
}

int f_strmatcher_init(FStrMatcher *strmatcher, const char *pattern, int flags)
{
	FStrMatcherData *data = NULL;

	ASSERT(strmatcher != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(!f_strempty(pattern), RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT((data = f_zalloc(sizeof(*data))) != NULL, return -1);

	strmatcher->fn = f_strmatcher_match;
	strmatcher->data = data;
	if(flags & (F_STRMATCHER_EQUAL | F_STRMATCHER_SUBSTRING)) {
		if((data->str = strdup(pattern)) != NULL) {
			if(flags & F_STRMATCHER_IGNORE_CASE) {
				f_strtoupper(data->str);
			}
		} else {
			flags = 0; /* strdup fails, the matcher is invalid */
		}
	}
	if(flags & F_STRMATCHER_REGEXP) {
		int regex_flags = REG_EXTENDED | REG_NOSUB;
		
		regex_flags |= flags & F_STRMATCHER_IGNORE_CASE ? REG_ICASE : 0;
		if(regcomp(&data->regex, pattern, regex_flags) != 0) {
			flags |= ~F_STRMATCHER_REGEXP;
		}
	}
	data->flags = flags;
	if((flags & ~F_STRMATCHER_IGNORE_CASE) == 0) {
		f_strmatcher_fini(strmatcher);
		RET_ERR(PM_ERR_WRONG_ARGS, -1);
	}
	data->flags = flags;
	return 0;
}

int f_strmatcher_fini(FStrMatcher *strmatcher)
{
	FStrMatcherData *data = NULL;

	ASSERT(strmatcher != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(strmatcher->fn != f_strmatcher_match, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT((data = strmatcher->data) != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	free(data->str);
	if(data->flags & F_STRMATCHER_REGEXP) {
		regfree(&data->regex);
	}
	free(data);
	return 0;
}

/* vim: set ts=2 sw=2 noet: */
