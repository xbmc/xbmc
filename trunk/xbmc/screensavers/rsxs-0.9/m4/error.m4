#serial 11

# Copyright (C) 1996, 1997, 1998, 2001, 2002, 2003, 2004 Free Software
# Foundation, Inc.
#
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_ERROR],
[
  AC_FUNC_ERROR_AT_LINE
  dnl Note: AC_FUNC_ERROR_AT_LINE does AC_LIBSOURCES([error.h, error.c]).
  gl_PREREQ_ERROR
])

# Prerequisites of lib/error.c.
AC_DEFUN([gl_PREREQ_ERROR],
[
  AC_REQUIRE([AC_FUNC_STRERROR_R])
  :
])
