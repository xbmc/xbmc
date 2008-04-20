#!/bin/sh

# this runs the file serving tests that are expected to pass with samba3

if [ $# -lt 3 ]; then
cat <<EOF
Usage: test_posix_s3.sh UNC USERNAME PASSWORD <first> <smbtorture args>
EOF
exit 1;
fi

unc="$1"
username="$2"
password="$3"
start="$4"
shift 4
ADDARGS="$*"

incdir=`dirname $0`
. $incdir/test_functions.sh

base="BASE-ATTR BASE-CHARSET BASE-CHKPATH BASE-DEFER_OPEN BASE-DELAYWRITE BASE-DELETE"
base="$base BASE-DENY1 BASE-DENY2 BASE-DENY3 BASE-DENYDOS BASE-DIR1 BASE-DIR2"
base="$base BASE-DISCONNECT BASE-FDPASS BASE-LOCK1 BASE-LOCK2 BASE-LOCK3 BASE-LOCK4"
base="$base BASE-LOCK5 BASE-LOCK6 BASE-LOCK7 BASE-MANGLE BASE-NEGNOWAIT BASE-NTDENY1"
base="$base BASE-NTDENY2 BASE-OPEN BASE-OPENATTR BASE-PROPERTIES BASE-RENAME BASE-RW1"
base="$base BASE-SECLEAK BASE-TCON BASE-TCONDEV BASE-TRANS2 BASE-UNLINK BASE-VUID"
base="$base BASE-XCOPY"

raw="RAW-ACLS RAW-CHKPATH RAW-CLOSE RAW-COMPOSITE RAW-CONTEXT RAW-EAS"
raw="$raw RAW-IOCTL RAW-LOCK RAW-MKDIR RAW-MUX RAW-NOTIFY RAW-OPEN RAW-OPLOCK"
raw="$raw RAW-QFILEINFO RAW-QFSINFO RAW-READ RAW-RENAME RAW-SEARCH RAW-SEEK"
raw="$raw RAW-SFILEINFO RAW-SFILEINFO-BUG RAW-STREAMS RAW-UNLINK RAW-WRITE"

rpc="RPC-AUTHCONTEXT"

tests="$base $raw $rpc"

skipped="BASE-CHARSET BASE-DEFER_OPEN BASE-DELAYWRITE BASE-DELETE BASE-OPENATTR BASE-TCONDEV"
skipped="$skipped RAW-ACLS RAW-CLOSE RAW-COMPOSITE RAW-CONTEXT RAW-EAS"
skipped="$skipped RAW-IOCTL RAW-MKDIR RAW-MUX RAW-NOTIFY RAW-OPEN"
skipped="$skipped RAW-QFILEINFO RAW-QFSINFO RAW-RENAME RAW-SEARCH"
skipped="$skipped RAW-SFILEINFO RAW-STREAMS RAW-UNLINK RAW-WRITE"

echo "WARNING: Skipping tests $skipped"

failed=0
for t in $tests; do
    if [ ! -z "$start" -a "$start" != $t ]; then
	continue;
    fi
    skip=0
    for s in $skipped; do
    	if [ x"$s" = x"$t" ]; then
    	    skip=1;
	    break;
	fi
    done
    if [ $skip = 1 ]; then
    	continue;
    fi
    start=""
    name="$t"
    testit "$name" $VALGRIND $SMBTORTURE4 $TORTURE4_OPTIONS $ADDARGS $unc -U"$username"%"$password" $t || failed=`expr $failed + 1`
done

testok $0 $failed
