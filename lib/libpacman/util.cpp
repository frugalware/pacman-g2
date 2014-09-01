/*
 *  util.c
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005 by Christian Hamar <krics@linuxforum.hu>
 *  Copyright (c) 2006 by David Kimpe <dnaku@frugalware.org>
 *  Copyright (c) 2005, 2006 by Miklos Vajna <vmiklos@frugalware.org>
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
#include "util.h"

#include "io/archive.h"
#include "util/log.h"
#include "fstdlib.h"
#include "fstring.h"
#include "trans.h"
#include "sync.h"
#include "error.h"
#include "pacman.h"
#include "pacman_p.h"

#include <sys/wait.h>
#include <sys/statvfs.h>
#include <alloca.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h> /* PATH_MAX */
#include <mntent.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

using namespace libpacman;

/* does the same thing as 'mkdir -p' */
int _pacman_makepath(const char *path)
{
	char *orig, *str, *ptr;
	char full[PATH_MAX] = "";
	mode_t oldmask;

	oldmask = umask(0000);

	orig = strdup(path);
	str = orig;
	while((ptr = strsep(&str, "/"))) {
		if(!_pacman_strempty(ptr)) {
			struct stat buf;

			strcat(full, "/");
			strcat(full, ptr);
			if(stat(full, &buf)) {
				if(mkdir(full, 0755)) {
					free(orig);
					umask(oldmask);
					return(1);
				}
			}
		}
	}
	free(orig);
	umask(oldmask);
	return(0);
}

int _pacman_copyfile(char *src, char *dest)
{
	FILE *in, *out;
	size_t len;
	char buf[4097];

	in = fopen(src, "r");
	if(in == NULL) {
		return(1);
	}
	out = fopen(dest, "w");
	if(out == NULL) {
		fclose(in);
		return(1);
	}

	while((len = fread(buf, 1, 4096, in))) {
		fwrite(buf, 1, len, out);
	}

	fclose(in);
	fclose(out);
	return(0);
}

/* Compression functions
 */

/*
 * 1) scan the old dir
 * 2) scan the compressed archive, extract new entries and mark the
 *    skipped entries in the list
 * 3) delete the entries which are still in the list
 */

typedef struct __cache_t {
	char *str;
	int hit;
} cache_t;

static int list_startswith(char *needle, FPtrList *haystack)
{
	for (auto i = haystack->begin(), end = haystack->end(); i != end; i = i->next()) {
		cache_t *c = f_ptrlistitem_data(i);
		if (!strncmp(c->str, needle, strlen(c->str))) {
			c->hit = 1;
			return(1);
		}
	}
	return(0);
}

int _pacman_unpack(const char *archive, const char *prefix, const char *fn)
{
	register struct archive *_archive;
	struct archive_entry *entry;
	char expath[PATH_MAX];
	FPtrList *cache = NULL;
	DIR *handle;
	struct dirent *ent;
	struct stat buf;

	/* first scan though the old dir to see what package entries do we have */
	handle = opendir(prefix);
	if (handle != NULL) {
		while((ent = readdir(handle)) != NULL) {
			cache_t *c;
			if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
				continue;
			}
			/* we cache only dirs */
			snprintf(expath, PATH_MAX, "%s/%s", prefix, ent->d_name);
			if(stat(expath, &buf) || !S_ISDIR(buf.st_mode)) {
				continue;
			}
			c = _pacman_malloc(sizeof(cache_t));
			if (!c)
				return(-1);
			memset(c, 0, sizeof(cache_t));
			c->str = strdup(ent->d_name);
			cache = cache->add(c);
		}
	}
	closedir(handle);

	/* now extract the new entries */
	if((_archive = _pacman_archive_read_open_all_file(archive)) == NULL) {
		RET_ERR(PM_ERR_PKG_OPEN, -1);
	}
	while (archive_read_next_header (_archive, &entry) == ARCHIVE_OK) {
		if (fn && strcmp (fn, archive_entry_pathname (entry))) {
			if (archive_read_data_skip (_archive) != ARCHIVE_OK)
				return(1);
			continue;
		}
		if (list_startswith((char*)archive_entry_pathname(entry), cache)) {
			continue;
		}
		snprintf(expath, PATH_MAX, "%s/%s", prefix, archive_entry_pathname (entry));
		archive_entry_set_pathname (entry, expath);
		if (archive_read_extract (_archive, entry, ARCHIVE_EXTRACT_FLAGS) != ARCHIVE_OK) {
			fprintf(stderr, _("could not extract %s: %s\n"), archive_entry_pathname (entry), archive_error_string (_archive));
			 return(1);
		}

		if (fn)
			break;
	}

	archive_read_finish (_archive);

	/* finally delete the old ones */
	for (auto i = cache->begin(), end = cache->end(); i != end; i = i->next()) {
		cache_t *c = f_ptrlistitem_data(i);
		if (!c->hit) {
			snprintf(expath, PATH_MAX, "%s/%s", prefix, c->str);
			_pacman_rmrf(expath);
		}
	}
	FREELIST(cache);
	return(0);
}

