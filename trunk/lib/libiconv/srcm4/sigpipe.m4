# sigpipe.m4 serial 2
dnl Copyright (C) 2008, 2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Tests whether SIGPIPE is provided by <signal.h>.
dnl Sets gl_cv_header_signal_h_SIGPIPE.
AC_DEFUN([gl_SIGNAL_SIGPIPE],
[
  dnl Use AC_REQUIRE here, so that the default behavior below is expanded
  dnl once only, before all statements that occur in other macros.
  AC_REQUIRE([gl_SIGNAL_SIGPIPE_BODY])
])

AC_DEFUN([gl_SIGNAL_SIGPIPE_BODY],
[
  AC_REQUIRE([AC_PROG_CC])
  AC_CACHE_CHECK([for SIGPIPE], [gl_cv_header_signal_h_SIGPIPE], [
    AC_EGREP_CPP([booboo],[
#include <signal.h>
#if !defined SIGPIPE
booboo
#endif
      ],
      [gl_cv_header_signal_h_SIGPIPE=no],
      [gl_cv_header_signal_h_SIGPIPE=yes])
  ])
])
