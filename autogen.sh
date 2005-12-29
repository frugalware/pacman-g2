#!/bin/sh -ex

if [ "$1" == "--dist" ]; then
	ver=`grep AC_INIT configure.ac|sed 's/.*, \([0-9\.]*\), .*/\1/'`
	darcs changes >_darcs/current/ChangeLog
	darcs dist -d pacman-$ver
	rm _darcs/current/ChangeLog
	[ -d ../releases ] && mv pacman-$ver.tar.gz ../releases
	exit 0
fi

#intltoolize -f -c
libtoolize -f -c
aclocal --force
autoheader -f
autoconf -f
automake -a -c --gnu --foreign
