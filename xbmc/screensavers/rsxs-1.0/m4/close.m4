# close.m4 serial 2
dnl Copyright (C) 2008 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_CLOSE],
[
  m4_ifdef([gl_PREREQ_SYS_H_WINSOCK2], [
    gl_PREREQ_SYS_H_WINSOCK2
    if test $UNISTD_H_HAVE_WINSOCK2_H = 1; then
      gl_REPLACE_CLOSE
    fi
  ])
])

AC_DEFUN([gl_REPLACE_CLOSE],
[
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])
  if test $REPLACE_CLOSE != 1; then
    AC_LIBOBJ([close])
  fi
  REPLACE_CLOSE=1
  gl_REPLACE_FCLOSE
])
