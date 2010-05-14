#!/bin/sh
progname=mugeco version=0.1
programr='Alexander Leidinger'
progdate='7 Dec 2000'
progdesc='MUltiGEnerationCOding'
# NEEDS: getopt, lame
# Please have a look at the DEFAULTS section.

# $Id: mugeco.sh,v 1.6 2000/12/08 13:47:56 aleidinger Exp $

usage() {
cat << EOF
** $progname v$version, $progdate **
by $programr
$progdesc
usage: $progname [ <flags> ] -g <num> <file>
    -v	       use builtin VBR options
    -g <num>   number of generations
    -h help

 used
  - env vars:
    * LAME   : alternative encoder binary
    * LAMEOPT: alternative encoder options
  - VBR opts: $enc_vbr_opts
  - CBR opts: $enc_cbr_opts
EOF
}

# DEFAULTS

# if you can, use getopt(1) (c)1997 by Frodo Looijaard <frodol@dds.nl>
# it's in most modern unixen, or look at http://huizen.dds.nl/~frodol/
: ${GETOPT=getopt}	# helper program
# mktemp (optional) is also in most modern unixen (originally from OpenBSD)
: ${MKTEMP=mktemp}	# helper program
: ${TMPDIR:=/tmp}	# set default temp directory
: ${LAME:=lame}		# path to LAME

enc_cbr_opts="-b192 -h --lowpass 18 --lowpass-width 0"
enc_vbr_opts="--vbr-mtrh --nspsytune -v -h -d -Y -X3"
enc_opts=${LAMEOPT:-$enc_cbr_opts}
num=			# default number of generations

# DEFINE FUNCTIONS

e() { echo "$progname: $*"; }
die() {	    # usage:  die [ <exitcode> [ <errormessage> ] ]
    trap '' 1 2 3 13 15
    exitcode=0
    [ $# -gt 0 ] && { exitcode=$1; shift; }
    [ $# -gt 0 ] && e "Error: $*" >&2
    exit $exitcode
}

# tfile()
# this function creates temporary files.  'tfile temp' will make a tempfile
# and put the path to it in the variable $temp (defaults to variable $tf)
trap 'for f in $ztfiles; do rm -f "$f"; done' 0
trap 'trap "" 1 2 3 13 15; exit 10' 1 2 3 13 15
unset ztfiles
tfile() {	# usage: tfile <variable_name>
    ztf=`$MKTEMP -q $TMPDIR/$progname.XXXXXX 2>/dev/null`	# try mktemp
    if [ $? -gt 0 -o -z "$ztf" ]; then	# if mktemp fails, do it unsafely
	ztf=$TMPDIR/$LOGNAME.$progname.$$
	[ -e "$ztf" ] && ztf= || { touch $ztf && chmod 600 $ztf; }
    fi
    [ "$ztf" -a -f "$ztf" ] || { echo Could not make tempfile; exit 8; }
    ztfiles="$ztfiles $ztf"
    eval ${1:-tf}='$ztf'
}

# PARSE COMMAND LINE

options="g:vh"	# option string for getopt(1)
help=; [ "$1" = -h -o "$1" = -help -o "$1" = --help ] && help=yes
[ "$help" ] && { usage; die; }
$GETOPT -T >/dev/null 2>&1
[ $? -eq 4 ] && GETOPT="$GETOPT -n $progname -s sh" #frodol's getopt?
eval set -- `$GETOPT "$options" "$@"`
[ $# -lt 1 ] && { die 9 getopt failed; }
while [ $# -gt 0 ]; do
    case "$1" in
    -g) num=$2; shift ;;
    -v) enc_opts=$enc_cbr_opts ;;
    -h)	help=y ;;
    --)	shift; break ;;
    *)	usage; die 9 "invalid command line syntax!" ;;
    esac
    shift
done
[ "$help" ] && { usage; die; }
[ $# -eq 0 ] && { usage; die 9 no arguments; } #change or remove if desired
# sanity checking
[ "$num" ] && echo "$num"|grep -q '^[0-9]*$' && [ $num -ge 1 ] \
    || die 1 please use the -g flag with a valid number

# MAIN PROGRAM

# what version of lame are we using?
lame_vers=`$LAME 2>&1 | awk 'NR==1{print $3}'`

# check filename
[ -f "$1" ] || die 2 "'$1' isn't a file"
echo "$1"|grep -qi '\.wav$' || die 2 "'$1' isn't a .wav"

# make tempfiles
base=`echo "$1"|sed 's/\.[^.]*$//'`
dest=${base}_generation_$num.wav
[ -e "$dest" ] && die 2 "'$dest' already exists"
touch "$dest" || die 2 "couldn't create '$dest'"
TMPDIR=. tfile tmpwav
TMPDIR=. tfile tmpmp3
cp -f "$1" "$tmpwav"

# do the loop
start=`date`
i=1
while [ $i -le $num ]; do
    e "Working on file '$1', generation number $i..."

    $LAME $enc_opts --tc "lame $lame_vers; Generation: $i" \
	"$tmpwav" "$tmpmp3" || die 3 encoding failed
    $LAME --decode --mp3input "$tmpmp3" "$tmpwav" || die 3 decoding failed

    i=`expr $i + 1`
done
end=`date`

# save the result
ln -f "$tmpwav" "$dest"

echo
e "Start: $start"
e "Stop : $end"

die
