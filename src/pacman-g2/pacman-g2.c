/*
 *  pacman-g2.c
 * 
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2007-2008 by Miklos Vajna <vmiklos@frugalware.org>
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
#include <stdio.h>
#include <limits.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <libintl.h>
#include <locale.h>
#include <mcheck.h> /* debug */
#include <time.h>

#include <pacman.h>
/* pacman-g2 */
#include "list.h"
#include "util.h"
#include "log.h"
#include "download.h"
#include "conf.h"
#include "package.h"
#include "add.h"
#include "remove.h"
#include "upgrade.h"
#include "query.h"
#include "sync.h"
#include "deptest.h"
#include "ps.h"

#define PACCONF "/etc/pacman-g2.conf"

/* Operations */
enum {
	PM_OP_MAIN = 1,
	PM_OP_ADD,
	PM_OP_REMOVE,
	PM_OP_UPGRADE,
	PM_OP_QUERY,
	PM_OP_SYNC,
	PM_OP_DEPTEST,
	PM_OP_PS
};

/* Long operations */
enum {
	OPT_NOCONFIRM = 1000,
	OPT_CONFIG,
	OPT_IGNORE,
	OPT_DEBUG,
	OPT_NOPROGRESSBAR,
	OPT_NOSCRIPTLET,
	OPT_ASK,
	OPT_NOINTEGRITY,
	OPT_NOARCH,
	OPT_REGEX
};

config_t *config = NULL;

PM_DB *db_local;
/* list of (PM_DB *) structs for sync locations */
list_t *pmc_syncs = NULL;
/* list of targets specified on command line */
list_t *pm_targets  = NULL;

unsigned int maxcols = 80;

extern int neednl;

/* Display usage/syntax for the specified operation.
 *     op:     the operation code requested
 *     myname: basename(argv[0])
 */
