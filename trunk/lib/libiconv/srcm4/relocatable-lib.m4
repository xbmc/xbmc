# relocatable-lib.m4 serial 4
dnl Copyright (C) 2003, 2005-2007, 2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Bruno Haible.

dnl Support for relocatable libraries.
AC_DEFUN([gl_RELOCATABLE_LIBRARY],
[
  AC_REQUIRE([gl_RELOCATABLE_LIBRARY_BODY])
  if test $RELOCATABLE = yes; then
    AC_LIBOBJ([relocatable])
  fi
])
AC_DEFUN([gl_RELOCATABLE_LIBRARY_BODY],
[
  AC_REQUIRE([gl_RELOCATABLE_NOP])
  dnl Easier to put this here once, instead of into the DEFS of each Makefile.
  if test "X$prefix" = "XNONE"; then
    reloc_final_prefix="$ac_default_prefix"
  else
    reloc_final_prefix="$prefix"
  fi
  AC_DEFINE_UNQUOTED([INSTALLPREFIX], ["${reloc_final_prefix}"],
    [Define to the value of ${prefix}, as a string.])
  if test $RELOCATABLE = yes; then
    AC_DEFINE([ENABLE_RELOCATABLE], [1],
      [Define to 1 if the package shall run at any location in the filesystem.])
  fi
])

dnl Like gl_RELOCATABLE_LIBRARY, except prepare for separate compilation
dnl (no AC_LIBOBJ).
AC_DEFUN([gl_RELOCATABLE_LIBRARY_SEPARATE],
[
  AC_REQUIRE([gl_RELOCATABLE_LIBRARY_BODY])
])

dnl Support for relocatable packages for which it is a nop.
AC_DEFUN([gl_RELOCATABLE_NOP],
[
  AC_MSG_CHECKING([whether to activate relocatable installation])
  AC_ARG_ENABLE([relocatable],
    [  --enable-relocatable    install a package that can be moved in the filesystem],
    [if test "$enableval" != no; then
       RELOCATABLE=yes
     else
       RELOCATABLE=no
     fi
    ], RELOCATABLE=no)
  AC_SUBST([RELOCATABLE])
  AC_MSG_RESULT([$RELOCATABLE])
])

