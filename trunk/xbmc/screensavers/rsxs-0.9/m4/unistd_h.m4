# unistd_h.m4 serial 2
dnl Copyright (C) 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Written by Simon Josefsson

AC_DEFUN([gl_HEADER_UNISTD],
[
  dnl Prerequisites of lib/unistd.h.
  AC_CHECK_HEADERS([unistd.h], [
    UNISTD_H=''
  ], [
    UNISTD_H='unistd.h'
  ])
  AC_SUBST(UNISTD_H)
])
