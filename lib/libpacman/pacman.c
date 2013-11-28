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
#include <libintl.h>

/* pacman-g2 */
#include "pacman.h"

#include "db/syncdb.h"
#include "hash/md5.h"
#include "hash/sha1.h"
#include "util/list.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "config_parser.h"
#include "error.h"
#include "deps.h"
#include "versioncmp.h"
#include "package.h"
#include "group.h"
#include "util.h"
#include "db.h"
#include "cache.h"
#include "conflict.h"
#include "add.h"
#include "remove.h"
#include "sync.h"
#include "handle.h"
#include "server.h"

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

	if(_pacman_handle_unlock(handle) != 0) {
		return -1;
	}

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
	ASSERT(!_pacman_strempty(treename), RET_ERR(PM_ERR_WRONG_ARGS, NULL));
	/* Do not register a database if a transaction is on-going */
	ASSERT(handle->trans == NULL, RET_ERR(PM_ERR_TRANS_NOT_NULL, NULL));

	return(_pacman_db_register(treename, NULL));
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
		pmdb_t *data;
		handle->dbs_sync = _pacman_list_remove(handle->dbs_sync, db, _pacman_db_cmp, (void **)&data);
		if(data) {
			found = 1;
		}
	}

	if(!found) {
		RET_ERR(PM_ERR_DB_NOT_FOUND, -1);
	}

	_pacman_log(PM_LOG_FLOW1, _("unregistering database '%s'"), db->treename);

	/* Cleanup */
	_pacman_db_free_pkgcache(db);

	_pacman_log(PM_LOG_DEBUG, _("closing database '%s'"), db->treename);
	_pacman_db_close(db);

	_pacman_db_free(db);

	return(0);
}

/** Get information about a database.
 * @param db database pointer
 * @param parm name of the info to get
 * @return a void* on success (the value), NULL on error
 */
