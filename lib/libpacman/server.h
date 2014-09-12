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

#include "pacman.h"

#include "handle.h"
#include "timestamp.h"

/* Servers */
typedef struct __pmserver_t {
	char *protocol;
	char *server;
	char *path;
} pmserver_t;

struct __pmdownload_t {
	const char *source;
	struct timeval dst_begin;
	struct timeval dst_end;
	double dst_avg;
	double dst_eta;
	double dst_rate;
	off64_t dst_resume;
	off64_t dst_size;
	off64_t dst_tell;
};

pmserver_t *_pacman_server_new(const char *url);
void _pacman_server_free(void *data);
int _pacman_downloadfiles(::libpacman::Handle *handle, const FPtrList &servers, const char *localpath, const FStringList &files, int skip);
int _pacman_downloadfiles_forreal(::libpacman::Handle *handle, const FPtrList &servers, const char *localpath,
	const FStringList &files, const libpacman::Timestamp *mtime1, libpacman::Timestamp *mtime2, int skip);

char *_pacman_fetch_pkgurl(::libpacman::Handle *handle, char *target);

extern pacman_trans_cb_download pm_dlcb;

/* progress bar */
extern char *pm_dlfnm;
extern int *pm_dlxfered1;

#endif /* _PACMAN_SERVER_H */

/* vim: set ts=2 sw=2 noet: */
