bin_PROGRAMS = pacman-g2

if LINKSTATIC
bin_PROGRAMS += pacman-g2.static
endif

SUBDIRS = po

localedir = $(datadir)/locale
DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

AM_CFLAGS = -D_GNU_SOURCE \
	-I$(top_srcdir)/lib/libpacman

pacman_g2_SOURCES = util.c log.c list.c package.c download.c trans.c add.c \
	remove.c upgrade.c query.c sync.c conf.c deptest.c pacman-g2.c

pacman_g2_static_SOURCES = $(pacman_g2_SOURCES)

pacman_g2_LDADD = \
	$(top_srcdir)/lib/libpacman/libpacman.la

pacman_g2_static_LDADD = \
	$(top_srcdir)/lib/libpacman/.libs/libpacman.a $(LDFLAGS)

pacman_g2_static_LDFLAGS = -all-static

install-exec-hook:
	ln -s pacman-g2 $(DESTDIR)$(bindir)/pacman
