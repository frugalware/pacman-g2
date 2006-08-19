SUBDIRS = lib/libftp lib/libalpm src/util src/pacman scripts doc etc
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
	cd testsuite; sh runtests.sh
