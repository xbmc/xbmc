# sysexits.m4 serial 6
dnl Copyright (C) 2003, 2005, 2007, 2009-2011 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_SYSEXITS],
[
  AC_CHECK_HEADERS_ONCE([sysexits.h])
  if test $ac_cv_header_sysexits_h = yes; then
    HAVE_SYSEXITS_H=1
    gl_CHECK_NEXT_HEADERS([sysexits.h])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sysexits.h>]],
        [[switch (0)
          {
          case EX_OK:
          case EX_USAGE:
          case EX_DATAERR:
          case EX_NOINPUT:
          case EX_NOUSER:
          case EX_NOHOST:
          case EX_UNAVAILABLE:
          case EX_SOFTWARE:
          case EX_OSERR:
          case EX_OSFILE:
          case EX_CANTCREAT:
          case EX_IOERR:
          case EX_TEMPFAIL:
          case EX_PROTOCOL:
          case EX_NOPERM:
          case EX_CONFIG:
            break;
          }
        ]])],
      [SYSEXITS_H=],
      [SYSEXITS_H=sysexits.h])
  else
    HAVE_SYSEXITS_H=0
    SYSEXITS_H=sysexits.h
  fi
  AC_SUBST([HAVE_SYSEXITS_H])
  AC_SUBST([SYSEXITS_H])
  AM_CONDITIONAL([GL_GENERATE_SYSEXITS_H], [test -n "$SYSEXITS_H"])
])
