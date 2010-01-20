# Portability macros for glibc argz.                    -*- Autoconf -*-
# Written by Gary V. Vaughan <gary@gnu.org>

# Copyright (C) 2004, 2005, 2006 Free Software Foundation, Inc.

# This file is free software; the Free Software Foundation gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.

# serial 4 argz.m4

AC_DEFUN([gl_FUNC_ARGZ],
[gl_PREREQ_ARGZ

AC_CHECK_HEADERS([argz.h], [], [], [AC_INCLUDES_DEFAULT])

AC_CHECK_TYPES([error_t],
  [],
  [AC_DEFINE([error_t], [int],
   [Define to a type to use for `error_t' if it is not otherwise available.])
   AC_DEFINE([__error_t_defined], [1], [Define so that glibc/gnulib argp.h
    does not typedef error_t.])],
  [#if defined(HAVE_ARGZ_H)
#  include <argz.h>
#endif])

ARGZ_H=
AC_CHECK_FUNCS([argz_append argz_create_sep argz_insert argz_next \
	argz_stringify], [], [ARGZ_H=argz.h; AC_LIBOBJ([argz])])
AC_SUBST([ARGZ_H])
])

# Prerequisites of lib/argz.c.
AC_DEFUN([gl_PREREQ_ARGZ], [:])
