AUTOMAKE_OPTIONS = gnu
DEFINES = -D_GNU_SOURCE -DNETBSD -DINET6 -DWITH_SSL -DFTP_COMBINE_CWDS
AM_CFLAGS = $(DEFINES)

noinst_LTLIBRARIES = libfetch.la

httperr.h: errlist.sh http.errors
	./errlist.sh http_errlist HTTP http.errors > httperr.h

ftperr.h: errlist.sh ftp.errors
	./errlist.sh ftp_errlist FTP ftp.errors > ftperr.h

libfetch_la_SOURCES = \
	common.h \
	fetch.h \
	httperr.h \
	ftperr.h \
	common.c \
	fetch.c \
	file.c \
	ftp.c \
	http.c
