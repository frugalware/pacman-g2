#!/bin/sh

log=../`basename $0 .sh`.log

export PACMAN="`pwd`/../src/pacman/pacman -r ./"

passed=0
failed=0

if ! source functions; then
	echo "where are you?"
	exit 1
fi

: >`basename $log`

for i in *.sh
do
	if [ $i = "`basename $0`" ]; then
		continue
	fi

	echo -n "$i... "
	echo -e "\n--- $i" >>`basename $log`

	mkdir fakeroot
	cd fakeroot
	fakeroot sh ../$i >>$log 2>&1

	if [ "$?" = "0" ]; then
		echo passed
		passed=$(($passed+1))
	else
		echo failed
		failed=$(($failed+1))
	fi
	cd ..
	rm -rf fakeroot
done

if [ $passed -gt 0 ]; then
	echo -e "passed:\t$passed"
fi
if [ $failed -gt 0 ]; then
	echo -e "failed:\t$failed"
fi
