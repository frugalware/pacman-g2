/*
 *  package/packagecache.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2007 by Miklos Vajna <vmiklos@frugalware.org>
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

#include <dirent.h>

/* pacman-g2 */
#include "package/packagecache.h"

#include "package.h"
#include "handle.h"

using namespace libpacman;

int _pacman_packagecache_clean(int level)
{
	char dirpath[PATH_MAX];

	snprintf(dirpath, PATH_MAX, "%s%s", handle->root, handle->cachedir);

	if(!level) {
		/* incomplete cleanup: we keep latest packages and partial downloads */
		DIR *dir;
		struct dirent *ent;
		FStringList cache, clean;

		dir = opendir(dirpath);
		if(dir == NULL) {
			RET_ERR(PM_ERR_NO_CACHE_ACCESS, -1);
		}
		rewinddir(dir);
		while((ent = readdir(dir)) != NULL) {
			if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
				continue;
			}
			cache.add(ent->d_name);
		}
		closedir(dir);

		for(auto i = cache.begin(), end = cache.end(); i != end; ++i) {
			const char *str = *i;
			char name[PKG_NAME_LEN], version[PKG_VERSION_LEN];

			if(strstr(str, PM_EXT_PKG) == NULL) {
				clean.add(str);
				continue;
			}
			/* we keep partially downloaded files */
			if(strstr(str, PM_EXT_PKG ".part")) {
				continue;
			}
			if(!Package::splitname(str, name, version, 1)) {
				clean.add(str);
				continue;
			}
			for(FPtrList::iterator j = i.next(); j != end; ++j) {
				const char *s = *j;
				char n[PKG_NAME_LEN], v[PKG_VERSION_LEN];

				if(strstr(s, PM_EXT_PKG) == NULL) {
					continue;
				}
				if(strstr(s, PM_EXT_PKG ".part")) {
					continue;
				}
				if(!Package::splitname(s, n, v, 1)) {
					continue;
				}
				if(!strcmp(name, n)) {
					const char *ptr = (pacman_pkg_vercmp(version, v) < 0) ? str : s;
					if(!_pacman_list_is_strin(ptr, &clean)) {
						clean.add(ptr);
					}
				}
			}
		}
		for(auto i = clean.begin(), end = clean.end(); i != end; ++i) {
			char path[PATH_MAX];

			snprintf(path, PATH_MAX, "%s/%s", dirpath, *i);
			unlink(path);
		}
	} else {
		/* full cleanup */

		if(_pacman_rmrf(dirpath)) {
			RET_ERR(PM_ERR_CANT_REMOVE_CACHE, -1);
		}

		if(_pacman_makepath(dirpath)) {
			RET_ERR(PM_ERR_CANT_CREATE_CACHE, -1);
		}
	}

	return(0);
}

/* vim: set ts=2 sw=2 noet: */
