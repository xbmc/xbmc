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

dddie="die ERROR: creating files with dd"

if [ x = x"$1" ] ; then
	BUILD=debug
else
	BUILD="$1"
fi

# change to 'false' to show flac output (useful for debugging)
if true ; then
	SILENT='--silent'
	TOTALLY_SILENT='--totally-silent'
else
	SILENT=''
	TOTALLY_SILENT=''
fi

LD_LIBRARY_PATH=`pwd`/../src/libFLAC/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=`pwd`/../src/share/grabbag/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=`pwd`/../src/share/getopt/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=`pwd`/../src/share/replaygain_analysis/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=`pwd`/../src/share/replaygain_synthesis/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=`pwd`/../src/share/utf8/.libs:$LD_LIBRARY_PATH
LD_LIBRARY_PATH=`pwd`/../obj/$BUILD/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
PATH=`pwd`/../src/flac:$PATH
PATH=`pwd`/../src/metaflac:$PATH
PATH=`pwd`/../src/test_streams:$PATH
PATH=`pwd`/../obj/$BUILD/bin:$PATH

flac --help 1>/dev/null 2>/dev/null || die "ERROR can't find flac executable"

run_flac ()
{
	if [ x"$FLAC__TEST_WITH_VALGRIND" = xyes ] ; then
		echo "valgrind --leak-check=yes --show-reachable=yes --num-callers=100 flac $*" >>test_flac.valgrind.log
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --log-fd=4 flac $* 4>>test_flac.valgrind.log
	else
		flac $*
	fi
}

run_metaflac ()
{
	if [ x"$FLAC__TEST_WITH_VALGRIND" = xyes ] ; then
		echo "valgrind --leak-check=yes --show-reachable=yes --num-callers=100 metaflac $*" >>test_flac.valgrind.log
		valgrind --leak-check=yes --show-reachable=yes --num-callers=100 --log-fd=4 metaflac $* 4>>test_flac.valgrind.log
	else
		metaflac $*
	fi
}

md5cmp ()
{
	#n=`( [ -f "$1" ] && [ -f "$2" ] && metaflac --show-md5sum --no-filename "$1" "$2" 2>/dev/null || die "ERROR: comparing FLAC files $1 and $2 by MD5 sum" ) | uniq | wc -l`
	n=`( [ -f "$1" ] && [ -f "$2" ] && metaflac --show-md5sum --no-filename "$1" "$2" 2>/dev/null || exit 1 ) | uniq | wc -l`
	[ "$n" != "" ] && [ $n = 1 ]
}

if [ `env | grep -ic '^comspec='` != 0 ] ; then
	is_win=yes
else
	is_win=no
fi

echo "Checking for --ogg support in flac..."
if flac --ogg $SILENT --force-raw-format --endian=little --sign=signed --channels=1 --bps=8 --sample-rate=44100 -c $0 1>/dev/null 2>&1 ; then
	has_ogg=yes;
	echo "flac --ogg works"
else
	has_ogg=no;
	echo "flac --ogg doesn't work"
fi

echo "Generating streams..."
if [ ! -f wacky1.wav ] ; then
	test_streams || die "ERROR during test_streams"
fi

############################################################################
# test that flac doesn't automatically overwrite files unless -f is used
############################################################################

echo "Try encoding to a file that exists; should fail"
cp wacky1.wav exist.wav
touch exist.flac
if run_flac $TOTALLY_SILENT -0 exist.wav ; then
	die "ERROR: it should have failed but didn't"
else
	echo "OK, it failed as it should"
fi

echo "Try encoding with -f to a file that exists; should succeed"
if run_flac $TOTALLY_SILENT -0 --force exist.wav ; then
	echo "OK, it succeeded as it should"
else
	die "ERROR: it should have succeeded but didn't"
fi

echo "Try decoding to a file that exists; should fail"
if run_flac $TOTALLY_SILENT -d exist.flac ; then
	die "ERROR: it should have failed but didn't"
else
	echo "OK, it failed as it should"
fi

echo "Try decoding with -f to a file that exists; should succeed"
if run_flac $TOTALLY_SILENT -d -f exist.flac ; then
	echo "OK, it succeeded as it should"
else
	die "ERROR: it should have succeeded but didn't"
fi

rm -f exist.wav exist.flac

############################################################################
# test fractional block sizes
############################################################################

test_fractional ()
{
	blocksize=$1
	samples=$2
	dd if=noise.raw ibs=4 count=$samples of=pbs.raw 2>/dev/null || $dddie
	echo -n "fractional block size test (blocksize=$blocksize samples=$samples) encode... "
	run_flac $SILENT --force --verify --force-raw-format --endian=little --sign=signed --sample-rate=44100 --bps=16 --channels=2 --blocksize=$blocksize --no-padding --lax -o pbs.flac pbs.raw || die "ERROR"
	echo -n "decode... "
	run_flac $SILENT --force --decode --force-raw-format --endian=little --sign=signed -o pbs.cmp pbs.flac || die "ERROR"
	echo -n "compare... "
	cmp pbs.raw pbs.cmp || die "ERROR: file mismatch"
	echo "OK"
	rm -f pbs.raw pbs.flac pbs.cmp
}

# The special significance of 2048 is it's the # of samples that flac calls
# FLAC__stream_encoder_process() on.
#
# We're trying to make sure the 1-sample overread logic in the stream encoder
# (used for last-block checking) works; these values probe around common
# multiples of the flac sample chunk size (2048) and the blocksize.
for samples in 31 32 33 34 35 2046 2047 2048 2049 2050 ; do
	test_fractional 33 $samples
done
for samples in 254 255 256 257 258 510 511 512 513 514 1022 1023 1024 1025 1026 2046 2047 2048 2049 2050 4094 4095 4096 4097 4098 ; do
	test_fractional 256 $samples
done
for samples in 1022 1023 1024 1025 1026 2046 2047 2048 2049 2050 4094 4095 4096 4097 4098 ; do
	test_fractional 2048 $samples
done
for samples in 1022 1023 1024 1025 1026 2046 2047 2048 2049 2050 4094 4095 4096 4097 4098 4606 4607 4608 4609 4610 8190 8191 8192 8193 8194 16382 16383 16384 16385 16386 ; do
	test_fractional 4608 $samples
done

############################################################################
# basic 'round-trip' tests of various kinds of streams
############################################################################

