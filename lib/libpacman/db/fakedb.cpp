/*
 *  fakedb.c
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

#include "config.h"

/* pacman-g2 */
#include "fakedb.h"

#include "package.h"
#include "util.h"

#include <time.h>

using namespace libpacman;

Package *_pacman_fakedb_pkg_new(const char *name)
{
	char *ptr, *p;
	char *str = NULL;
	Package *dummy = NULL;

	dummy = new Package(NULL, NULL);
	if(dummy == NULL) {
		RET_ERR(PM_ERR_MEMORY, NULL);
	}

	/* Format: field1=value1|field2=value2|...
	 * Valid fields are "name", "version" and "depend"
	 */
	str = strdup(name);
	ptr = str;
	while((p = strsep(&ptr, "|")) != NULL) {
		char *q;
		if(p[0] == 0) {
			continue;
		}
		q = strchr(p, '=');
		if(q == NULL) { /* not a valid token */
			continue;
		}
		if(strncmp("name", p, q-p) == 0) {
			STRNCPY(dummy->m_name, q+1, PKG_NAME_LEN);
		} else if(strncmp("version", p, q-p) == 0) {
			STRNCPY(dummy->m_version, q+1, PKG_VERSION_LEN);
		} else if(strncmp("depend", p, q-p) == 0) {
			dummy->m_depends.add(q+1);
		} else {
			_pacman_log(PM_LOG_ERROR, _("could not parse token %s"), p);
		}
	}
	free(str);
	if(dummy->m_name[0] == 0 || dummy->m_version[0] == 0) {
		RET_ERR(PM_ERR_PKG_INVALID_NAME, NULL);
	}
	return dummy;
}

/* vim: set ts=2 sw=2 noet: */
