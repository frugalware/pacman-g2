/*
 * pacman.h
 *
 *  Copyright (c) 2002-2006 by Judd Vinet <jvinet@zeroflux.org>
 *  Copyright (c) 2005 by Aurelien Foret <orelien@chez.com>
 *  Copyright (c) 2005 by Christian Hamar <krics@linuxforum.hu>
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
#ifndef _PACMAN_H
#define _PACMAN_H

#include <sys/types.h> // off_t

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Package Management library
 */

#define PM_ROOT     "/"
#define PM_DBPATH   "var/lib/pacman-g2"
#define PM_CACHEDIR "var/cache/pacman-g2/pkg"
#define PM_LOCK     "/tmp/pacman-g2.lck"
#define PM_HOOKSDIR "etc/pacman-g2/hooks"
#define PM_TRIGGERSDIR "etc/pacman-g2/triggers"

#define PM_EXT_PKG ".fpm"
#define PM_EXT_DB  ".fdb"

/*
 * Structures (opaque)
 */

typedef struct __pmconflict_t pmconflict_t;
typedef struct __pmdb_t pmdb_t;
typedef struct __pmdepmissing_t pmdepmissing_t;
typedef struct __pmdownload_t pmdownload_t;
typedef struct __pmgrp_t pmgrp_t;
typedef struct __pmlist_t pmlist_t;
typedef struct __pmpkg_t pmpkg_t;
typedef struct __pmsyncpkg_t pmsyncpkg_t;

/* Compatibility definitions */
typedef struct __pmlist_t PM_LIST;
typedef struct __pmdb_t PM_DB;
typedef struct __pmpkg_t PM_PKG;
typedef struct __pmgrp_t PM_GRP;
typedef struct __pmsyncpkg_t PM_SYNCPKG;
typedef struct __pmdepmissing_t PM_DEPMISS;
typedef struct __pmconflict_t PM_CONFLICT;

/*
 * Library
 */

int pacman_initialize(const char *root);
int pacman_release(void);

/*
 * Logging facilities
 */

/* Levels */
#define PM_LOG_DEBUG    0x01
#define PM_LOG_ERROR    0x02
#define PM_LOG_WARNING  0x04
#define PM_LOG_FLOW1    0x08
#define PM_LOG_FLOW2    0x10
#define PM_LOG_FUNCTION 0x20

/* Log callback */
typedef void (*pacman_cb_log)(unsigned short, const char *msg);

void pacman_logaction(const char *format, ...);

/*
 * Options
 */

#define PM_DLFNM_LEN 1024

/* Parameters */
enum {
	PM_OPT_LOGCB = 1,
	PM_OPT_LOGMASK,
	PM_OPT_USESYSLOG,
	PM_OPT_ROOT,
	PM_OPT_DBPATH,
	PM_OPT_CACHEDIR,
	PM_OPT_LOGFILE,
	PM_OPT_LOCALDB,
	PM_OPT_SYNCDB,
	PM_OPT_NOUPGRADE,
	PM_OPT_NOEXTRACT,
	PM_OPT_IGNOREPKG,
	PM_OPT_UPGRADEDELAY,
	/* Download */
	PM_OPT_PROXYHOST,
	PM_OPT_PROXYPORT,
	PM_OPT_XFERCOMMAND,
	PM_OPT_NOPASSIVEFTP,
	PM_OPT_DLCB,
	PM_OPT_DLFNM,
	PM_OPT_DLT,
	PM_OPT_DLXFERED1,
	/* End of download */
	PM_OPT_HOLDPKG,
	PM_OPT_CHOMP,
	PM_OPT_NEEDLES,
	PM_OPT_MAXTRIES,
	PM_OPT_OLDDELAY,
	PM_OPT_DLREMAIN,
	PM_OPT_DLHOWMANY,
	PM_OPT_HOOKSDIR
};

int pacman_set_option(unsigned char parm, unsigned long data);
int pacman_get_option(unsigned char parm, long *data);

