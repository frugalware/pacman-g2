/*
 *  fstring.c
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

#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <unistd.h>

#include "fstdlib.h"

static inline
long f_eloop_threshold(void)
{
#ifdef _SC_SYMLOOP_MAX
	return sysconf(_SC_SYMLOOP_MAX);
#elif defined SYMLOOP_MAX
	return SYMLOOP_MAX;
#else
	return 8;
#endif
}

static inline
void f_set_errno(error_t error)
{
	errno = error;
}

/* Based on glibc __realpath from:
 * https://sourceware.org/git/?p=glibc.git;a=blob;f=stdlib/canonicalize.c;h=784c978435023ed774543f51b413be267dd95d85;hb=HEAD
 */
char *f_canonicalize_path(const char *name, char *resolved, size_t size, int flags)
{
	char *rpath, *dest, *extra_buf = NULL;
	const char *start, *end, *rpath_limit;
	long int path_max;
	int num_links = 0;

	if(name == NULL) {
		/* As per Single Unix Specification V2 we must return an error if
		   either parameter is a null pointer.  We extend this to allow
		   the RESOLVED parameter to be NULL in case the we are expected to
		   allocate the room for the return value.  */
		f_set_errno(EINVAL);
		return NULL;
	}

	if(name[0] == '\0') {
		/* As per Single Unix Specification V2 we must return an error if
		   the name argument points to an empty string.  */
		f_set_errno(ENOENT);
		return NULL;
	}

	if(size != 0) {
		path_max = size;
	} else {
#ifdef PATH_MAX
		path_max = PATH_MAX;
#else
		path_max = pathconf(name, _PC_PATH_MAX);
		if(path_max <= 0) {
			path_max = 1024;
		}
#endif
	}

	if(resolved == NULL) {
		rpath = malloc(path_max);
		if(rpath == NULL) {
			return NULL;
		}
	} else {
		rpath = resolved;
	}
	rpath_limit = rpath + path_max;

	if(flags & F_PATH_RELATIVE) {
		rpath[0] = '.';
		rpath[1] = '/';
		dest = rpath + 2;
	} else {
		if(name[0] != '/') {
			if(!getcwd(rpath, path_max)) {
				rpath[0] = '\0';
				goto error;
			}
			dest = rawmemchr(rpath, '\0');
		}  else {
			rpath[0] = '/';
			dest = rpath + 1;
		}
	}

	for(start = end = name; *start; start = end) {
		/* Skip sequence of multiple path-separators. */
		while(*start == '/') {
			++start;
		}

		/* Find end of path component. */
		for(end = start; *end && *end != '/'; ++end) {
			/* Nothing. */;
		}

		if(end - start == 0) {
			break;
		} else if(end - start == 1 && start[0] == '.') {
			continue;
		}
		if(end - start == 2 && start[0] == '.' && start[1] == '.') {
#if 0
			if (dest > rpath + 1) {
				while ((--dest)[-1] != '/') {
					/* Nothing. */;
				}
			}
#endif
			char *prev = dest;

			while(prev > rpath && prev[-1] != '/') {
				--prev;
			}
			if(!(dest - prev == 2 && prev[0] == '.' && prev[1] == '.')) {
				/* Consume previous component if it is not allready ".." */
				dest = prev;
				continue;
			}
		}
		/* Append path component. */
		if(dest[-1] != '/')
			*dest++ = '/';

		if(dest + (end - start) >= rpath_limit) {
			ptrdiff_t dest_offset = dest - rpath;
			char *new_rpath;
			size_t new_size;

			if(resolved != NULL || size != 0) {
				f_set_errno(ENAMETOOLONG);
				if(dest > rpath + 1)
					dest--;
				*dest = '\0';
				goto error;
			}
			new_size = rpath_limit - rpath;
			if(end - start + 1 > path_max)
				new_size += end - start + 1;
			else
				new_size += path_max;
			new_rpath = (char *) realloc(rpath, new_size);
			if(new_rpath == NULL)
				goto error;
			rpath = new_rpath;
			rpath_limit = rpath + new_size;
				dest = rpath + dest_offset;
		}

		dest = __mempcpy(dest, start, end - start);
		*dest = '\0';

		if(!(flags & F_PATH_NOCHECK)) {
			struct stat64 st;

			if(__lxstat64(_STAT_VER, rpath, &st) < 0)
				goto error;

			if(S_ISLNK(st.st_mode)) {
				char *buf = alloca(path_max);
				size_t len;
				ssize_t n;

				if(++num_links > f_eloop_threshold()) {
					f_set_errno(ELOOP);
					goto error;
				}

				n = readlink(rpath, buf, path_max - 1);
				if(n < 0)
					goto error;
				buf[n] = '\0';

				if(!extra_buf)
					extra_buf = alloca(path_max);

				len = strlen(end);
				if((long int)(n + len) >= path_max) {
					f_set_errno(ENAMETOOLONG);
					goto error;
				}

				/* Careful here, end may be a pointer into extra_buf... */
				memmove(&extra_buf[n], end, len + 1);
				name = end = memcpy(extra_buf, buf, n);

				if(buf[0] == '/') {
					/* It's an absolute symlink */
					dest = rpath + 1;
				} else {
					/* Back up to previous component, ignore if at root already: */
					if(dest > rpath + 1) {
						while((--dest)[-1] != '/') {
							/* Nothing. */
						}
					}
				}
			} else if(!S_ISDIR(st.st_mode) && *end != '\0') {
				f_set_errno(ENOTDIR);
				goto error;
			}
		}
	}
	if(dest > rpath + 1 && dest[-1] == '/')
		--dest;
	*dest = '\0';

	assert(resolved == NULL || resolved == rpath);
	return rpath;

error:
	assert(resolved == NULL || resolved == rpath);
	if(resolved == NULL)
	free(rpath);
	return NULL;
}

char *f_canonicalize_file_name(const char *name, int flags)
{
	return f_canonicalize_path(name, NULL, 0, flags);
}

/* vim: set ts=2 sw=2 noet: */
