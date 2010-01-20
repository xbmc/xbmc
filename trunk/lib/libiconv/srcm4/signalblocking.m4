# signalblocking.m4 serial 10
dnl Copyright (C) 2001-2002, 2006-2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Determine available signal blocking primitives. Three different APIs exist:
# 1) POSIX: sigemptyset, sigaddset, sigprocmask
# 2) SYSV: sighold, sigrelse
# 3) BSD: sigblock, sigsetmask
# For simplicity, here we check only for the POSIX signal blocking.
AC_DEFUN([gl_SIGNALBLOCKING],
[
  AC_REQUIRE([gl_SIGNAL_H_DEFAULTS])
  signals_not_posix=
  AC_EGREP_HEADER([sigset_t], [signal.h], , [signals_not_posix=1])
  if test -z "$signals_not_posix"; then
    AC_CHECK_FUNC([sigprocmask], [gl_cv_func_sigprocmask=1])
  fi
  if test -z "$gl_cv_func_sigprocmask"; then
    HAVE_POSIX_SIGNALBLOCKING=0
    AC_LIBOBJ([sigprocmask])
    gl_PREREQ_SIGPROCMASK
  fi
])

# Prerequisites of the part of lib/signal.in.h and of lib/sigprocmask.c.
AC_DEFUN([gl_PREREQ_SIGPROCMASK],
[
  AC_REQUIRE([gl_SIGNAL_H_DEFAULTS])
  AC_CHECK_TYPES([sigset_t],
    [gl_cv_type_sigset_t=yes], [gl_cv_type_sigset_t=no],
    [#include <signal.h>
/* Mingw defines sigset_t not in <signal.h>, but in <sys/types.h>.  */
#include <sys/types.h>])
  if test $gl_cv_type_sigset_t != yes; then
    HAVE_SIGSET_T=0
  fi
  dnl HAVE_SIGSET_T is 1 if the system lacks the sigprocmask function but has
  dnl the sigset_t type.
  AC_SUBST([HAVE_SIGSET_T])
])
