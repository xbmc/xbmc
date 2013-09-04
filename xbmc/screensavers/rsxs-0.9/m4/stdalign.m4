# Check for stdalign.h that conforms to C1x.

dnl Copyright 2011 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Prepare for substituting <stdalign.h> if it is not supported.

AC_DEFUN([gl_STDALIGN_H],
[
  AC_CACHE_CHECK([for working stdalign.h],
    [gl_cv_header_working_stdalign_h],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <stdalign.h>
            int align_int = alignof (int) + _Alignof (double);

            /* Test _Alignas only on platforms where gnulib can help.  */
            #if \
                (__GNUC__ || __IBMC__ || __IBMCPP__ \
                 || 0x5110 <= __SUNPRO_C || 1300 <= _MSC_VER)
              int alignas (8) alignas_int = 1;
            #endif
          ]])],
       [gl_cv_header_working_stdalign_h=yes],
       [gl_cv_header_working_stdalign_h=no])])

  if test $gl_cv_header_working_stdalign_h = yes; then
    STDALIGN_H=''
  else
    STDALIGN_H='stdalign.h'
  fi

  AC_SUBST([STDALIGN_H])
  AM_CONDITIONAL([GL_GENERATE_STDALIGN_H], [test -n "$STDALIGN_H"])
])
