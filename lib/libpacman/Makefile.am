AUTOMAKE_OPTIONS = gnu
DEFINES = -pedantic -D_GNU_SOURCE
AM_CFLAGS = $(DEFINES) \
	-I$(top_srcdir)/lib/libflib \
	-I$(top_srcdir)/lib/libftp
SUBDIRS = po

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

libpacman_fakedb_SOURCES = \
	fakedb.c

libpacman_frugalware_SOURCES = \
	fdb.c

TARGETS = \
	$(libpacman_fakedb_SOURCES) \
	$(libpacman_frugalware_SOURCES) \
	cache.c \
	db.c \
	localdb.c \
	md5driver.c \
	sha1.c \
	util.c \
	log.c \
	error.c \
	package.c \
	group.c \
	conflict.c \
	deps.c \
	versioncmp.c \
	file.c \
	trans.c \
	trans_sysupgrade.c \
	sync.c \
	handle.c \
	server.c \
	pacman.c \
	be_files.c

lib_LTLIBRARIES = libpacman.la

include_HEADERS = pacman.h

libpacman_la_SOURCES = $(TARGETS)

libpacman_la_LDFLAGS = -no-undefined -version-info $(PM_VERSION_INFO)
libpacman_la_LIBADD = \
	$(top_builddir)/lib/libflib/libflib.la \
	$(top_builddir)/lib/libftp/libftp.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pacman.pc

if HAS_DOXYGEN
all: doxygen.in

doxygen.in:
	doxygen Doxyfile

endif
