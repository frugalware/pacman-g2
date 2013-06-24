/*
 *  ps.c
 *
 *  Copyright (c) 2011 by Miklos Vajna <vmiklos@frugalware.org>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <libintl.h>
#include <sys/wait.h>
#include <sys/stat.h>

/* pacman-g2 */
#include "util.h"
#include "log.h"
#include "conf.h"
#include "ps.h"

#include "flist.h"

extern config_t *config;

static int start_lsof(FILE** childout, pid_t* childpid, int silent)
{
	char *args[] = { "lsof", "-n", "-FLpcnf0", NULL };
	int pout[2];
	pid_t pid;

	if (pipe(pout) == -1) {
		perror("pipe");
		return -1;
	}
	pid = fork();
	if (pid == -1) {
		perror("fork");
		return -1;
	}
	if (pid == 0) {
		/* we are "in" the child */
		dup2(pout[1], STDOUT_FILENO);
		close(pout[1]);
		close(pout[0]);
		/* silence stderr */
		close(STDERR_FILENO);
		execvp(args[0], args);
		/* on sucess, execv never returns */
		if (!silent)
			ERR(NL, _("failed to execute \"lsof\".\n"));
		exit(1);
	}
	close(pout[1]);
	*childout = fdopen(pout[0], "r");
	if (!*childout) {
		ERR(NL, _("failed to open \"lsof\" output.\n"));
		return -1;
	}
	*childpid = pid;
	return 0;
}

static int end_lsof(FILE* out, pid_t pid)
{
	int status;

	fclose(out);
	waitpid(pid, &status, 0);
	return status == 0 ? 0 : -1;
}

static ps_t* ps_new()
{
	ps_t *ps;

	MALLOC(ps, sizeof(ps_t));

	memset(ps, 0, sizeof(ps_t));

	return ps;
}

static int ps_free(ps_t *ps)
{
	if (ps == NULL)
		return -1;

	FREE(ps->cmd);
	FREE(ps->user);
	FREELIST(ps->files);
	FREELIST(ps->cgroups);
	return 0;
}

static list_t* add_or_free(list_t* l, ps_t* ps)
{
	if (ps) {
		if (f_ptrlist_count (ps->files) > 0) {
			l = list_add(l, ps);
		} else
			ps_free(ps);
	}
	return l;
}

static list_t *ps_cgroup(pid_t pid) {
	struct stat buf;
	char *ptr, path[PATH_MAX+1], line[PATH_MAX+1];
	list_t *ret = NULL;

	snprintf(path, PATH_MAX, "/proc/%d/cgroup", pid);
	if(!stat(path, &buf)) {
		FILE *fp = fopen(path, "r");
		if (!fp)
			return ret;
		while(!feof(fp)) {
			if(fgets(line, PATH_MAX, fp) == NULL)
				break;
			if (line[strlen(line)-2] == '/')
				continue;
			// drop newline
			line[strlen(line)-1] = 0;
			if ((ptr = strchr(line, ':')))
				ret = f_stringlist_add (ret, ptr+1);
		}
	}
	return ret;
}

static list_t* ps_parse(FILE *fp)
{
	char buf[PATH_MAX+1], *ptr;
	ps_t* ps = NULL;
	list_t* ret = f_ptrlist_new ();

	while(!feof(fp)) {
		if(fgets(buf, PATH_MAX, fp) == NULL)
			break;

		if (buf[0] == 'p') {
			ret = add_or_free(ret, ps);
			ps = ps_new();

			ps->pid = atoi(buf+1);
			ps->cgroups = ps_cgroup(ps->pid);
			ptr = buf+strlen(buf)+1;
			ps->cmd = strdup(ptr+1);
			ptr = ptr + strlen(ptr)+1;
			ps->user = strdup(ptr+1);
		} else if (buf[0] == 'f') {
			ptr = buf+1;
			if (!strcmp(ptr, "DEL")) {
				ptr = buf+strlen(buf)+1;
				if (ptr[0] == 'n') {
					/* skip some wellknown nonlibrary memorymapped files */
					int skip = 0;
					if(config->verbose <= 0) {
						static const char * black[] = {
							"/SYSV",
							"/var/run/",
							"/dev/",
							"/drm"
						};
						int i;
						for (i = 0; i < ARRAY_SIZE(black); i++) {
							if (!strncmp(black[i], ptr+1, strlen(black[i]))) {
								skip = 1;
								break;
							}
						}
					}
					if (!skip)
						ps->files = f_stringlist_add (ps->files, ptr+1);
				}
			}
		}
	}

	ret = add_or_free(ret, ps);
	return ret;
}

int pspkg(int countonly)
{
	FILE *fpout = NULL;
	pid_t pid;
	list_t* i;
	int count = 0;

	if (strcmp(config->root, "/") != 0) {
		if (countonly)
			return 0;
		ERR(NL, _("changing root directory is not supported when listing open files.\n"));
		return -1;
	}

	if (start_lsof(&fpout, &pid, countonly) < 0)
		return -1;

	list_t* ret = ps_parse(fpout);

	f_foreach (i, ret) {
		ps_t *ps = i->data;
		if (!ps)
			continue;
		if (!countonly) {
			printf(      _("User    : %s\n"), ps->user);
			printf(      _("PID     : %d\n"), ps->pid);
			printf(      _("Command : %s\n"), ps->cmd);
			list_display(_("Files   :"), ps->files);
			list_display(_("CGroups :"), ps->cgroups);
			ps_free(ps);
			printf("\n");
		}
		count++;
	}
	FREELISTPTR(ret);

	end_lsof(fpout, pid);

	return count;
}

/* vim: set ts=2 sw=2 noet: */
