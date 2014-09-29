/*
 *  flog.c
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

#include "config.h"

#include <stdarg.h> /* for va_list */
#include <stddef.h> /* for NULL */

/* pacman-g2 */
#include "util/flog.h"

#include "util/fabstractlogger.h"

FAbstractLogger *_f_sys_logger = NULL;

void f_log(unsigned char flag, const char *format, ...)
{
	va_list ap;

	if (_f_sys_logger == NULL) {
		return;
	}

	va_start(ap, format);
	f_vlog(flag, format, ap);
	va_end(ap);
}

void f_logs(unsigned char flag, const char *message)
{
	if(_f_sys_logger == NULL) {
		return;
	}

	_f_sys_logger->logs(flag, message);
}

void f_vlog(unsigned char flag, const char *format, va_list ap)
{
	if(_f_sys_logger == NULL) {
		return;
	}

	_f_sys_logger->vlog(flag, format, ap);
}

/* vim: set ts=2 sw=2 noet: */
