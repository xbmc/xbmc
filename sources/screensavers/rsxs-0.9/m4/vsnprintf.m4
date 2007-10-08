# vsnprintf.m4 serial 2
dnl Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_VSNPRINTF],
[
  AC_REPLACE_FUNCS(vsnprintf)
  AC_CHECK_DECLS_ONCE(vsnprintf)
  gl_PREREQ_VSNPRINTF
])

# Prerequisites of lib/vsnprintf.c.
AC_DEFUN([gl_PREREQ_VSNPRINTF], [:])
