/*
 *  list.c
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
#include <string.h>
#include <stdio.h>
#include <libintl.h>
/* pacman-g2 */
#include "util.h"
#include "list.h"

extern int maxcols;

/* Display the content of a list_t struct of strings
 */

void list_display(const char *title, list_t *list)
{
	list_t *lp;
	int cols, len;

	len = strlen(title);
	printf("%s ", title);

	if(list) {
		for(lp = list, cols = len; lp; lp = lp->next) {
			int s = strlen((char *)lp->data)+1;
			if(s+cols >= maxcols) {
				int i;
				cols = len;
				printf("\n");
				for (i = 0; i < len+1; i++) {
					printf(" ");
				}
			}
			printf("%s ", (char *)lp->data);
			cols += s;
		}
		printf("\n");
	} else {
		printf(_("None\n"));
	}
}

/* vim: set ts=2 sw=2 noet: */
