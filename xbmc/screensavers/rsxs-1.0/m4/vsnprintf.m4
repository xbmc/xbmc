# vsnprintf.m4 serial 5
dnl Copyright (C) 2002-2004, 2007-2008 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_VSNPRINTF],
[
  AC_REQUIRE([gl_STDIO_H_DEFAULTS])
  gl_cv_func_vsnprintf_usable=no
  AC_CHECK_FUNCS([vsnprintf])
  if test $ac_cv_func_vsnprintf = yes; then
    gl_SNPRINTF_SIZE1
    case "$gl_cv_func_snprintf_size1" in
      *yes)
        gl_cv_func_vsnprintf_usable=yes
        ;;
    esac
  fi
  if test $gl_cv_func_vsnprintf_usable = no; then
    gl_REPLACE_VSNPRINTF
  fi
  AC_CHECK_DECLS_ONCE([vsnprintf])
  if test $ac_cv_have_decl_vsnprintf = no; then
    HAVE_DECL_VSNPRINTF=0
  fi
])

AC_DEFUN([gl_REPLACE_VSNPRINTF],
[
  AC_REQUIRE([gl_STDIO_H_DEFAULTS])
  AC_LIBOBJ([vsnprintf])
  if test $ac_cv_func_vsnprintf = yes; then
    REPLACE_VSNPRINTF=1
  fi
  gl_PREREQ_VSNPRINTF
])

# Prerequisites of lib/vsnprintf.c.
AC_DEFUN([gl_PREREQ_VSNPRINTF], [:])
