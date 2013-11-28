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

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libintl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <curl.h>

/* pacman-g2 */
#include "server.h"

#include "util/list.h"
#include "util/log.h"
#include "util/stringlist.h"
#include "util/time.h"
#include "error.h"
#include "pacman.h"
#include "util.h"
#include "handle.h"

pacman_trans_cb_download pm_dlcb = NULL;
/* progress bar */
char *pm_dlfnm=NULL;
struct timeval *pm_dlt=NULL;
int *pm_dlxfered1=NULL;

pmserver_t *_pacman_server_new(char *url)
{
	pmserver_t *server = _pacman_zalloc(sizeof(pmserver_t));
	char *ptr;

	if(server == NULL) {
		return(NULL);
	}

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
				if((server->path = _pacman_malloc(strlen(slash)+2)) == NULL) {
					return(NULL);
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
			server->path = _pacman_malloc(strlen(ptr)+2);
			if(server->path == NULL) {
				return(NULL);
			}
			sprintf(server->path, "%s/", ptr);
		}
	} else {
		RET_ERR(PM_ERR_SERVER_PROTOCOL_UNSUPPORTED, NULL);
	}

	return(server);
}

void _pacman_server_free(void *data)
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

typedef struct __pmcurldownloader_t pmcurldownloader_t;
typedef enum __pmdownloadsuccess_t pmdownloadsuccess_t;

struct __pmcurldownloader_t {
	CURL *curl;
	struct timeval previous_update;
	size_t previous_update_dltotal;
	pmdownload_t download;
};

enum __pmdownloadsuccess_t {
	PM_DOWNLOAD_ERROR = -1,
	PM_DOWNLOAD_OK = 0,
	PM_DOWNLOAD_OK_CACHE = 1,
};

/*
 * Progress callback used by libcurl, we then pass our own progress function
 */
static
int _pacman_curl_progresscb(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	if(dltotal > 0 && dlnow > 0) {
		pmcurldownloader_t *curldownloader = clientp;
		pmdownload_t *download = &curldownloader->download;
		struct timeval now;
		double time_delta;

		gettimeofday(&now, NULL);
		download->dst_tell = download->dst_resume + dlnow;
		download->dst_size = download->dst_resume + dltotal;
		if((time_delta = f_difftimeval(curldownloader->previous_update, now)) > 1) {
			curldownloader->download.dst_avg = dltotal / f_difftimeval(curldownloader->download.dst_begin, now);
			curldownloader->download.dst_rate = (curldownloader->previous_update_dltotal - dltotal) / time_delta;
			curldownloader->download.dst_eta = (download->dst_tell - download->dst_size) / curldownloader->download.dst_rate;
			curldownloader->previous_update = now;
			curldownloader->previous_update_dltotal = dltotal;
		}

		pm_dlcb(download);
	}
	return 0;
}

static
int _pacman_curl_init(pmcurldownloader_t *curldownloader) {
	CURL *curlHandle = NULL;

	ASSERT(curldownloader, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
		_pacman_log(PM_LOG_WARNING, _("fatal error initializing libcurl\n"));
		goto error;
	}
	curlHandle = curl_easy_init();
	if(!curlHandle) {
		_pacman_log(PM_LOG_WARNING, _("fatal error initializing libcurl\n"));
		pm_errno = PM_ERR_CONNECT_FAILED;
		goto error;
	} else {
		if(curl_easy_setopt(curlHandle,CURLOPT_NOPROGRESS , 0) != CURLE_OK) {
			_pacman_log(PM_LOG_DEBUG, _("error setting noprogress off\n"));
			goto error;
		}
		if(curl_easy_setopt(curlHandle,CURLOPT_PROGRESSDATA , (void *)&curldownloader) != CURLE_OK) {
			_pacman_log(PM_LOG_DEBUG, _("error passing our debug function pointer to progress callback\n"));
			goto error;
		}
		if(curl_easy_setopt(curlHandle, CURLOPT_PROGRESSFUNCTION, _pacman_curl_progresscb) != CURLE_OK) {
			_pacman_log(PM_LOG_DEBUG, _("error setting progress bar\n"));
			goto error;
		}
		if(handle->proxyhost) {
			if(!handle->proxyport) {
				_pacman_log(PM_LOG_WARNING, _("pacman proxy setting needs a port specified url:port\n"));
				pm_errno = PM_ERR_CONNECT_FAILED;
				goto error;
			}
			char pacmanProxy[PATH_MAX];
			sprintf(pacmanProxy, "%s:%d", handle->proxyhost, handle->proxyport);
			if(curl_easy_setopt(curlHandle,CURLOPT_PROXY, pacmanProxy) != CURLE_OK) {
				_pacman_log(PM_LOG_WARNING, _("error setting proxy\n"));
				pm_errno = PM_ERR_CONNECT_FAILED;
				goto error;
			}
		}
	}
	curldownloader->curl = curlHandle;
	return 0;

error:
	curl_easy_cleanup(curlHandle);
	return -1;
}

