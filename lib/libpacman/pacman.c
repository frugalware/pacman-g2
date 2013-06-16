/*
 *  pacman.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2005, 2006. 2007 by Miklos Vajna <vmiklos@frugalware.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <limits.h> /* PATH_MAX */
#include <stdarg.h>

#include <fstring.h>

/* pacman-g2 */
#include "pacman.h"

#include "log.h"
#include "error.h"
#include "deps.h"
#include "versioncmp.h"
#include "md5.h"
#include "sha1.h"
#include "package.h"
#include "group.h"
#include "util.h"
#include "db.h"
#include "cache.h"
#include "conflict.h"
#include "backup.h"
#include "sync.h"
#include "handle.h"
#include "provide.h"
#include "server.h"

#define min(X, Y)  ((X) < (Y) ? (X) : (Y))

/* Globals */
pmhandle_t *handle = NULL;
enum __pmerrno_t pm_errno;

/** @defgroup pacman_interface Interface Functions
 * @brief Function to initialize and release libpacman
 * @{
 */

/** Initializes the library.  This must be called before any other
 * functions are called.
 * @param root the full path of the root we'll be installing to (usually /)
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_initialize(const char *root)
{
	char str[PATH_MAX];

	ASSERT(handle == NULL, RET_ERR(PM_ERR_HANDLE_NOT_NULL, -1));

	handle = _pacman_handle_new();
	if(handle == NULL) {
		RET_ERR(PM_ERR_MEMORY, -1);
	}

	STRNCPY(str, (root) ? root : PM_ROOT, PATH_MAX);
	/* add a trailing '/' if there isn't one */
	if(str[strlen(str)-1] != '/') {
		strcat(str, "/");
	}
	handle->root = strdup(str);

	return(0);
}

/** Release the library.  This should be the last libpacman call you make.
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_release(void)
{
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	/* free the transaction if there is any */
	if(handle->trans) {
		pacman_trans_release();
	}

	/* close local database */
	if(handle->db_local) {
		/* db_unregister() will set handle->db_local to NULL */
		pacman_db_unregister(handle->db_local);
	}
	/* and also sync ones */
	while(handle->dbs_sync) {
		/* db_unregister() will also update the handle->dbs_sync list */
		pacman_db_unregister(handle->dbs_sync->data);
	}

	_pacman_handle_lock_release (handle);

	FREEHANDLE(handle);

	return(0);
}
/** @} */

/** @defgroup pacman_options Library Options
 * @brief Functions to set and get libpacman options
 * @{
 */

/** Set a library option.
 * @param parm the name of the parameter
 * @param data the value of the parameter
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_set_option(unsigned char parm, unsigned long data)
{
	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	return(_pacman_handle_set_option(handle, parm, data));
}

/** Get the value of a library option.
 * @param parm the parameter to get
 * @param data pointer argument to get the value in
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_get_option(unsigned char parm, long *data)
{
	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));
	ASSERT(data != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	return(_pacman_handle_get_option(handle, parm, data));
}
/** @} */

/** @defgroup pacman_databases Database Functions
 * @brief Frunctions to query and manipulate the database of libpacman
 * @{
 */

/** Register a package database
 * @param treename the name of the repository
 * @return a pmdb_t* on success (the value), NULL on error
 */
pmdb_t *pacman_db_register(const char *treename)
{
	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, NULL));
	ASSERT(treename != NULL && strlen(treename) != 0, RET_ERR(PM_ERR_WRONG_ARGS, NULL));
	/* Do not register a database if a transaction is on-going */
	ASSERT(handle->trans == NULL, RET_ERR(PM_ERR_TRANS_NOT_NULL, NULL));

	return(_pacman_db_register(treename));
}

/** Unregister a package database
 * @param db pointer to the package database to unregister
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_db_unregister(pmdb_t *db)
{
	int found = 0;

	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));
	ASSERT(db != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	/* Do not unregister a database if a transaction is on-going */
	ASSERT(handle->trans == NULL, RET_ERR(PM_ERR_TRANS_NOT_NULL, -1));

	if(db == handle->db_local) {
		handle->db_local = NULL;
		found = 1;
	} else {
		FPtrListItem *item = f_ptrlist_find (handle->dbs_sync, db);
		if (item != f_ptrlist_end (handle->dbs_sync)) {
			f_ptrlistitem_delete (item, NULL, NULL, &handle->dbs_sync);
			found = 1;
		}
	}

	if(!found) {
		RET_ERR(PM_ERR_DB_NOT_FOUND, -1);
	}

	_pacman_db_free(db);

	return(0);
}

/** Get information about a database.
 * @param db database pointer
 * @param parm name of the info to get
 * @return a void* on success (the value), NULL on error
 */
