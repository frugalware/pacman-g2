/*
 * pacman.vapi
 * Vala bindings for the pacman-g2 library
 *
 * Copyright (C) 2010 Gaetan Gourdin
 * Copyright (C) 2009 Priyank Gosalia
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *
 * As a special exception, if you use inline functions from this file, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU Lesser General Public License.
 *
 * Author: Priyank Gosalia <priyankmg@gmail.com>
 *
 */

[CCode (cprefix = "", lower_case_cprefix = "", cheader_filename = "pacman.h")]
namespace Pacman {
	[Compact]
	[CCode (cheader_filename = "pacman.h")]
	public class PM_CONFLICT {
	}
	[Compact]
	[CCode (cheader_filename = "pacman.h")]
	public class PM_DB {
	}
	[Compact]
	[CCode (cheader_filename = "pacman.h")]
	public class PM_DEPMISS {
	}
	[Compact]
	[CCode (cheader_filename = "pacman.h")]
	public class PM_GRP {
	}
	[Compact]
	[CCode (cheader_filename = "pacman.h")]
	public class PM_LIST {
	}
	[Compact]
	[CCode (cheader_filename = "pacman.h")]
	public class PM_NETBUF {
	}
	[Compact]
	[CCode (cheader_filename = "pacman.h")]
	public class PM_PKG {
	}
	[Compact]
	[CCode (cheader_filename = "pacman.h")]
	public class PM_SYNCPKG {
	}
	[Compact]
	[CCode (cheader_filename = "pacman.h")]
	public class PM_TRANS {
	}
	[CCode (cheader_filename = "pacman.h", has_target = false)]
	public delegate void pacman_cb_db_register (string p1, Pacman.PM_DB p2);
	[CCode (cheader_filename = "pacman.h", has_target = false)]
	public delegate void pacman_cb_log (uint p1, string p2);
	[CCode (cheader_filename = "pacman.h", has_target = false)]
	public delegate void pacman_trans_cb_conv (uint p1, void* p2, void* p3, void* p4, int p5);
	[CCode (cheader_filename = "pacman.h", has_target = false)]
	public delegate int pacman_trans_cb_download (Pacman.PM_NETBUF ctl, int xfered, void* arg);
	[CCode (cheader_filename = "pacman.h", has_target = false)]
	public delegate void pacman_trans_cb_event (uint p1, void* p2, void* p3);
	[CCode (cheader_filename = "pacman.h", has_target = false)]
	public delegate void pacman_trans_cb_progress (uint p1, string p2, int p3, int p4, int p5);
	[CCode (cheader_filename = "pacman.h")]
	public const string PM_CACHEDIR;
	[CCode (cheader_filename = "pacman.h")]
	public const string PM_DBPATH;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_DLFNM_LEN;
	[CCode (cheader_filename = "pacman.h")]
	public const string PM_EXT_DB;
	[CCode (cheader_filename = "pacman.h")]
	public const string PM_EXT_PKG;
	[CCode (cheader_filename = "pacman.h")]
	public const string PM_HOOKSDIR;
	[CCode (cheader_filename = "pacman.h")]
	public const string PM_LOCK;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_LOG_DEBUG;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_LOG_ERROR;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_LOG_FLOW1;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_LOG_FLOW2;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_LOG_FUNCTION;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_LOG_WARNING;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_PKG_REASON_DEPEND;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_PKG_REASON_EXPLICIT;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_PKG_WITHOUT_ARCH;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_PKG_WITH_ARCH;
	[CCode (cheader_filename = "pacman.h")]
	public const string PM_ROOT;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_ALLDEPS;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_CASCADE;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_DBONLY;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_DEPENDSONLY;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_DOWNLOADONLY;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_FORCE;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_FRESHEN;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_NOARCH;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_NOCONFLICTS;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_NODEPS;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_NOINTEGRITY;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_NOSAVE;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_NOSCRIPTLET;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_PRINTURIS;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_PRINTURIS_CACHED;
	[CCode (cheader_filename = "pacman.h")]
	public const int PM_TRANS_FLAG_RECURSE;
	[CCode (cheader_filename = "pacman.h")]
	public static void* conflict_getinfo (Pacman.PM_CONFLICT conflict, uint parm);
	[CCode (cheader_filename = "pacman.h")]
	public static unowned Pacman.PM_LIST db_getgrpcache (Pacman.PM_DB db);
	[CCode (cheader_filename = "pacman.h")]
	public static void* pacman_db_getinfo (Pacman.PM_DB db, uint parm);
	[CCode (cheader_filename = "pacman.h")]
	public static unowned Pacman.PM_LIST pacman_db_getpkgcache (Pacman.PM_DB db);
	[CCode (cheader_filename = "pacman.h")]
	public static unowned Pacman.PM_GRP pacman_db_readgrp (Pacman.PM_DB db, string name);
	[CCode (cheader_filename = "pacman.h")]
	public static unowned Pacman.PM_PKG pacman_db_readpkg (Pacman.PM_DB db, string name);
	[CCode (cheader_filename = "pacman.h")]
	public static unowned Pacman.PM_DB pacman_db_register (string treename);
	[CCode (cheader_filename = "pacman.h")]
	public static unowned Pacman.PM_LIST pacman_db_search (Pacman.PM_DB db);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_db_setserver (Pacman.PM_DB db, string url);
	[CCode (cheader_filename = "pacman.h")]
	public static unowned Pacman.PM_LIST pacman_db_test (Pacman.PM_DB db);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_db_unregister (Pacman.PM_DB db);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_db_update (int level, Pacman.PM_DB db);
	[CCode (cheader_filename = "pacman.h")]
	public static unowned Pacman.PM_LIST pacman_db_whatprovides (Pacman.PM_DB db, string name);
	[CCode (cheader_filename = "pacman.h")]
	public static void* pacman_dep_getinfo (Pacman.PM_DEPMISS miss, uint parm);
	[CCode (cheader_filename = "pacman.h")]
	public static unowned string pacman_fetch_pkgurl (string url);
	[CCode (cheader_filename = "pacman.h")]
	public static unowned string pacman_get_md5sum (string name);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_get_option (uint parm, long data);
	[CCode (cheader_filename = "pacman.h")]
	public static unowned string pacman_get_sha1sum (string name);
	[CCode (cheader_filename = "pacman.h")]
	public static void* pacman_grp_getinfo (Pacman.PM_GRP grp, uint parm);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_initialize (string root);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_list_count (Pacman.PM_LIST list);
	[CCode (cheader_filename = "pacman.h")]
	public static unowned Pacman.PM_LIST pacman_list_first (Pacman.PM_LIST list);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_list_free (Pacman.PM_LIST entry);
	[CCode (cheader_filename = "pacman.h")]
	public static void* pacman_list_getdata (Pacman.PM_LIST entry);
	[CCode (cheader_filename = "pacman.h")]
	public static unowned Pacman.PM_LIST pacman_list_next (Pacman.PM_LIST entry);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_logaction (string fmt);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_parse_config (string file, Pacman.pacman_cb_db_register callback, string this_section);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_pkg_checkmd5sum (Pacman.PM_PKG pkg);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_pkg_checksha1sum (Pacman.PM_PKG pkg);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_pkg_free (Pacman.PM_PKG pkg);
	[CCode (cheader_filename = "pacman.h")]
	public static void* pacman_pkg_getinfo (Pacman.PM_PKG pkg, uint parm);
	[CCode (cheader_filename = "pacman.h")]
	public static unowned Pacman.PM_LIST pacman_pkg_getowners (string filename);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_pkg_load (string filename, out unowned Pacman.PM_PKG pkg);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_pkg_vercmp (string ver1, string ver2);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_reg_match (string str, string pattern);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_release ();
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_set_option (uint parm, uint data);
	[CCode (cheader_filename = "pacman.h")]
	public static unowned string pacman_strerror (int err);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_sync_cleancache (int full);
	[CCode (cheader_filename = "pacman.h")]
	public static void* pacman_sync_getinfo (Pacman.PM_SYNCPKG sync, uint parm);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_trans_addtarget (string target);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_trans_commit (out unowned Pacman.PM_LIST data);
	[CCode (cheader_filename = "pacman.h")]
	public static void* pacman_trans_getinfo (uint parm);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_trans_init (uint type, uint flags, Pacman.pacman_trans_cb_event? cb_event = null, Pacman.pacman_trans_cb_conv? conv=null, Pacman.pacman_trans_cb_progress? cb_progress=null);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_trans_prepare (out unowned Pacman.PM_LIST data);
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_trans_release ();
	[CCode (cheader_filename = "pacman.h")]
	public static int pacman_trans_sysupgrade ();

