/*
 *  log.h
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
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
#ifndef _PACMAN_LOG_H
#define _PACMAN_LOG_H

#include "pacman.h"

#include <stdarg.h>

#define LOG_STR_LEN 256

extern pacman_cb_log pm_logcb;
extern unsigned char pm_logmask;

void _pacman_log(unsigned char flag, const char *format, ...);
void _pacman_vlog(unsigned char flag, const char *format, va_list ap);
void _pacman_vlogaction(unsigned char usesyslog, FILE *f, const char *format, va_list ap);

#endif /* _PACMAN_LOG_H */

/* vim: set ts=2 sw=2 noet: */
