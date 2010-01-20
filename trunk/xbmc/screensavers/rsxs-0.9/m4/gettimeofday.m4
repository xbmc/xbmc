#serial 7

# Copyright (C) 2001, 2002, 2003, 2005 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl From Jim Meyering.
dnl
dnl See if gettimeofday clobbers the static buffer that localtime uses
dnl for its return value.  The gettimeofday function from Mac OS X 10.0.4
dnl (i.e., Darwin 1.3.7) has this problem.
dnl
dnl If it does, then arrange to use gettimeofday and localtime only via
dnl the wrapper functions that work around the problem.

AC_DEFUN([AC_FUNC_GETTIMEOFDAY_CLOBBER],
[
 AC_REQUIRE([AC_HEADER_TIME])
 AC_CACHE_CHECK([whether gettimeofday clobbers localtime buffer],
  jm_cv_func_gettimeofday_clobber,
  [AC_TRY_RUN([
#include <stdio.h>
#include <string.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <stdlib.h>

int
main ()
{
  time_t t = 0;
  struct tm *lt;
  struct tm saved_lt;
  struct timeval tv;
  lt = localtime (&t);
  saved_lt = *lt;
  gettimeofday (&tv, NULL);
  if (memcmp (lt, &saved_lt, sizeof (struct tm)) != 0)
    exit (1);

  exit (0);
}
	  ],
	 jm_cv_func_gettimeofday_clobber=no,
	 jm_cv_func_gettimeofday_clobber=yes,
	 dnl When crosscompiling, assume it is broken.
	 jm_cv_func_gettimeofday_clobber=yes)
  ])
  if test $jm_cv_func_gettimeofday_clobber = yes; then
    gl_GETTIMEOFDAY_REPLACE_LOCALTIME

    AC_DEFINE(gettimeofday, rpl_gettimeofday,
      [Define to rpl_gettimeofday if the replacement function should be used.])
    gl_PREREQ_GETTIMEOFDAY
  fi
])

AC_DEFUN([gl_GETTIMEOFDAY_REPLACE_LOCALTIME], [
  AC_LIBOBJ(gettimeofday)
  AC_DEFINE(gmtime, rpl_gmtime,
    [Define to rpl_gmtime if the replacement function should be used.])
  AC_DEFINE(localtime, rpl_localtime,
    [Define to rpl_localtime if the replacement function should be used.])
])

# Prerequisites of lib/gettimeofday.c.
AC_DEFUN([gl_PREREQ_GETTIMEOFDAY], [
  AC_REQUIRE([AC_HEADER_TIME])
])
