/*
 *  deptest.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
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
#include <pacman.h>
/* pacman-g2 */
#include "util.h"
#include "list.h"
#include "conf.h"
#include "log.h"
#include "sync.h"
#include "deptest.h"

extern config_t *config;
extern list_t *pmc_syncs;

int pacman_output_generate(pmlist_t *targets, pmlist_t *dblist);

int vercmp(list_t *targets)
{
	// FIXME: should count targets

	if(targets != NULL && targets->data && targets->next && targets->next->data) {
		int ret = pacman_pkg_vercmp(targets->data, targets->next->data);
		printf("%d\n", ret);
		return(ret);
	}
	return(0);
}

/* vim: set ts=2 sw=2 noet: */
