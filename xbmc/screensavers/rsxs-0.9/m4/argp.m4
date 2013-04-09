# argp.m4 serial 13
dnl Copyright (C) 2003-2011 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_ARGP],
[
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([AC_C_RESTRICT])
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])

  AC_CHECK_DECL([program_invocation_name],
                [AC_DEFINE([HAVE_DECL_PROGRAM_INVOCATION_NAME], [1],
                           [Define if program_invocation_name is declared])],
                [AC_DEFINE([GNULIB_PROGRAM_INVOCATION_NAME], [1],
                           [Define to 1 to add extern declaration of program_invocation_name to argp.h])],
                [[#include <errno.h>]])
  AC_CHECK_DECL([program_invocation_short_name],
                [AC_DEFINE([HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME], [1],
                           [Define if program_invocation_short_name is declared])],
                [AC_DEFINE([GNULIB_PROGRAM_INVOCATION_SHORT_NAME], [1],
                           [Define to 1 to add extern declaration of program_invocation_short_name to argp.h])],
                [[#include <errno.h>]])

  # Check if program_invocation_name and program_invocation_short_name
  # are defined elsewhere. It is improbable that only one of them will
  # be defined and other not, I prefer to stay on the safe side and to
  # test each one separately.
  AC_MSG_CHECKING([whether program_invocation_name is defined])
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <argp.h>]],
                                  [[program_invocation_name = "test";]])],
                 [AC_DEFINE([HAVE_PROGRAM_INVOCATION_NAME], [1],
                            [Define if program_invocation_name is defined])
                  AC_MSG_RESULT([yes])],
                 [AC_MSG_RESULT([no])])

  AC_MSG_CHECKING([whether program_invocation_short_name is defined])
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <argp.h>]],
                                  [[program_invocation_short_name = "test";]])],
                 [AC_DEFINE([HAVE_PROGRAM_INVOCATION_SHORT_NAME], [1],
                            [Define if program_invocation_short_name is defined])
                  AC_MSG_RESULT([yes])],
                 [AC_MSG_RESULT([no])])

  AC_CHECK_DECLS_ONCE([clearerr_unlocked])
  AC_CHECK_DECLS_ONCE([feof_unlocked])
  AC_CHECK_DECLS_ONCE([ferror_unlocked])
  AC_CHECK_DECLS_ONCE([fflush_unlocked])
  AC_CHECK_DECLS_ONCE([fgets_unlocked])
  AC_CHECK_DECLS_ONCE([fputc_unlocked])
  AC_CHECK_DECLS_ONCE([fputs_unlocked])
  AC_CHECK_DECLS_ONCE([fread_unlocked])
  AC_CHECK_DECLS_ONCE([fwrite_unlocked])
  AC_CHECK_DECLS_ONCE([getc_unlocked])
  AC_CHECK_DECLS_ONCE([getchar_unlocked])
  AC_CHECK_DECLS_ONCE([putc_unlocked])
  AC_CHECK_DECLS_ONCE([putchar_unlocked])
  AC_CHECK_FUNCS_ONCE([flockfile funlockfile])
  AC_CHECK_HEADERS_ONCE([features.h linewrap.h])
])

dnl argp-parse.c depends on GNU getopt internals, therefore use GNU getopt
dnl always.
AC_DEFUN([gl_REPLACE_GETOPT_ALWAYS], [])