/* does the same thing as 'rm -rf' */
int _pacman_rmrf(char *path)
{
	int errflag = 0;
	struct dirent *dp;
	DIR *dirp;
	char name[PATH_MAX];

	if(!unlink(path)) {
		return(0);
	} else {
		if(errno == ENOENT) {
			return(0);
		} else if(errno == EPERM) {
			/* fallthrough */
		} else if(errno == EISDIR) {
			/* fallthrough */
		} else if(errno == ENOTDIR) {
			return(1);
		} else {
			/* not a directory */
			return(1);
		}

		if((dirp = opendir(path)) == (DIR *)-1) {
			return(1);
		}
		for(dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
			if(dp->d_ino) {
				sprintf(name, "%s/%s", path, dp->d_name);
				if(strcmp(dp->d_name, "..") && strcmp(dp->d_name, ".")) {
					errflag += _pacman_rmrf(name);
				}
			}
		}
		closedir(dirp);
		if(rmdir(path)) {
			errflag++;
		}
		return(errflag);
	}
	return(0);
}

int _pacman_ldconfig(char *root)
{
	char line[PATH_MAX];
	struct stat buf;

	_pacman_log(PM_LOG_FLOW1, _("running \"ldconfig -r %s\""), root);
	snprintf(line, PATH_MAX, "%setc/ld.so.conf", root);
	if(!stat(line, &buf)) {
		snprintf(line, PATH_MAX, "%ssbin/ldconfig", root);
		if(!stat(line, &buf)) {
			char cmd[PATH_MAX];
			snprintf(cmd, PATH_MAX, "%s -r %s", line, root);
			system(cmd);
		}
	}

	return(0);
}

/* A cheap grep for text files, returns 1 if a substring
 * was found in the text file fn, 0 if it wasn't
 */
static int grep(const char *fn, const char *needle)
{
	FILE *fp;

	if((fp = fopen(fn, "r")) == NULL) {
		return(0);
	}
	while(!feof(fp)) {
		char line[1024];
		int sline = sizeof(line)-1;
		fgets(line, sline, fp);
		if(feof(fp)) {
			continue;
		}
		if(strstr(line, needle)) {
			fclose(fp);
			return(1);
		}
	}
	fclose(fp);
	return(0);
}

