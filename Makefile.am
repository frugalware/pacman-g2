ACLOCAL_AMFLAGS = -I m4

SUBDIRS = lib/libftp lib/libpacman src/vercmp src/versort src/pacman-g2 scripts doc etc

if HAS_PERL
SUBDIRS += bindings/perl
endif
if HAS_PYTHON
SUBDIRS += bindings/python
endif
if HAS_JAVA
SUBDIRS += bindings/java
endif
if HAS_CSHARP
SUBDIRS += bindings/csharp
endif
if HAS_VALA
SUBDIRS += bindings/vala
endif

EXTRA_DIST =		\
	ChangeLog  	\
	COPYING		\
	README 		\
	TODO

check:
	python $(top_srcdir)/pactest/pactest.py --test=$(top_srcdir)/pactest/tests/*.py -p $(top_builddir)/src/pacman-g2/pacman-g2 --debug=-1

check-gdb:
	python $(top_srcdir)/pactest/pactest.py --test=$(top_srcdir)/pactest/tests/*.py -p $(top_builddir)/src/pacman-g2/pacman-g2 --debug=-1 --gdb

check-valgrind:
	python $(top_srcdir)/pactest/pactest.py --test=$(top_srcdir)/pactest/tests/*.py -p $(top_builddir)/src/pacman-g2/pacman-g2 --debug=-1 --valgrind

