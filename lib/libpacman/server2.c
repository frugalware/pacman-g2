/*
 *  server.c
 *
 *  Copyright (c) 2006 by Miklos Vajna <vmiklos@frugalware.org>
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <ftplib.h>
#include <errno.h>

/* pacman-g2 */
#include "config.h"
#include "server.h"
#include "error.h"
#include "log.h"
#include "pacman.h"
#include "util.h"
#include "handle.h"

FtpCallback pm_dlcb = NULL;
/* progress bar */
char *pm_dlfnm=NULL;
int *pm_dloffset=NULL;
struct timeval *pm_dlt0=NULL, *pm_dlt=NULL;
float *pm_dlrate=NULL;
int *pm_dlxfered1=NULL;
unsigned int *pm_dleta_h=NULL, *pm_dleta_m=NULL, *pm_dleta_s=NULL;

void _pacman_server_free(void *data)
{
	struct url *server = data;

	if(server == NULL) {
		return;
	}

	/* free memory */
	fetchFreeURL(server);
}

/*
 * Download a list of files from a list of servers
 *   - if one server fails, we try the next one in the list
 *
 * RETURN:  0 for successful download, -1 on error
 */
int _pacman_downloadfiles(pmlist_t *servers, const char *localpath, pmlist_t *files, int skip)
{
	if(_pacman_downloadfiles_forreal(servers, localpath, files, NULL, NULL, skip) != 0) {
		return(-1);
	} else {
		return(0);
	}
}

/*
 * This is the real downloadfiles, used directly by sync_synctree() to check
 * modtimes on remote files.
 *   - if *mtime1 is non-NULL, then only download files
 *     if they are different than *mtime1.  String should be in the form
 *     "YYYYMMDDHHMMSS" to match the form of ftplib's FtpModDate() function.
 *   - if *mtime2 is non-NULL, then it will be filled with the mtime
 *     of the remote file (from MDTM FTP cmd or Last-Modified HTTP header).
 *
 * RETURN:  0 for successful download
 *          1 if the mtimes are identical
 *         -1 on error
 */
int _pacman_downloadfiles_forreal(pmlist_t *servers, const char *localpath,
	pmlist_t *files, const char *mtime1, char *mtime2, int skip)
{
	int *remain = handle->dlremain;
	int *howmany = handle->dlhowmany;
	pmlist_t *i;
	int count;
	int done = 0;
	struct url *server;
	char *serverurl;
	pmlist_t *lp;
	pmlist_t *complete = NULL;

	if(files == NULL) {
		return (0);
	}

	pm_errno = 0;

	if(remain) {
		*remain = 1;
	}

	if(howmany) {
		*howmany = _pacman_list_count(files);
	}

	unsetenv("FTP_PASSIVE_MODE");

	setenv("FTP_PASSIVE_MODE",(handle->nopassiveftp) ? "no" : "yes",1);

	unsetenv("HTTP_PROXY");

	if(handle->proxyhost) {
		char url[PATH_MAX];
		
		snprintf(url,sizeof(url),
			"http://%s:%d",
			handle->proxyhost,
			(handle->proxyport) ? (int) handle->proxyport : 80
		);
		
		setenv("HTTP_PROXY",url,1);
	}

	for( i = servers, count = 0 ; i && !done ; i = i->next, ++count ) {
		if(count < skip)
			continue;
		
		server = (struct url *) i->data;
		
		serverurl = fetchStringifyURL(server);
		
		for( lp = files ; lp ; lp = lp->next ) {
			char *fn = (char *) lp->data;
			char url[PATH_MAX];
			
			if(_pacman_list_is_strin(fn,complete)) {
				continue;
			}
			
			snprintf(url,sizeof(url),"%s/%s",serverurl,fn);
			
			if(handle->xfercommand && strcmp(server->scheme,SCHEME_FILE)) {
			} else {
				struct url *dlurl;
				struct url_stat dlurl_st;
				char mtime[15];
				char outpath[PATH_MAX];
				struct stat st; 
				off_t offset;
				off_t size;
				fetchIO *in;
				int out;
			
				if((dlurl = fetchParseURL(url)) == NULL) {
					_pacman_log(PM_LOG_WARNING,_("failed to parse url for %s"),fn);
					continue;
				}
			
				if(fetchStat(dlurl,&dlurl_st,"") == -1) {
					_pacman_log(PM_LOG_WARNING,_("failed to get stats for %s"),fn);
					fetchFreeURL(dlurl);
					continue;
				}
			
				if(strftime(mtime,sizeof(mtime),"%Y%m%d%H%M%S",localtime(&dlurl_st.mtime)) == sizeof(mtime)-1) {
					if(mtime1 && !strcmp(mtime1,mtime)) {
						_pacman_log(PM_LOG_DEBUG,_("mtimes are identical, skipping %s\n"),fn);
						fetchFreeURL(dlurl);
						continue;					
					}
				
					if(mtime2) {
						snprintf(mtime2,15,"%s",mtime);
					}
				}
			
				snprintf(outpath,sizeof(outpath),"%s/%s.part",localpath,fn);
			
				dlurl->offset = offset = (off_t) ((!stat(outpath,&st)) ? st.st_size : 0);
			
				size = dlurl_st.size;
			
				if((in = fetchGet(dlurl,"")) == NULL) {
					if(dlurl->offset) {
						_pacman_log(PM_LOG_WARNING,_("failed to resume download -- restarting\n"));
						unlink(outpath);
						dlurl->offset = offset = 0;
						in = fetchGet(dlurl,"");
					}
					
					if(in == NULL) {
						_pacman_log(PM_LOG_WARNING,_("\nfailed downloading %s from %s: %s\n"),fn,server->host,fetchLastErrString);
						fetchFreeURL(dlurl);
						continue;
					}
				}

				if((out = open(outpath,O_WRONLY|O_APPEND|O_CREAT,0644)) == -1) {
					_pacman_log(PM_LOG_WARNING,_("\nfailed to open %s for writing: %s\n"),outpath,strerror(errno));
					fetchIO_close(in);
					fetchFreeURL(dlurl);
					continue;
				}
				
				if(pm_dlfnm) {
					char *s;
					size_t k;
				
					snprintf(pm_dlfnm,PM_DLFNM_LEN,"%s",fn);
					
					for( s = pm_dlfnm ; *s ; ++s ) {
						if(!strncmp(s,PM_EXT_DB,sizeof(PM_EXT_DB)-1)) {
							break;
						}
						
						if(!strncmp(s,PM_EXT_PKG,sizeof(PM_EXT_PKG)-1)) {
							break;
						}
					}
					
					*s = '\0';
					
					for( k = strlen(pm_dlfnm) ; k < PM_DLFNM_LEN ; ++k ) {
						pm_dlfnm[k] = ' ';
					}
					
					pm_dlfnm[k] = '\0';
				}
			
				if(pm_dloffset) {
					*pm_dloffset = (int) offset;
				}
				
				if(pm_dlt0 && pm_dlt) {
					gettimeofday(pm_dlt0,NULL);
					*pm_dlt = *pm_dlt0;
				}
				
				if(pm_dlrate) {
					*pm_dlrate = 0;
				}
				
				if(pm_dlxfered1) {
					*pm_dlxfered1 = 0;
				}
				
				if(pm_dleta_h) {
					*pm_dleta_h = 0;
				}
				
				if(pm_dleta_m) {
					*pm_dleta_m = 0;
				}
				
				if(pm_dleta_s) {
					*pm_dleta_s = 0;
				}
			}
		}
		
		if(_pacman_list_count(files) == pacman_list_count(complete)) {
			done = 1;
		}
		
		free(serverurl);
	}

	unsetenv("FTP_PASSIVE_MODE");
	
	unsetenv("HTTP_PROXY");

	return (-1);
}

