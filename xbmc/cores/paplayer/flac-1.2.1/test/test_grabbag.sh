#!/bin/sh

#  FLAC - Free Lossless Audio Codec
#  Copyright (C) 2001,2002,2003,2004,2005,2006,2007  Josh Coalson
#
#  This file is part the FLAC project.  FLAC is comprised of several
#  components distributed under difference licenses.  The codec libraries
#  are distributed under Xiph.Org's BSD-like license (see the file
#  COPYING.Xiph in this distribution).  All other programs, libraries, and
#  plugins are distributed under the GPL (see COPYING.GPL).  The documentation
#  is distributed under the Gnu FDL (see COPYING.FDL).  Each file in the
#  FLAC distribution contains at the top the terms under which it may be
#  distributed.
#
#  Since this particular file is relevant to all components of FLAC,
#  it may be distributed under the Xiph.Org license, which is the least
#  restrictive of those mentioned above.  See the file COPYING.Xiph in this
#  distribution.

die ()
{
	echo $* 1>&2
	exit 1
}

if [ x = x"$1" ] ; then 
	BUILD=debug
else
	BUILD="$1"
fi

LD_LIBRARY_PATH=../src/libFLAC/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../src/share/grabbag/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../src/share/replaygain_analysis/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../obj/$BUILD/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=../src/test_grabbag/cuesheet:$PATH
PATH=../src/test_grabbag/picture:$PATH
PATH=../obj/$BUILD/bin:$PATH

test_cuesheet -h 1>/dev/null 2>/dev/null || die "ERROR can't find test_cuesheet executable"
test_picture -h 1>/dev/null 2>/dev/null || die "ERROR can't find test_picture executable"

run_test_cuesheet ()
{
	if [ x"$FLAC__TEST_WITH_VALGRIND" = xyes ] ; then
		echo "valgrind --leak-check=yes --show-reachable=yes --num-callers=100 test_cuesheet $*" >>test_grabbag.valgrind.log
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --log-fd=4 test_cuesheet $* 4>>test_grabbag.valgrind.log
	else
		test_cuesheet $*
	fi
}

run_test_picture ()
{
	if [ x"$FLAC__TEST_WITH_VALGRIND" = xyes ] ; then
		echo "valgrind --leak-check=yes --show-reachable=yes --num-callers=100 test_picture $*" >>test_grabbag.valgrind.log
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --log-fd=4 test_picture $* 4>>test_grabbag.valgrind.log
	else
		test_picture $*
	fi
}

if [ `env | grep -ic '^comspec='` != 0 ] ; then
	is_win=yes
else
	is_win=no
fi

########################################################################
#
# test_picture
#
########################################################################

log=picture.log
picture_dir=pictures

echo "Running test_picture..."

rm -f $log

run_test_picture $picture_dir >> $log 2>&1

if [ $is_win = yes ] ; then
	diff -w picture.ok $log > picture.diff || die "Error: .log file does not match .ok file, see picture.diff"
else
	diff picture.ok $log > picture.diff || die "Error: .log file does not match .ok file, see picture.diff"
fi

echo "PASSED (results are in $log)"

########################################################################
#
# test_cuesheet
#
########################################################################

log=cuesheet.log
bad_cuesheets=cuesheets/bad.*.cue
good_cuesheets=cuesheets/good.*.cue
good_leadout=`expr 80 \* 60 \* 44100`
bad_leadout=`expr $good_leadout + 1`

echo "Running test_cuesheet..."

rm -f $log

#
# negative tests
#
for cuesheet in $bad_cuesheets ; do
	echo "NEGATIVE $cuesheet" >> $log 2>&1
	run_test_cuesheet $cuesheet $good_leadout cdda >> $log 2>&1
	exit_code=$?
	if [ "$exit_code" = 255 ] ; then
		die "Error: test script is broken"
	fi
	cuesheet_pass1=${cuesheet}.1
	cuesheet_pass2=${cuesheet}.2
	rm -f $cuesheet_pass1 $cuesheet_pass2
done

#
# positve tests
#
for cuesheet in $good_cuesheets ; do
	echo "POSITIVE $cuesheet" >> $log 2>&1
	run_test_cuesheet $cuesheet $good_leadout cdda >> $log 2>&1
	exit_code=$?
	if [ "$exit_code" = 255 ] ; then
		die "Error: test script is broken"
	elif [ "$exit_code" != 0 ] ; then
		die "Error: good cuesheet is broken"
	fi
	cuesheet_pass1=${cuesheet}.1
	cuesheet_pass2=${cuesheet}.2
	diff $cuesheet_pass1 $cuesheet_pass2 >> $log 2>&1 || die "Error: pass1 and pass2 output differ"
	rm -f $cuesheet_pass1 $cuesheet_pass2
done

if [ $is_win = yes ] ; then
	diff -w cuesheet.ok $log > cuesheet.diff || die "Error: .log file does not match .ok file, see cuesheet.diff"
else
	diff cuesheet.ok $log > cuesheet.diff || die "Error: .log file does not match .ok file, see cuesheet.diff"
fi

echo "PASSED (results are in $log)"
