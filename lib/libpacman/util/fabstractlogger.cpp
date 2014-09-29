/*
 *  fabstractlogger.c
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

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

/* pacman-g2 */
#include "util/fabstractlogger.h"

#include "fstdlib.h"
#include "fstring.h"

FAbstractLogger::FAbstractLogger(unsigned char mask, FLogFunc fn, void *data)
	: m_mask(mask), m_fn(fn), m_data(data)
{ }

FAbstractLogger::~FAbstractLogger()
{ }

void FAbstractLogger::log(unsigned char flag, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vlog(flag, format, ap);
	va_end(ap);
}

void FAbstractLogger::logs(unsigned char flag, const char *s)
{
	if(m_fn == NULL ||
			(flag & m_mask) == 0) {
		return;
	}

	m_fn(flag, s, m_data);
}

void FAbstractLogger::vlog(unsigned char flag, const char *format, va_list ap)
{
	char str[LOG_STR_LEN];

	if(m_fn == NULL ||
			(flag & m_mask) == 0) {
		return;
	}

	vsnprintf(str, LOG_STR_LEN, format, ap);
	logs(flag, f_strtrim(str));
}

/* vim: set ts=2 sw=2 noet: */
