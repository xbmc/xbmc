# malloc.m4 serial 13
dnl Copyright (C) 2007, 2009-2011 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# gl_FUNC_MALLOC_GNU
# ------------------
# Test whether 'malloc (0)' is handled like in GNU libc, and replace malloc if
# it is not.
AC_DEFUN([gl_FUNC_MALLOC_GNU],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  dnl _AC_FUNC_MALLOC_IF is defined in Autoconf.
  _AC_FUNC_MALLOC_IF(
    [AC_DEFINE([HAVE_MALLOC_GNU], [1],
               [Define to 1 if your system has a GNU libc compatible 'malloc'
                function, and to 0 otherwise.])],
    [AC_DEFINE([HAVE_MALLOC_GNU], [0])
     REPLACE_MALLOC=1
    ])
])

# gl_FUNC_MALLOC_POSIX
# --------------------
# Test whether 'malloc' is POSIX compliant (sets errno to ENOMEM when it
# fails), and replace malloc if it is not.
AC_DEFUN([gl_FUNC_MALLOC_POSIX],
[
  AC_REQUIRE([gl_STDLIB_H_DEFAULTS])
  AC_REQUIRE([gl_CHECK_MALLOC_POSIX])
  if test $gl_cv_func_malloc_posix = yes; then
    AC_DEFINE([HAVE_MALLOC_POSIX], [1],
      [Define if the 'malloc' function is POSIX compliant.])
  else
    REPLACE_MALLOC=1
  fi
])

# Test whether malloc, realloc, calloc are POSIX compliant,
# Set gl_cv_func_malloc_posix to yes or no accordingly.
AC_DEFUN([gl_CHECK_MALLOC_POSIX],
[
  AC_CACHE_CHECK([whether malloc, realloc, calloc are POSIX compliant],
    [gl_cv_func_malloc_posix],
    [
      dnl It is too dangerous to try to allocate a large amount of memory:
      dnl some systems go to their knees when you do that. So assume that
      dnl all Unix implementations of the function are POSIX compliant.
      AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
           [[]],
           [[#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__
             choke me
             #endif
            ]])],
        [gl_cv_func_malloc_posix=yes],
        [gl_cv_func_malloc_posix=no])
    ])
])
