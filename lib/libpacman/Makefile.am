AUTOMAKE_OPTIONS = gnu
DEFINES = -pedantic -D_GNU_SOURCE
AM_CFLAGS = $(DEFINES) \
	    -I$(top_srcdir)/lib/libftp
SUBDIRS = po

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

TARGETS = md5driver.c \
	md5.c \
	sha1.c \
	util.c \
	list.c \
	log.c \
	error.c \
	package.c \
	group.c \
	db.c \
	cache.c \
	conflict.c \
	deps.c \
	provide.c \
	versioncmp.c \
	backup.c \
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
