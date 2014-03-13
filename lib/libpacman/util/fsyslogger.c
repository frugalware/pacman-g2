/*
 *  fsyslogger.c
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
#include <syslog.h>

/* pacman-g2 */
#include "util/fsyslogger.h"

#include "fstdlib.h"

struct FSysLogger
{
	FLogger base;
};

static
void f_syslogger_log(unsigned char flag, const char *message, void *data)
{
	syslog(LOG_WARNING, "%s", message);
}

FSysLogger *f_syslogger_new(unsigned char mask)
{
	FSysLogger *syslogger;

	if ((syslogger = f_zalloc(sizeof(*syslogger))) == NULL) {
			return NULL;
	}
	f_logger_init(&syslogger->base, mask, f_syslogger_log, NULL);
	return syslogger;
}

void f_syslogger_delete(FSysLogger *syslogger)
{
	f_logger_fini(&syslogger->base);
	free(syslogger);
}

/* vim: set ts=2 sw=2 noet: */