#!/bin/sh

grep -- -DFAKEROOT ../../Makefile
if [ $? != 0 ]; then
	echo "re-run configure with --disable fakeroot"
	exit $?
fi

if ! type -p fakeroot; then
	echo "can't find fakeroot"
	exit 1
fi

if [ "`whoami`" != "root" ]; then
	echo "fakeroot is broken"
	exit 1
fi
