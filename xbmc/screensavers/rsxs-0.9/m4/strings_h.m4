# Configure a replacement for <strings.h>.
# serial 6

# Copyright (C) 2007, 2009-2011 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_HEADER_STRINGS_H],
[
  dnl Use AC_REQUIRE here, so that the default behavior below is expanded
  dnl once only, before all statements that occur in other macros.
  AC_REQUIRE([gl_HEADER_STRINGS_H_BODY])
])

AC_DEFUN([gl_HEADER_STRINGS_H_BODY],
[
  AC_REQUIRE([gl_HEADER_STRINGS_H_DEFAULTS])

  gl_CHECK_NEXT_HEADERS([strings.h])
  if test $ac_cv_header_strings_h = yes; then
    HAVE_STRINGS_H=1
  else
    HAVE_STRINGS_H=0
  fi
  AC_SUBST([HAVE_STRINGS_H])

  dnl Check for declarations of anything we want to poison if the
  dnl corresponding gnulib module is not in use.
  gl_WARN_ON_USE_PREPARE([[
    /* Minix 3.1.8 has a bug: <sys/types.h> must be included before
       <strings.h>.  */
    #include <sys/types.h>
    #include <strings.h>
    ]], [ffs strcasecmp strncasecmp])
])

AC_DEFUN([gl_STRINGS_MODULE_INDICATOR],
[
  dnl Use AC_REQUIRE here, so that the default settings are expanded once only.
  AC_REQUIRE([gl_HEADER_STRINGS_H_DEFAULTS])
  gl_MODULE_INDICATOR_SET_VARIABLE([$1])
])

AC_DEFUN([gl_HEADER_STRINGS_H_DEFAULTS],
[
  GNULIB_FFS=0;            AC_SUBST([GNULIB_FFS])
  dnl Assume proper GNU behavior unless another module says otherwise.
  HAVE_FFS=1;              AC_SUBST([HAVE_FFS])
  HAVE_STRCASECMP=1;       AC_SUBST([HAVE_STRCASECMP])
  HAVE_DECL_STRNCASECMP=1; AC_SUBST([HAVE_DECL_STRNCASECMP])
])