void *pacman_db_getinfo(PM_DB *db, unsigned char parm)
{
	void *data = NULL;
	char path[PATH_MAX];
	pmserver_t *server;

	/* Sanity checks */
	ASSERT(handle != NULL, return(NULL));
	ASSERT(db != NULL, return(NULL));

	switch(parm) {
		case PM_DB_TREENAME:   data = db->treename; break;
		case PM_DB_FIRSTSERVER:
			server = (pmserver_t*)db->servers->data;
			if(!strcmp(server->protocol, "file")) {
				snprintf(path, PATH_MAX, "%s://%s", server->protocol, server->path);
			} else {
				snprintf(path, PATH_MAX, "%s://%s%s", server->protocol,
						server->server, server->path);
			}
			data = strdup(path);
		break;
		default:
			free(data);
			data = NULL;
	}

	return(data);
}

/** Set the serverlist of a database.
 * @param db database pointer
 * @param url url of the server
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_db_setserver(pmdb_t *db, char *url)
{
	int found = 0;

	/* Sanity checks */
	ASSERT(db != NULL, RET_ERR(PM_ERR_DB_NULL, -1));

	if(strcmp(db->treename, "local") == 0) {
		if(handle->db_local != NULL) {
			found = 1;
		}
	} else {
		pmlist_t *i;
		f_foreach (i, handle->dbs_sync) {
			pmdb_t *sdb = i->data;
			if(strcmp(db->treename, sdb->treename) == 0) {
				found = 1;
				break;
			}
		}
	}
	if(!found) {
		RET_ERR(PM_ERR_DB_NOT_FOUND, -1);
	}

	if(url && strlen(url)) {
		pmserver_t *server;
		if((server = _pacman_server_new(url)) == NULL) {
			/* pm_errno is set by _pacman_server_new */
			return(-1);
		}
		db->servers = _pacman_list_add(db->servers, server);
		_pacman_log(PM_LOG_FLOW2, _("adding new server to database '%s': protocol '%s', server '%s', path '%s'"),
				db->treename, server->protocol, server->server, server->path);
	} else {
		FREELIST(db->servers);
		_pacman_log(PM_LOG_FLOW2, _("serverlist flushed for '%s'"), db->treename);
	}

	return(0);
}

/** Update a package database
 * @param force if true, then forces the update, otherwise update only in case
 * the database isn't up to date
 * @param db pointer to the package database to update
 * @return 0 on success, -1 on error (pm_errno is set accordingly), 1 if up
 * to date
 */
int pacman_db_update(int force, PM_DB *db)
{
	char path[PATH_MAX], dirpath[PATH_MAX];
	pmlist_t *files = NULL;
	char newmtime[16] = "";
	char lastupdate[16] = "";
	int ret, updated=0, status=0;

	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));
	ASSERT(db != NULL && db != handle->db_local, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	/* Do not update a database if a transaction is on-going */
	ASSERT(handle->trans == NULL, RET_ERR(PM_ERR_TRANS_NOT_NULL, -1));

	/* lock db */
	if(_pacman_handle_lock_acquire (handle) != 0) {
		return -1;
	}

	if(!f_ptrlist_find (handle->dbs_sync, db)) {
		RET_ERR(PM_ERR_DB_NOT_FOUND, -1);
	}

	if(!force) {
		/* get the lastupdate time */
		_pacman_db_getlastupdate(db, lastupdate);
		if(strlen(lastupdate) == 0) {
			_pacman_log(PM_LOG_DEBUG, _("failed to get lastupdate time for %s (no big deal)\n"), db->treename);
		}
	}

	/* build a one-element list */
	snprintf(path, PATH_MAX, "%s" PM_EXT_DB, db->treename);
	files = _pacman_list_add(files, strdup(path));

	snprintf(path, PATH_MAX, "%s%s", handle->root, handle->dbpath);

	ret = _pacman_downloadfiles_forreal(db->servers, path, files, lastupdate, newmtime, 0);
	FREELIST(files);
	if(ret != 0) {
		if(ret == -1) {
			_pacman_log(PM_LOG_DEBUG, _("failed to sync db: %s [%d]\n"),  pacman_strerror(ret), ret);
			pm_errno = PM_ERR_DB_SYNC;
		}
		status = 1;
		goto rmlck;
	} else {
		if(strlen(newmtime)) {
			_pacman_log(PM_LOG_DEBUG, _("sync: new mtime for %s: %s\n"), db->treename, newmtime);
			updated = 1;
		}
		snprintf(dirpath, PATH_MAX, "%s%s/%s", handle->root, handle->dbpath, db->treename);
		snprintf(path, PATH_MAX, "%s%s/%s" PM_EXT_DB, handle->root, handle->dbpath, db->treename);

		/* remove the old dir */
		_pacman_rmrf(dirpath);

		/* Cache needs to be rebuild */
		_pacman_db_free_pkgcache(db);

		if(updated) {
			_pacman_db_setlastupdate(db, newmtime);
		}
	}

rmlck:
	_pacman_handle_lock_release (handle);
	return status;
}

