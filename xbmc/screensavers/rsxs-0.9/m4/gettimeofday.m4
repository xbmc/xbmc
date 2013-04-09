# serial 17

# Copyright (C) 2001-2003, 2005, 2007, 2009-2011 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl From Jim Meyering.

AC_DEFUN([gl_FUNC_GETTIMEOFDAY],
[
  AC_REQUIRE([AC_C_RESTRICT])
  AC_REQUIRE([gl_HEADER_SYS_TIME_H])
  AC_REQUIRE([gl_HEADER_SYS_TIME_H_DEFAULTS])
  AC_CHECK_FUNCS_ONCE([gettimeofday])

  gl_gettimeofday_timezone=void
  if test $ac_cv_func_gettimeofday != yes; then
    HAVE_GETTIMEOFDAY=0
  else
    gl_FUNC_GETTIMEOFDAY_CLOBBER
    AC_CACHE_CHECK([for gettimeofday with POSIX signature],
      [gl_cv_func_gettimeofday_posix_signature],
      [AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM(
            [[#include <sys/time.h>
              struct timeval c;
              int gettimeofday (struct timeval *restrict, void *restrict);
            ]],
            [[/* glibc uses struct timezone * rather than the POSIX void *
                 if _GNU_SOURCE is defined.  However, since the only portable
                 use of gettimeofday uses NULL as the second parameter, and
                 since the glibc definition is actually more typesafe, it is
                 not worth wrapping this to get a compliant signature.  */
              int (*f) (struct timeval *restrict, void *restrict)
                = gettimeofday;
              int x = f (&c, 0);
              return !(x | c.tv_sec | c.tv_usec);
            ]])],
          [gl_cv_func_gettimeofday_posix_signature=yes],
          [AC_COMPILE_IFELSE(
            [AC_LANG_PROGRAM(
              [[#include <sys/time.h>
int gettimeofday (struct timeval *restrict, struct timezone *restrict);
              ]])],
            [gl_cv_func_gettimeofday_posix_signature=almost],
            [gl_cv_func_gettimeofday_posix_signature=no])])])
    if test $gl_cv_func_gettimeofday_posix_signature = almost; then
      gl_gettimeofday_timezone='struct timezone'
    elif test $gl_cv_func_gettimeofday_posix_signature != yes; then
      REPLACE_GETTIMEOFDAY=1
    fi
    m4_ifdef([gl_FUNC_TZSET_CLOBBER], [
      gl_FUNC_TZSET_CLOBBER
      if test $gl_cv_func_tzset_clobber = yes; then
        REPLACE_GETTIMEOFDAY=1
        gl_GETTIMEOFDAY_REPLACE_LOCALTIME
        AC_DEFINE([tzset], [rpl_tzset],
          [Define to rpl_tzset if the wrapper function should be used.])
        AC_DEFINE([TZSET_CLOBBERS_LOCALTIME], [1],
          [Define if tzset clobbers localtime's static buffer.])
      fi
    ])
  fi
  AC_DEFINE_UNQUOTED([GETTIMEOFDAY_TIMEZONE], [$gl_gettimeofday_timezone],
    [Define this to 'void' or 'struct timezone' to match the system's
     declaration of the second argument to gettimeofday.])
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
   gl_GETTIMEOFDAY_REPLACE_LOCALTIME
   AC_DEFINE([GETTIMEOFDAY_CLOBBERS_LOCALTIME], [1],
     [Define if gettimeofday clobbers the localtime buffer.])
 fi
])

AC_DEFUN([gl_GETTIMEOFDAY_REPLACE_LOCALTIME], [
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
