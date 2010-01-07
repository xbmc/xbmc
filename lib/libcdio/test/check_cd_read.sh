#!/bin/sh
#$Id: check_cd_read.sh,v 1.7 2005/01/29 20:54:20 rocky Exp $
#
#    Copyright (C) 2003, 2005 Rocky Bernstein <rocky@panix.com>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Tests to see that CD reading is correct (via cd-read). 

if test -z $srcdir ; then
  srcdir=`pwd`
fi

. ${srcdir}/check_common_fn

if test ! -x ../src/cd-read ; then
  exit 77
fi

BASE=`basename $0 .sh`

fname=cdda
testnum=CD-DA
opts="-c ${srcdir}/${fname}.cue --mode=red --start=0"
test_cd_read  "$opts" ${fname}-read.dump ${srcdir}/${fname}-read.right
RC=$?
check_result $RC "cd-read CUE test $testnum" "cd-read $opts"

fname=isofs-m1
testnum=MODE1
opts="-i ${srcdir}/${fname}.cue --mode m1f1 -s 26 -n 2"
test_cd_read "$opts" ${fname}-read.dump ${srcdir}/${fname}-read.right
RC=$?
check_result $RC "cd-read CUE test $testnum" "$CD_READ $opts"

exit $RC

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