/** Get a package entry from a package database
 * @param db pointer to the package database to get the package from
 * @param name of the package
 * @return the package entry on success, NULL on error
 */
pmpkg_t *pacman_db_readpkg(pmdb_t *db, const char *name)
{
	/* Sanity checks */
	ASSERT(handle != NULL, return(NULL));
	ASSERT(db != NULL, return(NULL));
	ASSERT(name != NULL && strlen(name) != 0, return(NULL));

	return(_pacman_db_get_pkgfromcache(db, name));
}

/** Get the package cache of a package database
 * @param db pointer to the package database to get the package from
 * @return the list of packages on success, NULL on error. Returned list
 * is an internally cached list and shouldn't be freed.
 */
pmlist_t *pacman_db_getpkgcache(pmdb_t *db)
{
	/* Sanity checks */
	ASSERT(handle != NULL, return(NULL));
	ASSERT(db != NULL, return(NULL));

	return(_pacman_db_get_pkgcache(db));
}

/** Get the list of packages that a package provides
 * @param db pointer to the package database to get the package from
 * @param name name of the package
 * @return the list of packages on success, NULL on error
 */
pmlist_t *pacman_db_whatprovides(pmdb_t *db, char *name)
{
	/* Sanity checks */
	ASSERT(handle != NULL, return(NULL));
	ASSERT(db != NULL, return(NULL));
	ASSERT(name != NULL && strlen(name) != 0, return(NULL));

	return(_pacman_db_whatprovides(db, name));
}

/** Get a group entry from a package database
 * @param db pointer to the package database to get the group from
 * @param name of the group
 * @return the groups entry on success, NULL on error
 */
pmgrp_t *pacman_db_readgrp(pmdb_t *db, char *name)
{
	/* Sanity checks */
	ASSERT(handle != NULL, return(NULL));
	ASSERT(db != NULL, return(NULL));
	ASSERT(name != NULL && strlen(name) != 0, return(NULL));

	return(_pacman_db_get_grpfromcache(db, name));
}

/** Get the group cache of a package database
 * @param db pointer to the package database to get the group from
 * @return the list of groups on success, NULL on error. Returned list
 * is an internally cached list and shouldn't be freed.
 */
pmlist_t *pacman_db_getgrpcache(pmdb_t *db)
{
	/* Sanity checks */
	ASSERT(handle != NULL, return(NULL));
	ASSERT(db != NULL, return(NULL));

	return(_pacman_db_get_grpcache(db));
}

/** @} */

/** @defgroup pacman_packages Package Functions
 * @brief Functions to manipulate libpacman packages
 * @{
 */

/** Get information about a package.
 * @param pkg package pointer
 * @param parm name of the info to get
 * @return a void* on success (the value), NULL on error
 */
void *pacman_pkg_getinfo(pmpkg_t *pkg, unsigned char parm)
{
	/* Sanity checks */
	ASSERT(handle != NULL, return(NULL));
	ASSERT(pkg != NULL, return(NULL));

	return(_pacman_pkg_getinfo(pkg, parm));
}

/** Get a list of packages that own the specified file
 * @param filename name of the file
 * @return the list of packages on success, NULL on error. The returned
 * list is an internally cached list and shouldn't be freed.
 */
pmlist_t *pacman_pkg_getowners(char *filename)
{
	/* Sanity checks */
	ASSERT(handle->db_local != NULL, RET_ERR(PM_ERR_DB_NULL, NULL));
	ASSERT(filename != NULL && strlen(filename) != 0, RET_ERR(PM_ERR_WRONG_ARGS, NULL));

	return(_pacman_pkg_getowners(filename));
}

/** Create a package from a file.
 * @param filename location of the package tarball
 * @param pkg address of the package pointer
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_pkg_load(char *filename, pmpkg_t **pkg)
{
	_pacman_log(PM_LOG_FUNCTION, "enter pacman_pkg_load");

	/* Sanity checks */
	ASSERT(filename != NULL && strlen(filename) != 0, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(pkg != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	*pkg = _pacman_pkg_load(filename);
	if(*pkg == NULL) {
		/* pm_errno is set by pkg_load */
		return(-1);
	}

	return(0);
}

/** Free a package.
 * @param pkg package pointer to free
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_pkg_free(pmpkg_t *pkg)
{
	_pacman_log(PM_LOG_FUNCTION, "enter pacman_pkg_free");

	ASSERT(pkg != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	/* Only free packages loaded in user space */
	if(pkg->origin != PKG_FROM_CACHE) {
		_pacman_pkg_free(pkg);
	}

	return(0);
}