/*
 * Databases
 */

/* Info parameters */
enum {
	PM_DB_TREENAME = 1,
	PM_DB_FIRSTSERVER
};

/* Database registration callback */
typedef void (*pacman_cb_db_register)(const char *, pmdb_t *);

pmdb_t *pacman_db_register(const char *treename);
int pacman_db_unregister(pmdb_t *db);

void *pacman_db_getinfo(pmdb_t *db, unsigned char parm);
int pacman_db_setserver(pmdb_t *db, char *url);

int pacman_db_update(int level, pmdb_t *db);

pmpkg_t *pacman_db_readpkg(pmdb_t *db, const char *name);
pmlist_t *pacman_db_getpkgcache(pmdb_t *db);
pmlist_t *pacman_db_whatprovides(pmdb_t *db, char *name);

pmgrp_t *pacman_db_readgrp(pmdb_t *db, char *name);
pmlist_t *pacman_db_getgrpcache(pmdb_t *db);
pmlist_t *pacman_db_search(pmdb_t *db);
pmlist_t *pacman_db_test(pmdb_t *db);

/*
 * Downloads
 */

int pacman_download_avg(const pmdownload_t *download, double *avg);
int pacman_download_begin(const pmdownload_t *download, struct timeval *timeval);
int pacman_download_end(const pmdownload_t *download, struct timeval *timeval);
int pacman_download_eta(const pmdownload_t *download, double *eta);
int pacman_download_rate(const pmdownload_t *download, double *rate);
int pacman_download_resume(const pmdownload_t *download, off_t *offset);
int pacman_download_size(const pmdownload_t *download, off_t *offset);
int pacman_download_tell(const pmdownload_t *download, off_t *offset);
int pacman_download_xfered(const pmdownload_t *download, off_t *offset);

/*
 * Packages
 */

/* Info parameters */
enum {
	/* Desc entry */
	PM_PKG_NAME = 1,
	PM_PKG_VERSION,
	PM_PKG_DESC,
	PM_PKG_GROUPS,
	PM_PKG_URL,
	PM_PKG_LICENSE,
	PM_PKG_ARCH,
	PM_PKG_BUILDDATE,
	PM_PKG_BUILDTYPE,
	PM_PKG_INSTALLDATE,
	PM_PKG_PACKAGER,
	PM_PKG_SIZE,
	PM_PKG_USIZE,
	PM_PKG_REASON,
	PM_PKG_MD5SUM, /* Sync DB only */
	PM_PKG_SHA1SUM, /* Sync DB only */
	PM_PKG_TRIGGERS,
	/* Depends entry */
	PM_PKG_DEPENDS,
	PM_PKG_REMOVES,
	PM_PKG_REQUIREDBY,
	PM_PKG_CONFLICTS,
	PM_PKG_PROVIDES,
	PM_PKG_REPLACES, /* Sync DB only */
	/* Files entry */
	PM_PKG_FILES,
	PM_PKG_BACKUP,
	/* Sciplet */
	PM_PKG_SCRIPLET,
	/* Misc */
	PM_PKG_DATA,
	PM_PKG_FORCE,
	PM_PKG_STICK
};

/* reasons -- ie, why the package was installed */
#define PM_PKG_REASON_EXPLICIT  0  /* explicitly requested by the user */
#define PM_PKG_REASON_DEPEND    1  /* installed as a dependency for another package */

/* package name formats */
#define PM_PKG_WITHOUT_ARCH 0 /* pkgname-pkgver-pkgrel, used under PM_DBPATH */
#define PM_PKG_WITH_ARCH    1 /* ie, pkgname-pkgver-pkgrel-arch, used under PM_CACHEDIR */

void *pacman_pkg_getinfo(pmpkg_t *pkg, unsigned char parm);
pmlist_t *pacman_pkg_getowners(char *filename);
int pacman_pkg_load(char *filename, pmpkg_t **pkg);
int pacman_pkg_free(pmpkg_t *pkg);
char *pacman_fetch_pkgurl(char *url);
int pacman_parse_config(const char *file, pacman_cb_db_register callback);
int pacman_pkg_vercmp(const char *ver1, const char *ver2);
int pacman_reg_match(const char *string, const char *pattern);

