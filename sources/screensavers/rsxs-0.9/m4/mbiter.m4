# mbiter.m4 serial 2
dnl Copyright (C) 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl autoconf tests required for use of mbiter.h
dnl From Bruno Haible.

AC_DEFUN([gl_MBITER],
[
  AC_REQUIRE([AC_TYPE_MBSTATE_T])
  dnl The following line is that so the user can test HAVE_MBRTOWC before
  dnl #include "mbiter.h" or "mbuiter.h".
  AC_REQUIRE([gl_FUNC_MBRTOWC])
  :
])
