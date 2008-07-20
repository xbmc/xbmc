# getopt.m4 serial 13
dnl Copyright (C) 2002, 2003, 2004, 2005, 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# The getopt module assume you want GNU getopt, with getopt_long etc,
# rather than vanilla POSIX getopt.  This means your code should
# always include <getopt.h> for the getopt prototypes.

AC_DEFUN([gl_GETOPT_SUBSTITUTE],
[
  AC_LIBOBJ([getopt])
  AC_LIBOBJ([getopt1])
  gl_GETOPT_SUBSTITUTE_HEADER
  gl_PREREQ_GETOPT
])

AC_DEFUN([gl_GETOPT_SUBSTITUTE_HEADER],
[
  GETOPT_H=getopt.h
  AC_DEFINE([__GETOPT_PREFIX], [[rpl_]],
    [Define to rpl_ if the getopt replacement functions and variables
     should be used.])
  AC_SUBST([GETOPT_H])
])

AC_DEFUN([gl_GETOPT_CHECK_HEADERS],
[
  if test -z "$GETOPT_H"; then
    AC_CHECK_HEADERS([getopt.h], [], [GETOPT_H=getopt.h])
  fi

  if test -z "$GETOPT_H"; then
    AC_CHECK_FUNCS([getopt_long_only], [], [GETOPT_H=getopt.h])
  fi

  dnl BSD getopt_long uses an incompatible method to reset option processing,
  dnl and (as of 2004-10-15) mishandles optional option-arguments.
  if test -z "$GETOPT_H"; then
    AC_CHECK_DECL([optreset], [GETOPT_H=getopt.h], [], [#include <getopt.h>])
  fi

  dnl Solaris 10 getopt doesn't handle `+' as a leading character in an
  dnl option string (as of 2005-05-05).
  if test -z "$GETOPT_H"; then
    AC_CACHE_CHECK([for working GNU getopt function], [gl_cv_func_gnu_getopt],
      [AC_RUN_IFELSE(
	[AC_LANG_PROGRAM([#include <getopt.h>],
	   [[
	     char *myargv[3];
	     myargv[0] = "conftest";
	     myargv[1] = "-+";
	     myargv[2] = 0;
	     return getopt (2, myargv, "+a") != '?';
	   ]])],
	[gl_cv_func_gnu_getopt=yes],
	[gl_cv_func_gnu_getopt=no],
	[dnl cross compiling - pessimistically guess based on decls
	 dnl Solaris 10 getopt doesn't handle `+' as a leading character in an
	 dnl option string (as of 2005-05-05).
	 AC_CHECK_DECL([getopt_clip],
	   [gl_cv_func_gnu_getopt=no], [gl_cv_func_gnu_getopt=yes],
	   [#include <getopt.h>])])])
    if test "$gl_cv_func_gnu_getopt" = "no"; then
      GETOPT_H=getopt.h
    fi
  fi
])

AC_DEFUN([gl_GETOPT_IFELSE],
[
  AC_REQUIRE([gl_GETOPT_CHECK_HEADERS])
  AS_IF([test -n "$GETOPT_H"], [$1], [$2])
])

AC_DEFUN([gl_GETOPT], [gl_GETOPT_IFELSE([gl_GETOPT_SUBSTITUTE])])

# Prerequisites of lib/getopt*.
AC_DEFUN([gl_PREREQ_GETOPT],
[
  AC_CHECK_DECLS_ONCE([getenv])
])
