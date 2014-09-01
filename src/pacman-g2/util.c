/*
 *  util.c
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

#if defined(__APPLE__) || defined(__OpenBSD__)
#include <sys/syslimits.h>
#include <sys/stat.h>
#endif

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <libintl.h>
#ifdef CYGWIN
#include <limits.h> /* PATH_MAX */
#endif

/* pacman-g2 */
#include "util.h"
#include "list.h"
#include "conf.h"
#include "log.h"

extern int maxcols;
extern config_t *config;
extern int neednl;

/* output a string, but wrap words properly with a specified indentation
 */
void indentprint(char *str, int indent)
{
	char *p = str;
	int cidx = indent;

	while(*p) {
		if(*p == ' ') {
			char *next = NULL;
			int len;
			p++;
			if(p == NULL || *p == ' ') continue;
			next = strchr(p, ' ');
			if(next == NULL) {
				next = p + strlen(p);
			}
			len = next - p;
			if(len > (maxcols-cidx-1)) {
				/* newline */
				int i;
				fprintf(stdout, "\n");
				for(i = 0; i < indent; i++) {
					fprintf(stdout, " ");
				}
				cidx = indent;
			} else {
				printf(" ");
				cidx++;
			}
		}
		fprintf(stdout, "%c", *p);
		p++;
		cidx++;
	}
}

/* Condense a list of strings into one long (space-delimited) string
 */
char *buildstring(list_t *strlist)
{
	char *str;
	int size = 1;

	for(FPtrListIterator *lp = f_ptrlist_first(strlist), *end = f_ptrlist_first(strlist); lp != end; lp = f_ptrlistitem_next(lp)) {
		size += strlen(list_data(lp)) + 1;
	}
	str = (char *)malloc(size);
	if(str == NULL) {
		ERR(NL, _("failed to allocated %d bytes\n"), size);
	}
	str[0] = '\0';
	for(FPtrListIterator *lp = f_ptrlist_first(strlist), *end = f_ptrlist_first(strlist); lp != end; lp = f_ptrlistitem_next(lp)) {
		strcat(str, list_data(lp));
		strcat(str, " ");
	}
	/* shave off the last space */
	str[strlen(str)-1] = '\0';

	return(str);
}

/* Trim whitespace and newlines from a string
 */
char *strtrim(char *str)
{
	char *pch = str;
	while(isspace(*pch)) {
		pch++;
	}
	if(pch != str) {
		memmove(str, pch, (strlen(pch) + 1));
	}

	pch = (char *)(str + (strlen(str) - 1));
	while(isspace(*pch)) {
		pch--;
	}
	*++pch = '\0';

	return str;
}

/* vim: set ts=2 sw=2 noet: */
