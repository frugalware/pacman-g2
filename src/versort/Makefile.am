bin_PROGRAMS = versort

AM_CFLAGS = \
	-I$(top_srcdir)/lib/libpacman \
	-I$(top_srcdir)/src/pacman-g2

versort_SOURCES = versort.c

versort_LDADD = $(top_builddir)/lib/libpacman/libpacman.la \
	        $(top_builddir)/lib/libftp/libftp.la
