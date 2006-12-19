bin_PROGRAMS = vercmp

AM_CFLAGS = \
	-I$(top_srcdir)/lib/libalpm \
	-I$(top_srcdir)/src/pacman-g2

vercmp_SOURCES = vercmp.c

vercmp_LDADD = $(top_srcdir)/lib/libalpm/libalpm.la \
	       $(top_srcdir)/lib/libftp/libftp.la
