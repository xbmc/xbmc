#! /bin/sh
# @(#) $Id: test-aliases.sh,v 1.5 2003/11/17 12:27:39 yeti Exp $
# Purpose: check for iconv charsets, namely without libiconv.  Enca used to
# keep the @...@ in alias list.
. $srcdir/setup.sh
$ENCA --name aliases --list charsets | grep '@' && DIE=1
. $srcdir/finish.sh
