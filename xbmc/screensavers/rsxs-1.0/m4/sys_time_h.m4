# Configure a replacement for <sys/time.h>.

# Copyright (C) 2007 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# Written by Paul Eggert and Martin Lambers.

AC_DEFUN([gl_HEADER_SYS_TIME_H],
[
  dnl Use AC_REQUIRE here, so that the REPLACE_GETTIMEOFDAY=0 statement
  dnl below is expanded once only, before all REPLACE_GETTIMEOFDAY=1
  dnl statements that occur in other macros.
  AC_REQUIRE([gl_HEADER_SYS_TIME_H_BODY])
])

AC_DEFUN([gl_HEADER_SYS_TIME_H_BODY],
[
  AC_REQUIRE([AC_C_RESTRICT])
  gl_CHECK_NEXT_HEADERS([sys/time.h])

  if test $ac_cv_header_sys_time_h = yes; then
    HAVE_SYS_TIME_H=1
  else
    HAVE_SYS_TIME_H=0
  fi
  AC_SUBST([HAVE_SYS_TIME_H])

  AC_CACHE_CHECK([for struct timeval], [gl_cv_sys_struct_timeval],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
	  [[#if HAVE_SYS_TIME_H
	     #include <sys/time.h>
	    #endif
	    #include <time.h>
	  ]],
	  [[static struct timeval x; x.tv_sec = x.tv_usec;]])],
       [gl_cv_sys_struct_timeval=yes],
       [gl_cv_sys_struct_timeval=no])])
  if test $gl_cv_sys_struct_timeval = yes; then
    HAVE_STRUCT_TIMEVAL=1
  else
    HAVE_STRUCT_TIMEVAL=0
  fi
  AC_SUBST([HAVE_STRUCT_TIMEVAL])

  dnl Assume POSIX behavior unless another module says otherwise.
  REPLACE_GETTIMEOFDAY=0
  AC_SUBST([REPLACE_GETTIMEOFDAY])
  if test $HAVE_SYS_TIME_H = 0 || test $HAVE_STRUCT_TIMEVAL = 0; then
    SYS_TIME_H=sys/time.h
  else
    SYS_TIME_H=
  fi
  AC_SUBST([SYS_TIME_H])
])
