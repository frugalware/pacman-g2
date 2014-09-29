/*
 *  ffilelogger.c
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

/* pacman-g2 */
#include "util/ffilelogger.h"
#include "util/time.h"

#include "fstdlib.h"

static
void f_filelogger_log(unsigned char flag, const char *message, void *data)
{
	FILE *file = data;
	struct tm *tm;

	tm = f_localtime(NULL);
	fprintf(file, "[%02d/%02d/%02d %02d:%02d] %s\n",
		tm->tm_mon+1, tm->tm_mday, tm->tm_year-100,
		tm->tm_hour, tm->tm_min,
		message);
	fflush(file);
}

FFileLogger::FFileLogger(unsigned char mask, FILE *file)
	: FAbstractLogger(mask, f_filelogger_log, file)
{ }

FFileLogger::~FFileLogger()
{ }

/* vim: set ts=2 sw=2 noet: */
