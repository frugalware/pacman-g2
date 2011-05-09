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
	remove.c upgrade.c query.c sync.c ps.c conf.c deptest.c pacman-g2.c

pacman_g2_static_SOURCES = $(pacman_g2_SOURCES)

pacman_g2_LDADD = \
	$(top_builddir)/lib/libpacman/libpacman.la

pacman_g2_static_LDADD = $(pacman_g2_LDADD)

pacman_g2_static_LDFLAGS = -all-static $(LDFLAGS)

install-exec-hook:
	ln -sf pacman-g2 $(DESTDIR)$(bindir)/pacman