/*
 * Groups
 */

/* Info parameters */
enum {
	PM_GRP_NAME = 1,
	PM_GRP_PKGNAMES
};

void *pacman_grp_getinfo(pmgrp_t *grp, unsigned char parm);

/*
 * Sync
 */

/* Types */
enum {
	PM_SYNC_TYPE_REPLACE = 1,
	PM_SYNC_TYPE_UPGRADE,
	PM_SYNC_TYPE_DEPEND
};
/* Info parameters */
enum {
	PM_SYNC_TYPE = 1,
	PM_SYNC_PKG,
	PM_SYNC_DATA
};

void *pacman_sync_getinfo(pmsyncpkg_t *sync, unsigned char parm);
int pacman_sync_cleancache(int full);

/*
 * Transactions
 */

/* Types */
typedef enum _pmtranstype_t {
	PM_TRANS_TYPE_ADD = 1,
	PM_TRANS_TYPE_REMOVE,
	PM_TRANS_TYPE_UPGRADE,
	PM_TRANS_TYPE_SYNC
} pmtranstype_t;

/* Flags */
#define PM_TRANS_FLAG_NODEPS  0x01
#define PM_TRANS_FLAG_FORCE   0x02
#define PM_TRANS_FLAG_NOSAVE  0x04
#define PM_TRANS_FLAG_FRESHEN 0x08
#define PM_TRANS_FLAG_CASCADE 0x10
#define PM_TRANS_FLAG_RECURSE 0x20
#define PM_TRANS_FLAG_DBONLY  0x40
#define PM_TRANS_FLAG_DEPENDSONLY 0x80
#define PM_TRANS_FLAG_ALLDEPS 0x100
#define PM_TRANS_FLAG_DOWNLOADONLY 0x200
#define PM_TRANS_FLAG_NOSCRIPTLET 0x400
#define PM_TRANS_FLAG_NOCONFLICTS 0x800
#define PM_TRANS_FLAG_PRINTURIS 0x1000
#define PM_TRANS_FLAG_NOINTEGRITY 0x2000
#define PM_TRANS_FLAG_NOARCH 0x4000
#define PM_TRANS_FLAG_PRINTURIS_CACHED 0x8000 /* print uris for pkgs that are already cached */
#define PM_TRANS_FLAG_DOWNGRADE 0x10000

/* Transaction Events */
enum {
	PM_TRANS_EVT_CHECKDEPS_START = 1,
	PM_TRANS_EVT_CHECKDEPS_DONE,
	PM_TRANS_EVT_FILECONFLICTS_START,
	PM_TRANS_EVT_FILECONFLICTS_DONE,
	PM_TRANS_EVT_CLEANUP_START,
	PM_TRANS_EVT_CLEANUP_DONE,
	PM_TRANS_EVT_RESOLVEDEPS_START,
	PM_TRANS_EVT_RESOLVEDEPS_DONE,
	PM_TRANS_EVT_INTERCONFLICTS_START,
	PM_TRANS_EVT_INTERCONFLICTS_DONE,
	PM_TRANS_EVT_ADD_START,
	PM_TRANS_EVT_ADD_DONE,
	PM_TRANS_EVT_REMOVE_START,
	PM_TRANS_EVT_REMOVE_DONE,
	PM_TRANS_EVT_UPGRADE_START,
	PM_TRANS_EVT_UPGRADE_DONE,
	PM_TRANS_EVT_EXTRACT_DONE,
	PM_TRANS_EVT_INTEGRITY_START,
	PM_TRANS_EVT_INTEGRITY_DONE,
	PM_TRANS_EVT_SCRIPTLET_INFO,
	PM_TRANS_EVT_SCRIPTLET_START,
	PM_TRANS_EVT_SCRIPTLET_DONE,
	PM_TRANS_EVT_PRINTURI,
	PM_TRANS_EVT_RETRIEVE_START,
	PM_TRANS_EVT_RETRIEVE_LOCAL
};