static void usage(int op, char *myname)
{
	if(op == PM_OP_MAIN) {
		printf(_("usage:  %s {-h --help}\n"), myname);
		printf(_("        %s {-V --version}\n"), myname);
		printf(_("        %s {-A --add}     [options] <file>\n"), myname);
		printf(_("        %s {-R --remove}  [options] <package>\n"), myname);
		printf(_("        %s {-U --upgrade} [options] <file>\n"), myname);
		printf(_("        %s {-F --freshen} [options] <file>\n"), myname);
		printf(_("        %s {-Q --query}   [options] [package]\n"), myname);
		printf(_("        %s {-S --sync}    [options] [package]\n"), myname);
		printf(_("        %s {-P --ps}      [options]\n"), myname);
		printf(_("\nuse '%s --help' with other options for more syntax\n"), myname);
	} else {
		if(op == PM_OP_ADD) {
			printf(_("usage:  %s {-A --add} [options] <file>\n"), myname);
			printf(_("options:\n"));
			printf(_("  -d, --nodeps        skip dependency checks\n"));
			printf(_("  -f, --force         force install, overwrite conflicting files\n"));
			printf(_("      --noarch        install the package, even if it is for an other arch\n"));
		} else if(op == PM_OP_REMOVE) {
			printf(_("usage:  %s {-R --remove} [options] <package>\n"), myname);
			printf(_("options:\n"));
			printf(_("  -c, --cascade       remove packages and all packages that depend on them\n"));
			printf(_("  -d, --nodeps        skip dependency checks\n"));
			printf(_("  -k, --dbonly        only remove database entry, do not remove files\n"));
			printf(_("  -n, --nosave        remove configuration files as well\n"));
			printf(_("  -s, --recursive     remove dependencies also (that won't break packages)\n"));
		} else if(op == PM_OP_UPGRADE) {
			if(config->flags & PM_TRANS_FLAG_FRESHEN) {
				printf(_("usage:  %s {-F --freshen} [options] <file>\n"), myname);
			} else {
				printf(_("usage:  %s {-U --upgrade} [options] <file>\n"), myname);
			}
			printf(_("options:\n"));
			printf(_("  -d, --nodeps        skip dependency checks\n"));
			printf(_("  -f, --force         force install, overwrite conflicting files\n"));
		} else if(op == PM_OP_QUERY) {
			printf(_("usage:  %s {-Q --query} [options] [package]\n"), myname);
			printf(_("options:\n"));
			printf(_("  -c, --changelog     view the changelog of a package\n"));
			printf(_("  -d, --nodeps        if used with -e, list packages that were not installed\n"));
			printf(_("                      as a dependency\n"));
			printf(_("  -e, --orphans       list all packages that were installed as a dependency\n"));
			printf(_("                      and are not required by any other packages\n"));
			printf(_("  -f, --fsck          check the integrity of packages' files\n"));
			printf(_("  -g, --groups        view all members of a package group\n"));
			printf(_("  -i, --info          view package information\n"));
			printf(_("  -l, --list          list the contents of the queried package\n"));
			printf(_("  -m, --foreign       list all packages that were not found in the sync db(s)\n"));
			printf(_("  -o, --owns <file>   query the package that owns <file>\n"));
			printf(_("  -p, --file          pacman-g2 will query the package file [package] instead of\n"));
			printf(_("                      looking in the database\n"));
			printf(_("  -s, --search        search locally-installed packages for matching strings\n"));
			printf(_("  -t, --test          search problems in the local database\n"));
		} else if(op == PM_OP_SYNC) {
			printf(_("usage:  %s {-S --sync} [options] [package]\n"), myname);
			printf(_("options:\n"));
			printf(_("  -c, --clean         remove old packages from cache directory (use -cc for all)\n"));
			printf(_("  -d, --nodeps        skip dependency checks\n"));
			printf(_("  -e, --dependsonly   install dependencies only\n"));
			printf(_("  -f, --force         force install, overwrite conflicting files\n"));
			printf(_("  -g, --groups        view all members of a package group\n"));
			printf(_("  -p, --print-uris    print out URIs for given packages and their dependencies\n"));
			printf(_("  -s, --search        search remote repositories for matching strings\n"));
			printf(_("  -u, --sysupgrade    upgrade all packages that are out of date\n"));
			printf(_("  -w, --downloadonly  download packages but do not install/upgrade anything\n"));
			printf(_("  -y, --refresh       download fresh package databases from the server\n"));
			printf(_("      --ignore <pkg>  ignore a package upgrade (can be used more than once)\n"));
			printf(_("      --nointegrity   don't check the integrity of the packages using sha1\n"));
		}
		printf(_("      --config <path> set an alternate configuration file\n"));
		printf(_("      --noconfirm     do not ask for anything confirmation\n"));
		printf(_("      --ask  <number> pre-specify answers for questions (see manpage)\n"));
		printf(_("      --noprogressbar do not show a progress bar when downloading files\n"));
		printf(_("      --noscriptlet   do not execute the install scriptlet if there is any\n"));
		printf(_("      --regex         treat targets as regexs if no package found\n"));
		printf(_("  -v, --verbose       be verbose\n"));
		printf(_("  -r, --root <path>   set an alternate installation root\n"));
		printf(_("  -b, --dbpath <path> set an alternate database location\n"));
	}
}

/* Version
 */
static void version(void)
{
	printf("\n");
	printf(" .--.                  Pacman-G2 v%s - libpacman v%s\n", PACKAGE_VERSION, PM_VERSION);
	printf("/ _.-' .-.  .-.  .-.   Copyright (C) 2002-2018 Pacman-G2 Team\n");
	printf("\\  '-. '-'  '-'  '-'   See /usr/share/doc/pacman-g2-%s/AUTHORS for more info.\n", PACKAGE_VERSION);
	printf(" '--'                  \n");
	printf(_("                       This program may be freely redistributed under\n"));
	printf(_("                       the terms of the GNU General Public License\n"));
	printf("\n");
}

static void cleanup(int exitcode)
{
	if(exitcode != 0 && config->op_d_vertest == 0) {
		fprintf(stderr, "\n");
	}

	/* free libpacman library resources */
	if(pacman_release() == -1) {
		ERR(NL, "%s\n", pacman_strerror(pm_errno));
	}

	/* free memory */
	FREELISTPTR(pmc_syncs);
	FREELIST(pm_targets);
	FREECONF(config);

#ifndef CYGWIN
#ifndef BSD
	/* debug */
	muntrace();
#endif
#endif

	if(neednl) {
		putchar('\n');
	}
	fflush(stdout);

	exit(exitcode);
}

