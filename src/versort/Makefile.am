bin_PROGRAMS = vercmp versort

AM_CFLAGS = \
	-I$(top_srcdir)/lib/libpacman \
	-I$(top_srcdir)/src/pacman-g2

vercmp_SOURCES = vercmp.c
vercmp_LDADD = \
	$(top_builddir)/lib/libpacman/libpacman.la \
	$(top_builddir)/lib/libftp/libftp.la

versort_SOURCES = versort.c
versort_LDADD = $(top_builddir)/lib/libpacman/libpacman.la \
	        $(top_builddir)/lib/libftp/libftp.la