/** Compare versions.
 * @param ver1 first version
 * @param ver2 second version
 * @return negative, 0 or positive if ver1 is less, equal or more than ver2,
 * respectively.
 */
int pacman_pkg_vercmp(const char *ver1, const char *ver2)
{
	return(_pacman_versioncmp(ver1, ver2));
}

/** @} */

/** @defgroup pacman_groups Group Functions
 * @brief Functions to get information about libpacman groups
 * @{
 */

/** Get information about a group.
 * @param grp group pointer
 * @param parm name of the info to get
 * @return a void* on success (the value), NULL on error
 */
void *pacman_grp_getinfo(pmgrp_t *grp, unsigned char parm)
{
	void *data = NULL;

	/* Sanity checks */
	ASSERT(grp != NULL, return(NULL));

	switch(parm) {
		case PM_GRP_NAME:     data = grp->name; break;
		case PM_GRP_PKGNAMES: data = grp->packages; break;
		default:
			data = NULL;
		break;
	}

	return(data);
}
/** @} */

/** @defgroup pacman_sync Sync Functions
 * @brief Functions to get information about libpacman syncs
 * @{
 */

/** Get information about a sync.
 * @param sync pointer
 * @param parm name of the info to get
 * @return a void* on success (the value), NULL on error
 */
void *pacman_sync_getinfo(pmsyncpkg_t *ps, unsigned char parm)
{
	void *data;

	/* Sanity checks */
	ASSERT(ps != NULL, return(NULL));

	switch(parm) {
		case PM_SYNC_TYPE: data = (void *)(long)ps->type; break;
		case PM_SYNC_PKG:  data = ps->pkg_new != NULL ? ps->pkg_new : ps->pkg_local; break;
		default:
			data = NULL;
		break;
	}

	return(data);
}

/** Cleans the cache
 * @param full 0: only old packages, 1: all packages
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_sync_cleancache(int full)
{
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	return(_pacman_sync_cleancache(full));
}

/** Tests a database
 * @param db pointer to the package database to search in
 * @return the list of problems found on success, NULL on error
 */
pmlist_t *pacman_db_test(pmdb_t *db)
{
	/* Sanity checks */
	ASSERT(handle != NULL, return(NULL));
	ASSERT(db != NULL, return(NULL));

	return(_pacman_db_test(db));
}

/** Searches a database
 * @param db pointer to the package database to search in
 * @return the list of packages on success, NULL on error
 */
pmlist_t *pacman_db_search(pmdb_t *db)
{
	pmlist_t *ret;

	/* Sanity checks */
	ASSERT(handle != NULL, return(NULL));
	ASSERT(handle->needles != NULL, return(NULL));
	ASSERT(handle->needles->data != NULL, return(NULL));
	ASSERT(db != NULL, return(NULL));

	ret = _pacman_db_search(db, handle->needles);
	FREELIST(handle->needles);
	return(ret);
}
/** @} */

/** @defgroup pacman_trans Transaction Functions
 * @brief Functions to manipulate libpacman transactions
 * @{
 */

/** Get information about the transaction.
 * @param parm name of the info to get
 * @return a void* on success (the value), NULL on error
 */
void *pacman_trans_getinfo(unsigned char parm)
{
	pmtrans_t *trans;
	void *data;

	/* Sanity checks */
	ASSERT(handle != NULL, return(NULL));
	ASSERT(handle->trans != NULL, return(NULL));

	trans = handle->trans;

	switch(parm) {
		case PM_TRANS_TYPE:     data = (void *)(long)trans->type; break;
		case PM_TRANS_FLAGS:    data = (void *)(long)trans->flags; break;
		case PM_TRANS_TARGETS:  data = trans->targets; break;
		case PM_TRANS_PACKAGES: data = trans->packages; break;
		default:
			data = NULL;
		break;
	}

	return(data);
}

