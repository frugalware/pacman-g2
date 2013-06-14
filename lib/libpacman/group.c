/*
 *  group.c
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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <fstringlist.h>

/* pacman-g2 */
#include "group.h"

#include "util.h"
#include "error.h"
#include "log.h"
#include "pacman.h"

pmgrp_t *_pacman_grp_new()
{
	pmgrp_t* group = _pacman_malloc(sizeof(*group));

	if(group != NULL) {
		group->name[0] = '\0';
		group->packages = f_ptrlist_new ();
	}
	return group;
}

void _pacman_grp_delete (pmgrp_t *group)
{
	if(group == NULL) {
		return;
	}

	f_stringlist_delete (group->packages);
	free(group);
}

/* Helper function for sorting groups
 */
int _pacman_grp_cmp(const pmgrp_t *group1, const pmgrp_t *group2) {
	return strcmp (group1->name, group2->name);
}

void _pacman_grp_add (pmgrp_t *group, pmpkg_t *pkg) {
	const char *pkgname;

	assert (group != NULL);
	assert (pkg != NULL);

	pkgname = pkg->name;
	if (f_ptrlist_find (group->packages, pkgname) == NULL) {
		group->packages = f_stringlist_add_sorted(group->packages, pkgname);
	}
}

/* vim: set ts=2 sw=2 noet: */
