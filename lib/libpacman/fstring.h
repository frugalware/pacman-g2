/*
 *  fstring.h
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
#ifndef _F_STRING_H
#define _F_STRING_H

#include <string.h>

static inline
int f_strempty(const char *s)
{
	return s != NULL ? s[0] == '\0' : !0;
}

static inline
size_t f_strlen(const char *s)
{
	return s != NULL ? strlen(s) : 0;
}

char *f_strtoupper(char *str);
char *f_strtrim(char *str);

#endif /* _F_STRING_H */

/* vim: set ts=2 sw=2 noet: */
