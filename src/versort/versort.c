/*
 *  versort.c
 * 
 *  Copyright (c) 2007 by Michel Hermier <hermier@frugalware.org>
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

#include <pacman.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_CHUNK 4096

static char *buffer = NULL;
static size_t buffer_nmemb = 0;
static size_t buffer_size = 0;

static char **index = NULL;
static size_t index_nmemb = 0;
static size_t index_size = 0;

static void readfd(int fd)
{
	size_t buffer_read = 0;
	ssize_t buffer_left = -1; // Extra '\0' at the end.

	do {
		buffer_nmemb += buffer_read;
		buffer_left -= buffer_read;

		if (buffer_left <= 0) {
			buffer_size += BUFFER_CHUNK;
			buffer_left += BUFFER_CHUNK;
			buffer = realloc(buffer, buffer_size * sizeof(char));
			if (!buffer)
				exit(EXIT_FAILURE);
		}
	} while((buffer_read = read(fd, &buffer[buffer_nmemb], buffer_left)) > 0);

	if (buffer_read == -1)
	{
		perror("read");
		exit(EXIT_FAILURE);
	}

	buffer[buffer_nmemb] = '\0';
}

#define STATUS_MARK   0
#define STATUS_SEARCH 1

static void makeindex()
{
	size_t tmp_index = 0;
	int status = STATUS_MARK;

	if (buffer_nmemb > 0)
	{
		while(tmp_index != buffer_nmemb)
		{
			if (tmp_index == index_size) {
				index_size += BUFFER_CHUNK;
				index = realloc(index, index_size * sizeof(char *));
				if (!index)
					exit(EXIT_FAILURE);
			}

			switch (status)
			{
			case STATUS_MARK: // Mark a new buffer
				index[index_nmemb] = &buffer[tmp_index];
				++index_nmemb;
				status = STATUS_SEARCH;
				break;
			case STATUS_SEARCH:
				switch(buffer[tmp_index])
				{
				case '\n':
					// Seek for \r if needed
					buffer[tmp_index] = '\0';
					// Intentional fall down
				case '\0':
					status = STATUS_MARK;
				}
				break;
			}
			++tmp_index;
		}
	}
}

static void printindex()
{
	size_t i;

	for (i = 0; i < index_nmemb; i++)
		puts(index[i]);
}

static int vercmpp(const void *p1, const void *p2)
{
	return pacman_pkg_vercmp(*(const char **)p1, *(const char **)p2);
}

int main(int argc, char *argv[])
{

	readfd(0); //stdin

	makeindex();

	qsort(index, index_nmemb, sizeof(char *), vercmpp);

	printindex();

	free(index);
	free(buffer);

	exit(EXIT_SUCCESS);
}
