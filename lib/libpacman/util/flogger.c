/*
 *  flogger.c
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
#include "util/flogger.h"

#include "fstdlib.h"
#include "fstring.h"

FLogger *f_logger_new(unsigned char mask, FLogFunc fn, void *data)
{
	FLogger *logger;

	if ((logger = f_zalloc(sizeof(*logger))) == NULL) {
			return NULL;
	}
	f_logger_init(logger, mask, fn, data);
	return logger;
}

void f_logger_delete(FLogger *logger)
{
	f_logger_fini(logger);
	free(logger);
}

int f_logger_init(FLogger *logger, unsigned char mask, FLogFunc fn, void *data)
{
	if(logger == NULL) {
		return -1;
	}

	logger->mask = mask;
	logger->fn = fn;
	logger->data = data;
	return 0;
}

int f_logger_fini(FLogger *logger)
{
	return 0;
}

void f_logger_log(FLogger *logger, unsigned char flag, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	f_logger_vlog(logger, flag, format, ap);
	va_end(ap);
}

void f_logger_logs(FLogger *logger, unsigned char flag, const char *s)
{
	if(logger == NULL ||
			logger->fn == NULL ||
			(flag & logger->mask) == 0) {
		return;
	}

	logger->fn(flag, s, logger->data);
}

void f_logger_vlog(FLogger *logger, unsigned char flag, const char *format, va_list ap)
{
	char str[LOG_STR_LEN];

	if(logger == NULL ||
			logger->fn == NULL ||
			(flag & logger->mask) == 0) {
		return;
	}

	vsnprintf(str, LOG_STR_LEN, format, ap);
	f_logger_logs(logger, flag, f_strtrim(str));
}

/* vim: set ts=2 sw=2 noet: */
