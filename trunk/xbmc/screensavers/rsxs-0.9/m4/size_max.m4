# size_max.m4 serial 4
dnl Copyright (C) 2003, 2005-2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Bruno Haible.

AC_DEFUN([gl_SIZE_MAX],
[
  AC_CHECK_HEADERS(stdint.h)
  dnl First test whether the system already has SIZE_MAX.
  AC_MSG_CHECKING([for SIZE_MAX])
  result=
  AC_EGREP_CPP([Found it], [
#include <limits.h>
#if HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef SIZE_MAX
Found it
#endif
], result=yes)
  if test -z "$result"; then
    dnl Define it ourselves. Here we assume that the type 'size_t' is not wider
    dnl than the type 'unsigned long'. Try hard to find a definition that can
    dnl be used in a preprocessor #if, i.e. doesn't contain a cast.
    _AC_COMPUTE_INT([sizeof (size_t) * CHAR_BIT - 1], size_t_bits_minus_1,
      [#include <stddef.h>
#include <limits.h>], size_t_bits_minus_1=)
    _AC_COMPUTE_INT([sizeof (size_t) <= sizeof (unsigned int)], fits_in_uint,
      [#include <stddef.h>], fits_in_uint=)
    if test -n "$size_t_bits_minus_1" && test -n "$fits_in_uint"; then
      if test $fits_in_uint = 1; then
        dnl Even though SIZE_MAX fits in an unsigned int, it must be of type
        dnl 'unsigned long' if the type 'size_t' is the same as 'unsigned long'.
        AC_TRY_COMPILE([#include <stddef.h>
          extern size_t foo;
          extern unsigned long foo;
          ], [], fits_in_uint=0)
      fi
      dnl We cannot use 'expr' to simplify this expression, because 'expr'
      dnl works only with 'long' integers in the host environment, while we
      dnl might be cross-compiling from a 32-bit platform to a 64-bit platform.
      if test $fits_in_uint = 1; then
        result="(((1U << $size_t_bits_minus_1) - 1) * 2 + 1)"
      else
        result="(((1UL << $size_t_bits_minus_1) - 1) * 2 + 1)"
      fi
    else
      dnl Shouldn't happen, but who knows...
      result='((size_t)~(size_t)0)'
    fi
  fi
  AC_MSG_RESULT([$result])
  if test "$result" != yes; then
    AC_DEFINE_UNQUOTED([SIZE_MAX], [$result],
      [Define as the maximum value of type 'size_t', if the system doesn't define it.])
  fi
])