int _pacman_chroot_system(const char *cmdline, pmtrans_t *trans)
{
	pid_t pid;

	_pacman_log(PM_LOG_DEBUG, "%s", cmdline);
	pid = fork();
	if(pid == -1) {
		_pacman_log(PM_LOG_ERROR, _("could not fork a new process (%s)"), strerror(errno));
		return 1;
	}
	if(pid == 0) {
		FILE *pp;
		const char *root = trans->m_handle->root;

		_pacman_log(PM_LOG_DEBUG, _("chrooting in %s"), root);
		if(chroot(root) != 0) {
			_pacman_log(PM_LOG_ERROR, _("could not change the root directory (%s)"), strerror(errno));
			exit(1);
		}
		if(chdir("/") != 0) {
			_pacman_log(PM_LOG_ERROR, _("could not change directory to / (%s)"), strerror(errno));
			exit(1);
		}
		umask(0022);
		_pacman_log(PM_LOG_DEBUG, _("executing \"%s\""), cmdline);
		pp = popen(cmdline, "r");
		if(!pp) {
			_pacman_log(PM_LOG_ERROR, _("call to popen failed (%s)"), strerror(errno));
			exit(1);
		}
		while(!feof(pp)) {
			char line[1024];
			int sline = sizeof(line)-1;
			if(fgets(line, sline, pp) == NULL)
				break;
			/* "START <event desc>" */
			if((strlen(line) > strlen(STARTSTR)) && !strncmp(line, STARTSTR, strlen(STARTSTR))) {
				EVENT(trans, PM_TRANS_EVT_SCRIPTLET_START, f_strtrim(line + strlen(STARTSTR)), NULL);
			/* "DONE <ret code>" */
			} else if((strlen(line) > strlen(DONESTR)) && !strncmp(line, DONESTR, strlen(DONESTR))) {
				EVENT(trans, PM_TRANS_EVT_SCRIPTLET_DONE, (void*)atol(f_strtrim(line + strlen(DONESTR))), NULL);
			} else {
				EVENT(trans, PM_TRANS_EVT_SCRIPTLET_INFO, f_strtrim(line), NULL);
			}
		}
		pclose(pp);
		exit(0);
	} else {
		if(waitpid(pid, 0, 0) == -1) {
			_pacman_log(PM_LOG_ERROR, _("call to waitpid failed (%s)"), strerror(errno));
			return 1;
		}
	}
	return 0;
}

int _pacman_runscriptlet(char *root, char *installfn, const char *script, char *ver, char *oldver, pmtrans_t *trans)
{
	char scriptfn[PATH_MAX];
	char cmdline[PATH_MAX];
	char tmpdir[PATH_MAX] = "";
	char *scriptpath;
	struct stat buf;
	int retval = 0;

	if(stat(installfn, &buf)) {
		/* not found */
		return(0);
	}

	if(!strcmp(script, "pre_upgrade") || !strcmp(script, "pre_install")) {
		snprintf(tmpdir, PATH_MAX, "%stmp/", root);
		if(stat(tmpdir, &buf)) {
			_pacman_makepath(tmpdir);
		}
		snprintf(tmpdir, PATH_MAX, "%stmp/pacman_XXXXXX", root);
		if(mkdtemp(tmpdir) == NULL) {
			_pacman_log(PM_LOG_ERROR, _("could not create temp directory"));
			return(1);
		}
		_pacman_unpack(installfn, tmpdir, ".INSTALL");
		snprintf(scriptfn, PATH_MAX, "%s/.INSTALL", tmpdir);
		/* chop off the root so we can find the tmpdir in the chroot */
		scriptpath = scriptfn + strlen(root) - 1;
	} else {
		STRNCPY(scriptfn, installfn, PATH_MAX);
		/* chop off the root so we can find the tmpdir in the chroot */
		scriptpath = scriptfn + strlen(root) - 1;
	}

	if(!grep(scriptfn, script)) {
		/* script not found in scriptlet file */
		goto cleanup;
	}

	_pacman_log(PM_LOG_FLOW2, _("executing %s script..."), script);

	if(oldver) {
		snprintf(cmdline, PATH_MAX, "source %s %s %s %s 2>&1",
				scriptpath, script, ver, oldver);
	} else {
		snprintf(cmdline, PATH_MAX, "source %s %s %s 2>&1",
				scriptpath, script, ver);
	}
	retval = _pacman_chroot_system(cmdline, trans);

cleanup:
	if(!_pacman_strempty(tmpdir) && _pacman_rmrf(tmpdir)) {
		_pacman_log(PM_LOG_WARNING, _("could not remove tmpdir %s"), tmpdir);
	}

	return(retval);
}

