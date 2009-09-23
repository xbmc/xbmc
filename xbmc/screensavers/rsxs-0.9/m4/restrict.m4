#serial 1003
dnl Copyright (C) 2003 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# This macro can be removed once we can rely on Autoconf 2.57a or later,
# since we can then use its AC_C_RESTRICT.

# gl_C_RESTRICT
# --------------
# Determine whether the C/C++ compiler supports the "restrict" keyword
# introduced in ANSI C99, or an equivalent.  Do nothing if the compiler
# accepts it.  Otherwise, if the compiler supports an equivalent,
# define "restrict" to be that.  Here are some variants:
# - GCC supports both __restrict and __restrict__
# - older DEC Alpha C compilers support only __restrict
# - _Restrict is the only spelling accepted by Sun WorkShop 6 update 2 C
# Otherwise, define "restrict" to be empty.
AC_DEFUN([gl_C_RESTRICT],
[AC_CACHE_CHECK([for C/C++ restrict keyword], gl_cv_c_restrict,
  [gl_cv_c_restrict=no
   # Try the official restrict keyword, then gcc's __restrict, and
   # the less common variants.
   for ac_kw in restrict __restrict __restrict__ _Restrict; do
     AC_COMPILE_IFELSE([AC_LANG_SOURCE(
      [float * $ac_kw x;])],
      [gl_cv_c_restrict=$ac_kw; break])
   done
  ])
 case $gl_cv_c_restrict in
   restrict) ;;
   no) AC_DEFINE(restrict,,
	[Define to equivalent of C99 restrict keyword, or to nothing if this
	is not supported.  Do not define if restrict is supported directly.]) ;;
   *)  AC_DEFINE_UNQUOTED(restrict, $gl_cv_c_restrict) ;;
 esac
])