static
pmdownloadsuccess_t _pacman_curl_download(pmcurldownloader_t *curldownloader, const char *url,
		const time_t *mtime1, time_t *mtime2, const char *output)
{
	CURL *curlHandle = curldownloader->curl;
	FILE *outputFile;
	CURLcode retc = CURLE_OK;
	pmdownloadsuccess_t ret = PM_DOWNLOAD_OK;

	/* ETA setup */
	gettimeofday(&curldownloader->download.dst_begin, NULL);
	if(pm_dlt && pm_dlxfered1) {
		*pm_dlt = curldownloader->download.dst_begin;
		*pm_dlxfered1 = 0;
	}

	if(mtime1 && mtime2 && !handle->proxyhost) {
		curl_easy_setopt(curlHandle, CURLOPT_FILETIME, 1);
		curl_easy_setopt(curlHandle, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
		curl_easy_setopt(curlHandle, CURLOPT_TIMEVALUE , mtime1);
		outputFile = fopen(output,"wb");
	} else {
		struct stat st;

		curl_easy_setopt(curlHandle, CURLOPT_FILETIME, 0);
		curl_easy_setopt(curlHandle, CURLOPT_TIMECONDITION, CURL_TIMECOND_NONE);
		if(!stat(output, &st)) {
			curldownloader->download.dst_resume = st.st_size;
			curl_easy_setopt(curlHandle, CURLOPT_RESUME_FROM, curldownloader->download.dst_resume);
			outputFile = fopen(output,"ab");
		} else {
			curl_easy_setopt(curlHandle, CURLOPT_RESUME_FROM, 0);
			outputFile = fopen(output,"wb");
		}
	}
	if(!outputFile){
		_pacman_log(PM_LOG_WARNING, _("error opening output file: %s\n"), output);
		pm_errno = PM_ERR_BADPERMS;
		goto error;
	}
	//set libcurl options
	retc = curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, outputFile);
	if(retc != CURLE_OK) {
		_pacman_log(PM_LOG_WARNING, _("error setting output file: %s\n"), output);
		pm_errno = PM_ERR_RETRIEVE;
		goto error;
	}
	retc = curl_easy_setopt(curlHandle, CURLOPT_URL, url);
	if(retc != CURLE_OK) {
		_pacman_log(PM_LOG_WARNING, _("error setting url: %s\n"), url);
		pm_errno = PM_ERR_RETRIEVE;
		goto error;
	}
	retc = curl_easy_perform(curlHandle);
	if(retc != CURLE_OK) {
		_pacman_log(PM_LOG_WARNING, _("error downloading file: %s\n"), url);
		pm_errno = PM_ERR_RETRIEVE;
		goto error;
	}
	if(mtime2) {
		long int rcCode = 0;
		curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &rcCode);
		double downSize = 0;
		curl_easy_getinfo(curlHandle, CURLINFO_SIZE_DOWNLOAD, &downSize);
		if(rcCode==213 || rcCode==304) {
			//return codes for when timestamp was the same (FTP and HTTP)
			//also check for download size as in some cases (proxy) ret codes won't work
			ret = PM_DOWNLOAD_OK_CACHE;
		} else {
			*mtime2 = PM_TIME_INVALID;
			retc = curl_easy_getinfo(curlHandle, CURLINFO_FILETIME,  mtime2);
		}
	}
	fclose(outputFile);
	return ret;