static void signal_callback(int signum)
{
	if(signum==SIGSEGV)
	{
		fprintf(stderr, "Internal pacman-g2 error: Segmentation fault\n"
			"Please submit a full bug report, with the given package if appropriate.\n");
		exit(signum);
	} else if((signum == SIGINT) && (pacman_trans_release() == -1) &&
			((pm_errno == PM_ERR_TRANS_COMMITING) || (pm_errno == PM_ERR_TRANS_DOWNLOADING))) {
		if(pm_errno == PM_ERR_TRANS_DOWNLOADING)
			config->dl_interrupted = 1;
		return;
	}

	cleanup(signum);
}


/* Parse command-line arguments for each operation
 *     argc: argc
 *     argv: argv
 *     
 * Returns: 0 on success, 1 on error
 */
static int parseargs(int argc, char *argv[])
{
	int opt;
	int option_index = 0;
	static struct option opts[] =
	{
		{"add",        no_argument,       0, 'A'},
		{"resolve",    no_argument,       0, 'D'}, /* used by 'makepkg -s' */
		{"freshen",    no_argument,       0, 'F'},
		{"query",      no_argument,       0, 'Q'},
		{"remove",     no_argument,       0, 'R'},
		{"sync",       no_argument,       0, 'S'},
		{"ps",         no_argument,       0, 'P'},
		{"deptest",    no_argument,       0, 'T'}, /* used by makepkg */
		{"upgrade",    no_argument,       0, 'U'},
		{"version",    no_argument,       0, 'V'},
		{"vertest",    no_argument,       0, 'Y'}, /* does the same as the 'vercmp' binary */
		{"dbpath",     required_argument, 0, 'b'},
		{"cascade",    no_argument,       0, 'c'},
		{"changelog",  no_argument,       0, 'c'},
		{"clean",      no_argument,       0, 'c'},
		{"nodeps",     no_argument,       0, 'd'},
		{"dependsonly",no_argument,       0, 'e'},
		{"orphans",    no_argument,       0, 'e'},
		{"force",      no_argument,       0, 'f'},
		{"fsck",       no_argument,       0, 'f'},
		{"groups",     no_argument,       0, 'g'},
		{"help",       no_argument,       0, 'h'},
		{"info",       no_argument,       0, 'i'},
		{"dbonly",     no_argument,       0, 'k'},
		{"list",       no_argument,       0, 'l'},
		{"nosave",     no_argument,       0, 'n'},
		{"foreign",    no_argument,       0, 'm'},
		{"owns",       no_argument,       0, 'o'},
		{"file",       no_argument,       0, 'p'},
		{"print-uris", no_argument,       0, 'p'},
		{"root",       required_argument, 0, 'r'},
		{"recursive",  no_argument,       0, 's'},
		{"search",     no_argument,       0, 's'},
		{"sysupgrade", no_argument,       0, 'u'},
		{"test",       no_argument,       0, 't'},
		{"verbose",    no_argument,       0, 'v'},
		{"downloadonly", no_argument,     0, 'w'},
		{"refresh",    no_argument,       0, 'y'},
		{"noconfirm",  no_argument,       0, OPT_NOCONFIRM},
		{"config",     required_argument, 0, OPT_CONFIG},
		{"ignore",     required_argument, 0, OPT_IGNORE},
		{"debug",      required_argument, 0, OPT_DEBUG},
		{"noprogressbar",  no_argument,   0, OPT_NOPROGRESSBAR},
		{"noscriptlet", no_argument,      0, OPT_NOSCRIPTLET},
		{"ask",        required_argument, 0, OPT_ASK},
		{"nointegrity", no_argument,      0, OPT_NOINTEGRITY},
		{"noarch", no_argument, 0, OPT_NOARCH},
		{"regex", no_argument, 0, OPT_REGEX},
		{0, 0, 0, 0}
	};
	char root[PATH_MAX];

	while((opt = getopt_long(argc, argv, "ARUFQSPTDYr:b:vkhscVfmnoldepituwyg", opts, &option_index))) {
		if(opt < 0) {
			break;
		}
		switch(opt) {
			case 0: break;
			case OPT_NOCONFIRM: config->noconfirm = 1; break;
			case OPT_CONFIG:
				if(config->configfile) {
					free(config->configfile);
				}
				#if defined(__OpenBSD__) || defined(__APPLE__)
				config->configfile = strdup(optarg);
				#else
				config->configfile = strndup(optarg, PATH_MAX);
				#endif
			break;
			case OPT_IGNORE: config->op_s_ignore = list_add(config->op_s_ignore, strdup(optarg)); break;
			case OPT_DEBUG: config->debug = atoi(optarg); break;
			case OPT_NOPROGRESSBAR: config->noprogressbar = 1; break;
			case OPT_NOSCRIPTLET: config->flags |= PM_TRANS_FLAG_NOSCRIPTLET; break;
			case OPT_ASK: config->noask = 1; config->ask = atoi(optarg); break;
			case OPT_NOINTEGRITY: config->flags |= PM_TRANS_FLAG_NOINTEGRITY; break;
			case OPT_NOARCH: config->flags |= PM_TRANS_FLAG_NOARCH; break;
			case OPT_REGEX: config->regex = 1; break;
			case 'A': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_ADD); break;
			case 'D':
				config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_DEPTEST);
				config->op_d_resolve = 1;
				config->flags |= PM_TRANS_FLAG_ALLDEPS;
			break;
			case 'F':
				config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_UPGRADE);
				config->flags |= PM_TRANS_FLAG_FRESHEN;
			break;
			case 'Q': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_QUERY); break;
			case 'R': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_REMOVE); break;
			case 'S': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_SYNC); break;
			case 'P': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_PS); break;
			case 'T': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_DEPTEST); break;
			case 'U': config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_UPGRADE); break;
			case 'V': config->version = 1; break;
			case 'Y':
				config->op = (config->op != PM_OP_MAIN ? 0 : PM_OP_DEPTEST);
				config->op_d_vertest = 1;
			break;
			case 'b':
				if(config->dbpath) {
					free(config->dbpath);
				}
				config->dbpath = strdup(optarg);
			break;
			case 'c':
				config->op_s_clean++;
				config->flags |= PM_TRANS_FLAG_CASCADE;
				config->op_q_changelog = 1;
			break;
			case 'd': config->flags |= PM_TRANS_FLAG_NODEPS; config->op_q_orphans_deps = 1; break;
			case 'e': config->op_q_orphans = 1; config->flags |= PM_TRANS_FLAG_DEPENDSONLY; break;
			case 'f': config->flags |= PM_TRANS_FLAG_FORCE; config->op_q_fsck = 1; break;
			case 'g': config->group++; break;
			case 'h': config->help = 1; break;
			case 'i':
				config->op_q_info++;
				config->op_s_info++;
			break;
			case 'k': config->flags |= PM_TRANS_FLAG_DBONLY; break;
			case 'l': config->op_q_list = 1; break;
			case 'm': config->op_q_foreign = 1; break;
			case 'n': config->flags |= PM_TRANS_FLAG_NOSAVE; break;
			case 'o': config->op_q_owns = 1; break;
			case 'p':
				config->op_q_isfile = 1;
				if (config->flags & PM_TRANS_FLAG_PRINTURIS) {
					config->flags |= PM_TRANS_FLAG_PRINTURIS_CACHED;
				}
				config->flags |= PM_TRANS_FLAG_PRINTURIS;
			break;
			case 'r':
				if(realpath(optarg, root) == NULL) {
					perror(_("bad root path"));
					return(1);
				}
				if(config->root) {
					free(config->root);
				}
				config->root = strdup(root);
			break;
			case 's':
				config->op_s_search = 1;
				config->op_q_search = 1;
				config->flags |= PM_TRANS_FLAG_RECURSE;
			break;
			case 'u':
				config->op_s_upgrade++;
				if(config->op_s_upgrade == 2){
					config->flags |= PM_TRANS_FLAG_DOWNGRADE;
				}
			break;
			case 't': config->op_q_test = 1; break;
			case 'v': config->verbose++; break;
			case 'w':
				config->op_s_downloadonly = 1;
				config->flags |= PM_TRANS_FLAG_DOWNLOADONLY;
				config->flags |= PM_TRANS_FLAG_NOCONFLICTS;
			break;
			case 'y': config->op_s_sync++; break;
			case '?': return(1);
			default: return(1);
		}
	}

	if(config->op == 0) {
		ERR(NL, _("only one operation may be used at a time\n"));
		return(1);
	}

	if(config->help) {
		usage(config->op, basename(argv[0]));
		return(2);
	}
	if(config->version) {
		version();
		return(2);
	}

	while(optind < argc) {
		/* add the target to our target array */
		pm_targets = list_add(pm_targets, strdup(argv[optind]));
		optind++;
	}

	return(0);
}

