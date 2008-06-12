#! /bin/sh
# @(#) $Id: test-empty.sh,v 1.4 2003/11/17 12:27:39 yeti Exp $
# Purpose: check how enca reacts on various incorrect inputs
. $srcdir/setup.sh
# These should succeede
$ENCA -L cs -x koi8cs2 </dev/null >/dev/null 2>&1 || DIE=1
$ENCA -L ru -x anoldoak </dev/null >/dev/null 2>&1 || DIE=1
# These should fail.
$ENCA -L pl </dev/null >/dev/null 2>&1 && DIE=1
. $srcdir/finish.sh