rt_test_raw ()
{
	f="$1"
	channels=`echo $f | awk -F- '{print $2}'`
	bps=`echo $f | awk -F- '{print $3}'`
	echo -n "round-trip test ($f) encode... "
	run_flac $SILENT --force --verify --force-raw-format --endian=little --sign=signed --sample-rate=44100 --bps=$bps --channels=$channels --no-padding --lax -o rt.flac $f || die "ERROR"
	echo -n "decode... "
	run_flac $SILENT --force --decode --force-raw-format --endian=little --sign=signed -o rt.raw rt.flac || die "ERROR"
	echo -n "compare... "
	cmp $f rt.raw || die "ERROR: file mismatch"
	echo "OK"
	rm -f rt.flac rt.raw
}

rt_test_wav ()
{
	f="$1"
	echo -n "round-trip test ($f) encode... "
	run_flac $SILENT --force --verify --channel-map=none --no-padding --lax -o rt.flac $f || die "ERROR"
	echo -n "decode... "
	run_flac $SILENT --force --decode --channel-map=none -o rt.wav rt.flac || die "ERROR"
	echo -n "compare... "
	cmp $f rt.wav || die "ERROR: file mismatch"
	echo "OK"
	rm -f rt.flac rt.wav
}

rt_test_aiff ()
{
	f="$1"
	echo -n "round-trip test ($f) encode... "
	run_flac $SILENT --force --verify --channel-map=none --no-padding --lax -o rt.flac $f || die "ERROR"
	echo -n "decode... "
	run_flac $SILENT --force --decode --channel-map=none -o rt.aiff rt.flac || die "ERROR"
	echo -n "compare... "
	cmp $f rt.aiff || die "ERROR: file mismatch"
	echo "OK"
	rm -f rt.flac rt.aiff
}

# assumes input file is WAVE; does not check the metadata-preserving features of flac-to-flac; that is checked later
rt_test_flac ()
{
	f="$1"
	echo -n "round-trip test ($f->flac->flac->wav) encode... "
	run_flac $SILENT --force --verify --channel-map=none --no-padding --lax -o rt.flac $f || die "ERROR"
	echo -n "re-encode... "
	run_flac $SILENT --force --verify --lax -o rt2.flac rt.flac || die "ERROR"
	echo -n "decode... "
	run_flac $SILENT --force --decode --channel-map=none -o rt.wav rt2.flac || die "ERROR"
	echo -n "compare... "
	cmp $f rt.wav || die "ERROR: file mismatch"
	echo "OK"
	rm -f rt.wav rt.flac rt2.flac
}

# assumes input file is WAVE; does not check the metadata-preserving features of flac-to-flac; that is checked later
rt_test_ogg_flac ()
{
	f="$1"
	echo -n "round-trip test ($f->oggflac->oggflac->wav) encode... "
	run_flac $SILENT --force --verify --channel-map=none --no-padding --lax -o rt.oga --ogg $f || die "ERROR"
	echo -n "re-encode... "
	run_flac $SILENT --force --verify --lax -o rt2.oga --ogg rt.oga || die "ERROR"
	echo -n "decode... "
	run_flac $SILENT --force --decode --channel-map=none -o rt.wav rt2.oga || die "ERROR"
	echo -n "compare... "
	cmp $f rt.wav || die "ERROR: file mismatch"
	echo "OK"
	rm -f rt.wav rt.oga rt2.oga
}

for f in rt-*.raw ; do
	rt_test_raw $f
done
for f in rt-*.wav ; do
	rt_test_wav $f
done
for f in rt-*.aiff ; do
	rt_test_aiff $f
done
for f in rt-*.wav ; do
	rt_test_flac $f
done
if [ $has_ogg = yes ] ; then
	for f in rt-*.wav ; do
		rt_test_ogg_flac $f
	done
fi

############################################################################
# test --skip and --until
############################################################################

#
# first make some chopped-up raw files
#
echo "abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMN" > master.raw
dd if=master.raw ibs=1 count=50 of=50c.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=10 count=40 of=50c.skip10.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=11 count=39 of=50c.skip11.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=20 count=30 of=50c.skip20.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=30 count=20 of=50c.skip30.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=40 count=10 of=50c.skip40.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 count=10 of=50c.until10.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 count=20 of=50c.until20.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 count=30 of=50c.until30.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 count=39 of=50c.until39.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 count=40 of=50c.until40.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=10 count=20 of=50c.skip10.until30.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=10 count=29 of=50c.skip10.until39.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=10 count=30 of=50c.skip10.until40.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=20 count=10 of=50c.skip20.until30.raw 2>/dev/null || $dddie
dd if=master.raw ibs=1 skip=20 count=20 of=50c.skip20.until40.raw 2>/dev/null || $dddie

wav_eopt="$SILENT --force --verify --no-padding --lax"
wav_dopt="$SILENT --force --decode"

raw_eopt="$wav_eopt --force-raw-format --endian=big --sign=signed --sample-rate=10 --bps=8 --channels=1"
raw_dopt="$wav_dopt --force-raw-format --endian=big --sign=signed"

#
# convert them to WAVE/AIFF/Ogg FLAC files
#
convert_to_wav ()
{
	run_flac "$2" $1.raw || die "ERROR converting $1.raw to WAVE"
	run_flac "$3" $1.flac || die "ERROR converting $1.raw to WAVE"
}
convert_to_wav 50c "$raw_eopt" "$wav_dopt"
convert_to_wav 50c.skip10 "$raw_eopt" "$wav_dopt"
convert_to_wav 50c.skip11 "$raw_eopt" "$wav_dopt"
convert_to_wav 50c.skip20 "$raw_eopt" "$wav_dopt"
convert_to_wav 50c.skip30 "$raw_eopt" "$wav_dopt"
convert_to_wav 50c.skip40 "$raw_eopt" "$wav_dopt"
convert_to_wav 50c.until10 "$raw_eopt" "$wav_dopt"
convert_to_wav 50c.until20 "$raw_eopt" "$wav_dopt"
convert_to_wav 50c.until30 "$raw_eopt" "$wav_dopt"
convert_to_wav 50c.until39 "$raw_eopt" "$wav_dopt"
convert_to_wav 50c.until40 "$raw_eopt" "$wav_dopt"
convert_to_wav 50c.skip10.until30 "$raw_eopt" "$wav_dopt"
convert_to_wav 50c.skip10.until39 "$raw_eopt" "$wav_dopt"
convert_to_wav 50c.skip10.until40 "$raw_eopt" "$wav_dopt"
convert_to_wav 50c.skip20.until30 "$raw_eopt" "$wav_dopt"
convert_to_wav 50c.skip20.until40 "$raw_eopt" "$wav_dopt"

