/*
 *  archive_file.h
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
#ifndef _PACMAN_ARCHIVE_FILE_H
#define _PACMAN_ARCHIVE_FILE_H

#include <archive.h>
#include <archive_entry.h>

#ifdef __cplusplus
extern "C" {
#endif

enum __pmfiletype_t {
    PM_FILE_UNKNOWN = -1,
    PM_FILE_DB = 0,
    PM_FILE_PKG = 1,
};

typedef enum __pmfiletype_t pmfiletype_t;

struct archive *_pacman_archive_read_open_all_file(const char *file);
FILE *_pacman_archive_read_fropen(struct archive *a);
int checkFile(pmfiletype_t fileType, char *file);
int checkXZ(char *file);

#ifdef __cplusplus
}
#endif
#endif /* _PACMAN_ARCHIVE_FILE_H */

/* vim: set ts=2 sw=2 noet: */
