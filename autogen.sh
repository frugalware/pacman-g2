#!/bin/sh -e


import_pootle()
{
	po_dir=~/git/translations/po

	if [ -d $po_dir ]; then
		: > lib/libpacman/po/LINGUAS
		: > src/pacman-g2/po/LINGUAS
		for i in $(/bin/ls $po_dir/pacman)
		do
			[ "$i" = "sk" ] && continue
			cp $po_dir/pacman/$i/libpacman.po lib/libpacman/po/$i.po
			if msgfmt -c --statistics -o lib/libpacman/po/$i.gmo lib/libpacman/po/$i.po; then
				echo $i >> lib/libpacman/po/LINGUAS
			else
				echo "WARNING: lib/libpacman/po/$i.po will break your build!"
			fi
			cp $po_dir/pacman/$i/pacman-g2.po src/pacman-g2/po/$i.po
			if msgfmt -c --statistics -o src/pacman-g2/po/$i.gmo src/pacman-g2/po/$i.po; then
				echo $i >> src/pacman-g2/po/LINGUAS
			else
				echo "WARNING: src/pacman-g2/po/$i.po will break your build!"
			fi
		done
	else
		echo "WARNING: no po files will be used"
	fi

	# generate the pot files
	for i in lib/libpacman/po src/pacman-g2/po
	do
		cd $i
		mv Makevars Makevars.tmp
		package=`pwd|sed 's|.*/\(.*\)/.*|\1|'`
		cp /usr/bin/intltool-extract ./
		intltool-update --pot --gettext-package=$package
		rm intltool-extract
		mv Makevars.tmp Makevars
		cd - >/dev/null
	done
}

cd `dirname $0`

if [ "$1" == "--dist" ]; then
	if [ -d ../releases ]; then
		release="yes"
	fi
	ver=`grep AC_INIT configure.ac|sed 's/.*, \([0-9\.]*\), .*/\1/'`
	if [ ! "$release" ]; then
		ver="${ver}_`date +%Y%m%d`"
	fi
	git-archive --format=tar --prefix=pacman-g2-$ver/ HEAD | tar xf -
	git log --no-merges |git name-rev --tags --stdin > pacman-g2-$ver/ChangeLog
	cd pacman-g2-$ver
	./autogen.sh --git
	cd ..
	tar czf pacman-g2-$ver.tar.gz pacman-g2-$ver
	rm -rf pacman-g2-$ver
	if [ "$release" ]; then
		dest="../releases"
		gpg --comment "See http://ftp.frugalware.org/pub/README.GPG for info" \
			-ba -u 20F55619 pacman-g2-$ver.tar.gz
		mv pacman-g2-$ver.tar.gz.asc $dest
	else
		dest="dist"
	fi
	mv pacman-g2-$ver.tar.gz $dest
	if [ ! "$release" ]; then
		sed "s/@PACKAGE_VERSION@/$ver/;
			s/@SHA1SUM@/`sha1sum $dest/pacman-g2-$ver.tar.gz|sed 's/  .*//'`/" \
			dist/FrugalBuild.in > dist/FrugalBuild
		echo "Now type: 'cd dist; makepkg -ci'."
	fi
	exit 0
elif [ "$1" == "--gettext-only" ]; then
	sh autoclean.sh
	for i in lib/libpacman/po src/pacman-g2/po
	do
		cd $i
		mv Makevars Makevars.tmp
		package=`pwd|sed 's|.*/\(.*\)/.*|\1|'`
		cp /usr/bin/intltool-extract ./
		intltool-update --pot --gettext-package=$package
		rm intltool-extract
		if [ "$2" != "--pot-only" ]; then
		for j in *.po
		do
			if msgmerge $j $package.pot -o $j.new; then
				mv -f $j.new $j
				echo -n "$i/$j: "
				msgfmt -c --statistics -o $j.gmo $j
				rm -f $j.gmo
			else
				echo "msgmerge for $j failed!"
				rm -f $j.new
			fi
		done
		fi
		mv Makevars.tmp Makevars
		cd - >/dev/null
	done
	# FIXME: implement --pot-only for po4a, too
	[ "$2" == "--pot-only" ] && exit 0
	cd doc
	po4a -k 0 po4a.cfg
	cd po
	for i in *po
	do
		if msgmerge $i $package.pot -o $i.new; then
			mv -f $i.new $i
			echo -n "man/$i: "
			msgfmt -c --statistics -o $i.gmo $i
			rm -f $i.gmo
		else
			echo "msgmerge for $i failed!"
			rm -f $i.new
		fi
	done
	exit 0
fi

# copy in the po files
import_pootle

cp /usr/share/aclocal/libtool.m4 aclocal.m4
libtoolize -f -c
aclocal --force
autoheader -f
autoconf -f
cp -f $(dirname $(which automake))/../share/automake-$(automake --version|sed 's/.* //;q')/mkinstalldirs ./
cp -f $(dirname $(which automake))/../share/gettext/config.rpath ./
automake -a -c --gnu --foreign

if [ "$1" == "--git" ]; then
	rm -rf autom4te.cache
fi