void *pacman_db_getinfo(pmdb_t *db, unsigned char parm)
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
		for(i = handle->dbs_sync; i && !found; i = i->next) {
			pmdb_t *sdb = i->data;
			if(strcmp(db->treename, sdb->treename) == 0) {
				found = 1;
			}
		}
	}
	if(!found) {
		RET_ERR(PM_ERR_DB_NOT_FOUND, -1);
	}

	if(!_pacman_strempty(url)) {
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
int pacman_db_update(int force, pmdb_t *db)
{
	int status=0;

	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));
	ASSERT(db != NULL && db != handle->db_local, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	/* Do not update a database if a transaction is on-going */
	ASSERT(handle->trans == NULL, RET_ERR(PM_ERR_TRANS_NOT_NULL, -1));

	if(_pacman_handle_lock(handle) != 0) {
		return -1;
	}
	if(!f_ptrlist_contains_ptr(handle->dbs_sync, db)) {
		RET_ERR(PM_ERR_DB_NOT_FOUND, -1);
	}

	status = _pacman_syncdb_update(db, force);
	if(_pacman_handle_unlock(handle) != 0) {
		return -1;
	}
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
	ASSERT(!_pacman_strempty(name), return(NULL));

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
	ASSERT(!_pacman_strempty(name), return(NULL));

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
	ASSERT(!_pacman_strempty(name), return(NULL));

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

/** @defgroup pacman_downloadstate Download States Functions
 * @brief Functions to informations from libpacman downloads
 * @{
 */

/** Get the average download speed (in bytes per seconds).
 * @param downloadstate pointer to the download state to get the informations from.
 * @param offset pointer to the value to be written.
 * @return return 0 in case of success, !0 otherwise.
 */
int pacman_downloadstate_avg(const pmdownloadstate_t *downloadstate, double *avg)
{
	ASSERT(downloadstate != NULL, return -1);
	ASSERT(avg != NULL, return -1);

	*avg = downloadstate->dst_avg;
	return 0;
}

/** Get the download start time
 * @param downloadstate pointer to the download state to get the informations from.
 * @return return 0 in case of success, !0 otherwise.
 */
int pacman_downloadstate_begin(const pmdownloadstate_t *downloadstate, struct timeval *timeval)
{
	ASSERT(downloadstate != NULL, return -1);
	ASSERT(timeval != NULL, return -1);

	*timeval = downloadstate->dst_begin;
	return 0;
}

/** Get the download end time.
 * Calling this method before pacman_downloadstate_tell give the same value as pacman_downloadstate_size
 * has no meaning and can give any value.
 * @param downloadstate pointer to the download state to get the informations from.
 * @param offset pointer to the value to be written.
 * @return return 0 in case of success, !0 otherwise.
 */
int pacman_downloadstate_end(const pmdownloadstate_t *downloadstate, struct timeval *timeval)
{
	ASSERT(downloadstate != NULL, return -1);
	ASSERT(timeval != NULL, return -1);

	*timeval = downloadstate->dst_end;
	return 0;
}

/** Get the estimate time to arrival of the download (in seconds).
 * @param downloadstate pointer to the download state to get the informations from.
 * @param offset pointer to the value to be written.
 * @return return 0 in case of success, !0 otherwise.
 */
int pacman_downloadstate_eta(const pmdownloadstate_t *downloadstate, double *eta)
{
	ASSERT(downloadstate != NULL, return -1);
	ASSERT(eta != NULL, return -1);

	*eta = downloadstate->dst_eta;
	return 0;
}

/** Get the current download rate (in bytes per second).
 * @param downloadstate pointer to the download state to get the informations from.
 * @param offset pointer to the value to be written.
 * @return return 0 in case of success, !0 otherwise.
 */
int pacman_downloadstate_rate(const pmdownloadstate_t *downloadstate, double *rate)
{
	ASSERT(downloadstate != NULL, return -1);
	ASSERT(rate != NULL, return -1);

	*rate = downloadstate->dst_rate;
	return 0;
}

/** Get the size at the start of the download resume.
 * @param downloadstate pointer to the download state to get the informations from.
 * @param offset pointer to the value to be written.
 * @return return 0 in case of success, !0 otherwise.
 */
int pacman_downloadstate_resume(const pmdownloadstate_t *downloadstate, off_t *offset)
{
	ASSERT(downloadstate != NULL, return -1);
	ASSERT(offset != NULL, return -1);

	*offset = downloadstate->dst_resume;
	return 0;
}

/** Get the final size of the download
 * @param downloadstate pointer to the download state to get the informations from.
 * @param offset pointer to the value to be written.
 * @return return 0 in case of success, !0 otherwise.
 */
int pacman_downloadstate_size(const pmdownloadstate_t *downloadstate, off_t *offset)
{
	ASSERT(downloadstate != NULL, return -1);
	ASSERT(offset != NULL, return -1);

	*offset = downloadstate->dst_size;
	return 0;
}

/** Get the current size of the download
 * @param downloadstate pointer to the download state to get the informations from.
 * @param offset pointer to the value to be written.
 * @return return 0 in case of success, !0 otherwise.
 */
int pacman_downloadstate_tell(const pmdownloadstate_t *downloadstate, off_t *offset)
{
	ASSERT(downloadstate != NULL, return -1);
	ASSERT(offset != NULL, return -1);

	*offset = downloadstate->dst_tell;
	return 0;
}

/** Get the xfered size of the download
 * @param downloadstate pointer to the download state to get the informations from.
 * @param offset pointer to the value to be written.
 * @return return 0 in case of success, !0 otherwise.
 */
int pacman_downloadstate_xfered(const pmdownloadstate_t *downloadstate, off_t *offset)
{
	ASSERT(downloadstate != NULL, return -1);
	ASSERT(offset != NULL, return -1);

	*offset = downloadstate->dst_tell - downloadstate->dst_resume;
	return 0;
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
	ASSERT(!_pacman_strempty(filename), RET_ERR(PM_ERR_WRONG_ARGS, NULL));

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
	ASSERT(!_pacman_strempty(filename), RET_ERR(PM_ERR_WRONG_ARGS, -1));
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
		case PM_SYNC_PKG:  data = ps->pkg; break;
		case PM_SYNC_DATA: data = ps->data; break;
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
		case PM_TRANS_PACKAGES: data = trans->packages ? trans->packages : trans->syncpkgs; break;
		case PM_TRANS_SYNCPKGS: data = trans->syncpkgs; break;
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

	if(_pacman_handle_lock(handle) != 0) {
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

	_pacman_packages_transaction_init(handle->trans);
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

	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));
	ASSERT(!_pacman_strempty(target), RET_ERR(PM_ERR_WRONG_ARGS, -1));

	trans = handle->trans;
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->state == STATE_INITIALIZED, RET_ERR(PM_ERR_TRANS_NOT_INITIALIZED, -1));

	return(_pacman_trans_addtarget(trans, target));
}

/** Prepare a transaction.
 * @param data the address of a pmlist_t where detailed description
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
 * @param data the address of a pmlist_t where detailed description
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

	_pacman_db_settimestamp(handle->db_local, NULL);

	if(_pacman_handle_unlock(handle) != 0) {
		return -1;
	}
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
 * @param format output format
 */
void pacman_logaction(const char *format, ...)
{
	va_list ap;

	/* Sanity checks */
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	va_start(ap, format);
	_pacman_vlogaction(handle->usesyslog, handle->logfd, format, ap);
	va_end(ap);

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
	return(list);
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

	return(_pacman_list_count(list));
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
 * @param callback a function to be called upon new database creation
 * @param this_section the config current section being parsed
 * @return 0 on success, -1 on error (pm_errno is set accordingly)
 */
int pacman_parse_config(const char *file, pacman_cb_db_register callback)
{
	ASSERT(handle != NULL, RET_ERR(PM_ERR_HANDLE_NULL, -1));

	return _pacman_parse_config(file, callback, NULL);
}

/* @} */

/* vim: set ts=2 sw=2 noet: */