/** Initialize the transaction.
 * @param type type of the transaction
 * @param flags flags of the transaction (like nodeps, etc)
 * @param event event callback function pointer
 * @param conv question callback function pointer
 * @param progress progress callback function pointer
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_trans_init(unsigned char type, unsigned int flags, pacman_trans_cb_event event, pacman_trans_cb_conv conv, pacman_trans_cb_progress progress)
{
	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	ASSERT(handle->trans == NULL, RET_ERR(PM_ERR_TRANS_NOT_NULL, -1));

	if(_pacman_handle_lock_acquire (handle) != 0) {
		return -1;
	}

	handle->trans = _pacman_trans_new();
	if(handle->trans == NULL) {
		RET_ERR(PM_ERR_MEMORY, -1);
	}

	pmtrans_cbs_t cbs = {
		.event = event,
		.conv = conv,
		.progress = progress
	};

	return(_pacman_trans_init(handle->trans, type, flags, cbs));
}

/** Search for packages to upgrade and add them to the transaction.
 * You must register the local database or you it won't find anything.
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_trans_sysupgrade()
{
	pmtrans_t *trans;

	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	trans = handle->trans;
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->state == STATE_INITIALIZED, RET_ERR(PM_ERR_TRANS_NOT_INITIALIZED, -1));
	ASSERT(trans->type == PM_TRANS_TYPE_SYNC, RET_ERR(PM_ERR_TRANS_TYPE, -1));

	return(_pacman_trans_sysupgrade(trans));
}

/** Add a target to the transaction.
 * @param target the name of the target to add
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_trans_addtarget(const char *target)
{
	pmtrans_t *trans;
	pmtranstype_t type = 0;
	int flag = PM_TRANS_FLAG_EXPLICIT;

	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));
	ASSERT(target != NULL && strlen(target) != 0, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	trans = handle->trans;

	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->state == STATE_INITIALIZED, RET_ERR(PM_ERR_TRANS_NOT_INITIALIZED, -1));

	type = trans->type;
	if (handle->sysupgrade != 0 ||
			type == PM_TRANS_TYPE_UPGRADE) {
		flag &= ~PM_TRANS_FLAG_EXPLICIT;
	}
	flag |= trans->type & PM_TRANS_FLAG_DEPENDSONLY;
	if (type == PM_TRANS_TYPE_SYNC) {
		type = PM_TRANS_TYPE_UPGRADE;
	}
	return(_pacman_trans_addtarget(trans, target, type, flag));
}

/** Prepare a transaction.
 * @param data the address of a PM_LIST where detailed description
 * of an error can be dumped (ie. list of conflicting files)
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_trans_prepare(pmlist_t **data)
{
	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));
	ASSERT(data != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	ASSERT(handle->trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(handle->trans->state == STATE_INITIALIZED, RET_ERR(PM_ERR_TRANS_NOT_INITIALIZED, -1));

	return(_pacman_trans_prepare(handle->trans, data));
}

/** Commit a transaction.
 * @param data the address of a PM_LIST where detailed description
 * of an error can be dumped (ie. list of conflicting files)
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_trans_commit(pmlist_t **data)
{
	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	ASSERT(handle->trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(handle->trans->state == STATE_PREPARED, RET_ERR(PM_ERR_TRANS_NOT_PREPARED, -1));

	/* Check for database R/W permission */
	ASSERT((handle->access == PM_ACCESS_RW) || (handle->trans->flags & PM_TRANS_FLAG_PRINTURIS),
			RET_ERR(PM_ERR_BADPERMS, -1));

	return(_pacman_trans_commit(handle->trans, data));
}

/** Release a transaction.
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_trans_release()
{
	pmtrans_t *trans;
	char lastupdate[15] = "";
	time_t t;

	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	trans = handle->trans;
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->state != STATE_IDLE, RET_ERR(PM_ERR_TRANS_NULL, -1));

	/* during a commit do not interrupt inmediatelly, just after a target */
	if(trans->state == STATE_COMMITING || trans->state == STATE_DOWNLOADING) {
		if(trans->state == STATE_COMMITING) {
			trans->state = STATE_INTERRUPTED;
			pm_errno = PM_ERR_TRANS_COMMITING;
		} else if(trans->state == STATE_DOWNLOADING) {
			trans->state = STATE_INTERRUPTED;
			pm_errno = PM_ERR_TRANS_DOWNLOADING;
		}
		return(-1);
	}

	FREETRANS(handle->trans);

	t = time(NULL);
	strftime(lastupdate, 15, "%Y%m%d%H%M%S", localtime(&t));
	_pacman_db_setlastupdate(handle->db_local, lastupdate);

	_pacman_handle_lock_release (handle);

	return(0);
}
/** @} */

/** @defgroup pacman_dep Dependency Functions
 * @brief Functions to get information about a libpacman dependency
 * @{
 */

/** Get information about a dependency.
 * @param miss dependency pointer
 * @param parm name of the info to get
 * @return a void* on success (the value), NULL on error
 */
