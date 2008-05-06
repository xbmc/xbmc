#!/bin/sh

if [ -z $1 ]; then
    echo "Usage: $0 <mplayer source dir>"
    exit 1
fi

MPLAYERSRCDIR=$1
cp $MPLAYERSRCDIR/libass/ass*.[ch] libass/
rm libass/ass_mp.* libass/help_mp.h

cat >libass/help_mp.h <<EOF
#ifndef __LIBASS_HELP_MP_H__
#define __LIBASS_HELP_MP_H__
EOF
cat $MPLAYERSRCDIR/help/help_mp-en.h | grep "#define MSGTR_LIBASS_" >>libass/help_mp.h
cat >>libass/help_mp.h <<EOF
#endif

EOF
