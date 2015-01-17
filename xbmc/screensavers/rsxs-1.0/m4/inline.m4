# inline.m4 serial 3
dnl Copyright (C) 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl Test for the 'inline' keyword or equivalent.
dnl Define 'inline' to a supported equivalent, or to nothing if not supported,
dnl like AC_C_INLINE does.  Also, define HAVE_INLINE if 'inline' or an
dnl equivalent is effectively supported, i.e. if the compiler is likely to
dnl drop unused 'static inline' functions.
AC_DEFUN([gl_INLINE],
[
  AC_REQUIRE([AC_C_INLINE])
  AC_CACHE_CHECK([whether the compiler generally respects inline],
    [gl_cv_c_inline_effective],
    [if test $ac_cv_c_inline = no; then
       gl_cv_c_inline_effective=no
     else
       dnl GCC defines __NO_INLINE__ if not optimizing or if -fno-inline is
       dnl specified.
       dnl Use AC_COMPILE_IFELSE here, not AC_EGREP_CPP, because the result
       dnl depends on optimization flags, which can be in CFLAGS.
       dnl (AC_EGREP_CPP looks only at the CPPFLAGS.)
       AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM([[]],
           [[#ifdef __NO_INLINE__
               #error "inline is not effective"
             #endif]])],
         [gl_cv_c_inline_effective=yes],
         [gl_cv_c_inline_effective=no])
     fi
    ])
  if test $gl_cv_c_inline_effective = yes; then
    AC_DEFINE([HAVE_INLINE], 1,
      [Define to 1 if the compiler supports one of the keywords
       'inline', '__inline__', '__inline' and effectively inlines
       functions marked as such.])
  fi
])
