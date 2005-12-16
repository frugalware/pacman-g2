#!/bin/sh -exu

if [ "$1" == "--dist" ]; then
	ver=`grep AC_INIT configure.in|sed 's/.*, \([^[,]*\), .*/\1/'`
	darcs dist -d pacman-$ver
	[ -d ../releases ] && mv pacman-$ver.tar.gz ../releases
	exit 0
fi

#intltoolize -f -c
libtoolize -f -c
aclocal --force
autoheader -f
autoconf -f
automake -a -c --gnu --foreign