/* Transaction Conversations (ie, questions) */
enum {
	PM_TRANS_CONV_INSTALL_IGNOREPKG = 0x01,
	PM_TRANS_CONV_REPLACE_PKG = 0x02,
	PM_TRANS_CONV_CONFLICT_PKG = 0x04,
	PM_TRANS_CONV_CORRUPTED_PKG = 0x08,
	PM_TRANS_CONV_LOCAL_NEWER = 0x10,
	PM_TRANS_CONV_LOCAL_UPTODATE = 0x20,
	PM_TRANS_CONV_REMOVE_HOLDPKG = 0x40
};

/* Transaction Progress */
enum {
	PM_TRANS_PROGRESS_ADD_START,
	PM_TRANS_PROGRESS_UPGRADE_START,
	PM_TRANS_PROGRESS_REMOVE_START,
	PM_TRANS_PROGRESS_CONFLICTS_START,
	PM_TRANS_PROGRESS_INTERCONFLICTS_START
};

/* Transaction Event callback */
typedef void (*pacman_trans_cb_event)(unsigned char, void *, void *);

/* Transaction Conversation callback */
typedef void (*pacman_trans_cb_conv)(unsigned char, void *, void *, void *, int *);

/* Transaction Progress callback */
typedef void (*pacman_trans_cb_progress)(unsigned char, const char *, int, int, int);

/* Download Progress callback */
typedef int (*pacman_trans_cb_download)(const pmdownload_t *downloads);

/* Info parameters */
enum {
	PM_TRANS_TYPE = 1,
	PM_TRANS_FLAGS,
	PM_TRANS_TARGETS,
	PM_TRANS_PACKAGES,
	PM_TRANS_SYNCPKGS
};

void *pacman_trans_getinfo(unsigned char parm);
int pacman_trans_init(unsigned char type, unsigned int flags, pacman_trans_cb_event cb_event, pacman_trans_cb_conv conv, pacman_trans_cb_progress cb_progress);
int pacman_trans_sysupgrade(void);
int pacman_trans_addtarget(const char *target);
int pacman_trans_prepare(pmlist_t **data);
int pacman_trans_commit(pmlist_t **data);
int pacman_trans_release(void);

/*
 * Dependencies and conflicts
 */

enum {
	PM_DEP_MOD_ANY = 1,
	PM_DEP_MOD_EQ,
	PM_DEP_MOD_GE,
	PM_DEP_MOD_LE,
	PM_DEP_MOD_GT,
	PM_DEP_MOD_LT
};
enum {
	PM_DEP_TYPE_DEPEND = 1,
	PM_DEP_TYPE_REQUIRED,
	PM_DEP_TYPE_CONFLICT
};
/* Info parameters */
enum {
	PM_DEP_TARGET = 1,
	PM_DEP_TYPE,
	PM_DEP_MOD,
	PM_DEP_NAME,
	PM_DEP_VERSION
};

void *pacman_dep_getinfo(pmdepmissing_t *miss, unsigned char parm);

/*
 * File conflicts
 */

enum {
	PM_CONFLICT_TYPE_TARGET = 1,
	PM_CONFLICT_TYPE_FILE
};
/* Info parameters */
enum {
	PM_CONFLICT_TARGET = 1,
	PM_CONFLICT_TYPE,
	PM_CONFLICT_FILE,
	PM_CONFLICT_CTARGET
};

void *pacman_conflict_getinfo(pmconflict_t*conflict, unsigned char parm);

/*
 * Helpers
 */

/* pmlist_t */
pmlist_t *pacman_list_first(pmlist_t *list);
pmlist_t *pacman_list_next(pmlist_t *entry);
void *pacman_list_getdata(pmlist_t *entry);
int pacman_list_free(pmlist_t *entry);
int pacman_list_count(pmlist_t *list);

