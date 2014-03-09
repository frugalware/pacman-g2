/*
 *  archive.c
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>

#include "archive.h"

#include "fstring.h"
#include "util.h"

struct archive *_pacman_archive_read_open_all_file(const char *file)
{
	struct archive *archive;

	ASSERT(!_pacman_strempty(file), RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	if((archive = archive_read_new ()) == NULL)
		RET_ERR(PM_ERR_LIBARCHIVE_ERROR, NULL);

	archive_read_support_compression_all (archive);
	archive_read_support_format_all (archive);

	if(archive_read_open_file(archive, file, PM_DEFAULT_BYTES_PER_BLOCK) != ARCHIVE_OK) {
		archive_read_finish(archive);
		RET_ERR(PM_ERR_PKG_OPEN, NULL);
	}
	return archive;
}

static const
cookie_io_functions_t _pacman_archive_io_functions =
{
	.read = (cookie_read_function_t *)&archive_read_data
};

FILE *_pacman_archive_read_fropen(struct archive *a)
{
	ASSERT(a != NULL, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return fopencookie(a, "r", _pacman_archive_io_functions);
}

int checkFile(pmfiletype_t fileType, char *file)
{
    switch(fileType) {
    case PM_FILE_DB: return checkXZ(file);
    case PM_FILE_PKG: return checkXZ(file);
    default: RET_ERR(PM_FILE_INVALID, 0);
    }
}

int checkXZ(char *file)
{
    FILE *fp;
    fp = fopen(file, "rb");
    if (fp == NULL) {
        RET_ERR(PM_FILE_INVALID, 0);
    }
    uint8_t magicCh;
    const uint8_t XZ_HEADER_MAGIC[6] = { 0xFD, '7', 'z', 'X', 'Z', 0x00 };
    for(int i=0;i<6;i++) {
        fscanf(fp, "%c", &magicCh);
        if(magicCh != XZ_HEADER_MAGIC[i]) {
            fclose(fp);
            RET_ERR(PM_FILE_INVALID, 0);
        }
    }
    fclose(fp);
    return 1;
}

/* vim: set ts=2 sw=2 noet: */
