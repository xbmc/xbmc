# mempcpy.m4 serial 9
dnl Copyright (C) 2003, 2004, 2006, 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MEMPCPY],
[
  dnl Persuade glibc <string.h> to declare mempcpy().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  dnl The mempcpy() declaration in lib/string.in.h uses 'restrict'.
  AC_REQUIRE([AC_C_RESTRICT])

  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])
  AC_REPLACE_FUNCS(mempcpy)
  if test $ac_cv_func_mempcpy = no; then
    HAVE_MEMPCPY=0
    gl_PREREQ_MEMPCPY
  fi
])

# Prerequisites of lib/mempcpy.c.
AC_DEFUN([gl_PREREQ_MEMPCPY], [
  :
])
