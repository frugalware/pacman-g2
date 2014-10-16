/*
 *  packages_transaction.c
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

#include <limits.h> /* PATH_MAX */

#include "packages_transaction.h"

#include "handle.h"

#include "util.h"

#include "fstring.h"

static const char *trigger_function_table[STATE_MAX] = {
	[STATE_IDLE] = NULL,
	[STATE_INITIALIZED] = NULL,
	[STATE_PREPARED] = NULL,
	[STATE_DOWNLOADING] = NULL,
	[STATE_COMMITING] = NULL,
	[STATE_COMMITED] = "commited",
	[STATE_INTERRUPTED] = NULL,
};

static int
_pacman_packages_transaction_set_state(pmtrans_t *trans, int new_state)
{
	const char *root, *triggersdir, *trigger_function;

	triggersdir = trans->m_handle->triggersdir;
	root = trans->m_handle->root;
	trigger_function = trigger_function_table[new_state];

	if(_pacman_strempty(trigger_function)) {
		_pacman_log(PM_LOG_ERROR, _("Missing trigger_function for state: %d"), new_state);
		/* Nothing to do */
		return 0;
	}

	_pacman_log(PM_LOG_FLOW2, _("executing %s triggers..."), trigger_function);
	for(auto lp = trans->triggers.begin(), end = trans->triggers.end(); lp != end; ++lp) {
		const char *trigger = *lp;
		char buf[PATH_MAX];

		snprintf(buf, sizeof(buf), "%s/%s/%s", root, triggersdir, trigger);
		if(access(buf, F_OK) != 0) { /* FIXME: X_OK instead ? */
			_pacman_log(PM_LOG_WARNING, _("Skipping missing trigger file (%s)"), buf);
			continue;
		}
#if 0
		/* FIXME: do we really need this cheap optimisation ? */
		if(!grep(buf, trigger_function)) {
			/* trigger_function not found in trigger */
			continue;
		}
#endif
		snprintf(buf, sizeof(buf), "source %s/%s %s", triggersdir, trigger, trigger_function);
		_pacman_chroot_system(buf, trans);
	}

	return 0;
}

int _pacman_packages_transaction_init(pmtrans_t *trans)
{
	ASSERT(trans != NULL, RET_ERR(PM_ERR_TRANS_NULL, -1));

	trans->set_state = _pacman_packages_transaction_set_state;
	return 0;
}

/* vim: set ts=2 sw=2 noet: */
