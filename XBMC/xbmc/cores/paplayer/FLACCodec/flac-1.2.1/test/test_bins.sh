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
LD_LIBRARY_PATH=../src/share/getopt/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../src/share/replaygain_analysis/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../src/share/replaygain_synthesis/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../src/share/utf8/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=../obj/$BUILD/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=../src/flac:$PATH
PATH=../obj/$BUILD/bin:$PATH
BINS_PATH=../../test_files/bins

if [ x"$FLAC__TEST_LEVEL" = x ] ; then
	FLAC__TEST_LEVEL=1
fi

flac --help 1>/dev/null 2>/dev/null || die "ERROR can't find flac executable"

run_flac ()
{
	if [ x"$FLAC__TEST_WITH_VALGRIND" = xyes ] ; then
		echo "valgrind --leak-check=yes --show-reachable=yes --num-callers=100 flac $*" >>test_bins.valgrind.log
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --log-fd=4 flac $* 4>>test_bins.valgrind.log
	else
		flac $*
	fi
}

test -d ${BINS_PATH} || exit 77

test_file ()
{
	name=$1
	channels=$2
	bps=$3
	encode_options="$4"

	echo -n "$name.bin (--channels=$channels --bps=$bps $encode_options): encode..."
	cmd="run_flac --verify --silent --force --force-raw-format --endian=big --sign=signed --sample-rate=44100 --bps=$bps --channels=$channels $encode_options --no-padding $name.bin"
	echo "### ENCODE $name #######################################################" >> ./streams.log
	echo "###    cmd=$cmd" >> ./streams.log
	$cmd 2>>./streams.log || die "ERROR during encode of $name"

	echo -n "decode..."
	cmd="run_flac --silent --force --endian=big --sign=signed --decode --force-raw-format $name.flac";
	echo "### DECODE $name #######################################################" >> ./streams.log
	echo "###    cmd=$cmd" >> ./streams.log
	$cmd 2>>./streams.log || die "ERROR during decode of $name"

	ls -1l $name.bin >> ./streams.log
	ls -1l $name.flac >> ./streams.log
	ls -1l $name.raw >> ./streams.log

	echo -n "compare..."
	cmp $name.bin $name.raw || die "ERROR during compare of $name"

	echo OK
}

echo "Testing bins..."
for f in b00 b01 b02 b03 b04 ; do
	binfile=$BINS_PATH/$f
	if [ -f $binfile.bin ] ; then
		for disable in '' '--disable-verbatim-subframes --disable-constant-subframes' '--disable-verbatim-subframes --disable-constant-subframes --disable-fixed-subframes' ; do
			for channels in 1 2 4 8 ; do
				for bps in 8 16 24 ; do
					for opt in 0 1 2 4 5 6 8 ; do
						for extras in '' '-p' '-e' ; do
							for blocksize in '' '--lax -b 32' '--lax -b 32768' '--lax -b 65535' ; do
								test_file $binfile $channels $bps "-$opt $extras $blocksize $disable"
							done
						done
					done
					if [ "$FLAC__TEST_LEVEL" -gt 1 ] ; then
						test_file $binfile $channels $bps "--lax -b 16384 -m -r 8 -l 32 -e -p $disable"
					fi
				done
			done
		done
	else
		echo "$binfile not found, skipping."
	fi
done
