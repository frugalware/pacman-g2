/*
 *  ffilelock.c
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

#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

#include "io/ffilelock.h"

#include "fstdlib.h"
#include "fstring.h"
#include "util.h"

struct FFileLock {
	int fd, flags;
	char pathname[PATH_MAX];
};

FFileLock *f_filelock_aquire(const char *pathname, int flags)
{
	FFileLock *filelock;

	ASSERT(!f_strempty(pathname), RET_ERR(PM_ERR_WRONG_ARGS, NULL));
	ASSERT((filelock = f_zalloc(sizeof(*filelock))) != NULL, return NULL);

	filelock->flags = flags;
	f_canonicalize_path(pathname, filelock->pathname, PATH_MAX, F_PATH_NOCHECK);
	if((flags & F_FILELOCK_CREATE_HOLD_DIR) && 
			_pacman_makepath(f_dirname(filelock->pathname)) != 0) {
		goto error;
	}
	if((filelock->fd = open(pathname, O_CREAT | O_RDWR, 0)) == -1) {
		goto error;
	}
	if(flock(filelock->fd, flags & F_FILELOCK_EXCLUSIVE ? LOCK_EX : LOCK_SH) == -1) {
		goto error;
	}
	return filelock;

error:
	/* Do not unlink if we down't own it */
	filelock->flags &= ~F_FILELOCK_UNLINK_ON_CLOSE;
	f_filelock_release(filelock);
	return NULL;
}

int f_filelock_release(FFileLock *filelock)
{
	ASSERT(filelock != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	if(filelock->flags & F_FILELOCK_UNLINK_ON_CLOSE) {
		unlink(filelock->pathname);
	}
	if(filelock->fd != -1) {
		close(filelock->fd);
	}
	free(filelock);
	return 0;

error:
	_pacman_log(PM_LOG_WARNING, _("could not remove lock file %s"), filelock->pathname);
	return -1;
}

/* vim: set ts=2 sw=2 noet: */
