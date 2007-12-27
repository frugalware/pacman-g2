#!/bin/sh -e


import_pootle()
{
	po_dir=~/git/translations/po

	if [ -d $po_dir ]; then
		: > lib/libpacman/po/LINGUAS
		: > src/pacman-g2/po/LINGUAS
		for i in $(/bin/ls $po_dir/pacman)
		do
			if [ -e $po_dir/pacman/$i/libpacman.po ]; then
			cp $po_dir/pacman/$i/libpacman.po lib/libpacman/po/$i.po
			if msgfmt -c --statistics -o lib/libpacman/po/$i.gmo lib/libpacman/po/$i.po; then
				echo $i >> lib/libpacman/po/LINGUAS
			else
				echo "WARNING: lib/libpacman/po/$i.po would break your build!"
			fi
			fi
			if [ -e $po_dir/pacman/$i/pacman-g2.po ]; then
			cp $po_dir/pacman/$i/pacman-g2.po src/pacman-g2/po/$i.po
			if msgfmt -c --statistics -o src/pacman-g2/po/$i.gmo src/pacman-g2/po/$i.po; then
				echo $i >> src/pacman-g2/po/LINGUAS
			else
				echo "WARNING: src/pacman-g2/po/$i.po would break your build!"
			fi
			fi
			if [ -e $po_dir/pacman/$i/mans.po ]; then
			cp $po_dir/pacman/$i/mans.po doc/po/$i.po
			if ! msgfmt -c --statistics -o doc/po/$i.gmo doc/po/$i.po; then
				echo "WARNING: doc/po/$i.po will break your build!"
			fi
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
	# avoing having the Makevars file as modified ones
	git update-index --refresh >/dev/null
}

cd `dirname $0`

ver=`grep AC_INIT configure.ac|sed 's/.*, \([0-9\.]*\), .*/\1/'`
if [ "$1" == "--dist" ]; then
	git-archive --format=tar --prefix=pacman-g2-$ver/ HEAD | tar xf -
	git log --no-merges |git name-rev --tags --stdin > pacman-g2-$ver/ChangeLog
	cd pacman-g2-$ver
	./autogen.sh --git
	cd ..
	tar czf pacman-g2-$ver.tar.gz pacman-g2-$ver
	rm -rf pacman-g2-$ver
	exit 0
elif [ "$1" == "--release" ]; then
	git tag -l |grep -q $ver || dg tag $ver
	sh $0 --dist
	gpg --comment "See http://ftp.frugalware.org/pub/README.GPG for info" \
		-ba -u 20F55619 pacman-g2-$ver.tar.gz
	mv pacman-g2-$ver.tar.gz{,.asc} ../releases
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
	cd doc
	po4a -k 0 po4a.cfg
	if [ "$2" == "--pot-only" ]; then
		rm -rf po/*.po `grep '\[po4a_langs\]' po4a.cfg |sed 's/\[po4a_langs\] //'`
		exit 0
	fi
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
