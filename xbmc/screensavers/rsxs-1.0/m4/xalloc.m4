# xalloc.m4 serial 16
dnl Copyright (C) 2002, 2003, 2004, 2005, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_XALLOC],
[
  AC_LIBOBJ([xmalloc])

  gl_PREREQ_XALLOC
  gl_PREREQ_XMALLOC
])

# Prerequisites of lib/xalloc.h.
AC_DEFUN([gl_PREREQ_XALLOC], [
  AC_REQUIRE([gl_INLINE])
  :
])

# Prerequisites of lib/xmalloc.c.
AC_DEFUN([gl_PREREQ_XMALLOC], [
  :
])
