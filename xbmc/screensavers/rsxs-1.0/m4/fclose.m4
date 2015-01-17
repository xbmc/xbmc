# fclose.m4 serial 1
dnl Copyright (C) 2008 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FCLOSE],
[
])

AC_DEFUN([gl_REPLACE_FCLOSE],
[
  AC_REQUIRE([gl_STDIO_H_DEFAULTS])
  if test $REPLACE_FCLOSE != 1; then
    AC_LIBOBJ([fclose])
  fi
  REPLACE_FCLOSE=1
])
