#!/bin/sh

# Run this script to build samba from SVN.

## insert all possible names (only works with 
## autoconf 2.x
TESTAUTOHEADER="autoheader autoheader-2.53 autoheader2.50"
TESTAUTOCONF="autoconf autoconf-2.53 autoconf2.50"

AUTOHEADERFOUND="0"
AUTOCONFFOUND="0"


##
## Look for autoheader 
##
for i in $TESTAUTOHEADER; do
	if which $i > /dev/null 2>&1; then
		if test `$i --version | head -n 1 | cut -d.  -f 2 | tr -d [:alpha:]` -ge 53; then
			AUTOHEADER=$i
			AUTOHEADERFOUND="1"
			break
		fi
	fi
done

## 
## Look for autoconf
##

for i in $TESTAUTOCONF; do
	if which $i > /dev/null 2>&1; then
		if test `$i --version | head -n 1 | cut -d.  -f 2 | tr -d [:alpha:]` -ge 53; then
			AUTOCONF=$i
			AUTOCONFFOUND="1"
			break
		fi
	fi
done


## 
## do we have it?
##
if test "$AUTOCONFFOUND" = "0" -o "$AUTOHEADERFOUND" = "0"; then
	echo "$0: need autoconf 2.53 or later to build samba from SVN" >&2
	exit 1
fi

echo "$0: running script/mkversion.sh"
./script/mkversion.sh || exit 1

rm -rf autom4te*.cache
rm -f configure include/config.h*

echo "$0: running $AUTOHEADER"
$AUTOHEADER || exit 1

echo "$0: running $AUTOCONF"
$AUTOCONF || exit 1

rm -rf autom4te*.cache

echo "Now run ./configure and then make."
exit 0

