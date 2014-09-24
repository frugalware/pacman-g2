/*
 *  case.c
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
#include "util.h"

#include <ctype.h>

char *f_strtolower(char *str)
{
	return f_str_tolower(str, str) >= 0 ? str: NULL;
}

char *f_strtoupper(char *str)
{
	return f_str_toupper(str, str) >= 0 ? str: NULL;
}

int f_str_tolower(char *dst, const char *src)
{
	ASSERT(dst != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(src != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	do {
		(*(dst++)) = tolower(*src);
	} while(*(src++) != '\0');
	return 0;
}

int f_str_toupper(char *dst, const char *src)
{
	ASSERT(dst != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(src != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	do {
		(*(dst++)) = toupper(*src);
	} while(*(src++) != '\0');
	return 0;
}

/* vim: set ts=2 sw=2 noet: */
