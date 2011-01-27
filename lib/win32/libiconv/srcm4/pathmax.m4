# pathmax.m4 serial 8
dnl Copyright (C) 2002, 2003, 2005, 2006, 2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_PATHMAX],
[
  dnl Prerequisites of lib/pathmax.h.
  AC_CHECK_FUNCS_ONCE([pathconf])
  AC_CHECK_HEADERS_ONCE([sys/param.h])
])