int main(int argc, char *argv[])
{
	int ret = 0;
	char *cenv = NULL, *lang = NULL;
#ifndef CYGWIN
	uid_t myuid;
#endif
	list_t *lp;

#ifndef CYGWIN
#ifndef BSD
	/* debug */
	mtrace();
#endif
#endif

	cenv = getenv("COLUMNS");
	if(cenv != NULL) {
		maxcols = atoi(cenv);
	}

	/* set signal handlers */
	signal(SIGINT, signal_callback);
	signal(SIGTERM, signal_callback);
	signal(SIGSEGV, signal_callback);

	/* i18n init */
	lang=getenv("LC_ALL");
	if(lang==NULL || lang[0]=='\0')
		lang=getenv("LC_MESSAGES");
	if (lang==NULL || lang[0]=='\0')
		lang=getenv("LANG");

	setlocale(LC_ALL, lang);
	// workaround for tr_TR
	if(lang && !strcmp(lang, "tr_TR"))
		setlocale(LC_CTYPE, "C");
	bindtextdomain("pacman-g2", "/usr/share/locale");
	textdomain("pacman-g2");

	/* init config data */
	config = config_new();
	config->op = PM_OP_MAIN;
	config->debug |= PM_LOG_WARNING;
	/* disable progressbar if the output is redirected */
	if(!isatty(1)) {
		config->noprogressbar = 1;
	}

	/* parse the command line */
	ret = parseargs(argc, argv);
	if(ret != 0) {
		config_free(config);
		exit(ret);
	}

#ifndef CYGWIN
	/* see if we're root or not */
	myuid = geteuid();
#ifndef FAKEROOT
	if(!myuid && getenv("FAKEROOTKEY")) {
		/* fakeroot doesn't count, we're non-root */
		myuid = 99;
	}
#endif

	/* check if we have sufficient permission for the requested operation */
	if(myuid > 0) {
		if(config->op != PM_OP_MAIN && config->op != PM_OP_QUERY && config->op != PM_OP_DEPTEST && config->op != PM_OP_PS) {
			if((config->op == PM_OP_SYNC && !config->op_s_sync && (config->op_s_search
				 || config->group || config->op_q_list || config->op_q_info
				 || (config->flags & PM_TRANS_FLAG_PRINTURIS)))
				 || (config->op == PM_OP_DEPTEST && !config->op_d_resolve)
				 || (config->root != NULL)) {
				/* special case: PM_OP_SYNC can be used w/ config->op_s_search by any user */
				/* special case: ignore root user check if -r is specified, fall back on
				 * normal FS checking */
			} else {
				ERR(NL, _("you cannot perform this operation unless you are root.\n"));
				config_free(config);
				exit(1);
			}
		}
	}
#endif

	if(config->root == NULL) {
		config->root = strdup(PM_ROOT);
	}

	/* add a trailing '/' if there isn't one */
	if(config->root[strlen(config->root)-1] != '/') {
		char *ptr;
		MALLOC(ptr, strlen(config->root)+2);
		strcpy(ptr, config->root);
		strcat(ptr, "/");
		FREE(config->root);
		config->root = ptr;
	}

	/* initialize pm library */
	if(pacman_initialize(config->root) == -1) {
		ERR(NL, _("failed to initialize pacman library (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}

	/* Setup logging as soon as possible, to print out maximum debugging info */
	if(pacman_set_option(PM_OPT_LOGMASK, (long)config->debug) == -1) {
		ERR(NL, _("failed to set option LOGMASK (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}
	if(pacman_set_option(PM_OPT_DBPATH, (long)config->dbpath) == -1) {
		ERR(NL, _("failed to set option DBPATH (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}
	if(pacman_set_option(PM_OPT_LOGCB, (long)cb_log) == -1) {
		ERR(NL, _("failed to set option LOGCB (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}

	if(config->configfile == NULL) {
		config->configfile = strdup(PACCONF);
	}
	if(pacman_parse_config(config->configfile, cb_db_register, "") != 0) {
		ERR(NL, _("failed to parse config (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}

	/* set library parameters */
	if(pacman_set_option(PM_OPT_DLCB, (long)log_progress) == -1) {
		ERR(NL, _("failed to set option DLCB (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}
	if(pacman_set_option(PM_OPT_DLFNM, (long)sync_fnm) == -1) {
		ERR(NL, _("failed to set option DLFNM (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}
	if(pacman_set_option(PM_OPT_DLOFFSET, (long)&offset) == -1) {
		ERR(NL, _("failed to set option DLOFFSET (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}
	if(pacman_set_option(PM_OPT_DLT0, (long)&t0) == -1) {
		ERR(NL, _("failed to set option DLT0 (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}
	if(pacman_set_option(PM_OPT_DLT, (long)&t) == -1) {
		ERR(NL, _("failed to set option DLT (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}
	if(pacman_set_option(PM_OPT_DLRATE, (long)&rate) == -1) {
		ERR(NL, _("failed to set option DLRATE (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}
	if(pacman_set_option(PM_OPT_DLXFERED1, (long)&xfered1) == -1) {
		ERR(NL, _("failed to set option DLXFERED1 (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}
	if(pacman_set_option(PM_OPT_DLETA_H, (long)&eta_h) == -1) {
		ERR(NL, _("failed to set option DLETA_H (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}
	if(pacman_set_option(PM_OPT_DLETA_M, (long)&eta_m) == -1) {
		ERR(NL, _("failed to set option DLETA_M (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}
	if(pacman_set_option(PM_OPT_DLETA_S, (long)&eta_s) == -1) {
		ERR(NL, _("failed to set option DLETA_S (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}
	if(pacman_set_option(PM_OPT_DLREMAIN, (long)&remain) == -1) {
		ERR(NL, _("failed to set option DLREMAIN (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}
	if(pacman_set_option(PM_OPT_DLHOWMANY, (long)&howmany) == -1) {
		ERR(NL, _("failed to set option DLHOWMANY (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}
	FREE(config->cachedir);
	pacman_get_option(PM_OPT_CACHEDIR, (long *)&config->cachedir);

	for(lp = config->op_s_ignore; lp; lp = lp->next) {
		if(pacman_set_option(PM_OPT_IGNOREPKG, (long)lp->data) == -1) {
			ERR(NL, _("failed to set option IGNOREPKG (%s)\n"), pacman_strerror(pm_errno));
			cleanup(1);
		}
	}
	
	if(config->verbose > 0) {
		printf("Root  : %s\n", config->root);
		printf("DBPath: %s\n", config->dbpath);
		list_display(_("Targets:"), pm_targets);
	}

	/* Opening local database */
	db_local = pacman_db_register("local");
	if(db_local == NULL) {
		ERR(NL, _("could not register 'local' database (%s)\n"), pacman_strerror(pm_errno));
		cleanup(1);
	}

	if(list_count(pm_targets) == 0 && !(config->op == PM_OP_QUERY || (config->op == PM_OP_SYNC
	   && (config->op_s_sync || config->op_s_upgrade || config->op_s_clean || config->group 
	   || config->op_q_list)) || config->op == PM_OP_PS)) {
		ERR(NL, _("no targets specified (use -h for help)\n"));
		cleanup(1);
	}

	/* start the requested operation */
	switch(config->op) {
		case PM_OP_ADD:     ret = addpkg(pm_targets);     break;
		case PM_OP_REMOVE:  ret = removepkg(pm_targets);  break;
		case PM_OP_UPGRADE: ret = upgradepkg(pm_targets); break;
		case PM_OP_QUERY:   ret = querypkg(pm_targets);   break;
		case PM_OP_SYNC:    ret = syncpkg(pm_targets);    break;
		case PM_OP_PS:      ret = pspkg(0);      break;
		case PM_OP_DEPTEST: ret = deptestpkg(pm_targets); break;
		default:
			ERR(NL, _("no operation specified (use -h for help)\n"));
			ret = 1;
	}

	cleanup(ret);
	/* not reached */
	return(0);
}

/* vim: set ts=2 sw=2 noet: */