convert_to_aiff ()
{
	run_flac "$2" $1.raw || die "ERROR converting $1.raw to AIFF"
	run_flac "$3" $1.flac -o $1.aiff || die "ERROR converting $1.raw to AIFF"
}
convert_to_aiff 50c "$raw_eopt" "$wav_dopt"
convert_to_aiff 50c.skip10 "$raw_eopt" "$wav_dopt"
convert_to_aiff 50c.skip11 "$raw_eopt" "$wav_dopt"
convert_to_aiff 50c.skip20 "$raw_eopt" "$wav_dopt"
convert_to_aiff 50c.skip30 "$raw_eopt" "$wav_dopt"
convert_to_aiff 50c.skip40 "$raw_eopt" "$wav_dopt"
convert_to_aiff 50c.until10 "$raw_eopt" "$wav_dopt"
convert_to_aiff 50c.until20 "$raw_eopt" "$wav_dopt"
convert_to_aiff 50c.until30 "$raw_eopt" "$wav_dopt"
convert_to_aiff 50c.until39 "$raw_eopt" "$wav_dopt"
convert_to_aiff 50c.until40 "$raw_eopt" "$wav_dopt"
convert_to_aiff 50c.skip10.until30 "$raw_eopt" "$wav_dopt"
convert_to_aiff 50c.skip10.until39 "$raw_eopt" "$wav_dopt"
convert_to_aiff 50c.skip10.until40 "$raw_eopt" "$wav_dopt"
convert_to_aiff 50c.skip20.until30 "$raw_eopt" "$wav_dopt"
convert_to_aiff 50c.skip20.until40 "$raw_eopt" "$wav_dopt"

convert_to_ogg ()
{
	run_flac "$wav_eopt" --ogg $1.wav || die "ERROR converting $1.raw to Ogg FLAC"
}
if [ $has_ogg = yes ] ; then
	convert_to_ogg 50c
	convert_to_ogg 50c.skip10
	convert_to_ogg 50c.skip11
	convert_to_ogg 50c.skip20
	convert_to_ogg 50c.skip30
	convert_to_ogg 50c.skip40
	convert_to_ogg 50c.until10
	convert_to_ogg 50c.until20
	convert_to_ogg 50c.until30
	convert_to_ogg 50c.until39
	convert_to_ogg 50c.until40
	convert_to_ogg 50c.skip10.until30
	convert_to_ogg 50c.skip10.until39
	convert_to_ogg 50c.skip10.until40
	convert_to_ogg 50c.skip20.until30
	convert_to_ogg 50c.skip20.until40
fi

