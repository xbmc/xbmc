#!/bin/sh

# this runs the file serving tests that are expected to pass with samba3

if [ $# -lt 3 ]; then
cat <<EOF
Usage: test_smbtorture_s3.sh UNC USERNAME PASSWORD <first> <smbtorture args>
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

tests="FDPASS LOCK1 LOCK2 LOCK3 LOCK4 LOCK5 LOCK6 LOCK7"
tests="$tests UNLINK BROWSE ATTR TRANS2 MAXFID TORTURE "
tests="$tests OPLOCK1 OPLOCK2 OPLOCK3"
tests="$tests DIR DIR1 TCON TCONDEV RW1 RW2 RW3"
tests="$tests OPEN XCOPY RENAME DELETE PROPERTIES W2K"
tests="$tests PIPE_NUMBER TCON2 IOCTL CHKPATH FDSESS"

skipped1="RANDOMIPC NEGNOWAIT NBENCH ERRMAPEXTRACT TRANS2SCAN NTTRANSSCAN"
skipped2="DENY1 DENY2 OPENATTR CASETABLE EATEST"
skipped3="MANGLE UTABLE"
echo "Skipping the following tests:"
echo "$skipped1"
echo "$skipped2"
echo "$skipped3"

failed=0
for t in $tests; do
    if [ ! -z "$start" -a "$start" != $t ]; then
	continue;
    fi
    start=""
    name="$t"
    testit "$name" $VALGRIND $SRCDIR/bin/smbtorture $ADDARGS $unc -U"$username"%"$password" $t || failed=`expr $failed + 1`
done

testok $0 $failed
