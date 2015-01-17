#serial 7   -*- autoconf -*-
dnl Copyright (C) 2002-2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_DIRNAME],
[
  AC_LIBOBJ([basename])
  AC_LIBOBJ([dirname])
  AC_LIBOBJ([stripslash])

  dnl Prerequisites of lib/dirname.h.
  AC_REQUIRE([gl_AC_DOS])
  AC_REQUIRE([gl_DOUBLE_SLASH_ROOT])

  dnl No prerequisites of lib/basename.c, lib/dirname.c, lib/stripslash.c.
])
