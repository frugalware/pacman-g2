/*
 *  server.h
 *
 *  Copyright (c) 2006 by Miklos Vajna <vmiklos@frugalware.org>
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
#ifndef _PACMAN_SERVER_H
#define _PACMAN_SERVER_H

#include <time.h>

#include "pacman.h"

#define FREESERVER(p) \
do { \
	if(p) { \
		_pacman_server_free(p); \
		p = NULL; \
	} \
} while(0)

#define FREELISTSERVERS(p) _FREELIST(p, _pacman_server_free)

/* Servers */
typedef struct __pmserver_t {
	char *protocol;
	char *server;
	char *path;
} pmserver_t;

struct __pmdownloadstate_t {
	// FIXME: change int to off_t when the download backend will permit that.
	struct timeval dst_begin;
	struct timeval dst_end;
	double dst_eta;
	double dst_rate;
	int dst_resume;
	int dst_size;
	int dst_tell;
};

pmserver_t *_pacman_server_new(char *url);
void _pacman_server_free(void *data);
int _pacman_downloadfiles(pmlist_t *servers, const char *localpath, pmlist_t *files, int skip);
int _pacman_downloadfiles_forreal(pmlist_t *servers, const char *localpath,
	pmlist_t *files, const time_t *mtime1, time_t *mtime2, int skip);

char *_pacman_fetch_pkgurl(char *target);

extern pacman_trans_cb_download pm_dlcb;

/* progress bar */
extern char *pm_dlfnm;
extern struct timeval *pm_dlt;
extern float *pm_dlrate;
extern int *pm_dlxfered1;
extern unsigned int *pm_dleta_h, *pm_dleta_m, *pm_dleta_s;

#endif /* _PACMAN_SERVER_H */

/* vim: set ts=2 sw=2 noet: */
