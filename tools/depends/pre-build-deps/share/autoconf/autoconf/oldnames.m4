# This file is part of Autoconf.                           -*- Autoconf -*-
# Support old macros, and provide automated updates.
# Copyright (C) 1994, 1999, 2000, 2001, 2003, 2009, 2010 Free Software
# Foundation, Inc.

# This file is part of Autoconf.  This program is free
# software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# Under Section 7 of GPL version 3, you are granted additional
# permissions described in the Autoconf Configure Script Exception,
# version 3.0, as published by the Free Software Foundation.
#
# You should have received a copy of the GNU General Public License
# and a copy of the Autoconf Configure Script Exception along with
# this program; see the files COPYINGv3 and COPYING.EXCEPTION
# respectively.  If not, see <http://www.gnu.org/licenses/>.

# Originally written by David J. MacKenzie.


## ---------------------------- ##
## General macros of Autoconf.  ##
## ---------------------------- ##

AU_ALIAS([AC_WARN],		[AC_MSG_WARN])
AU_ALIAS([AC_ERROR],		[AC_MSG_ERROR])
AU_ALIAS([AC_HAVE_HEADERS],	[AC_CHECK_HEADERS])
AU_ALIAS([AC_HEADER_CHECK],	[AC_CHECK_HEADER])
AU_ALIAS([AC_HEADER_EGREP],	[AC_EGREP_HEADER])
AU_ALIAS([AC_PREFIX],		[AC_PREFIX_PROGRAM])
AU_ALIAS([AC_PROGRAMS_CHECK],	[AC_CHECK_PROGS])
AU_ALIAS([AC_PROGRAMS_PATH],	[AC_PATH_PROGS])
AU_ALIAS([AC_PROGRAM_CHECK],	[AC_CHECK_PROG])
AU_ALIAS([AC_PROGRAM_EGREP],	[AC_EGREP_CPP])
AU_ALIAS([AC_PROGRAM_PATH],	[AC_PATH_PROG])
AU_ALIAS([AC_SIZEOF_TYPE],	[AC_CHECK_SIZEOF])
AU_ALIAS([AC_TEST_CPP],		[AC_TRY_CPP])
AU_ALIAS([AC_TEST_PROGRAM],	[AC_TRY_RUN])



## ----------------------------- ##
## Specific macros of Autoconf.  ##
## ----------------------------- ##

AU_ALIAS([AC_CHAR_UNSIGNED],	[AC_C_CHAR_UNSIGNED])
AU_ALIAS([AC_CONST],		[AC_C_CONST])
AU_ALIAS([AC_CROSS_CHECK],	[AC_C_CROSS])
AU_ALIAS([AC_FIND_X],		[AC_PATH_X])
AU_ALIAS([AC_FIND_XTRA],	[AC_PATH_XTRA])
AU_ALIAS([AC_GCC_TRADITIONAL],	[AC_PROG_GCC_TRADITIONAL])
AU_ALIAS([AC_GETGROUPS_T],	[AC_TYPE_GETGROUPS])
AU_ALIAS([AC_INLINE],		[AC_C_INLINE])
AU_ALIAS([AC_LN_S],		[AC_PROG_LN_S])
AU_ALIAS([AC_LONG_DOUBLE],	[AC_C_LONG_DOUBLE])
AU_ALIAS([AC_LONG_FILE_NAMES],	[AC_SYS_LONG_FILE_NAMES])
AU_ALIAS([AC_MAJOR_HEADER],	[AC_HEADER_MAJOR])
AU_ALIAS([AC_MINUS_C_MINUS_O],	[AC_PROG_CC_C_O])
AU_ALIAS([AC_MODE_T],		[AC_TYPE_MODE_T])
AU_ALIAS([AC_OFF_T],		[AC_TYPE_OFF_T])
AU_ALIAS([AC_PID_T],		[AC_TYPE_PID_T])
AU_ALIAS([AC_RESTARTABLE_SYSCALLS],		[AC_SYS_RESTARTABLE_SYSCALLS])
AU_ALIAS([AC_RETSIGTYPE],	[AC_TYPE_SIGNAL])
AU_ALIAS([AC_SET_MAKE],		[AC_PROG_MAKE_SET])
AU_ALIAS([AC_SIZE_T],		[AC_TYPE_SIZE_T])
AU_ALIAS([AC_STAT_MACROS_BROKEN],		[AC_HEADER_STAT])
AU_ALIAS([AC_STDC_HEADERS],	[AC_HEADER_STDC])
AU_ALIAS([AC_ST_BLKSIZE],	[AC_STRUCT_ST_BLKSIZE])
AU_ALIAS([AC_ST_BLOCKS],	[AC_STRUCT_ST_BLOCKS])
AU_ALIAS([AC_ST_RDEV],		[AC_STRUCT_ST_RDEV])
AU_ALIAS([AC_SYS_SIGLIST_DECLARED],		[AC_DECL_SYS_SIGLIST])
AU_ALIAS([AC_TIMEZONE],		[AC_STRUCT_TIMEZONE])
AU_ALIAS([AC_TIME_WITH_SYS_TIME],		[AC_HEADER_TIME])
AU_ALIAS([AC_UID_T],		[AC_TYPE_UID_T])
AU_ALIAS([AC_WORDS_BIGENDIAN],	[AC_C_BIGENDIAN])
AU_ALIAS([AC_YYTEXT_POINTER],	[AC_DECL_YYTEXT])
AU_ALIAS([AM_CYGWIN32],		[AC_CYGWIN32])
AU_ALIAS([AC_CYGWIN32],         [AC_CYGWIN])
AU_ALIAS([AM_EXEEXT],		[AC_EXEEXT])
# We cannot do this, because in libtool.m4 yet they provide
# this update.  Some solution is needed.
# AU_ALIAS([AM_PROG_LIBTOOL],		[AC_PROG_LIBTOOL])
AU_ALIAS([AM_MINGW32],		[AC_MINGW32])
AU_ALIAS([AM_PROG_INSTALL],	[AC_PROG_INSTALL])
