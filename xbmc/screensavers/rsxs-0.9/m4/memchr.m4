# memchr.m4 serial 4
dnl Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MEMCHR],
[
  AC_REPLACE_FUNCS(memchr)
  if test $ac_cv_func_memchr = no; then
    gl_PREREQ_MEMCHR
  fi
])

# Prerequisites of lib/memchr.c.
AC_DEFUN([gl_PREREQ_MEMCHR], [
  AC_CHECK_HEADERS(bp-sym.h)
])
