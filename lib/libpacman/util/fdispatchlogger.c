/*
 *  fdispatchlogger.c
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

/* pacman-g2 */
#include "util/fdispatchlogger.h"

#include "util/fptrlist.h"

struct FDispatchLogger
{
	FLogger base;
};

static
void f_dispatchlogger_log(unsigned char flag, const char *message, void *data)
{
	FPtrList *list = (FPtrList *)data;
	FPtrListItem *end, *it;

	end = f_ptrlist_end(list);
	for (it = f_ptrlist_first(list); it != end; it = it->next) {
		f_logger_logs((FLogger *)it->data, flag, message);
	}
}

FDispatchLogger *f_dispatchlogger_new(unsigned char mask)
{
	FDispatchLogger *dispatchlogger;

	if ((dispatchlogger = f_zalloc(sizeof(*dispatchlogger))) == NULL) {
			return NULL;
	}
	f_logger_init(&dispatchlogger->base, mask, f_dispatchlogger_log, f_ptrlist_new());
	return dispatchlogger;
}

void f_dispatchlogger_delete(FDispatchLogger *dispatchlogger)
{
	f_logger_fini(&dispatchlogger->base);
	free(dispatchlogger);
}

/* vim: set ts=2 sw=2 noet: */
