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

#include "packages_transaction.h"

#include <config.h>
#include "util.h"

static int
_pacman_packages_transaction_set_state(pmtrans_t *trans, int new_state)
{
	static const char *hooks[STATE_MAX] = {
		[STATE_COMMITED] = "triggered",
	};

	if (new_state == STATE_COMMITED) {
		return _pacman_runhook(hooks[new_state], trans);
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