error:
	fclose(outputFile);
	return PM_DOWNLOAD_ERROR;
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
	pmlist_t *files, const time_t *mtime1, time_t *mtime2, int skip)
{
	pmlist_t *lp;
	int done = 0;
	pmlist_t *complete = NULL;
	pmlist_t *i;
	pmserver_t *server;
	CURL * curlHandle = NULL;
	int *remain = handle->dlremain, *howmany = handle->dlhowmany;

	if(files == NULL) {
		return(0);
	}

	if(howmany) {
		*howmany = _pacman_list_count(files);
	}
	if(remain) {
		*remain = 1;
	}

	if(mtime1 && *mtime1 == PM_TIME_INVALID) {
		mtime1 = NULL;
	}

	_pacman_log(PM_LOG_DEBUG, _("server check, %d\n"),servers);
	int count;
	for(i = servers, count = 0; i && !done; i = i->next, count++) {
		pm_errno = 0;
		if (count < skip)
			continue; /* the caller requested skip of this server */
		_pacman_log(PM_LOG_DEBUG, _("server check, done? %d\n"),done);
		server = (pmserver_t*)i->data;

		/* get each file in the list */
		for(lp = files; lp; lp = lp->next) {
			char *fn = (char *)lp->data;

			if(_pacman_list_is_strin(fn, complete)) {
				continue;
			}

			if(handle->xfercommand && strcmp(server->protocol, "file")) {
				int ret;
				int usepart = 0;
				char *ptr1, *ptr2;
				char origCmd[PATH_MAX];
				char parsedCmd[PATH_MAX] = "";
				char url[PATH_MAX];
				char cwd[PATH_MAX];
				/* build the full download url */
				snprintf(url, PATH_MAX, "%s://%s%s%s", server->protocol, server->server,
								 server->path, fn);
				/* replace all occurrences of %o with fn.part */
				strncpy(origCmd, handle->xfercommand, sizeof(origCmd));
				ptr1 = origCmd;
				while((ptr2 = strstr(ptr1, "%o"))) {
					usepart = 1;
					ptr2[0] = '\0';
					strcat(parsedCmd, ptr1);
					strcat(parsedCmd, fn);
					strcat(parsedCmd, ".part");
					ptr1 = ptr2 + 2;
				}
				strcat(parsedCmd, ptr1);
				/* replace all occurrences of %u with the download URL */
				strncpy(origCmd, parsedCmd, sizeof(origCmd));
				parsedCmd[0] = '\0';
				ptr1 = origCmd;
				while((ptr2 = strstr(ptr1, "%u"))) {
					ptr2[0] = '\0';
					strcat(parsedCmd, ptr1);
					strcat(parsedCmd, url);
					ptr1 = ptr2 + 2;
				}
				strcat(parsedCmd, ptr1);
				/* replace all occurrences of %c with the current position */
				if (remain) {
					strncpy(origCmd, parsedCmd, sizeof(origCmd));
					parsedCmd[0] = '\0';
					ptr1 = origCmd;
					while((ptr2 = strstr(ptr1, "%c"))) {
						char numstr[PATH_MAX];
						ptr2[0] = '\0';
						strcat(parsedCmd, ptr1);
						snprintf(numstr, PATH_MAX, "%d", *remain);
						strcat(parsedCmd, numstr);
						ptr1 = ptr2 + 2;
					}
					strcat(parsedCmd, ptr1);
				}
				/* replace all occurrences of %t with the current position */
				if (howmany) {
					strncpy(origCmd, parsedCmd, sizeof(origCmd));
					parsedCmd[0] = '\0';
					ptr1 = origCmd;
					while((ptr2 = strstr(ptr1, "%t"))) {
						char numstr[PATH_MAX];
						ptr2[0] = '\0';
						strcat(parsedCmd, ptr1);
						snprintf(numstr, PATH_MAX, "%d", *howmany);
						strcat(parsedCmd, numstr);
						ptr1 = ptr2 + 2;
					}
					strcat(parsedCmd, ptr1);
				}
				/* cwd to the download directory */
				getcwd(cwd, PATH_MAX);
				if(chdir(localpath)) {
					_pacman_log(PM_LOG_WARNING, _("could not chdir to %s\n"), localpath);
					pm_errno = PM_ERR_CONNECT_FAILED;
					goto error;
				}
				/* execute the parsed command via /bin/sh -c */
				_pacman_log(PM_LOG_DEBUG, _("running command: %s\n"), parsedCmd);
				ret = system(parsedCmd);
				if(ret == -1) {
					_pacman_log(PM_LOG_WARNING, _("running XferCommand: fork failed!\n"));
					pm_errno = PM_ERR_FORK_FAILED;
					goto error;
				} else if(ret != 0) {
					/* download failed */
					_pacman_log(PM_LOG_DEBUG, _("XferCommand command returned non-zero status code (%d)\n"), ret);
				} else {
					/* download was successful */
					complete = _pacman_list_add(complete, fn);
					if(usepart) {
						char fnpart[PATH_MAX];
						/* rename "output.part" file to "output" file */
						snprintf(fnpart, PATH_MAX, "%s.part", fn);
						rename(fnpart, fn);
					}
				}
				chdir(cwd);
			} else {
				char output[PATH_MAX];
				unsigned int j;
				char *ptr;
				struct stat st;

				snprintf(output, PATH_MAX, "%s/%s.part", localpath, fn);
				if(pm_dlfnm) {
					strncpy(pm_dlfnm, fn, PM_DLFNM_LEN);
				}
				/* drop filename extension */
				ptr = strstr(fn, PM_EXT_DB);
				if(pm_dlfnm && ptr && (ptr-fn) < PM_DLFNM_LEN) {
					pm_dlfnm[ptr-fn] = '\0';
				}
				ptr = strstr(fn, PM_EXT_PKG);
				if(ptr && (ptr-fn) < PM_DLFNM_LEN) {
					pm_dlfnm[ptr-fn] = '\0';
				}

				if(!strcmp(server->protocol, "file")) {
					char src[PATH_MAX];
					snprintf(src, PATH_MAX, "%s%s", server->path, fn);
					_pacman_makepath((char*)localpath);
					_pacman_log(PM_LOG_DEBUG, _("copying %s to %s/%s\n"), src, localpath, fn);
					/* local repository, just copy the file */
					if(_pacman_copyfile(src, output)) {
						_pacman_log(PM_LOG_WARNING, _("failed copying %s\n"), src);
					}
				} else {
					//download files using libcurl
					CURLcode retc = CURLE_OK;
					pmcurldownloader_t curldownloader = { 0 };
					char url[PATH_MAX];

					/* build the full download url */
					snprintf(url, PATH_MAX, "%s://%s%s%s", server->protocol, server->server,
									 server->path, fn);

					_pacman_curl_init(&curldownloader);
					switch(_pacman_curl_download(&curldownloader, url, mtime1, mtime2, output)) {
					case PM_DOWNLOAD_ERROR:
						goto error;
					case PM_DOWNLOAD_OK: {
						char completefile[PATH_MAX];
						if(!strcmp(server->protocol, "file")) {
							EVENT(handle->trans, PM_TRANS_EVT_RETRIEVE_LOCAL, pm_dlfnm, server->path);
						} else if(pm_dlcb) {
							pm_dlcb(&curldownloader.download);
						}
						complete = _pacman_list_add(complete, fn);
						/* rename "output.part" file to "output" file */
						snprintf(completefile, PATH_MAX, "%s/%s", localpath, fn);
						rename(output, completefile);
						}
						break;
					case PM_DOWNLOAD_OK_CACHE:
						/* -1 means here that the file is up to date, not a real error, so
										 * don't go to error: */
						remove(output);
						FREELISTPTR(complete);
						return(1);
					}
				}
			}
			if(remain) {
				(*remain)++;
			}
		}

		if(_pacman_list_count(complete) == _pacman_list_count(files)) {
			done = 1;
		}
	}
	_pacman_log(PM_LOG_DEBUG, _("end _pacman_downloadfiles_forreal - return %d"),!done);

error:
	FREELISTPTR(complete);
	if(curlHandle) {
		curl_easy_cleanup(curlHandle);
		curlHandle = NULL;
	}
	return(pm_errno == 0 ? !done : -1);
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