void *pacman_dep_getinfo(pmdepmissing_t *miss, unsigned char parm)
{
	void *data;

	/* Sanity checks */
	ASSERT(miss != NULL, return(NULL));

	switch(parm) {
		case PM_DEP_TARGET:  data = (void *)(long)miss->target; break;
		case PM_DEP_TYPE:    data = (void *)(long)miss->type; break;
		case PM_DEP_MOD:     data = (void *)(long)miss->depend.mod; break;
		case PM_DEP_NAME:    data = miss->depend.name; break;
		case PM_DEP_VERSION: data = miss->depend.version; break;
		default:
			data = NULL;
		break;
	}

	return(data);
}
/** @} */

/** @defgroup pacman_conflict File Conflicts Functions
 * @brief Functions to get information about a libpacman file conflict
 * @{
 */

/** Get information about a file conflict.
 * @param conflict database conflict structure
 * @param parm name of the info to get
 * @return a void* on success (the value), NULL on error
 */
void *pacman_conflict_getinfo(pmconflict_t *conflict, unsigned char parm)
{
	void *data;

	/* Sanity checks */
	ASSERT(conflict != NULL, return(NULL));

	switch(parm) {
		case PM_CONFLICT_TARGET:  data = conflict->target; break;
		case PM_CONFLICT_TYPE:    data = (void *)(long)conflict->type; break;
		case PM_CONFLICT_FILE:    data = conflict->file; break;
		case PM_CONFLICT_CTARGET: data = conflict->ctarget; break;
		default:
			data = NULL;
		break;
	}

	return(data);
}
/** @} */

/** @defgroup pacman_log Logging Functions
 * @brief Functions to log using libpacman
 * @{
 */

/** A printf-like function for logging.
 * @param fmt output format
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_logaction(const char *fmt, ...)
{
	char str[LOG_STR_LEN];
	va_list args;

	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	va_start(args, fmt);
	vsnprintf(str, LOG_STR_LEN, fmt, args);
	va_end(args);

	/* TODO
	We should add a prefix to log strings depending on who called us.
	If logaction was called by the frontend:
		USER: <the frontend log>
	and if called internally:
		LIBPACMAN: <the library log>
	Moreover, the frontend should be able to choose its prefix (USER by default?):
		pacman-g2: "PACMAN-g2"
		kpacman: "KPACMAN"
		...
	It allows to share the log file between several frontends and to actually
	know who does what */

	return(_pacman_logaction(handle->usesyslog, handle->logfd, str));
}
/** @} */

/** @defgroup pacman_list List Functions
 * @brief Functions to manipulate libpacman linked lists
 * @{
 */

/** Get the first element of a list.
 * @param list the list
 * @return the first element
 */
pmlist_t *pacman_list_first(pmlist_t *list)
{
	return f_ptrlist_begin (list);
}

/** Get the next element of a list.
 * @param entry the list entry
 * @return the next element on success, NULL on error
 */
pmlist_t *pacman_list_next(pmlist_t *entry)
{
	ASSERT(entry != NULL, return(NULL));

	return(entry->next);
}

/** Get the data of a list entry.
 * @param entry the list entry
 * @return the data on success, NULL on error
 */
void *pacman_list_getdata(pmlist_t *entry)
{
	ASSERT(entry != NULL, return(NULL));

	return(entry->data);
}

/** Free a list.
 * @param entry list to free
 * @return 0 on success, -1 on error
 */
int pacman_list_free(pmlist_t *entry)
{
	ASSERT(entry != NULL, return(-1));

	FREELIST(entry);

	return(0);
}

/** Count the entries in a list.
 * @param list the list to count
 * @return number of entries on success, NULL on error
 */
int pacman_list_count(pmlist_t *list)
{
	ASSERT(list != NULL, return(-1));

	return f_ptrlist_count (list);
}
/** @} */

/** @defgroup pacman_misc Miscellaneous Functions
 * @brief Various libpacman functions
 * @{
 */

/** Get the md5 sum of file.
 * @param name name of the file
 * @return the checksum on success, NULL on error
 */
char *pacman_get_md5sum(char *name)
{
	ASSERT(name != NULL, return(NULL));

	return(_pacman_MDFile(name));
}

/** Get the sha1 sum of file.
 * @param name name of the file
 * @return the checksum on success, NULL on error
 */
char *pacman_get_sha1sum(char *name)
{
	ASSERT(name != NULL, return(NULL));

	return(_pacman_SHAFile(name));
}

/** Fetch a remote pkg.
 * @param url
 * @return the downloaded filename on success, NULL on error
 */
char *pacman_fetch_pkgurl(char *url)
{
	ASSERT(strstr(url, "://"), return(NULL));

	return(_pacman_fetch_pkgurl(url));
}

/** Match a string against a regular expression.
 * @param string the input string
 * @param pattern the pattern to search for
 * @return 1 on match, 0 on non-match, -1 on error (pm_errno is set accordingly)
 */