test_skip_until ()
{
	in_fmt=$1
	out_fmt=$2

	[ "$in_fmt" = wav ] || [ "$in_fmt" = aiff ] || [ "$in_fmt" = raw ] || [ "$in_fmt" = flac ] || [ "$in_fmt" = ogg ] || die "ERROR: internal error, bad 'in' format '$in_fmt'"

	[ "$out_fmt" = flac ] || [ "$out_fmt" = ogg ] || die "ERROR: internal error, bad 'out' format '$out_fmt'"

	if [ $in_fmt = raw ] ; then
		eopt="$raw_eopt"
		dopt="$raw_dopt"
	else
		eopt="$wav_eopt"
		dopt="$wav_dopt"
	fi

	if ( [ $in_fmt = flac ] || [ $in_fmt = ogg ] ) && ( [ $out_fmt = flac ] || [ $out_fmt = ogg ] ) ; then
		CMP=md5cmp
	else
		CMP=cmp
	fi

	if [ $out_fmt = ogg ] ; then
		eopt="--ogg $eopt"
	fi

	#
	# test --skip when encoding
	#

	desc="($in_fmt<->$out_fmt)"

	echo -n "testing --skip=# (encode) $desc... "
	run_flac $eopt --skip=10 -o z50c.skip10.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.skip10.$in_fmt z50c.skip10.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.$in_fmt z50c.skip10.$in_fmt || die "ERROR: file mismatch for --skip=10 (encode) $desc"
	rm -f z50c.skip10.$out_fmt z50c.skip10.$in_fmt
	echo OK

	echo -n "testing --skip=mm:ss (encode) $desc... "
	run_flac $eopt --skip=0:01 -o z50c.skip0:01.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.skip0:01.$in_fmt z50c.skip0:01.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.$in_fmt z50c.skip0:01.$in_fmt || die "ERROR: file mismatch for --skip=0:01 (encode) $desc"
	rm -f z50c.skip0:01.$out_fmt z50c.skip0:01.$in_fmt
	echo OK

	echo -n "testing --skip=mm:ss.sss (encode) $desc... "
	run_flac $eopt --skip=0:01.1001 -o z50c.skip0:01.1001.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.skip0:01.1001.$in_fmt z50c.skip0:01.1001.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip11.$in_fmt z50c.skip0:01.1001.$in_fmt || die "ERROR: file mismatch for --skip=0:01.1001 (encode) $desc"
	rm -f z50c.skip0:01.1001.$out_fmt z50c.skip0:01.1001.$in_fmt
	echo OK

	#
	# test --skip when decoding
	#

	if [ $in_fmt != $out_fmt ] ; then run_flac $eopt -o z50c.$out_fmt 50c.$in_fmt ; else cp -f 50c.$in_fmt z50c.$out_fmt ; fi || die "ERROR generating FLAC file $desc"

	echo -n "testing --skip=# (decode) $desc... "
	run_flac $dopt --skip=10 -o z50c.skip10.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.$in_fmt z50c.skip10.$in_fmt || die "ERROR: file mismatch for --skip=10 (decode) $desc"
	rm -f z50c.skip10.$in_fmt
	echo OK

	echo -n "testing --skip=mm:ss (decode) $desc... "
	run_flac $dopt --skip=0:01 -o z50c.skip0:01.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.$in_fmt z50c.skip0:01.$in_fmt || die "ERROR: file mismatch for --skip=0:01 (decode) $desc"
	rm -f z50c.skip0:01.$in_fmt
	echo OK

	echo -n "testing --skip=mm:ss.sss (decode) $desc... "
	run_flac $dopt --skip=0:01.1001 -o z50c.skip0:01.1001.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip11.$in_fmt z50c.skip0:01.1001.$in_fmt || die "ERROR: file mismatch for --skip=0:01.1001 (decode) $desc"
	rm -f z50c.skip0:01.1001.$in_fmt
	echo OK

	rm -f z50c.$out_fmt

	#
	# test --until when encoding
	#

	echo -n "testing --until=# (encode) $desc... "
	run_flac $eopt --until=40 -o z50c.until40.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.until40.$in_fmt z50c.until40.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.until40.$in_fmt z50c.until40.$in_fmt || die "ERROR: file mismatch for --until=40 (encode) $desc"
	rm -f z50c.until40.$out_fmt z50c.until40.$in_fmt
	echo OK

	echo -n "testing --until=mm:ss (encode) $desc... "
	run_flac $eopt --until=0:04 -o z50c.until0:04.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.until0:04.$in_fmt z50c.until0:04.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.until40.$in_fmt z50c.until0:04.$in_fmt || die "ERROR: file mismatch for --until=0:04 (encode) $desc"
	rm -f z50c.until0:04.$out_fmt z50c.until0:04.$in_fmt
	echo OK

	echo -n "testing --until=mm:ss.sss (encode) $desc... "
	run_flac $eopt --until=0:03.9001 -o z50c.until0:03.9001.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.until0:03.9001.$in_fmt z50c.until0:03.9001.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.until39.$in_fmt z50c.until0:03.9001.$in_fmt || die "ERROR: file mismatch for --until=0:03.9001 (encode) $desc"
	rm -f z50c.until0:03.9001.$out_fmt z50c.until0:03.9001.$in_fmt
	echo OK

	echo -n "testing --until=-# (encode) $desc... "
	run_flac $eopt --until=-10 -o z50c.until-10.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.until-10.$in_fmt z50c.until-10.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.until40.$in_fmt z50c.until-10.$in_fmt || die "ERROR: file mismatch for --until=-10 (encode) $desc"
	rm -f z50c.until-10.$out_fmt z50c.until-10.$in_fmt
	echo OK

	echo -n "testing --until=-mm:ss (encode) $desc... "
	run_flac $eopt --until=-0:01 -o z50c.until-0:01.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.until-0:01.$in_fmt z50c.until-0:01.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.until40.$in_fmt z50c.until-0:01.$in_fmt || die "ERROR: file mismatch for --until=-0:01 (encode) $desc"
	rm -f z50c.until-0:01.$out_fmt z50c.until-0:01.$in_fmt
	echo OK

	echo -n "testing --until=-mm:ss.sss (encode) $desc... "
	run_flac $eopt --until=-0:01.1001 -o z50c.until-0:01.1001.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.until-0:01.1001.$in_fmt z50c.until-0:01.1001.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.until39.$in_fmt z50c.until-0:01.1001.$in_fmt || die "ERROR: file mismatch for --until=-0:01.1001 (encode) $desc"
	rm -f z50c.until-0:01.1001.$out_fmt z50c.until-0:01.1001.$in_fmt
	echo OK

	#
	# test --until when decoding
	#

	if [ $in_fmt != $out_fmt ] ; then run_flac $eopt -o z50c.$out_fmt 50c.$in_fmt ; else cp -f 50c.$in_fmt z50c.$out_fmt ; fi || die "ERROR generating FLAC file $desc"

	echo -n "testing --until=# (decode) $desc... "
	run_flac $dopt --until=40 -o z50c.until40.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.until40.$in_fmt z50c.until40.$in_fmt || die "ERROR: file mismatch for --until=40 (decode) $desc"
	rm -f z50c.until40.$in_fmt
	echo OK

	echo -n "testing --until=mm:ss (decode) $desc... "
	run_flac $dopt --until=0:04 -o z50c.until0:04.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.until40.$in_fmt z50c.until0:04.$in_fmt || die "ERROR: file mismatch for --until=0:04 (decode) $desc"
	rm -f z50c.until0:04.$in_fmt
	echo OK

	echo -n "testing --until=mm:ss.sss (decode) $desc... "
	run_flac $dopt --until=0:03.9001 -o z50c.until0:03.9001.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.until39.$in_fmt z50c.until0:03.9001.$in_fmt || die "ERROR: file mismatch for --until=0:03.9001 (decode) $desc"
	rm -f z50c.until0:03.9001.$in_fmt
	echo OK

	echo -n "testing --until=-# (decode) $desc... "
	run_flac $dopt --until=-10 -o z50c.until-10.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.until40.$in_fmt z50c.until-10.$in_fmt || die "ERROR: file mismatch for --until=-10 (decode) $desc"
	rm -f z50c.until-10.$in_fmt
	echo OK

	echo -n "testing --until=-mm:ss (decode) $desc... "
	run_flac $dopt --until=-0:01 -o z50c.until-0:01.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.until40.$in_fmt z50c.until-0:01.$in_fmt || die "ERROR: file mismatch for --until=-0:01 (decode) $desc"
	rm -f z50c.until-0:01.$in_fmt
	echo OK

	echo -n "testing --until=-mm:ss.sss (decode) $desc... "
	run_flac $dopt --until=-0:01.1001 -o z50c.until-0:01.1001.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.until39.$in_fmt z50c.until-0:01.1001.$in_fmt || die "ERROR: file mismatch for --until=-0:01.1001 (decode) $desc"
	rm -f z50c.until-0:01.1001.$in_fmt
	echo OK

	rm -f z50c.$out_fmt

	#
	# test --skip and --until when encoding
	#

	echo -n "testing --skip=10 --until=# (encode) $desc... "
	run_flac $eopt --skip=10 --until=40 -o z50c.skip10.until40.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.skip10.until40.$in_fmt z50c.skip10.until40.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until40.$in_fmt z50c.skip10.until40.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=40 (encode) $desc"
	rm -f z50c.skip10.until40.$out_fmt z50c.skip10.until40.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=mm:ss (encode) $desc... "
	run_flac $eopt --skip=10 --until=0:04 -o z50c.skip10.until0:04.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.skip10.until0:04.$in_fmt z50c.skip10.until0:04.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until40.$in_fmt z50c.skip10.until0:04.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=0:04 (encode) $desc"
	rm -f z50c.skip10.until0:04.$out_fmt z50c.skip10.until0:04.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=mm:ss.sss (encode) $desc... "
	run_flac $eopt --skip=10 --until=0:03.9001 -o z50c.skip10.until0:03.9001.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.skip10.until0:03.9001.$in_fmt z50c.skip10.until0:03.9001.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until39.$in_fmt z50c.skip10.until0:03.9001.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=0:03.9001 (encode) $desc"
	rm -f z50c.skip10.until0:03.9001.$out_fmt z50c.skip10.until0:03.9001.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=+# (encode) $desc... "
	run_flac $eopt --skip=10 --until=+30 -o z50c.skip10.until+30.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.skip10.until+30.$in_fmt z50c.skip10.until+30.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until40.$in_fmt z50c.skip10.until+30.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=+30 (encode) $desc"
	rm -f z50c.skip10.until+30.$out_fmt z50c.skip10.until+30.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=+mm:ss (encode) $desc... "
	run_flac $eopt --skip=10 --until=+0:03 -o z50c.skip10.until+0:03.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.skip10.until+0:03.$in_fmt z50c.skip10.until+0:03.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until40.$in_fmt z50c.skip10.until+0:03.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=+0:03 (encode) $desc"
	rm -f z50c.skip10.until+0:03.$out_fmt z50c.skip10.until+0:03.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=+mm:ss.sss (encode) $desc... "
	run_flac $eopt --skip=10 --until=+0:02.9001 -o z50c.skip10.until+0:02.9001.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.skip10.until+0:02.9001.$in_fmt z50c.skip10.until+0:02.9001.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until39.$in_fmt z50c.skip10.until+0:02.9001.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=+0:02.9001 (encode) $desc"
	rm -f z50c.skip10.until+0:02.9001.$out_fmt z50c.skip10.until+0:02.9001.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=-# (encode) $desc... "
	run_flac $eopt --skip=10 --until=-10 -o z50c.skip10.until-10.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.skip10.until-10.$in_fmt z50c.skip10.until-10.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until40.$in_fmt z50c.skip10.until-10.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=-10 (encode) $desc"
	rm -f z50c.skip10.until-10.$out_fmt z50c.skip10.until-10.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=-mm:ss (encode) $desc... "
	run_flac $eopt --skip=10 --until=-0:01 -o z50c.skip10.until-0:01.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.skip10.until-0:01.$in_fmt z50c.skip10.until-0:01.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until40.$in_fmt z50c.skip10.until-0:01.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=-0:01 (encode) $desc"
	rm -f z50c.skip10.until-0:01.$out_fmt z50c.skip10.until-0:01.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=-mm:ss.sss (encode) $desc... "
	run_flac $eopt --skip=10 --until=-0:01.1001 -o z50c.skip10.until-0:01.1001.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"
	[ $in_fmt = $out_fmt ] || run_flac $dopt -o z50c.skip10.until-0:01.1001.$in_fmt z50c.skip10.until-0:01.1001.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until39.$in_fmt z50c.skip10.until-0:01.1001.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=-0:01.1001 (encode) $desc"
	rm -f z50c.skip10.until-0:01.1001.$out_fmt z50c.skip10.until-0:01.1001.$in_fmt
	echo OK

	#
	# test --skip and --until when decoding
	#

	if [ $in_fmt != $out_fmt ] ; then run_flac $eopt -o z50c.$out_fmt 50c.$in_fmt ; else cp -f 50c.$in_fmt z50c.$out_fmt ; fi || die "ERROR generating FLAC file $desc"


	echo -n "testing --skip=10 --until=# (decode) $desc... "
	run_flac $dopt --skip=10 --until=40 -o z50c.skip10.until40.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until40.$in_fmt z50c.skip10.until40.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=40 (decode) $desc"
	rm -f z50c.skip10.until40.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=mm:ss (decode) $desc... "
	run_flac $dopt --skip=10 --until=0:04 -o z50c.skip10.until0:04.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until40.$in_fmt z50c.skip10.until0:04.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=0:04 (decode) $desc"
	rm -f z50c.skip10.until0:04.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=mm:ss.sss (decode) $desc... "
	run_flac $dopt --skip=10 --until=0:03.9001 -o z50c.skip10.until0:03.9001.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until39.$in_fmt z50c.skip10.until0:03.9001.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=0:03.9001 (decode) $desc"
	rm -f z50c.skip10.until0:03.9001.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=-# (decode) $desc... "
	run_flac $dopt --skip=10 --until=-10 -o z50c.skip10.until-10.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until40.$in_fmt z50c.skip10.until-10.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=-10 (decode) $desc"
	rm -f z50c.skip10.until-10.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=-mm:ss (decode) $desc... "
	run_flac $dopt --skip=10 --until=-0:01 -o z50c.skip10.until-0:01.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until40.$in_fmt z50c.skip10.until-0:01.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=-0:01 (decode) $desc"
	rm -f z50c.skip10.until-0:01.$in_fmt
	echo OK

	echo -n "testing --skip=10 --until=-mm:ss.sss (decode) $desc... "
	run_flac $dopt --skip=10 --until=-0:01.1001 -o z50c.skip10.until-0:01.1001.$in_fmt z50c.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until39.$in_fmt z50c.skip10.until-0:01.1001.$in_fmt || die "ERROR: file mismatch for --skip=10 --until=-0:01.1001 (decode) $desc"
	rm -f z50c.skip10.until-0:01.1001.$in_fmt
	echo OK

	rm -f z50c.$out_fmt
}

