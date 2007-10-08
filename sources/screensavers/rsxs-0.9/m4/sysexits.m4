# sysexits.m4 serial 2
dnl Copyright (C) 2003,2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_SYSEXITS],
[
  AC_CHECK_HEADERS(sysexits.h,
    [SYSEXITS_H=],
    [SYSEXITS_H=sysexits.h])
  AC_SUBST([SYSEXITS_H])
])
