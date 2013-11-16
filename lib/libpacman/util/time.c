/*
 *  time.c
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

#ifndef __USE_XOPEN2K8
#define __USE_XOPEN2K8
#endif

#include <locale.h>

/* pacman-g2 */
#include "util/time.h"

#include "util.h"

/* Return the C locale
 */
static
locale_t _pacman_locale_c(void)
{
	static locale_t locale_c = NULL;
	
	if(locale_c == NULL) {
		locale_c = newlocale(LC_ALL_MASK, "C", NULL);
		/* BUG_ON(locale_c == NULL); */
	}
	return locale_c;
}

double _pacman_difftimeval(struct timeval timeval1, struct timeval timeval2)
{
	return difftime(timeval1.tv_sec , timeval2.tv_sec) +
		((double)(timeval1.tv_usec - timeval2.tv_usec) / 1000000);
}

struct tm *_pacman_localtime(const time_t *timep)
{
	time_t now;

	if(timep == NULL &&
		(now = time(NULL)) != PM_TIME_INVALID) {
		timep = &now;
	}
	return localtime(timep);
}

size_t _pacman_strftime_lc(char *s, size_t max, const char *format, const struct tm *tm)
{
	return strftime_l(s, max, format, tm, _pacman_locale_c());
}

char *_pacman_strptime_lc(const char *s, const char *format, struct tm *tm)
{
	return strptime_l(s, format, tm, _pacman_locale_c());
}

/* vim: set ts=2 sw=2 noet: */
