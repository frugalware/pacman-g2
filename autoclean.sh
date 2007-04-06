#!/bin/sh -xu

[ -f Makefile ] && make distclean
rm -rf autom4te.cache
rm -rf Makefile
rm -rf Makefile.in
rm -rf configure
rm -rf config.*
rm -rf stamp*
rm -rf depcomp
rm -rf install-sh
rm -rf missing
rm -rf src/pacman-g2/Makefile
rm -rf src/pacman-g2/Makefile.in
rm -rf src/util/Makefile
rm -rf src/util/Makefile.in
rm -rf lib/libftp/Makefile
rm -rf lib/libftp/Makefile.in
rm -rf lib/libpacman/Makefile.in
rm -rf lib/libpacman/Makefile
rm -rf aclocal.m4
rm -rf ltmain.sh
rm -rf doc/Makefile
rm -rf doc/Makefile.in
rm -rf doc/hu/Makefile
rm -rf doc/hu/Makefile.in
rm -rf doc/hu/*.8
rm -rf doc/html/*.html
rm -rf doc/man3/*.3
rm -rf compile
rm -rf libtool
rm -rf mkinstalldirs
rm -rf config.rpath
rm -rf scripts/.deps/
rm -rf scripts/Makefile.in
rm -rf etc/Makefile.in
rm -rf etc/Makefile
rm -rf etc/pacman.d/Makefile.in
rm -rf etc/pacman.d/Makefile
rm -rf etc/pacman.d/frugalware
rm -rf etc/pacman.d/frugalware-current
rm -f dist/Changelog
rm -f dist/FrugalBuild
rm -f dist/pacman-g2-*.tar.gz
rm -f dist/pacman-g2-*.fpm

rm -rf src/pacman-g2/po/Makefile
rm -rf src/pacman-g2/po/Makefile.in
rm -rf src/pacman-g2/po/{POTFILES,LINGUAS}
rm -rf src/pacman-g2/po/stamp-po
rm -rf src/pacman-g2/po/*.{gmo,pot,po}

rm -rf lib/libpacman/po/Makefile
rm -rf lib/libpacman/po/Makefile.in
rm -rf lib/libpacman/po/{POTFILES,LINGUAS}
rm -rf lib/libpacman/po/stamp-po
rm -rf lib/libpacman/po/*.{gmo,pot,po}

rm -f bindings/*/*.i
