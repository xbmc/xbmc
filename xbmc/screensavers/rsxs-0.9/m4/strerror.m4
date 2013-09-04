# strerror.m4 serial 16
dnl Copyright (C) 2002, 2007-2011 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRERROR],
[
  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])
  AC_REQUIRE([gl_HEADER_ERRNO_H])
  AC_REQUIRE([gl_FUNC_STRERROR_0])
  m4_ifdef([gl_FUNC_STRERROR_R_WORKS], [
    AC_REQUIRE([gl_FUNC_STRERROR_R_WORKS])
  ])
  if test "$ERRNO_H:$REPLACE_STRERROR_0" = :0; then
    AC_CACHE_CHECK([for working strerror function],
     [gl_cv_func_working_strerror],
     [AC_RUN_IFELSE(
        [AC_LANG_PROGRAM(
           [[#include <string.h>
           ]],
           [[if (!*strerror (-2)) return 1;]])],
        [gl_cv_func_working_strerror=yes],
        [gl_cv_func_working_strerror=no],
        [dnl Be pessimistic on cross-compiles for now.
         gl_cv_func_working_strerror="guessing no"])
    ])
    if test "$gl_cv_func_working_strerror" != yes; then
      dnl The system's strerror() fails to return a string for out-of-range
      dnl integers. Replace it.
      REPLACE_STRERROR=1
    fi
    m4_ifdef([gl_FUNC_STRERROR_R_WORKS], [
      dnl If the system's strerror_r or __xpg_strerror_r clobbers strerror's
      dnl buffer, we must replace strerror.
      case "$gl_cv_func_strerror_r_works" in
        *no) REPLACE_STRERROR=1 ;;
      esac
    ])
  else
    dnl The system's strerror() cannot know about the new errno values we add
    dnl to <errno.h>, or any fix for strerror(0). Replace it.
    REPLACE_STRERROR=1
  fi
])

dnl Detect if strerror(0) passes (that is, does not set errno, and does not
dnl return a string that matches strerror(-1)).
AC_DEFUN([gl_FUNC_STRERROR_0],
[
  REPLACE_STRERROR_0=0
  AC_CACHE_CHECK([whether strerror(0) succeeds],
   [gl_cv_func_strerror_0_works],
   [AC_RUN_IFELSE(
      [AC_LANG_PROGRAM(
         [[#include <string.h>
           #include <errno.h>
         ]],
         [[int result = 0;
           char *str;
           errno = 0;
           str = strerror (0);
           if (!*str) result |= 1;
           if (errno) result |= 2;
           if (strstr (str, "nknown") || strstr (str, "ndefined"))
             result |= 4;
           return result;]])],
      [gl_cv_func_strerror_0_works=yes],
      [gl_cv_func_strerror_0_works=no],
      [dnl Be pessimistic on cross-compiles for now.
       gl_cv_func_strerror_0_works="guessing no"])
  ])
  if test "$gl_cv_func_strerror_0_works" != yes; then
    REPLACE_STRERROR_0=1
    AC_DEFINE([REPLACE_STRERROR_0], [1], [Define to 1 if strerror(0)
      does not return a message implying success.])
  fi
])
