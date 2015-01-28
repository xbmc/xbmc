# strchrnul.m4 serial 6
dnl Copyright (C) 2003, 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRCHRNUL],
[
  dnl Persuade glibc <string.h> to declare strchrnul().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])
  AC_REPLACE_FUNCS(strchrnul)
  if test $ac_cv_func_strchrnul = no; then
    HAVE_STRCHRNUL=0
    gl_PREREQ_STRCHRNUL
  fi
])

# Prerequisites of lib/strchrnul.c.
AC_DEFUN([gl_PREREQ_STRCHRNUL], [:])
