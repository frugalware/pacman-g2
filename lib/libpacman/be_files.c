/*
 *  be_files.c
 *
 *  Copyright (c) 2006 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2006-2008 by Miklos Vajna <vmiklos@frugalware.org>
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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#ifdef __sun__
#include <strings.h>
#endif
#include <sys/stat.h>
#include <dirent.h>
#include <libintl.h>
#include <locale.h>
#ifdef CYGWIN
#include <limits.h> /* PATH_MAX */
#endif

/* pacman-g2 */

#include "db/localdb.h"
#include "db/localdb_files.h"
#include "db/syncdb.h"
#include "io/archive.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "util.h"
#include "db.h"
#include "package.h"
#include "pacman.h"
#include "error.h"
#include "handle.h"


/* vim: set ts=2 sw=2 noet: */
