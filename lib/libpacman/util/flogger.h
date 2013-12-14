/*
 *  flogger.h
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
#ifndef F_LOGGER_H
#define F_LOGGER_H

#include "pacman.h"

#define LOG_STR_LEN 256

typedef struct FLogger FLogger;
typedef void (*FLogFunc)(unsigned char flag, const char *message, void *data);

struct FLogger {
	unsigned char mask;
	FLogFunc fn;
	void *data;
};

FLogger *f_logger_new(unsigned char mask, FLogFunc fn, void *data);
void f_logger_delete(FLogger *logger);

int f_logger_init(FLogger *logger, unsigned char mask, FLogFunc fn, void *data);
int f_logger_fini(FLogger *logger);

void f_logger_log(FLogger *logger, unsigned char flag, const char *format, ...);
void f_logger_vlog(FLogger *logger, unsigned char flag, const char *format, va_list ap);

#endif /* F_LOGGER_H */

/* vim: set ts=2 sw=2 noet: */
