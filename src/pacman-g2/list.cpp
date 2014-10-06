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
void list_display(const char *title, const FStringList *list)
{
	FStringList::const_iterator lp, end;
	int cols, len;

	len = strlen(title);
	printf("%s ", title);

	if(list) {
		for(lp = list->begin(), end = list->end(), cols = len; lp != end; ++lp) {
			int s = strlen(*lp)+1;
			if(s+cols >= maxcols) {
				int i;
				cols = len;
				printf("\n");
				for (i = 0; i < len+1; i++) {
					printf(" ");
				}
			}
			printf("%s ", *lp);
			cols += s;
		}
		printf("\n");
	} else {
		printf(_("None\n"));
	}
}

/* Filter out any duplicate strings in a PM_LIST
 *
 * Not the most efficient way, but simple to implement -- we assemble
 * a new list, using is_in() to check for dupes at each iteration.
 *
 * This function takes a PM_LIST* and returns a list_t*
 *
 */
FStringList *PM_LIST_remove_dupes(PM_LIST *list)
{
	FStringList *newlist = NULL;

	for(pmlist_iterator_t *i = pacman_list_begin(list), *end = pacman_list_end(list); i != end; i = pacman_list_next(i)) {
		char *data = pacman_list_getdata(i);
		if(!_pacman_list_is_strin(data, newlist)) {
			newlist = f_stringlist_add(newlist, data);
		}
	}
	return newlist;
}

/* vim: set ts=2 sw=2 noet: */
