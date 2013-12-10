/*
 *  util.h
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2006 by David Kimpe <dnaku@frugalware.org>
 *  Copyright (c) 2005, 2006 by Miklos Vajna <vmiklos@frugalware.org>
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
#ifndef _PACMAN_UTIL_H
#define _PACMAN_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__OpenBSD__)
#include "/usr/local/include/archive.h"
#include "/usr/local/include/archive_entry.h"
#else
#include <archive.h>
#include <archive_entry.h>
#endif
#include <libintl.h>

#include "error.h"

#define FREE(p) do { if (p) { free(p); p = NULL; } } while(0)

#define ASSERT(cond, action) do { if(!(cond)) { action; } } while(0)

#define STRNCPY(s1, s2, len) do { \
	strncpy(s1, s2, (len)-1); \
	s1[(len)-1] = 0; \
} while(0)

#define ARCHIVE_EXTRACT_FLAGS ARCHIVE_EXTRACT_OWNER | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_TIME

#ifdef ENABLE_NLS
#define _(str) dgettext ("libpacman", str)
#else
#define _(s) s
#endif

#define STARTSTR "START "
#define DONESTR "DONE "

#include "fstdlib.h"
#include "fstring.h"

char *_pacman_basename(const char *path);
int _pacman_makepath(char *path);
int _pacman_copyfile(char *src, char *dest);
int _pacman_lckmk(char *file);
int _pacman_lckrm(char *file);
int _pacman_unpack(const char *archive, const char *prefix, const char *fn);
int _pacman_rmrf(char *path);
int _pacman_ldconfig(char *root);
#ifdef _PACMAN_TRANS_H
int _pacman_chroot_system(const char *cmdline, pmtrans_t *trans);
int _pacman_runscriptlet(char *util, char *installfn, const char *script, char *ver, char *oldver, pmtrans_t *trans);
int _pacman_runhook(const char *hookname, pmtrans_t *trans);
#ifndef __sun__
int _pacman_check_freespace(pmtrans_t *trans, pmlist_t **data);
#endif
#endif
int _pacman_reg_match(const char *string, const char *pattern);
#ifdef __sun__
char* strsep(char** str, const char* delims);
char* mkdtemp(char *template);
#endif

#define _pacman_strempty f_strempty
#define _pacman_malloc f_malloc
#define _pacman_zalloc f_zalloc

#endif /* _PACMAN_UTIL_H */

/* vim: set ts=2 sw=2 noet: */
