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

EXTRA_DIST =		\
	ChangeLog  	\
	COPYING		\
	README 		\
	TODO

check:
	cd pactest; python pactest.py --test=tests/*.py -p ../src/pacman-g2/pacman-g2 --debug=-1