int pacman_reg_match(const char *string, const char *pattern)
{
	ASSERT(string != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(pattern != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	return(_pacman_reg_match(string, pattern));
}

/** Parses a configuration file.
 * @param file path to the config file.
 * @param this_section the config current section being parsed
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
static
int _pacman_parse_config(char *file, const char *this_section)
{
	FILE *fp = NULL;
	char line[PATH_MAX+1];
	char *ptr = NULL;
	char *key = NULL;
	int linenum = 0;
	char section[256] = "";
	PM_DB *db = NULL;

	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	fp = fopen(file, "r");
	if(fp == NULL) {
		return(0);
	}

	if (f_strlen (this_section) > 0) {
		strncpy(section, this_section, min(255, strlen(this_section)));
		if(!strcmp(section, "local")) {
			RET_ERR(PM_ERR_CONF_LOCAL, -1);
		}
		if(strcmp(section, "options")) {
			db = _pacman_db_register(section);
		}
	} else {
		FREELIST(handle->ignorepkg);
		FREELIST(handle->holdpkg);
	}

	while(fgets(line, PATH_MAX, fp)) {
		linenum++;
		_pacman_strtrim(line);
		if(line[0] == '\0' || line[0] == '#') {
			continue;
		}
		if((ptr = strchr(line, '#'))) {
			*ptr = '\0';
		}
		if(line[0] == '[' && line[strlen(line)-1] == ']') {
			/* new config section */
			ptr = line;
			ptr++;
			strncpy(section, ptr, min(255, strlen(ptr)-1));
			section[min(255, strlen(ptr)-1)] = '\0';
			_pacman_log(PM_LOG_DEBUG, _("config: new section '%s'\n"), section);
			if(!strlen(section)) {
				RET_ERR(PM_ERR_CONF_BAD_SECTION, -1);
			}
			if(!strcmp(section, "local")) {
				RET_ERR(PM_ERR_CONF_LOCAL, -1);
			}
			if(strcmp(section, "options")) {
				db = _pacman_db_register(section);
				if(db == NULL) {
					/* pm_errno is set by pacman_db_register */
					return(-1);
				}
			}
		} else {
			/* directive */
			ptr = line;
			key = strsep(&ptr, "=");
			if(key == NULL) {
				RET_ERR(PM_ERR_CONF_BAD_SYNTAX, -1);
			}
			_pacman_strtrim(key);
			key = _pacman_strtoupper(key);
			if(!strlen(section) && strcmp(key, "INCLUDE")) {
				RET_ERR(PM_ERR_CONF_DIRECTIVE_OUTSIDE_SECTION, -1);
			}
			if(ptr == NULL) {
				if(!strcmp(key, "NOPASSIVEFTP")) {
					pacman_set_option(PM_OPT_NOPASSIVEFTP, (long)1);
				} else if(!strcmp(key, "USESYSLOG")) {
					pacman_set_option(PM_OPT_USESYSLOG, (long)1);
					_pacman_log(PM_LOG_DEBUG, _("config: usesyslog\n"));
				} else if(!strcmp(key, "ILOVECANDY")) {
					pacman_set_option(PM_OPT_CHOMP, (long)1);
				} else {
					RET_ERR(PM_ERR_CONF_BAD_SYNTAX, -1);
				}
			} else {
				_pacman_strtrim(ptr);
				if(!strcmp(key, "INCLUDE")) {
					char conf[PATH_MAX];
					strncpy(conf, ptr, PATH_MAX);
					_pacman_log(PM_LOG_DEBUG, _("config: including %s\n"), conf);
					_pacman_parse_config(conf, section);
				} else if(!strcmp(section, "options")) {
					if(!strcmp(key, "NOUPGRADE")) {
						char *p = ptr;
						char *q;
						while((q = strchr(p, ' '))) {
							*q = '\0';
							if(pacman_set_option(PM_OPT_NOUPGRADE, (long)p) == -1) {
								/* pm_errno is set by pacman_set_option */
								return(-1);
							}
							_pacman_log(PM_LOG_DEBUG, _("config: noupgrade: %s\n"), p);
							p = q;
							p++;
						}
						if(pacman_set_option(PM_OPT_NOUPGRADE, (long)p) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
						_pacman_log(PM_LOG_DEBUG, _("config: noupgrade: %s\n"), p);
					} else if(!strcmp(key, "NOEXTRACT")) {
						char *p = ptr;
						char *q;
						while((q = strchr(p, ' '))) {
							*q = '\0';
							if(pacman_set_option(PM_OPT_NOEXTRACT, (long)p) == -1) {
								/* pm_errno is set by pacman_set_option */
								return(-1);
							}
							_pacman_log(PM_LOG_DEBUG, _("config: noextract: %s\n"), p);
							p = q;
							p++;
						}
						if(pacman_set_option(PM_OPT_NOEXTRACT, (long)p) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
						_pacman_log(PM_LOG_DEBUG, _("config: noextract: %s\n"), p);
					} else if(!strcmp(key, "IGNOREPKG")) {
						char *p = ptr;
						char *q;
						while((q = strchr(p, ' '))) {
							*q = '\0';
							if(pacman_set_option(PM_OPT_IGNOREPKG, (long)p) == -1) {
								/* pm_errno is set by pacman_set_option */
								return(-1);
							}
							_pacman_log(PM_LOG_DEBUG, _("config: ignorepkg: %s\n"), p);
							p = q;
							p++;
						}
						if(pacman_set_option(PM_OPT_IGNOREPKG, (long)p) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
						_pacman_log(PM_LOG_DEBUG, _("config: ignorepkg: %s\n"), p);
					} else if(!strcmp(key, "HOLDPKG")) {
						char *p = ptr;
						char *q;
						while((q = strchr(p, ' '))) {
							*q = '\0';
							if(pacman_set_option(PM_OPT_HOLDPKG, (long)p) == -1) {
								/* pm_errno is set by pacman_set_option */
								return(-1);
							}
							_pacman_log(PM_LOG_DEBUG, _("config: holdpkg: %s\n"), p);
							p = q;
							p++;
						}
						if(pacman_set_option(PM_OPT_HOLDPKG, (long)p) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
						_pacman_log(PM_LOG_DEBUG, _("config: holdpkg: %s\n"), p);
					} else if(!strcmp(key, "DBPATH")) {
						/* shave off the leading slash, if there is one */
						if(*ptr == '/') {
							ptr++;
						}
						if(pacman_set_option(PM_OPT_DBPATH, (long)ptr) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
						_pacman_log(PM_LOG_DEBUG, _("config: dbpath: %s\n"), ptr);
					} else if(!strcmp(key, "CACHEDIR")) {
						/* shave off the leading slash, if there is one */
						if(*ptr == '/') {
							ptr++;
						}
						if(pacman_set_option(PM_OPT_CACHEDIR, (long)ptr) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
						_pacman_log(PM_LOG_DEBUG, _("config: cachedir: %s\n"), ptr);
					} else if (!strcmp(key, "LOGFILE")) {
						if(pacman_set_option(PM_OPT_LOGFILE, (long)ptr) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
						_pacman_log(PM_LOG_DEBUG, _("config: log file: %s\n"), ptr);
					} else if (!strcmp(key, "XFERCOMMAND")) {
						if(pacman_set_option(PM_OPT_XFERCOMMAND, (long)ptr) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
					} else if (!strcmp(key, "UPGRADEDELAY")) {
						/* The config value is in days, we use seconds */
						_pacman_log(PM_LOG_DEBUG, _("config: UpgradeDelay: %i\n"), (60*60*24) * atol(ptr));
						if(pacman_set_option(PM_OPT_UPGRADEDELAY, (60*60*24) * atol(ptr)) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
					} else if (!strcmp(key, "OLDDELAY")) {
						/* The config value is in days, we use seconds */
						_pacman_log(PM_LOG_DEBUG, _("config: OldDelay: %i\n"), (60*60*24) * atol(ptr));
						if(pacman_set_option(PM_OPT_OLDDELAY, (60*60*24) * atol(ptr)) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
					} else if (!strcmp(key, "PROXYSERVER")) {
						if(pacman_set_option(PM_OPT_PROXYHOST, (long)ptr) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
					} else if (!strcmp(key, "PROXYPORT")) {
						if(pacman_set_option(PM_OPT_PROXYPORT, (long)atoi(ptr)) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
					} else if (!strcmp(key, "MAXTRIES")) {
						if(pacman_set_option(PM_OPT_MAXTRIES, (long)atoi(ptr)) == -1) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
					} else {
						RET_ERR(PM_ERR_CONF_BAD_SYNTAX, -1);
					}
				} else {
					if(!strcmp(key, "SERVER")) {
						/* add to the list */
						_pacman_log(PM_LOG_DEBUG, _("config: %s: server: %s\n"), section, ptr);
						if(pacman_db_setserver(db, ptr) != 0) {
							/* pm_errno is set by pacman_set_option */
							return(-1);
						}
					} else {
						RET_ERR(PM_ERR_CONF_BAD_SYNTAX, -1);
					}
				}
				line[0] = '\0';
			}
		}
	}
	fclose(fp);

	return(0);
}

/** Parses a configuration file.
 * @param file path to the config file.
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_parse_config (char *file) {
	return _pacman_parse_config (file, NULL);
}

/* @} */

/* vim: set ts=2 sw=2 noet: */