test_skip_until raw flac
test_skip_until wav flac
test_skip_until aiff flac
test_skip_until flac flac
#@@@if [ $has_ogg = yes ] ; then
#@@@	#@@@ doesn't work yet because md5cmp doesn't work because metaflac doesn't work on ogg flac yet
#@@@	test_skip_until ogg flac
#@@@fi

if [ $has_ogg = yes ] ; then
	test_skip_until raw ogg
	test_skip_until wav ogg
	test_skip_until aiff ogg
	#@@@ doesn't work yet because md5cmp doesn't work because metaflac doesn't work on ogg flac yet
	#@@@test_skip_until flac ogg
	#@@@test_skip_until ogg ogg
fi

echo "testing seek extremes:"

run_flac --verify --force $SILENT --no-padding --force-raw-format --endian=big --sign=signed --sample-rate=44100 --bps=16 --channels=2 --blocksize=576 noise.raw || die "ERROR generating FLAC file"

if [ $is_win = no ] ; then
	total_noise_cdda_samples=`run_metaflac --show-total-samples noise.flac`
	[ $? = 0 ] || die "ERROR getting total sample count from noise.flac"
else
	# some flavors of cygwin don't seem to treat the \x0d as a word
	# separator, so we hard code it.  we'll just have to fix it later
	# if we change the way noise.flac is made.
	total_noise_cdda_samples=393216
fi

echo -n "testing --skip=0... "
run_flac $wav_dopt --skip=0 -o z.wav noise.flac || die "ERROR decoding FLAC file noise.flac"
echo OK

for delta in 2 1 ; do
	n=`expr $total_noise_cdda_samples - $delta`
	echo -n "testing --skip=$n... "
	run_flac $wav_dopt --skip=$n -o z.wav noise.flac || die "ERROR decoding FLAC file noise.flac"
	echo OK
