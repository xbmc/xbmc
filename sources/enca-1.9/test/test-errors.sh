#! /bin/sh
# @(#) $Id: test-errors.sh,v 1.6 2003/11/17 12:27:39 yeti Exp $
# Purpose: check how enca reacts on various incorrect inputs
. $srcdir/setup.sh
TEST_TEXT=$srcdir/cs-s.iso88592
LC_CTYPE=
LC_COLLATE=
LC_MESSAGES=
LC_ALL=
LANG=
# This is necessary when $ENCA is in fact a libtool script
export LC_CTYPE LC_ALL LC_COLLATE LC_MESSAGES LANG
# These should succeede
# If they set some options, they should keep defaults.
$ENCA -L cs --name puskin >/dev/null 2>/dev/null <$TEST_TEXT || DIE=1
$ENCA -L cs -C gogol >/dev/null 2>/dev/null <$TEST_TEXT || DIE=1
# These should fail.
$ENCA -L bulgakov >/dev/null 2>/dev/null <$TEST_TEXT && DIE=1
$ENCA -L none dostojevskij 2>/dev/null && DIE=1
touch zombie
chmod 000 zombie
$ENCA -L none zombie 2>/dev/null && DIE=1
chmod 700 zombie
rm -f zombie 2>/dev/null
. $srcdir/finish.sh
