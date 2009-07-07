/*
 * pacman.vapi
 * Vala bindings for the pacman-g2 library
 *
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

[CCode (cprefix = "pacman_", lower_case_cprefix = "pacman_", cheader_filename = "pacman.h")]
namespace Pacman
{
	const string	ROOT		= "/";
	const string	DBPATH		= "var/lib/pacman-g2";
	const string	CACHEDIR	= "var/cache/pacman-g2/pkg";
	const string	LOCK		= "/tmp/pacman-g2.lck";
	const string	HOOKSDIR	= "etc/pacman-g2/hooks";
	const string	EXT_PKG		= ".fpm";
	const string	EXT_DB		= ".fdb";
	
	const ushort	LOG_DEBUG	= 0x01;
	const ushort	LOG_ERROR	= 0x02;
	const ushort	LOG_WARNING	= 0x04;
	const ushort	LOG_FLOW1	= 0x08;
	const ushort	LOG_FLOW2	= 0x010;
	const ushort	LOG_FUNCTION	= 0x020;
	
	const ushort	TRANS_FLAG_NODEPS		= 0x01;
	const ushort	TRANS_FLAG_FORCE		= 0x02;
	const ushort	TRANS_FLAG_NOSAVE		= 0x04;
	const ushort	TRANS_FLAG_FRESHEN		= 0x08;
	const ushort	TRANS_FLAG_CASCADE		= 0x10;
	const ushort	TRANS_FLAG_RECURSE		= 0x20;
	const ushort	TRANS_FLAG_DBONLY		= 0x40;
	const ushort	TRANS_FLAG_DEPENDSONLY		= 0x80;
	const ushort	TRANS_FLAG_ALLDEPS		= 0x100;
	const ushort	TRANS_FLAG_DOWNLOADONLY		= 0x200;
	const ushort	TRANS_FLAG_NOSCRIPTLET		= 0x400;
	const ushort	TRANS_FLAG_NOCONFLICTS		= 0x800;
	const ushort	TRANS_FLAG_PRINTURIS		= 0x1000;
	const ushort	TRANS_FLAG_NOINTEGRITY		= 0x2000;
	const ushort	TRANS_FLAG_NOARCH		= 0x4000;
	
	const uint	DLFNM_LEN		= 1024;
	
	const uint	PKG_REASON_EXPLICIT	= 0;
	const uint	PKG_REASON_DEPEND	= 1;
	const uint	PKG_WITHOUT_ARCH	= 0;
	const uint	PKG_WITH_ARCH		= 1;
 
	public static int initialize (string root);
	public static int release ();
	
	public static int logaction (string fmt, ...);
	
	public static string	get_md5sum (string name);
	public static string	get_sha1sum (string mame);
	
	public static int	get_option (uchar param, ulong *data);
	public static int	set_option (uchar param, long data);

	public static string	strerror (int err);

	public static int parse_config (string file, void* db_register, string? section);

	/* library callbacks */
	[CCode (cprefix = "pacman_cb_")]
	public delegate void* db_register (string db, Database d);
	public delegate void* log (ushort s, string str);
	
	[CCode (cprefix = "PM_OPT")]
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

	[Compact]
	[CCode (cname = "PM_DB", cprefix = "pacman_db_", free_function = "", cheader_filename = "pacman.h")]
	public class Database
	{
		[CCode (cprefix = "PM_DB_")]
		public enum Info
		{
			TREENAME = 1,
			FIRSTSERVER
		}

		public static Database		register (string treename);
		public int			unregister ();
		public weak void		getinfo (Database.Info di);
		public Group			readgrp (string name);
		public Package			readpkg (string name);
		public int			setserver (string url);
		public List			getgrpcache ();
		public List			getpkgcache ();
		public List			search ();
		public List			test ();
		public List			whatprovides (string name);
		
	}

	[Compact]
	[CCode (cname = "PM_PKG", cprefix = "pacman_pkg_", free_function = "")]
	public class Package
	{
		[CCode (cprefix = "PM_PKG_")]
		public enum Info
		{
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
			MD5SUM,
			SHA1SUM,
			DEPENDS,
			REMOVES,
			REQUIREDBY,
			CONFLICTS,
			PROVIDES,
			REPLACES,
			FILES,
			BACKUP,
			SCRIPLET,
			DATA,
			FORCE,
			STICK
		}
		
		public weak void* getinfo (Package.Info param);
		
		public int checkmd5sum ();
		public int checksha1sum ();
		public void free ();
		
		public static List	getowners (string filename);
		public static string	fetch_pkgurl (string url);
		public static int	vercmp (string ver1, string ver2);
		public static int	reg_match (string str, string pattern);
	}
	
	[Compact]
	[CCode (cname = "PM_GRP", cprefix = "pacman_grp_", free_function = "")]
	public class Group
	{
		[CCode (cprefix = "PM_GRP_")]
		public enum Info
		{
			NAME = 1,
			PKGNAMES
		}
		
		public weak void* getinfo (Group.Info param);
	}
	
	[Compact]
	[CCode (cname = "PM_LIST", cprefix = "pacman_list_", free_function = "")]
	public class List
	{
		public List first ();
		public List next ();
		public int free ();
		public int count ();
		public void* getdata ();	
	}
	
	[Compact]
	[CCode (cname = "PM_SYNCPKG", cprefix = "pacman_sync_", free_function = "")]
	public class Sync
	{
		[CCode (cprefix = "PM_SYNC_TYPE_")]
		public enum Type
		{
			REPLACE = 1,
			UPGRADE,
			DEPEND
		}
		
		[CCode (cprefix = "PM_SYNC_")]
		public enum Info
		{
			TYPE = 1,
			PKG,
			DATA
		}

		public void* getinfo (Sync.Info param);
		
		public static int cleancache (int full);
	}
	
	[Compact]
	[CCode (cname="PM_NETBUF")]
	public class NetBuf
	{
	}
	
	[Compact]
	[CCode (cprefix = "pacman_trans_", free_function = "")]
	public class Transaction
	{
		
		[CCode (cprefix = "PM_TRANS_TYPE_")]
		public enum Type
		{
			ADD = 1,
			REMOVE,
			UPGRADE,
			SYNC
		}
		
		[CCode (cprefix = "PM_TRANS_CONV_")]
		public enum Conversation
		{
			INSTALL_IGNOREPKG = 0x01,
			REPLACE_PKG = 0x02,
			CONFLICT_PKG = 0x04,
			CORRUPTED_PKG = 0x08,
			LOCAL_NEWER = 0x10,
			LOCAL_UPTODATE = 0x20,
			REMOVE_HOLDPKG = 0x40
		}
		
		[CCode (cprefix = "PM_TRANS_EVT_")]
		public enum Event
		{
			CHECKDEPS_START = 1,
			CHECKDEPS_DONE,
			FILECONFLICTS_START,
			FILECONFLICTS_DONE,
			CLEANUP_START,
			CLEANUP_DONE,
			RESOLVEDEPS_START,
			RESOLVEDEPS_DONE,
			INTERCONFLICTS_START,
			INTERCONFLICTS_DONE,
			ADD_START,
			ADD_DONE,
			REMOVE_START,
			REMOVE_DONE,
			UPGRADE_START,
			UPGRADE_DONE,
			EXTRACT_DONE,
			INTEGRITY_START,
			INTEGRITY_DONE,
			SCRIPTLET_INFO,
			SCRIPTLET_START,
			SCRIPTLET_DONE,
			PRINTURI
		}

		[CCode (cprefix = "PM_TRANS_PROGRESS_")]
		public enum Progress
		{
			ADD_START,
			UPGRADE_START,
			REMOVE_START,
			CONFLICTS_START,
			INTERCONFLICTS_START
		}
		
		[CCode (cprefix = "PM_TRANS_")]
		public enum Info
		{
			TYPE = 1,
			FLAGS,
			TARGETS,
			PACKAGES
		}
		
		public void* getinfo (Transaction.Info param);

		public static int init (uchar type, uint flags, void* event, void* conv, void* progress);
		public static int sysupgrade ();
		public static int addtarget (string target);
		public static int prepare (out List list);
		public static int commit (out List list);
		public static int release ();

		/* callbacks */
		[CCode (cprefix = "pacman_trans_cb_")]
		public delegate void* event (uchar event, void* data1, void* data2);
		public delegate void* conv (uchar event, void* data1, void* data2, void* data3, int* response);
		public delegate void* progress (uchar event, string pkgname, int percent, int count, int remaining);
		public delegate void* download (NetBuf nb, int xferred, void* arg);
	}
	
	[Compact]
	[CCode (cprefix = "PM_CONFLICT")]
	public class Conflict
	{
		[CCode (cprefix = "PM_CONFLICT_TYPE_")]
		public enum Type
		{
			TARGET = 1,
			FILE
		}

		[CCode (cprefix = "PM_CONFLICT_")]
		public enum Info
		{
			TARGET = 1,
			TYPE,
			FILE,
			CTARGET
		}

		[CCode (cprefix = "pacman_conflict_", free_function = "")]
		public void* get_info (Conflict.Info param);
	}
	
	[Compact]
	[CCode (cprefix = "PM_DEPMISS")]
	public class Dependency
	{
		[CCode (cprefix = "PM_DEP_TYPE_")]
		public enum Type
		{
			DEPEND = 1,
			REQUIRED,
			CONFLICT
		}
		
		[CCode (cprefix = "PM_DEP_MOD_")]
		public enum Mod
		{
			ANY = 1,
			EQ,
			GE,
			LE,
			GT,
			LT
		}
		
		[CCode (cprefix = "PM_DEP_")]
		public enum Info
		{
			TARGET = 1,
			TYPE,
			MOD,
			NAME,
			VERSION,
		}

		[CCode (cprefix = "pacman_dep_", free_function = "")]
		public void* get_info (Dependency.Info param);
	}

	[CCode (cname="pm_errno")]
	public static enum errno
	{
		MEMORY = 1,
		SYSTEM,
		BADPERMS,
		NOT_A_FILE,
		WRONG_ARGS,
		/* Interface */
		HANDLE_NULL,
		HANDLE_NOT_NULL,
		HANDLE_LOCK,
		/* Databases */
		DB_OPEN,
		DB_CREATE,
		DB_NULL,
		DB_NOT_NULL,
		DB_NOT_FOUND,
		DB_WRITE,
		DB_REMOVE,
		/* Servers */
		SERVER_BAD_LOCATION,
		SERVER_PROTOCOL_UNSUPPORTED,
		/* Configuration */
		OPT_LOGFILE,
		OPT_DBPATH,
		OPT_LOCALDB,
		OPT_SYNCDB,
		OPT_USESYSLOG,
		/* Transactions */
		TRANS_NOT_NULL,
		TRANS_NULL,
		TRANS_DUP_TARGET,
		TRANS_NOT_INITIALIZED,
		TRANS_NOT_PREPARED,
		TRANS_ABORT,
		TRANS_TYPE,
		TRANS_COMMITING,
		/* Packages */
		PKG_NOT_FOUND,
		PKG_INVALID,
		PKG_OPEN,
		PKG_LOAD,
		PKG_INSTALLED,
		PKG_CANT_FRESH,
		PKG_INVALID_NAME,
		PKG_CORRUPTED,
		/* Groups */
		GRP_NOT_FOUND,
		/* Dependencies */
		UNSATISFIED_DEPS,
		CONFLICTING_DEPS,
		FILE_CONFLICTS,
		/* Misc */
		USER_ABORT,
		INTERNAL_ERROR,
		LIBARCHIVE_ERROR,
		DISK_FULL,
		DB_SYNC,
		RETRIEVE,
		PKG_HOLD,
		/* Configuration file */
		CONF_BAD_SECTION,
		CONF_LOCAL,
		CONF_BAD_SYNTAX,
		CONF_DIRECTIVE_OUTSIDE_SECTION,
		INVALID_REGEX,
		TRANS_DOWNLOADING,
		/* Downloading */
		CONNECT_FAILED,
		FORK_FAILED,
		NO_OWNER,
		/* Cache */
		NO_CACHE_ACCESS,
		CANT_REMOVE_CACHE,
		CANT_CREATE_CACHE,
		WRONG_ARCH
	}
}
