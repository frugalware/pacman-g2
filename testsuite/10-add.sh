#!/bin/sh

# test pacman -A

. ../functions

mkdir -p usr/bin
touch usr/bin/foo
pkg_new foo 1.0-1
rm -rf usr
$PACMAN -A foo-1.0-1`pkg_ext`
exit $?
