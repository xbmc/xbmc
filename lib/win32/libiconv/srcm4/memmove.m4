# memmove.m4 serial 3
dnl Copyright (C) 2002, 2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MEMMOVE],
[
  AC_REPLACE_FUNCS([memmove])
  if test $ac_cv_func_memmove = no; then
    gl_PREREQ_MEMMOVE
  fi
])

# Prerequisites of lib/memmove.c.
AC_DEFUN([gl_PREREQ_MEMMOVE], [
  :
])
