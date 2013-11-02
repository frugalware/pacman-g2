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
#include "trans.h"

#include "util/list.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "error.h"
#include "package.h"
#include "util.h"
#include "handle.h"
#include "add.h"
#include "remove.h"
#include "sync.h"
#include "cache.h"
#include "pacman.h"

#include "trans_sysupgrade.h"

static int check_oldcache(void)
{
	pmdb_t *db = handle->db_local;
	char lastupdate[16] = "";

	if(_pacman_db_getlastupdate(db, lastupdate) == -1) {
		return(-1);
	}
	if(!_pacman_strempty(db->lastupdate) && strcmp(lastupdate, db->lastupdate) != 0) {
		_pacman_log(PM_LOG_DEBUG, _("cache for '%s' repo is too old"), db->treename);
		_pacman_db_free_pkgcache(db);
	} else {
		_pacman_log(PM_LOG_DEBUG, _("cache for '%s' repo is up to date"), db->treename);
	}
	return(0);
}

pmtrans_t *_pacman_trans_new()
{
	pmtrans_t *trans = _pacman_zalloc(sizeof(pmtrans_t));

	if(trans) {
		trans->state = STATE_IDLE;
	}

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
	FREELIST(trans->pretriggers);
	FREELIST(trans->posttriggers);

	_pacman_trans_fini(trans);
	free(trans);
}

int _pacman_trans_init(pmtrans_t *trans, pmtranstype_t type, unsigned int flags, pmtrans_cbs_t cbs)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	switch(type) {
		case PM_TRANS_TYPE_ADD:
		case PM_TRANS_TYPE_UPGRADE:
			trans->ops = &_pacman_add_pmtrans_opts;
		break;
		case PM_TRANS_TYPE_REMOVE:
			trans->ops = &_pacman_remove_pmtrans_opts;
		break;
		case PM_TRANS_TYPE_SYNC:
			trans->ops = &_pacman_sync_pmtrans_opts;
		break;
		default:
			trans->ops = NULL;
			// Be more verbose about the trans type
			_pacman_log(PM_LOG_ERROR,
					_("could not initialize transaction: Unknown Transaction Type %d"), type);
			return(-1);
	}

	trans->handle = handle;
	trans->type = type;
	trans->flags = flags;
	trans->cbs = cbs;
	trans->state = STATE_INITIALIZED;

	check_oldcache();

	return(0);
}

void _pacman_trans_fini(pmtrans_t *trans)
{
	if(trans !=NULL && trans->ops != NULL && trans->ops->fini != NULL) {
		trans->ops->fini(trans);
	}
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
	ASSERT(trans->ops != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->ops->addtarget != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(target != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	if(_pacman_list_is_strin(target, trans->targets)) {
		RET_ERR(PM_ERR_TRANS_DUP_TARGET, -1);
	}

	if(trans->ops->addtarget(trans, target) == -1) {
		/* pm_errno is set by trans->ops->addtarget() */
		return(-1);
	}

	trans->targets = _pacman_stringlist_append(trans->targets, target);

	return(0);
}

int _pacman_trans_set_state(pmtrans_t *trans, int new_state)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	/* Ignore unchanged state */
	if (trans->state == new_state) {
		return(0);
	}

	if (trans->set_state != NULL) {
		if (trans->set_state(trans, new_state) == -1) {
			/* pm_errno is set by trans->state_changed() */
			return(-1);
		}
	}
	trans->state = new_state;

	return(0);
}

int _pacman_trans_compute_triggers(pmtrans_t *trans)
{
	pmlist_t *lp;

	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	/* NOTE: Not the most efficient way, but will do until we add some string hash. */
	for(lp = trans->packages; lp; lp = lp->next) {
		pmpkg_t *pkg = lp->data;

		trans->pretriggers = _pacman_stringlist_append_stringlist(trans->pretriggers, pkg->pretriggers);
		trans->posttriggers = _pacman_stringlist_append_stringlist(trans->posttriggers, pkg->posttriggers);
	}
	trans->pretriggers = _pacman_list_remove_dupes(trans->pretriggers);
	trans->posttriggers = _pacman_list_remove_dupes(trans->posttriggers);
	/* FIXME: Sort the triggers to have a predictable execution order */

	return 0;
}

int _pacman_trans_prepare(pmtrans_t *trans, pmlist_t **data)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->ops != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->ops->prepare != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(data != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	*data = NULL;

	/* If there's nothing to do, return without complaining */
	if(trans->packages == NULL) {
		return(0);
	}

	if(trans->ops->prepare(trans, data) == -1) {
		/* pm_errno is set by trans->ops->prepare() */
		return(-1);
	}

	_pacman_trans_set_state(trans, STATE_PREPARED);

	return(0);
}

int _pacman_trans_commit(pmtrans_t *trans, pmlist_t **data)
{
	/* Sanity checks */
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->ops != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));
	ASSERT(trans->ops->commit != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	if(data!=NULL)
		*data = NULL;

	/* If there's nothing to do, return without complaining */
	if(trans->packages == NULL) {
		return(0);
	}

	_pacman_trans_set_state(trans, STATE_COMMITING);

	if(trans->ops->commit(trans, data) == -1) {
		/* pm_errno is set by trans->ops->commit() */
		_pacman_trans_set_state(trans, STATE_PREPARED);
		return(-1);
	}

	_pacman_trans_set_state(trans, STATE_COMMITED);

	return(0);
}

/* vim: set ts=2 sw=2 noet: */