done

rm noise.flac z.wav


############################################################################
# test --input-size
############################################################################

#@@@ cat will not work on old cygwin, need to fix
if [ $is_win = no ] ; then
	echo -n "testing --input-size=50 --skip=10... "
	cat 50c.raw | run_flac $raw_eopt --input-size=50 --skip=10 -o z50c.skip10.flac - || die "ERROR generating FLAC file"
	run_flac $raw_dopt -o z50c.skip10.raw z50c.skip10.flac || die "ERROR decoding FLAC file"
	cmp 50c.skip10.raw z50c.skip10.raw || die "ERROR: file mismatch for --input-size=50 --skip=10"
	rm -f z50c.skip10.raw z50c.skip10.flac
	echo OK
fi


############################################################################
# test --cue
############################################################################

#
# create the cue sheet
#
cuesheet=cuetest.cue
cat > $cuesheet << EOF
CATALOG 1234567890123
FILE "blah" WAVE
  TRACK 01 AUDIO
    INDEX 01 0
    INDEX 02 10
    INDEX 03 20
  TRACK 02 AUDIO
    INDEX 01 30
  TRACK 04 AUDIO
    INDEX 01 40
EOF

test_cue ()
{
	in_fmt=$1
	out_fmt=$2

	[ "$in_fmt" = wav ] || [ "$in_fmt" = aiff ] || [ "$in_fmt" = raw ] || [ "$in_fmt" = flac ] || [ "$in_fmt" = ogg ] || die "ERROR: internal error, bad 'in' format '$in_fmt'"

	[ "$out_fmt" = flac ] || [ "$out_fmt" = ogg ] || die "ERROR: internal error, bad 'out' format '$out_fmt'"

	if [ $in_fmt = raw ] ; then
		eopt="$raw_eopt"
		dopt="$raw_dopt"
	else
		eopt="$wav_eopt"
		dopt="$wav_dopt"
	fi

	if ( [ $in_fmt = flac ] || [ $in_fmt = ogg ] ) && ( [ $out_fmt = flac ] || [ $out_fmt = ogg ] ) ; then
		CMP=md5cmp
	else
		CMP=cmp
	fi

	if [ $out_fmt = ogg ] ; then
		eopt="--ogg $eopt"
	fi

	desc="($in_fmt<->$out_fmt)"

	#
	# for this we need just need just one FLAC file; --cue only works while decoding
	#
	run_flac $eopt --cuesheet=$cuesheet -o z50c.cue.$out_fmt 50c.$in_fmt || die "ERROR generating FLAC file $desc"

	# To make it easy to translate from cue point to sample numbers, the
	# file has a sample rate of 10 Hz and a cuesheet like so:
	#
	# TRACK 01, INDEX 01 : 0:00.00 -> sample 0
	# TRACK 01, INDEX 02 : 0:01.00 -> sample 10
	# TRACK 01, INDEX 03 : 0:02.00 -> sample 20
	# TRACK 02, INDEX 01 : 0:03.00 -> sample 30
	# TRACK 04, INDEX 01 : 0:04.00 -> sample 40
	#
	echo -n "testing --cue=- $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=- z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=- $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=1.0 $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=1.0 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=1.0 $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=1.0- $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=1.0- z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=1.0- $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=1.1 $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=1.1 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=1.1 $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=1.1- $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=1.1- z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=1.1- $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=1.2 $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=1.2 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=1.2 $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=1.2- $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=1.2- z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=1.2- $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=1.4 $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=1.4 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip20.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=1.4 $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=1.4- $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=1.4- z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip20.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=1.4- $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=-5.0 $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=-5.0 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=-5.0 $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=-4.1 $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=-4.1 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.until40.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=-4.1 $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=-3.1 $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=-3.1 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.until40.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=-3.1 $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=-1.4 $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=-1.4 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.until30.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=-1.4 $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=1.0-5.0 $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=1.0-5.0 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=1.0-5.0 $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=1.1-5.0 $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=1.1-5.0 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=1.1-5.0 $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=1.2-4.1 $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=1.2-4.1 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip10.until40.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=1.2-4.1 $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	echo -n "testing --cue=1.4-2.0 $desc... "
	run_flac $dopt -o z50c.cued.$in_fmt --cue=1.4-2.0 z50c.cue.$out_fmt || die "ERROR decoding FLAC file $desc"
	$CMP 50c.skip20.until30.$in_fmt z50c.cued.$in_fmt || die "ERROR: file mismatch for --cue=1.4-2.0 $desc"
	rm -f z50c.cued.$in_fmt
	echo OK

	rm -f z50c.cue.$out_fmt
}

test_cue raw flac
test_cue wav flac
test_cue aiff flac
test_cue flac flac
#@@@if [ $has_ogg = yes ] ; then
#@@@	#@@@ doesn't work yet because md5cmp doesn't work because metaflac doesn't work on ogg flac yet
#@@@	test_cue ogg flac
#@@@fi

if [ $has_ogg = yes ] ; then
	test_cue raw ogg
	test_cue wav ogg
	test_cue aiff ogg
	#@@@ doesn't work yet because md5cmp doesn't work because metaflac doesn't work on ogg flac yet
	#@@@test_cue flac ogg
	#@@@test_cue ogg ogg
fi

############################################################################
# test 'fixup' code that happens when a FLAC file with total_samples == 0
# in the STREAMINFO block is converted to WAVE or AIFF, requiring the
# decoder go back and fix up the chunk headers
############################################################################

echo -n "WAVE fixup test... "

echo -n "prepare... "
convert_to_wav noise "$raw_eopt" "$wav_dopt" || die "ERROR creating reference WAVE"

echo -n "encode... "
# the pipe from 'cat' to 'flac' does not work on cygwin because of the EOF/
# binary-mode stdin problem, so we use an undocumented option to metaflac to
# set the total sample count to 0
if [ $is_win = yes ] ; then
	run_flac $raw_eopt noise.raw -o fixup.flac || die "ERROR generating FLAC file"
	run_metaflac --set-total-samples=0 fixup.flac 2> /dev/null
else
	cat noise.raw | run_flac $raw_eopt - -c > fixup.flac || die "ERROR generating FLAC file"
fi

echo -n "decode... "
run_flac $wav_dopt fixup.flac -o fixup.wav || die "ERROR decoding FLAC file"

echo -n "compare... "
cmp noise.wav fixup.wav || die "ERROR: file mismatch"

echo OK
rm -f noise.wav fixup.wav fixup.flac

echo -n "AIFF fixup test... "

