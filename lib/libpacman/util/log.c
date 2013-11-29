/*
 *  log.c
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

#include "config.h"

#include <stdio.h>
#include <syslog.h>
#include <time.h>

/* pacman-g2 */
#include "util/log.h"

#include "util/time.h"
#include "util.h"

/* Internal library log mechanism */
pacman_cb_log pm_logcb     = NULL;
unsigned char pm_logmask = 0;

void _pacman_log(unsigned char flag, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	_pacman_vlog(flag, format, ap);
	va_end(ap);
}

void _pacman_vlog(unsigned char flag, const char *format, va_list ap)
{
	if(pm_logcb == NULL) {
		return;
	}

	if(flag & pm_logmask) {
		char str[LOG_STR_LEN];

		vsnprintf(str, LOG_STR_LEN, format, ap);
		pm_logcb(flag, str);
		pacman_logaction(str);
	}
}

void _pacman_vlogaction(unsigned char usesyslog, FILE *f, const char *format, va_list ap)
{
	char msg[1024];
	int smsg = sizeof(msg)-1;

	vsnprintf(msg, smsg, format, ap);

	if(usesyslog) {
		syslog(LOG_WARNING, "%s", msg);
	}

	if(f) {
		struct tm *tm;

		tm = _pacman_localtime(NULL);
		fprintf(f, "[%02d/%02d/%02d %02d:%02d] %s\n",
				tm->tm_mon+1, tm->tm_mday, tm->tm_year-100,
				tm->tm_hour, tm->tm_min,
				f_strtrim(msg));
		fflush(f);
	}
}

/* vim: set ts=2 sw=2 noet: */
