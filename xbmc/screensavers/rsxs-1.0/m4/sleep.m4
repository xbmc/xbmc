# sleep.m4 serial 2
dnl Copyright (C) 2007-2008 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_SLEEP],
[
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])
  dnl We expect to see the declaration of sleep() in a header file.
  dnl Older versions of mingw have a sleep() function that is an alias to
  dnl _sleep() in MSVCRT. It has a different signature than POSIX sleep():
  dnl it takes the number of milliseconds as argument and returns void.
  dnl mingw does not declare this function.
  AC_CHECK_DECLS([sleep], , , [#include <unistd.h>])
  if test $ac_cv_have_decl_sleep != yes; then
    HAVE_SLEEP=0
    AC_LIBOBJ([sleep])
    gl_PREREQ_SLEEP
  fi
])

# Prerequisites of lib/sleep.c.
AC_DEFUN([gl_PREREQ_SLEEP], [:])
