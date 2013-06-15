/*
 *  fstream.h
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
#ifndef F_BYTESTREAM_H
#define F_BYTESTREAM_H

#include <fstddef.h>
#include <fobject.h>

typedef struct FByteStream FByteStream;

struct FByteStreamOperations {
	FObjectOperations base;
	int (*close) (FByteStream *bytestream);

	/* MARK operations */
	int (*mark) (FByteStream *bytestream, size_t readaheadlimit);
	int (*reset_mark) (FByteStream *bytestream);

	/* READ operations */
	int (*read_buffer) (FByteStream *bytestream, void *bytes, size_t size);

	/* SEEK operations */
//	void (*seek) (FByteStream *bytestream);

	/* SKIP operations */
//	int (*skip) (FByteStream *bytestream, size_t size);

	/* WRITE operations */
	int (*write_buffer) (FByteStream *bytestream, const void *bytes, size_t size);
};

struct FByteStream {
	FObject base;
};

void f_bytestream_init (FByteStream *bytestream);
void f_bytestream_fini (FByteStream *bytestream);

FByteStream *f_bytestream_new (void);
void f_bytestream_delete (FByteStream *bytestream);

int f_bytestream_close(FByteStream *bytestream);

/* MARK operations */
int f_bytestream_mark (FByteStream *bytestream, size_t readaheadlimit);
int f_bytestream_reset_mark (FByteStream *bytestream);

/* READ operations */
int f_bytestream_read_buffer (FByteStream *bytestream, void *bytes, size_t size);

/* SEEK operations */
//void f_bytestream_seek (FByteStream *bytestream);

/* SKIP operations */
int f_bytestream_skip (FByteStream *bytestream, size_t size);

/* WRITE operations */
int f_bytestream_write_buffer (FByteStream *bytestream, const void *bytes, size_t size);

#endif /* F_BYTESTREAM_H */

/* vim: set ts=2 sw=2 noet: */
