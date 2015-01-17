#serial 11

# Copyright (C) 2001, 2002, 2003, 2005, 2007 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl From Jim Meyering.

AC_DEFUN([gl_FUNC_GETTIMEOFDAY],
[
  AC_REQUIRE([AC_C_RESTRICT])
  AC_REQUIRE([gl_HEADER_SYS_TIME_H])
  AC_CHECK_FUNCS_ONCE([gettimeofday])

  AC_CACHE_CHECK([for gettimeofday with POSIX signature],
    [gl_cv_func_gettimeofday_posix_signature],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
	  [[#include <sys/time.h>
	    struct timeval c;
	  ]],
	  [[
	    int (*f) (struct timeval *restrict, void *restrict) = gettimeofday;
	    int x = f (&c, 0);
	    return !(x | c.tv_sec | c.tv_usec);
	  ]])],
	[gl_cv_func_gettimeofday_posix_signature=yes],
	[gl_cv_func_gettimeofday_posix_signature=no])])

  gl_FUNC_GETTIMEOFDAY_CLOBBER

  if test $gl_cv_func_gettimeofday_posix_signature != yes; then
    REPLACE_GETTIMEOFDAY=1
    SYS_TIME_H=sys/time.h
    if test $gl_cv_func_gettimeofday_clobber != yes; then
      AC_LIBOBJ(gettimeofday)
      gl_PREREQ_GETTIMEOFDAY
    fi
  fi
])


dnl See if gettimeofday clobbers the static buffer that localtime uses
dnl for its return value.  The gettimeofday function from Mac OS X 10.0.4
dnl (i.e., Darwin 1.3.7) has this problem.
dnl
dnl If it does, then arrange to use gettimeofday and localtime only via
dnl the wrapper functions that work around the problem.

AC_DEFUN([gl_FUNC_GETTIMEOFDAY_CLOBBER],
[
 AC_REQUIRE([gl_HEADER_SYS_TIME_H])

 AC_CACHE_CHECK([whether gettimeofday clobbers localtime buffer],
  [gl_cv_func_gettimeofday_clobber],
  [AC_RUN_IFELSE(
     [AC_LANG_PROGRAM(
	[[#include <string.h>
	  #include <sys/time.h>
	  #include <time.h>
	  #include <stdlib.h>
	]],
	[[
	  time_t t = 0;
	  struct tm *lt;
	  struct tm saved_lt;
	  struct timeval tv;
	  lt = localtime (&t);
	  saved_lt = *lt;
	  gettimeofday (&tv, NULL);
	  return memcmp (lt, &saved_lt, sizeof (struct tm)) != 0;
	]])],
     [gl_cv_func_gettimeofday_clobber=no],
     [gl_cv_func_gettimeofday_clobber=yes],
     dnl When crosscompiling, assume it is broken.
     [gl_cv_func_gettimeofday_clobber=yes])])

 if test $gl_cv_func_gettimeofday_clobber = yes; then
   REPLACE_GETTIMEOFDAY=1
   SYS_TIME_H=sys/time.h
   gl_GETTIMEOFDAY_REPLACE_LOCALTIME
   AC_DEFINE([GETTIMEOFDAY_CLOBBERS_LOCALTIME], 1,
     [Define if gettimeofday clobbers the localtime buffer.])
 fi
])

AC_DEFUN([gl_GETTIMEOFDAY_REPLACE_LOCALTIME], [
  AC_LIBOBJ(gettimeofday)
  gl_PREREQ_GETTIMEOFDAY
  AC_DEFINE([gmtime], [rpl_gmtime],
    [Define to rpl_gmtime if the replacement function should be used.])
  AC_DEFINE([localtime], [rpl_localtime],
    [Define to rpl_localtime if the replacement function should be used.])
])

# Prerequisites of lib/gettimeofday.c.
AC_DEFUN([gl_PREREQ_GETTIMEOFDAY], [
  AC_CHECK_HEADERS([sys/timeb.h])
  AC_CHECK_FUNCS([_ftime])
])
