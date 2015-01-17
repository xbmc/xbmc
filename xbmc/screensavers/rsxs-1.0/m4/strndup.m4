# strndup.m4 serial 15
dnl Copyright (C) 2002-2003, 2005-2008 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRNDUP],
[
  dnl Persuade glibc <string.h> to declare strndup().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])
  AC_CHECK_DECLS_ONCE([strndup])
  if test $ac_cv_have_decl_strndup = no; then
    HAVE_DECL_STRNDUP=0
  fi

  # AIX 4.3.3, AIX 5.1 have a function that fails to add the terminating '\0'.
  AC_CACHE_CHECK([for working strndup], gl_cv_func_strndup,
    [AC_RUN_IFELSE([
       AC_LANG_PROGRAM([[#include <string.h>
			 #include <stdlib.h>]], [[
#ifndef HAVE_DECL_STRNDUP
  extern char *strndup (const char *, size_t);
#endif
  char *s;
  s = strndup ("some longer string", 15);
  free (s);
  s = strndup ("shorter string", 13);
  return s[13] != '\0';]])],
       [gl_cv_func_strndup=yes],
       [gl_cv_func_strndup=no],
       [AC_CHECK_FUNC([strndup],
          [AC_EGREP_CPP([too risky], [
#ifdef _AIX
               too risky
#endif
             ],
             [gl_cv_func_strndup=no],
             [gl_cv_func_strndup=yes])],
          [gl_cv_func_strndup=no])])])
  if test $gl_cv_func_strndup = yes; then
    AC_DEFINE([HAVE_STRNDUP], 1,
      [Define if you have the strndup() function and it works.])
  else
    HAVE_STRNDUP=0
    AC_LIBOBJ([strndup])
    gl_PREREQ_STRNDUP
  fi
])

# Prerequisites of lib/strndup.c.
AC_DEFUN([gl_PREREQ_STRNDUP], [:])
