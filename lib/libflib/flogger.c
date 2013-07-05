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

#include <assert.h>
#include <stdio.h>

#include "flogger.h"

#ifdef F_DEBUG
#define F_LOGGER_DEBUG
#endif

static
FLogger *f_default_logger = NULL;

void f_logger_init (FLogger *logger, FLoggerOperations *loggeroperations) {
	assert (logger != NULL);
	assert (loggeroperations != NULL);

	f_object_init (&logger->as_FObject, &loggeroperations->as_FObjectOperations);
	logger->flag_mask = ~0;
}

void f_logger_fini (FLogger *logger) {
	assert (logger != NULL);

	f_object_fini (&logger->as_FObject);
}

void f_logger_log (FLogger *logger, unsigned flag, const char *format, ...) {
	va_list ap;

	assert (logger != NULL);

	va_start (ap, format);
	f_logger_vlog (logger, flag, format, ap);
	va_end (ap);
}

void f_logger_vlog (FLogger *logger, unsigned flag, const char *format, va_list ap) {
	assert (logger != NULL);

	if ((logger->flag_mask & flag) != 0) {
		FLoggerOperations *loggeroperations = logger->as_FObject.operations;

#ifdef F_LOGGER_DEBUG
		vfprintf (stderr, format, ap);
#endif
		loggeroperations->vlog (logger, flag, format, ap);
	}
}

void f_log (unsigned flag, const char *format, ...) {
	va_list ap;

	va_start (ap, format);
	f_vlog (flag, format, ap);
	va_end (ap);
}

void f_vlog (unsigned flag, const char *format, va_list ap) {
	if (f_default_logger != NULL) {
		f_logger_vlog (f_default_logger, flag, format, ap);
	}
}

/* vim: set ts=2 sw=2 noet: */
