# setenv.m4 serial 11
dnl Copyright (C) 2001-2004, 2006-2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_SETENV],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  AC_CHECK_FUNCS_ONCE([setenv])
  if test $ac_cv_func_setenv = no; then
    HAVE_SETENV=0
    AC_LIBOBJ([setenv])
    gl_PREREQ_SETENV
  fi
])

# Like gl_FUNC_SETENV, except prepare for separate compilation (no AC_LIBOBJ).
AC_DEFUN([gl_FUNC_SETENV_SEPARATE],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  AC_CHECK_FUNCS_ONCE([setenv])
  if test $ac_cv_func_setenv = no; then
    HAVE_SETENV=0
  fi
  gl_PREREQ_SETENV
])

AC_DEFUN([gl_FUNC_UNSETENV],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  AC_CHECK_FUNCS([unsetenv])
  if test $ac_cv_func_unsetenv = no; then
    HAVE_UNSETENV=0
    AC_LIBOBJ([unsetenv])
    gl_PREREQ_UNSETENV
  else
    AC_CACHE_CHECK([for unsetenv() return type], [gt_cv_func_unsetenv_ret],
      [AC_TRY_COMPILE([#include <stdlib.h>
extern
#ifdef __cplusplus
"C"
#endif
#if defined(__STDC__) || defined(__cplusplus)
int unsetenv (const char *name);
#else
int unsetenv();
#endif
], , gt_cv_func_unsetenv_ret='int', gt_cv_func_unsetenv_ret='void')])
    if test $gt_cv_func_unsetenv_ret = 'void'; then
      VOID_UNSETENV=1
    fi
  fi
])

# Prerequisites of lib/setenv.c.
AC_DEFUN([gl_PREREQ_SETENV],
[
  AC_REQUIRE([AC_FUNC_ALLOCA])
  AC_REQUIRE([gl_ENVIRON])
  AC_CHECK_HEADERS_ONCE([unistd.h])
  AC_CHECK_HEADERS([search.h])
  AC_CHECK_FUNCS([tsearch])
])

# Prerequisites of lib/unsetenv.c.
AC_DEFUN([gl_PREREQ_UNSETENV],
[
  AC_REQUIRE([gl_ENVIRON])
  AC_CHECK_HEADERS_ONCE([unistd.h])
])
