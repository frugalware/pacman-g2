/*
 *  time.h
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
#ifndef _PACMAN_TIME_H
#define _PACMAN_TIME_H

#include <time.h>

/* Return the localtime for timep. If timep is NULL, return the conversion for time(NULL) (libc returns NULL instead).
 */
struct tm *_pacman_localtime(const time_t *timep);

/* _lc means localised C */
size_t _pacman_strftime_lc(char *s, size_t max, const char *format, const struct tm *tm);
char *_pacman_strptime_lc(const char *s, const char *format, struct tm *tm);

#define PM_RFC1123_FORMAT "%a, %d %b %Y %H:%M:%S %z"
#define _pacman_strftime_rfc1123_lc(s, max, tm) _pacman_strftime_lc((s), (max), PM_RFC1123_FORMAT, (tm))
#define _pacman_strptime_rfc1123_lc(s, tm) _pacman_strptime_lc((s), PM_RFC1123_FORMAT, (tm))

#endif /* _PACMAN_TIME_H */

/* vim: set ts=2 sw=2 noet: */
