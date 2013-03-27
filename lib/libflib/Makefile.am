AUTOMAKE_OPTIONS = gnu
DEFINES = -pedantic -D_GNU_SOURCE
AM_CFLAGS = $(DEFINES)

#SOMAJOR = 3
#SOMINOR = 1
#SOMICRO = 1
#SOVER = `expr $(SOMAJOR) + $(SOMINOR)`:$(SOMICRO):$(SOMINOR)

noinst_LTLIBRARIES = libflib.la
#lib_LTLIBRARIES = libflib.la

libflib_la_SOURCES = \
	flist.c \
	fobject.c \
	fstdlib.c \
	fstringlist.c \
	fvalue.c

#libflib_la_LDFLAGS = -L. -version-info $(SOVER)
