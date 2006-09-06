SUBDIRS = lib/libftp lib/libalpm src/util src/pacman scripts doc etc
if HAS_PERL
SUBDIRS += bindings/perl
endif
if HAS_PYTHON
SUBDIRS += bindings/python
endif

EXTRA_DIST =		\
	ChangeLog  	\
	COPYING		\
	README 		\
	TODO 		\
	TODO.autoconf

check:
	cd pactest; python pactest.py --test=tests/*.py -p ../src/pacman/pacman --debug=-1
