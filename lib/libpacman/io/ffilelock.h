/*
 *  ffilelock.h
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
#ifndef _F_FILELOCK_H
#define _F_FILELOCK_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FFileLock FFileLock;

#define F_FILELOCK_CREATE_HOLD_DIR (1<<0)
#define F_FILELOCK_EXCLUSIVE       (1<<1)
#define F_FILELOCK_UNLINK_ON_CLOSE (1<<2)

FFileLock *f_filelock_aquire(const char *pathname, int flags);
int f_filelock_release(FFileLock *filelock);

#ifdef __cplusplus
}
#endif
#endif /* _F_FILELOCK_H */

/* vim: set ts=2 sw=2 noet: */