echo -n "prepare... "
convert_to_aiff noise "$raw_eopt" "$wav_dopt" || die "ERROR creating reference AIFF"

echo -n "encode... "
# the pipe from 'cat' to 'flac' does not work on cygwin because of the EOF/
# binary-mode stdin problem, so we use an undocumented option to metaflac to
# set the total sample count to 0
if [ $is_win = yes ] ; then
	run_flac $raw_eopt noise.raw -o fixup.flac || die "ERROR generating FLAC file"
	run_metaflac --set-total-samples=0 fixup.flac 2> /dev/null
else
	cat noise.raw | run_flac $raw_eopt - -c > fixup.flac || die "ERROR generating FLAC file"
fi

echo -n "decode... "
run_flac $wav_dopt fixup.flac -o fixup.aiff || die "ERROR decoding FLAC file"

echo -n "compare... "
cmp noise.aiff fixup.aiff || die "ERROR: file mismatch"

echo OK
rm -f noise.aiff fixup.aiff fixup.flac


############################################################################
# multi-file tests
############################################################################

echo "Generating multiple input files from noise..."
multifile_format_decode="--endian=big --sign=signed"
multifile_format_encode="$multifile_format_decode --sample-rate=44100 --bps=16 --channels=2 --no-padding"
short_noise_cdda_samples=`expr $total_noise_cdda_samples / 8`
run_flac --verify --force $SILENT --force-raw-format $multifile_format_encode --until=$short_noise_cdda_samples -o shortnoise.flac noise.raw || die "ERROR generating FLAC file"
run_flac --decode --force $SILENT shortnoise.flac -o shortnoise.raw --force-raw-format $multifile_format_decode || die "ERROR generating RAW file"
run_flac --decode --force $SILENT shortnoise.flac || die "ERROR generating WAVE file"
run_flac --decode --force $SILENT shortnoise.flac -o shortnoise.aiff || die "ERROR generating AIFF file"
cp shortnoise.flac file0.flac
cp shortnoise.flac file1.flac
cp shortnoise.flac file2.flac
rm -f shortnoise.flac
cp shortnoise.wav file0.wav
cp shortnoise.wav file1.wav
cp shortnoise.wav file2.wav
rm -f shortnoise.wav
cp shortnoise.aiff file0.aiff
cp shortnoise.aiff file1.aiff
cp shortnoise.aiff file2.aiff
rm -f shortnoise.aiff
cp shortnoise.raw file0.raw
cp shortnoise.raw file1.raw
cp shortnoise.raw file2.raw
rm -f shortnoise.raw
# create authoritative sector-aligned files for comparison
file0_samples=`expr \( $short_noise_cdda_samples / 588 \) \* 588`
file0_remainder=`expr $short_noise_cdda_samples - $file0_samples`
file1_samples=`expr \( \( $file0_remainder + $short_noise_cdda_samples \) / 588 \) \* 588`
file1_remainder=`expr $file0_remainder + $short_noise_cdda_samples - $file1_samples`
file1_samples=`expr $file1_samples - $file0_remainder`
file2_samples=`expr \( \( $file1_remainder + $short_noise_cdda_samples \) / 588 \) \* 588`
file2_remainder=`expr $file1_remainder + $short_noise_cdda_samples - $file2_samples`
file2_samples=`expr $file2_samples - $file1_remainder`
if [ $file2_remainder != '0' ] ; then
	file2_samples=`expr $file2_samples + $file2_remainder`
	file2_remainder=`expr 588 - $file2_remainder`
fi

dd if=file0.raw ibs=4 count=$file0_samples of=file0s.raw 2>/dev/null || $dddie
dd if=file0.raw ibs=4 count=$file0_remainder of=file1s.raw skip=$file0_samples 2>/dev/null || $dddie
dd if=file1.raw ibs=4 count=$file1_samples of=z.raw 2>/dev/null || $dddie
cat z.raw >> file1s.raw || die "ERROR: cat-ing sector-aligned files"
dd if=file1.raw ibs=4 count=$file1_remainder of=file2s.raw skip=$file1_samples 2>/dev/null || $dddie
dd if=file2.raw ibs=4 count=$file2_samples of=z.raw 2>/dev/null || $dddie
cat z.raw >> file2s.raw || die "ERROR: cat-ing sector-aligned files"
dd if=/dev/zero ibs=4 count=$file2_remainder of=z.raw 2>/dev/null || $dddie
cat z.raw >> file2s.raw || die "ERROR: cat-ing sector-aligned files"
rm -f z.raw

convert_to_wav file0s "$multifile_format_encode --force" "$SILENT --force --decode" || die "ERROR creating authoritative sector-aligned WAVE"
convert_to_wav file1s "$multifile_format_encode --force" "$SILENT --force --decode" || die "ERROR creating authoritative sector-aligned WAVE"
convert_to_wav file2s "$multifile_format_encode --force" "$SILENT --force --decode" || die "ERROR creating authoritative sector-aligned WAVE"

convert_to_aiff file0s "$multifile_format_encode --force" "$SILENT --force --decode" || die "ERROR creating authoritative sector-aligned AIFF"
convert_to_aiff file1s "$multifile_format_encode --force" "$SILENT --force --decode" || die "ERROR creating authoritative sector-aligned AIFF"
convert_to_aiff file2s "$multifile_format_encode --force" "$SILENT --force --decode" || die "ERROR creating authoritative sector-aligned AIFF"

test_multifile ()
{
	input_type=$1
	streamtype=$2
	sector_align=$3
	encode_options="$4"

	extra_encode_options=""
	extra_decode_options=""
	if [ $input_type = "raw" ] ; then
		extra_encode_options="--force-raw-format $multifile_format_encode"
		extra_decode_options="--force-raw-format $multifile_format_decode"
	else
		if [ $input_type = "aiff" ] ; then
			extra_decode_options="--force-aiff-format"
		fi
	fi

	if [ $streamtype = ogg ] ; then
		suffix=oga
		encode_options="$encode_options --ogg"
	else
		suffix=flac
	fi

	if [ $sector_align = sector_align ] ; then
		encode_options="$encode_options --sector-align"
	fi

	if [ $input_type = flac ] || [ $input_type = ogg ] ; then
		CMP=md5cmp
	else
		CMP=cmp
	fi

	for n in 0 1 2 ; do
		cp file$n.$input_type file${n}x.$input_type
	done
	run_flac --force $encode_options $extra_encode_options file0x.$input_type file1x.$input_type file2x.$input_type || die "ERROR"
	run_flac --force --decode $extra_decode_options file0x.$suffix file1x.$suffix file2x.$suffix || die "ERROR"
	if [ $sector_align != sector_align ] ; then
		for n in 0 1 2 ; do
			$CMP file$n.$input_type file${n}x.$input_type || die "ERROR: file mismatch on file #$n"
		done
	else
		for n in 0 1 2 ; do
			$CMP file${n}s.$input_type file${n}x.$input_type || die "ERROR: file mismatch on file #$n"
		done
	fi
	for n in 0 1 2 ; do
		rm -f file${n}x.$suffix file${n}x.$input_type
	done
}

