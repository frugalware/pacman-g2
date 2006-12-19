bin_PROGRAMS = pacman-g2 pacman-g2.static

SUBDIRS = po

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

AM_CFLAGS = -D_GNU_SOURCE \
	-I$(top_srcdir)/lib/libalpm \
	-I$(top_srcdir)/lib/libftp

pacman-g2_SOURCES = util.c log.c list.c package.c download.c trans.c add.c \
	remove.c upgrade.c query.c sync.c conf.c deptest.c pacman-g2.c

pacman-g2_static_SOURCES = $(pacman-g2_SOURCES)

pacman-g2_LDADD = \
	$(top_srcdir)/lib/libalpm/libalpm.la

pacman-g2_static_LDADD = \
	$(top_srcdir)/lib/libalpm/.libs/libalpm.a $(LDFLAGS)

pacman-g2_static_LDFLAGS = -all-static

install-exec-hook:
	ln -s pacman-g2 $(DESTDIR)$(bindir)/pacman
