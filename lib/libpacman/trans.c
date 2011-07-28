/*
 *  trans.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005 by Christian Hamar <krics@linuxforum.hu>
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libintl.h>
/* pacman-g2 */
#include "error.h"
#include "package.h"
#include "util.h"
#include "log.h"
#include "list.h"
#include "handle.h"
#include "add.h"
#include "remove.h"
#include "sync.h"
#include "cache.h"
#include "pacman.h"

static int check_oldcache(void)
{
	pmdb_t *db = handle->db_local;
	char lastupdate[16] = "";

	if(_pacman_db_getlastupdate(db, lastupdate) == -1) {
		return(-1);
	}
	if(strlen(db->lastupdate) && strcmp(lastupdate, db->lastupdate) != 0) {
		_pacman_log(PM_LOG_DEBUG, _("cache for '%s' repo is too old"), db->treename);
		_pacman_db_free_pkgcache(db);
	} else {
		_pacman_log(PM_LOG_DEBUG, _("cache for '%s' repo is up to date"), db->treename);
	}
	return(0);
}

pmtrans_t *_pacman_trans_new()
{
	pmtrans_t *trans;

	if((trans = (pmtrans_t *)malloc(sizeof(pmtrans_t))) == NULL) {
		_pacman_log(PM_LOG_ERROR, _("malloc failure: could not allocate %d bytes"), sizeof(pmtrans_t));
		return(NULL);
	}

	trans->targets = NULL;
	trans->packages = NULL;
	trans->skiplist = NULL;
	trans->type = 0;
	trans->flags = 0;
	trans->cb_event = NULL;
	trans->cb_conv = NULL;
	trans->cb_progress = NULL;
	trans->state = STATE_IDLE;

	return(trans);
}

void _pacman_trans_free(pmtrans_t *trans)
{
	if(trans == NULL) {
		return;
	}

	FREELIST(trans->targets);
	if(trans->type == PM_TRANS_TYPE_SYNC) {
		pmlist_t *i;
		for(i = trans->packages; i; i = i->next) {
			FREESYNC(i->data);
		}
		FREELIST(trans->packages);
	} else {
		FREELISTPKGS(trans->packages);
	}

	FREELIST(trans->skiplist);

	free(trans);
}

int _pacman_trans_init(pmtrans_t *trans, pmtranstype_t type, unsigned int flags, pacman_trans_cb_event event, pacman_trans_cb_conv conv, pacman_trans_cb_progress progress)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	trans->type = type;
	trans->flags = flags;
	trans->cb_event = event;
	trans->cb_conv = conv;
	trans->cb_progress = progress;
	trans->state = STATE_INITIALIZED;

	check_oldcache();

	return(0);
}

int _pacman_trans_sysupgrade(pmtrans_t *trans)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	return(_pacman_sync_sysupgrade(trans, handle->db_local, handle->dbs_sync));
}

int _pacman_trans_addtarget(pmtrans_t *trans, const char *target)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(target != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	if(_pacman_list_is_strin(target, trans->targets)) {
		RET_ERR(PM_ERR_TRANS_DUP_TARGET, -1);
	}

	switch(trans->type) {
		case PM_TRANS_TYPE_ADD:
		case PM_TRANS_TYPE_UPGRADE:
			if(_pacman_add_loadtarget(trans, handle->db_local, target) == -1) {
				/* pm_errno is set by _pacman_add_loadtarget() */
				return(-1);
			}
		break;
		case PM_TRANS_TYPE_REMOVE:
			if(_pacman_remove_loadtarget(trans, handle->db_local, target) == -1) {
				/* pm_errno is set by remove_loadtarget() */
				return(-1);
			}
		break;
		case PM_TRANS_TYPE_SYNC:
			if(_pacman_sync_addtarget(trans, handle->db_local, handle->dbs_sync, target) == -1) {
				/* pm_errno is set by sync_loadtarget() */
				return(-1);
			}
		break;
	}

	trans->targets = _pacman_list_add(trans->targets, strdup(target));

	return(0);
}

int _pacman_trans_prepare(pmtrans_t *trans, pmlist_t **data)
{
	*data = NULL;

	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	/* If there's nothing to do, return without complaining */
	if(trans->packages == NULL) {
		return(0);
	}

	switch(trans->type) {
		case PM_TRANS_TYPE_ADD:
		case PM_TRANS_TYPE_UPGRADE:
			if(_pacman_add_prepare(trans, handle->db_local, data) == -1) {
				/* pm_errno is set by _pacman_add_prepare() */
				return(-1);
			}
		break;
		case PM_TRANS_TYPE_REMOVE:
			if(_pacman_remove_prepare(trans, handle->db_local, data) == -1) {
				/* pm_errno is set by _pacman_remove_prepare() */
				return(-1);
			}
		break;
		case PM_TRANS_TYPE_SYNC:
			if(_pacman_sync_prepare(trans, handle->db_local, handle->dbs_sync, data) == -1) {
				/* pm_errno is set by _pacman_sync_prepare() */
				return(-1);
			}
		break;
	}

	trans->state = STATE_PREPARED;

	return(0);
}

int _pacman_trans_commit(pmtrans_t *trans, pmlist_t **data)
{
	if(data!=NULL)
		*data = NULL;

	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	/* If there's nothing to do, return without complaining */
	if(trans->packages == NULL) {
		return(0);
	}

	trans->state = STATE_COMMITING;

	switch(trans->type) {
		case PM_TRANS_TYPE_ADD:
		case PM_TRANS_TYPE_UPGRADE:
			if(_pacman_add_commit(trans, handle->db_local) == -1) {
				/* pm_errno is set by _pacman_add_prepare() */
				return(-1);
			}
		break;
		case PM_TRANS_TYPE_REMOVE:
			if(_pacman_remove_commit(trans, handle->db_local) == -1) {
				/* pm_errno is set by _pacman_remove_prepare() */
				return(-1);
			}
		break;
		case PM_TRANS_TYPE_SYNC:
			if(_pacman_sync_commit(trans, handle->db_local, data) == -1) {
				/* pm_errno is set by _pacman_sync_commit() */
				return(-1);
			}
		break;
	}

	trans->state = STATE_COMMITED;

	return(0);
}

/* vim: set ts=2 sw=2 noet: */