/* md5sums */
char *pacman_get_md5sum(char *name);
char *pacman_get_sha1sum(char *name);

/*
 * Errors
 */

extern enum __pmerrno_t {
	PM_ERR_MEMORY = 1,
	PM_ERR_SYSTEM,
	PM_ERR_BADPERMS,
	PM_ERR_NOT_A_FILE,
	PM_ERR_WRONG_ARGS,
	/* Interface */
	PM_ERR_HANDLE_NULL,
	PM_ERR_HANDLE_NOT_NULL,
	PM_ERR_HANDLE_LOCK,
	/* Databases */
	PM_ERR_DB_OPEN,
	PM_ERR_DB_CREATE,
	PM_ERR_DB_NULL,
	PM_ERR_DB_NOT_NULL,
	PM_ERR_DB_NOT_FOUND,
	PM_ERR_DB_WRITE,
	PM_ERR_DB_REMOVE,
	/* Servers */
	PM_ERR_SERVER_BAD_LOCATION,
	PM_ERR_SERVER_PROTOCOL_UNSUPPORTED,
	/* Configuration */
	PM_ERR_OPT_LOGFILE,
	PM_ERR_OPT_DBPATH,
	PM_ERR_OPT_LOCALDB,
	PM_ERR_OPT_SYNCDB,
	PM_ERR_OPT_USESYSLOG,
	/* Transactions */
	PM_ERR_TRANS_NOT_NULL,
	PM_ERR_TRANS_NULL,
	PM_ERR_TRANS_DUP_TARGET,
	PM_ERR_TRANS_NOT_INITIALIZED,
	PM_ERR_TRANS_NOT_PREPARED,
	PM_ERR_TRANS_ABORT,
	PM_ERR_TRANS_TYPE,
	PM_ERR_TRANS_COMMITING,
	/* Packages */
	PM_ERR_PKG_NOT_FOUND,
	PM_ERR_PKG_INVALID,
	PM_ERR_PKG_OPEN,
	PM_ERR_PKG_LOAD,
	PM_ERR_PKG_INSTALLED,
	PM_ERR_PKG_CANT_FRESH,
	PM_ERR_PKG_INVALID_NAME,
	PM_ERR_PKG_CORRUPTED,
	/* Groups */
	PM_ERR_GRP_NOT_FOUND,
	/* Dependencies */
	PM_ERR_UNSATISFIED_DEPS,
	PM_ERR_CONFLICTING_DEPS,
	PM_ERR_FILE_CONFLICTS,
	/* Misc */
	PM_ERR_USER_ABORT,
	PM_ERR_INTERNAL_ERROR,
	PM_ERR_LIBARCHIVE_ERROR,
	PM_ERR_DISK_FULL,
	PM_ERR_DB_SYNC,
	PM_ERR_RETRIEVE,
	PM_ERR_PKG_HOLD,
	/* Configuration file */
	PM_ERR_CONF_BAD_SECTION,
	PM_ERR_CONF_LOCAL,
	PM_ERR_CONF_BAD_SYNTAX,
	PM_ERR_CONF_DIRECTIVE_OUTSIDE_SECTION,
	PM_ERR_INVALID_REGEX,
	PM_ERR_TRANS_DOWNLOADING,
	/* Downloading */
	PM_ERR_CONNECT_FAILED,
	PM_ERR_FORK_FAILED,
	PM_ERR_NO_OWNER,
	/* Cache */
	PM_ERR_NO_CACHE_ACCESS,
	PM_ERR_CANT_REMOVE_CACHE,
	PM_ERR_CANT_CREATE_CACHE,
	PM_ERR_WRONG_ARCH
} pm_errno;

char *pacman_strerror(int err);
enum __pmerrno_t pacman_geterror(void);

#ifdef __cplusplus
}
#endif
#endif /* _PACMAN_H */

/* vim: set ts=2 sw=2 noet: */
