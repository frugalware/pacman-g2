/*
 *  database_cache.cpp
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

#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* pacman-g2 */
#include "db/localdb_files.h"
#include "util/log.h"
#include "fstring.h"
#include "pacman.h"
#include "util.h"
#include "package.h"
#include "group.h"
#include "db.h"
#include "handle.h"
#include "error.h"

using namespace libpacman;

static
int _pacman_db_clear_grpcache(Database *db);

static
int _pacman_db_load_grpcache(Database *db);

/* Returns a new package cache from db.
 * It frees the cache if it already exists.
 */
static
int _pacman_db_load_pkgcache(Database *db)
{
	package_ptr info;

	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	db->free_pkgcache();

	unsigned int inforeq = 0;
	if (db != db->m_handle->db_local)
		inforeq = INFRQ_DESC | INFRQ_DEPENDS;
	_pacman_log(PM_LOG_DEBUG, _("loading package cache (infolevel=%#x) for repository '%s'"),
	                        inforeq, db->treename());

	db->rewind();
	while((info = db->readpkg(inforeq)) != nullptr) {
		/* add to the collective */
		db->pkgcache.add(info);
	}

	return(0);
}

void Database::free_pkgcache()
{
	if(pkgcache.empty()) {
		return;
	}

	_pacman_log(PM_LOG_DEBUG, _("freeing package cache for repository '%s'"),
	                        treename());

	pkgcache.clear();

	_pacman_db_clear_grpcache(this);
}

libpacman::package_set &Database::get_packages()
{
	if(pkgcache.empty()) {
		_pacman_db_load_pkgcache(this);
	}
	return pkgcache;
}

int Database::add_pkgincache(package_ptr pkg)
{
	if(pkg == nullptr) {
		return(-1);
	}

	_pacman_log(PM_LOG_DEBUG, _("adding entry '%s' in '%s' cache"), pkg->name(), treename());
	pkgcache.add(pkg);

	_pacman_db_clear_grpcache(this);

	return(0);
}

int Database::remove_pkgfromcache(package_ptr pkg)
{
	if(pkg == nullptr) {
		return(-1);
	}

	if(!pkgcache.remove(pkg)) {
		/* package not found */
		return(-1);
	}
	_pacman_log(PM_LOG_DEBUG, _("removing entry '%s' from '%s' cache"), pkg->name(), treename());
	_pacman_db_clear_grpcache(this);
	return 0;
}

static
Group *_pacman_db_get_grpfromlist(const FList<Group *> &list, const char *target)
{
	if(_pacman_strempty(target)) {
		return(NULL);
	}

	for(auto i = list.begin(), end = list.end(); i != end; ++i) {
		Group *info = *i;

		if(strcmp(info->name, target) == 0) {
			return(info);
		}
	}

	return(NULL);
}

/* Returns a new group cache from db.
 */
int _pacman_db_load_grpcache(Database *db)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	auto &cache = db->get_packages();

	_pacman_log(PM_LOG_DEBUG, _("loading group cache for repository '%s'"), db->treename());

	for(auto it = cache.begin(), end = cache.end(); it != end; ++it) {
		package_ptr pkg = *it;

		if(!(pkg->flags & INFRQ_DESC)) {
			pkg->read(INFRQ_DESC);
		}

		auto &groups = pkg->groups();
		for(auto git = groups.begin(), git_end = groups.end(); git != git_end; ++git) {
			const char *grp_name = *git;

			Group *grp = _pacman_db_get_grpfromlist(db->grpcache, grp_name);

			if(grp == NULL) {
				grp = new Group(grp_name);
				db->grpcache.add(grp);
			}
			if(!grp->packages.contains(pkg->name())) {
				grp->packages.add(pkg->name());
			}
		}
	}

	return(0);
}

int _pacman_db_clear_grpcache(Database *db)
{
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	db->grpcache.clear();
	return 0;
}

libpacman::group_set &Database::get_groups()
{
	if(grpcache.empty()) {
		_pacman_db_load_grpcache(this);
	}
	return grpcache;
}

Group *Database::find_group(const char *target)
{
	return _pacman_db_get_grpfromlist(get_groups(), target);
}

/* vim: set ts=2 sw=2 noet: */
