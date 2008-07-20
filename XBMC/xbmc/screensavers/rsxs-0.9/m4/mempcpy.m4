# mempcpy.m4 serial 3
dnl Copyright (C) 2003, 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MEMPCPY],
[
  AC_LIBSOURCES([mempcpy.c, mempcpy.h])

  dnl Persuade glibc <string.h> to declare mempcpy().
  AC_REQUIRE([AC_GNU_SOURCE])

  AC_REPLACE_FUNCS(mempcpy)
  if test $ac_cv_func_mempcpy = no; then
    gl_PREREQ_MEMPCPY
  fi
])

# Prerequisites of lib/mempcpy.c.
AC_DEFUN([gl_PREREQ_MEMPCPY], [
  :
])
