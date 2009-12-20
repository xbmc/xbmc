# canonicalize-lgpl.m4 serial 5
dnl Copyright (C) 2003, 2006-2007, 2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_CANONICALIZE_LGPL],
[
  dnl Do this replacement check manually because the file name is shorter
  dnl than the function name.
  AC_CHECK_DECLS_ONCE([canonicalize_file_name])
  AC_CHECK_FUNCS_ONCE([canonicalize_file_name])
  if test $ac_cv_func_canonicalize_file_name = no; then
    AC_LIBOBJ([canonicalize-lgpl])
    AC_DEFINE([realpath], [rpl_realpath],
      [Define to a replacement function name for realpath().])
    gl_PREREQ_CANONICALIZE_LGPL
  fi
])

# Like gl_CANONICALIZE_LGPL, except prepare for separate compilation
# (no AC_LIBOBJ).
AC_DEFUN([gl_CANONICALIZE_LGPL_SEPARATE],
[
  AC_CHECK_DECLS_ONCE([canonicalize_file_name])
  AC_CHECK_FUNCS_ONCE([canonicalize_file_name])
  gl_PREREQ_CANONICALIZE_LGPL
])

# Prerequisites of lib/canonicalize-lgpl.c.
AC_DEFUN([gl_PREREQ_CANONICALIZE_LGPL],
[
  AC_CHECK_HEADERS_ONCE([sys/param.h unistd.h])
  AC_CHECK_FUNCS_ONCE([getcwd readlink])
])
