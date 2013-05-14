AUTOMAKE_OPTIONS = gnu
DEFINES = -pedantic -D_GNU_SOURCE
AM_CFLAGS = $(DEFINES)

#SOMAJOR = 3
#SOMINOR = 1
#SOMICRO = 1
#SOVER = `expr $(SOMAJOR) + $(SOMINOR)`:$(SOMICRO):$(SOMINOR)

noinst_LTLIBRARIES = libflib.la
#lib_LTLIBRARIES = libflib.la

libflib_crypto_hash_SOURCES = \
	fmd5.c

libflib_crypto_SOURCES = \
	$(libflib_crypto_hash_SOURCES)

libflib_posix_SOURCES = \
	fstdlib.c \
	fstring.c

libflib_la_SOURCES = \
	$(libflib_crypto_SOURCES) \
	$(libflib_posix_SOURCES) \
	flist.c \
	flistaccumulator.c \
	fobject.c \
	fstringlist.c \
	fvalue.c

#libflib_la_LDFLAGS = -L. -version-info $(SOVER)
libflib_la_LIBADD = 
