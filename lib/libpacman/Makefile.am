AUTOMAKE_OPTIONS = gnu
DEFINES = -pedantic -D_GNU_SOURCE
AM_CFLAGS = $(DEFINES) \
	    -I$(top_srcdir)/lib/libftp
SUBDIRS = po

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

DB_TARGETS = \
	db/fakedb.c \
	db/localdb.c \
	db/localdb_files.c \
	db/syncdb.c

HASH_TARGETS = \
	hash/md5driver.c \
	hash/md5.c \
	hash/sha1.c

IO_TARGETS = \
	io/archive.c

PACKAGE_TARGETS = \
	package/pkginfo.c

UTIL_TARGETS = \
	util/list.c \
	util/log.c \
	util/stringlist.c

TARGETS = \
	$(DB_TARGETS) \
	$(HASH_TARGETS) \
	$(IO_TARGETS) \
	$(PACKAGE_TARGETS) \
	$(UTIL_TARGETS) \
	util.c \
	error.c \
	package.c \
	group.c \
	db.c \
	cache.c \
	conflict.c \
	deps.c \
	versioncmp.c \
	packages_transaction.c \
	trans.c \
	trans_sysupgrade.c \
	add.c \
	remove.c \
	sync.c \
	handle.c \
	server.c \
	pacman.c \
	be_files.c

lib_LTLIBRARIES = libpacman.la

include_HEADERS = pacman.h

libpacman_la_SOURCES = $(TARGETS)

libpacman_la_LDFLAGS = -no-undefined -version-info $(PM_VERSION_INFO)
libpacman_la_LIBADD = $(top_builddir)/lib/libftp/libftp.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pacman.pc

if HAS_DOXYGEN
all: doxygen.in

doxygen.in:
	doxygen Doxyfile

endif
