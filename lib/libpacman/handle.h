/*
 *  handle.h
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
#ifndef _PACMAN_HANDLE_H
#define _PACMAN_HANDLE_H

#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "pacman.h"

typedef struct __pmhandle_t pmhandle_t;

#include "list.h"
#include "db.h"
#include "trans.h"

typedef enum __pmaccess_t {
	PM_ACCESS_RO,
	PM_ACCESS_RW
} pmaccess_t;

struct __pmhandle_t {
	pmaccess_t access;
	uid_t uid;
	pmdb_t *db_local;
	pmlist_t *dbs_sync; /* List of (pmdb_t *) */
	FILE *logfd;
	int lckfd;
	pmtrans_t *trans;
	/* parameters */
	char *root;
	char *dbpath;
	char *cachedir;
	char *logfile;
	char *hooksdir;
	pmlist_t *noupgrade; /* List of strings */
	pmlist_t *noextract; /* List of strings */
	pmlist_t *ignorepkg; /* List of strings */
	pmlist_t *holdpkg; /* List of strings */
	unsigned char usesyslog;
	time_t upgradedelay;
	time_t olddelay;
	/* servers */
	char *proxyhost;
	unsigned short proxyport;
	char *xfercommand;
	unsigned short nopassiveftp;
	unsigned short chomp; /* if eye-candy features should be enabled or not */
	unsigned short maxtries; /* for downloading */
	pmlist_t *needles; /* for searching */
	char *language;
	int *dlremain;
	int *dlhowmany;
	int sysupgrade;
};

extern pmhandle_t *handle;

#define FREEHANDLE(p) do { if (p) { _pacman_handle_free(p); p = NULL; } } while (0)

pmhandle_t *_pacman_handle_new(void);
int _pacman_handle_free(pmhandle_t *handle);
int _pacman_handle_set_option(pmhandle_t *handle, unsigned char val, unsigned long data);
int _pacman_handle_get_option(pmhandle_t *handle, unsigned char val, long *data);

#endif /* _PACMAN_HANDLE_H */

/* vim: set ts=2 sw=2 noet: */
