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
PATH=../src/test_libFLAC:$PATH
PATH=../obj/$BUILD/bin:$PATH

run_test_libFLAC ()
{
	if [ x"$FLAC__TEST_WITH_VALGRIND" = xyes ] ; then
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --log-fd=4 test_libFLAC $* 4>>test_libFLAC.valgrind.log
	else
		test_libFLAC $*
	fi
}

run_test_libFLAC || die "ERROR during test_libFLAC"
