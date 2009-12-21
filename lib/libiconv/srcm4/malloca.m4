# malloca.m4 serial 1
dnl Copyright (C) 2003-2004, 2006-2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_MALLOCA],
[
  dnl Use the autoconf tests for alloca(), but not the AC_SUBSTed variables
  dnl @ALLOCA@ and @LTALLOCA@.
  dnl gl_FUNC_ALLOCA   dnl Already brought in by the module dependencies.
  AC_REQUIRE([gl_EEMALLOC])
  AC_REQUIRE([AC_TYPE_LONG_LONG_INT])
])