int _pacman_runhook(const char *hookname, pmtrans_t *trans)
{
	char *hookdir, *root, *scriptpath;
	char scriptfn[PATH_MAX];
	char hookpath[PATH_MAX];
	char cmdline[PATH_MAX];
	int retval = 0;
	DIR *dir;
	struct dirent *ent;

	hookdir = trans->m_handle->hooksdir;
	root = trans->m_handle->root;

	_pacman_log(PM_LOG_FLOW2, _("executing %s hooks..."), hookname);

	snprintf(hookpath, PATH_MAX, "%s/%s", root, hookdir);

	dir = opendir(hookpath);
	if (!dir) {
		_pacman_log(PM_LOG_ERROR, _("opening hooks directory failed (%s)"), strerror(errno));
		retval = 1;
		goto cleanup;
	}
	while ((ent = readdir(dir)) != NULL) {
		if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
			continue;
		}
		snprintf(scriptfn, PATH_MAX, "%s/%s", hookpath, ent->d_name);
		/* chop off the root so we can find the script in the chroot */
		scriptpath = scriptfn + strlen(root) - 1;
		if(!grep(scriptfn, hookname)) {
			/* script not found in scriptlet file */
			continue;
		}
		snprintf(cmdline, PATH_MAX, "source %s %s", scriptpath, hookname);
		retval = _pacman_chroot_system(cmdline, trans);
	}

cleanup:
	if(dir) {
		closedir(dir);
	}

	return(retval);
}

static long long get_freespace(void)
{
	struct mntent *mnt;
	const char *table = MOUNTED;
	FILE *fp;
	long long ret=0;

	fp = setmntent (table, "r");
	if(!fp)
		return(-1);
	while ((mnt = getmntent (fp)))
	{
		struct statvfs64 buf;

		statvfs64(mnt->mnt_dir, &buf);
		ret += buf.f_bavail * buf.f_bsize;
	}
	endmntent(fp);
	return(ret);
}

int _pacman_check_freespace(pmtrans_t *trans, pmlist_t **data)
{
	long long pkgsize=0, freespace;

	for(auto i = trans->packages->begin(), end = trans->packages->end(); i != end; i = i->next()) {
		Package *pkg = f_ptrlistitem_data(i);
		pkgsize += pkg->size;
	}
	for(auto i = trans->syncpkgs->begin(), end = trans->syncpkgs->end(); i != end; i = i->next()) {
		pmsyncpkg_t *ps = f_ptrlistitem_data(i);

		if(ps->type != PM_SYNC_TYPE_REPLACE) {
			Package *pkg = ps->pkg_new;
			pkgsize += pkg->usize;
		}
	}
	freespace = get_freespace();
	if(freespace < 0) {
		_pacman_log(PM_LOG_WARNING, _("check_freespace: total pkg size: %lld, disk space: unknown"), pkgsize);
		return(0);
	}
	_pacman_log(PM_LOG_DEBUG, _("check_freespace: total pkg size: %lld, disk space: %lld"), pkgsize, freespace);
	if(pkgsize > freespace) {
		if(data) {
			long long *ptr;
			if((ptr = _pacman_malloc(sizeof(long long)))==NULL) {
				return(-1);
			}
			*ptr = pkgsize;
			*data = c_cast(cxx_cast(*data)->add(ptr));
			if((ptr = (long long*)malloc(sizeof(long long)))==NULL) {
				FPtrList *tmp = cxx_cast(*data);
				FREELIST(tmp);
				*data = NULL;
				return(-1);
			}
			*ptr = freespace;
			*data = c_cast(cxx_cast(*data)->add(ptr));
		}
		pm_errno = PM_ERR_DISK_FULL;
		return(-1);
	}
	else {
		return(0);
	}
}

/* match a string against a regular expression */
int _pacman_reg_match(const char *string, const char *pattern)
{
	int result;
	regex_t reg;

	if(regcomp(&reg, pattern, REG_EXTENDED | REG_NOSUB | REG_ICASE) != 0) {
		RET_ERR(PM_ERR_INVALID_REGEX, -1);
	}
	result = regexec(&reg, string, 0, 0, 0);
	regfree(&reg);
	return(!(result));
}

/* vim: set ts=2 sw=2 noet: */