char *_pacman_fetch_pkgurl(char *target)
{
	char spath[PATH_MAX], lpath[PATH_MAX], lcache[PATH_MAX];
	char url[PATH_MAX];
	char *host, *path, *fn;
	struct stat buf;

	strncpy(url, target, PATH_MAX);
	host = strstr(url, "://");
	*host = '\0';
	host += 3;
	path = strchr(host, '/');
	*path = '\0';
	path++;
	fn = strrchr(path, '/');
	if(fn) {
		*fn = '\0';
		if(path[0] == '/') {
			snprintf(spath, PATH_MAX, "%s/", path);
		} else {
			snprintf(spath, PATH_MAX, "/%s/", path);
		}
		fn++;
	} else {
		fn = path;
		strcpy(spath, "/");
	}
	snprintf(lcache, PATH_MAX, "%s%s", handle->root, handle->cachedir);
	snprintf(lpath, PATH_MAX, "%s%s/%s", handle->root, handle->cachedir, fn);

	/* do not download the file if it exists in the current dir
	 */
	if(stat(lpath, &buf) == 0) {
		_pacman_log(PM_LOG_DEBUG, _("%s is already in the cache\n"), fn);
	} else {
		pmserver_t *server;
		pmlist_t *servers = NULL;
		pmlist_t *files;

		if((server = _pacman_malloc(sizeof(pmserver_t))) == NULL) {
			return(NULL);
		}
		server->protocol = url;
		server->server = host;
		server->path = spath;
		servers = _pacman_list_add(servers, server);

		files = _pacman_list_add(NULL, fn);
		if(_pacman_downloadfiles(servers, lcache, files, 0)) {
			_pacman_log(PM_LOG_WARNING, _("failed to download %s\n"), target);
			return(NULL);
		}
		FREELISTPTR(files);

		FREELIST(servers);
	}

	/* return the target with the raw filename, no URL */
	#if defined(__OpenBSD__) || defined(__APPLE__)
	return(strdup(lpath));
	#else
	return(strndup(lpath, PATH_MAX));
	#endif
}

/* vim: set ts=2 sw=2 noet: */