input_types="raw wav aiff flac"
#@@@ doesn't work yet because md5cmp doesn't work because metaflac doesn't work on ogg flac yet
#@@@if [ $has_ogg = yes ] ; then
#@@@	input_types="$input_types ogg"
#@@@fi
for input_type in $input_types ; do
	echo "Testing multiple $input_type files without verify..."
	test_multifile $input_type flac no_sector_align ""

	echo "Testing multiple $input_type files with verify..."
	test_multifile $input_type flac no_sector_align "--verify"

	if [ $input_type != flac ] && [ $input_type != ogg ] ; then # --sector-align not supported for FLAC input
		echo "Testing multiple $input_type files with --sector-align, without verify..."
		test_multifile $input_type flac sector_align ""

		echo "Testing multiple $input_type files with --sector-align, with verify..."
		test_multifile $input_type flac sector_align "--verify"
	fi

	if [ $has_ogg = yes ] ; then
		echo "Testing multiple $input_type files with --ogg, without verify..."
		test_multifile $input_type ogg no_sector_align ""

		echo "Testing multiple $input_type files with --ogg, with verify..."
		test_multifile $input_type ogg no_sector_align "--verify"

		if [ $input_type != flac ] ; then # --sector-align not supported for FLAC input
			echo "Testing multiple $input_type files with --ogg and --sector-align, without verify..."
			test_multifile $input_type ogg sector_align ""

			echo "Testing multiple $input_type files with --ogg and --sector-align, with verify..."
			test_multifile $input_type ogg sector_align "--verify"
		fi

		echo "Testing multiple $input_type files with --ogg and --serial-number, with verify..."
		test_multifile $input_type ogg no_sector_align "--serial-number=321 --verify"
	fi
done


############################################################################
# test the metadata-handling properties of flac-to-flac encoding
############################################################################

echo "Testing the metadata-handling properties of flac-to-flac encoding..."

testdir="flac-to-flac-metadata-test-files"
filter ()
{
	# minor danger, changing vendor strings might change the length of the
	# VORBIS_COMMENT block, but if we add "^  length: " to the patterns,
	# we lose info about PADDING size that we need
	grep -Ev '^  vendor string: |^  m..imum .....size: ' | sed -e 's/, stream_offset.*//'
}
flac2flac ()
{
	file="$1"
	case="$2"
	args="$3"
	expect="$case-expect.meta"
	echo -n "$case... "
	run_flac $SILENT -f -o out.flac $args $file || die "ERROR encoding FLAC file"
	run_metaflac --list out.flac | filter > out.meta || die "ERROR listing metadata of output FLAC file"
	diff -q -w $expect out.meta 2>/dev/null || die "ERROR: metadata does not match expected $expect"
	echo OK
}

#filter=', stream_offset.*|^  vendor string: |^  length: |^  m..imum .....size: '
cd $testdir || die "ERROR changing to directory $testdir"

# case 00a: no alterations on a file with all metadata types, keep all metadata, in same order
flac2flac input-SCVAUP.flac case00a ""
# case 01a: on file with multiple PADDING blocks, they should be aggregated into one at the end
flac2flac input-SCVPAP.flac case01a ""
# case 01b: on file with multiple PADDING blocks and --no-padding specified, they should all be deleted
flac2flac input-SCVPAP.flac case01b "--no-padding"
# case 01c: on file with multiple PADDING blocks and -P specified, they should all be overwritten with -P value
flac2flac input-SCVPAP.flac case01c "-P 1234"
# case 01d: on file with no PADDING blocks, use -P setting
flac2flac input-SCVA.flac case01d "-P 1234"
# case 01e: on file with no PADDING blocks and no -P given, use default padding
flac2flac input-SCVA.flac case01e ""
# case 02a: on file with no VORBIS_COMMENT block, add new VORBIS_COMMENT
flac2flac input-SCPAP.flac case02a ""
# case 02b: on file with no VORBIS_COMMENT block and --tag, add new VORBIS_COMMENT with tags
flac2flac input-SCPAP.flac case02b "--tag=artist=0"
# case 02c: on file with VORBIS_COMMENT block and --tag, replace existing VORBIS_COMMENT with new tags
flac2flac input-SCVAUP.flac case02c "$TOTALLY_SILENT --tag=artist=0"
# case 03a: on file with no CUESHEET block and --cuesheet specified, add it
flac2flac input-SVAUP.flac case03a "--cuesheet=input0.cue"
# case 03b: on file with CUESHEET block and --cuesheet specified, overwrite existing CUESHEET
flac2flac input-SCVAUP.flac case03b "$TOTALLY_SILENT --cuesheet=input0.cue"
# case 03c: on file with CUESHEET block and size-changing option specified, drop existing CUESHEET
flac2flac input-SCVAUP.flac case03c "$TOTALLY_SILENT --skip=1"
# case 04a: on file with no SEEKTABLE block and --no-seektable specified, no SEEKTABLE
flac2flac input-VA.flac case04a "--no-padding --no-seektable"
# case 04b: on file with no SEEKTABLE block and -S specified, new SEEKTABLE
flac2flac input-VA.flac case04b "--no-padding -S 5x"
# case 04c: on file with no SEEKTABLE block and no seektable options specified, new SEEKTABLE with default points
flac2flac input-VA.flac case04c "--no-padding"
# case 04d: on file with SEEKTABLE block and --no-seektable specified, drop existing SEEKTABLE
flac2flac input-SCVA.flac case04d "--no-padding --no-seektable"
# case 04e: on file with SEEKTABLE block and -S specified, overwrite existing SEEKTABLE
flac2flac input-SCVA.flac case04e "$TOTALLY_SILENT --no-padding -S 5x"
# case 04f: on file with SEEKTABLE block and size-changing option specified, drop existing SEEKTABLE, new SEEKTABLE with default points
#(already covered by case03c)

rm -f out.flac out.meta

#@@@ when metaflac handles ogg flac, duplicate flac2flac tests here

cd ..
