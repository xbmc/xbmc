# sleep.m4 serial 6
dnl Copyright (C) 2007-2011 Free Software Foundation, Inc.
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
  AC_CHECK_DECLS([sleep], , , [[#include <unistd.h>]])
  AC_CHECK_FUNCS_ONCE([sleep])
  if test $ac_cv_have_decl_sleep != yes; then
    HAVE_SLEEP=0
  else
    dnl Cygwin 1.5.x has a bug where sleep can't exceed 49.7 days.
    AC_CACHE_CHECK([for working sleep], [gl_cv_func_sleep_works],
      [AC_RUN_IFELSE([AC_LANG_PROGRAM([[
#include <errno.h>
#include <unistd.h>
#include <signal.h>
static void
handle_alarm (int sig)
{
  if (sig != SIGALRM)
    _exit (2);
}
]], [[
    /* Failure to compile this test due to missing alarm is okay,
       since all such platforms (mingw) also lack sleep.  */
    unsigned int pentecost = 50 * 24 * 60 * 60; /* 50 days.  */
    unsigned int remaining;
    signal (SIGALRM, handle_alarm);
    alarm (1);
    remaining = sleep (pentecost);
    if (remaining > pentecost)
      return 3;
    if (remaining <= pentecost - 10)
      return 4;
    return 0;
    ]])],
      [gl_cv_func_sleep_works=yes], [gl_cv_func_sleep_works=no],
      [gl_cv_func_sleep_works="guessing no"])])
    if test "$gl_cv_func_sleep_works" != yes; then
      REPLACE_SLEEP=1
    fi
  fi
])
