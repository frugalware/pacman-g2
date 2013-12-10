/*
 *  ftp.c
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

#include "io/ftp.h"

#include "util/time.h"
#include "util.h"

#define PM_FTP_MDTM_FORMAT "%Y%m%d%H%M%S"

size_t _pacman_ftp_strfmdtm(char *s, size_t max, const time_t *time)
{
	return strftime(s, max, PM_FTP_MDTM_FORMAT, f_localtime(time));
}

char *_pacman_ftp_strpmdtm(const char *s, time_t *time)
{
	char *ret;
	struct tm ptimestamp = { 0 };

	ASSERT(time != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	if((ret = strptime(s, PM_FTP_MDTM_FORMAT, &ptimestamp)) != NULL) {
		time_t tmp;
		
		if((tmp = mktime(&ptimestamp)) != PM_TIME_INVALID) {
			*time = tmp;
		} else {
			ret = NULL;
		}
	}
	return ret;
}

/* vim: set ts=2 sw=2 noet: */