	[CCode (cprefix = "PM_OPT_")]
	public enum Option
	{
		LOGCB = 1,
		LOGMASK,
		USESYSLOG,
		ROOT,
		DBPATH,
		CACHEDIR,
		LOGFILE,
		LOCALDB,
		SYNCDB,
		NOUPGRADE,
		NOEXTRACT,
		IGNOREPKG,
		UPGRADEDELAY,
		PROXYHOST,
		PROXYPORT,
		XFERCOMMAND,
		NOPASSIVEFTP,
		DLCB,
		DLFNM,
		DLOFFSET,
		DLT0,
		DLT,
		DLRATE,
		DLXFERED1,
		DLETA_H,
		DLETA_M,
		DLETA_S,
		HOLDPKG,
		CHOMP,
		NEEDLES,
		MAXTRIES,
		OLDDELAY,
		DLREMAIN,
		DLHOWMANY,
		HOOKSDIR
	}
	[CCode (cprefix = "PM_TRANS_")]
	public enum OptionTrans
	{
		TYPE_ADD = 1,
		TYPE_REMOVE,
		TYPE_UPGRADE,
		TYPE_SYNC
	}
	/* Info parameters */
	[CCode (cprefix = "PM_TRANS_")]
	enum OptionPM {
	TYPE = 1,
	FLAGS,
	TARGETS,
	PACKAGES
	}
	
	/* Info parameters */
	[CCode (cprefix = "PM_SYNC_")]
	enum OptionPMSYNC {
	TYPE = 1,
	PKG,
	DATA
	}
	
	[CCode (cprefix = "PM_PKG_")]
	enum OptionPMPKG{
	/* Desc entry */
	NAME = 1,
	VERSION,
	DESC,
	GROUPS,
	URL,
	LICENSE,
	ARCH,
	BUILDDATE,
	BUILDTYPE,
	INSTALLDATE,
	PACKAGER,
	SIZE,
	USIZE,
	REASON,
	MD5SUM, /* Sync DB only */
	SHA1SUM, /* Sync DB only */
	/* Depends entry */
	DEPENDS,
	REMOVES,
	REQUIREDBY,
	CONFLICTS,
	PROVIDES,
	REPLACES, /* Sync DB only */
	/* Files entry */
	FILES,
	BACKUP,
	/* Sciplet */
	SCRIPLET,
	/* Misc */
	PM_PKG_DATA,
	FORCE,
	STICK
	}
}
