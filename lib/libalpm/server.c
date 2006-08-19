/*
 *  server.c
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libintl.h>

/* pacman */
#include "server.h"
#include "error.h"
#include "log.h"
#include "alpm.h"
#include "util.h"

pmserver_t *_alpm_server_new(char *url)
{
	pmserver_t *server;
	char *ptr;

	server = (pmserver_t *)malloc(sizeof(pmserver_t));
	if(server == NULL) {
		_alpm_log(PM_LOG_ERROR, _("malloc failure: could not allocate %d bytes"), sizeof(pmserver_t));
		RET_ERR(PM_ERR_MEMORY, NULL);
	}

	memset(server, 0, sizeof(pmserver_t));

	/* parse our special url */
	ptr = strstr(url, "://");
	if(ptr == NULL) {
		RET_ERR(PM_ERR_SERVER_BAD_LOCATION, NULL);
	}
	*ptr = '\0';
	ptr++; ptr++; ptr++;
	if(ptr == NULL || *ptr == '\0') {
		RET_ERR(PM_ERR_SERVER_BAD_LOCATION, NULL);
	}
	server->protocol = strdup(url);
	if(!strcmp(server->protocol, "ftp") || !strcmp(server->protocol, "http")) {
		char *slash;
		/* split the url into domain and path */
		slash = strchr(ptr, '/');
		if(slash == NULL) {
			/* no path included, default to / */
			server->path = strdup("/");
		} else {
			/* add a trailing slash if we need to */
			if(slash[strlen(slash)-1] == '/') {
				server->path = strdup(slash);
			} else {
				if((server->path = (char *)malloc(strlen(slash)+2)) == NULL) {
					_alpm_log(PM_LOG_ERROR, _("malloc failure: could not allocate %d bytes"), sizeof(strlen(slash+2)));
					RET_ERR(PM_ERR_MEMORY, NULL);
				}
				sprintf(server->path, "%s/", slash);
			}
			*slash = '\0';
		}
		server->server = strdup(ptr);
	} else if(!strcmp(server->protocol, "file")){
		/* add a trailing slash if we need to */
		if(ptr[strlen(ptr)-1] == '/') {
			server->path = strdup(ptr);
		} else {
			server->path = (char *)malloc(strlen(ptr)+2);
			if(server->path == NULL) {
				_alpm_log(PM_LOG_ERROR, _("malloc failure: could not allocate %d bytes"), sizeof(strlen(ptr+2)));
				RET_ERR(PM_ERR_MEMORY, NULL);
			}
			sprintf(server->path, "%s/", ptr);
		}
	} else {
		RET_ERR(PM_ERR_SERVER_PROTOCOL_UNSUPPORTED, NULL);
	}

	return(server);
}

void _alpm_server_free(void *data)
{
	pmserver_t *server = data;

	if(server == NULL) {
		return;
	}

	/* free memory */
	FREE(server->protocol);
	FREE(server->server);
	FREE(server->path);
	free(server);
}

/* vim: set ts=2 sw=2 noet: */
