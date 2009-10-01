# vasnprintf.m4 serial 5
dnl Copyright (C) 2002-2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_VASNPRINTF],
[
  AC_REQUIRE([gl_EOVERFLOW])
  AC_REPLACE_FUNCS(vasnprintf)
  if test $ac_cv_func_vasnprintf = no; then
    AC_LIBOBJ(printf-args)
    AC_LIBOBJ(printf-parse)
    AC_LIBOBJ(asnprintf)
    gl_PREREQ_PRINTF_ARGS
    gl_PREREQ_PRINTF_PARSE
    gl_PREREQ_VASNPRINTF
    gl_PREREQ_ASNPRINTF
  fi
])

# Prequisites of lib/printf-args.h, lib/printf-args.c.
AC_DEFUN([gl_PREREQ_PRINTF_ARGS],
[
  AC_REQUIRE([bh_C_SIGNED])
  AC_REQUIRE([gl_AC_TYPE_LONG_LONG])
  AC_REQUIRE([gt_TYPE_LONGDOUBLE])
  AC_REQUIRE([gt_TYPE_WCHAR_T])
  AC_REQUIRE([gt_TYPE_WINT_T])
])

# Prequisites of lib/printf-parse.h, lib/printf-parse.c.
AC_DEFUN([gl_PREREQ_PRINTF_PARSE],
[
  AC_REQUIRE([gl_AC_TYPE_LONG_LONG])
  AC_REQUIRE([gt_TYPE_LONGDOUBLE])
  AC_REQUIRE([gt_TYPE_WCHAR_T])
  AC_REQUIRE([gt_TYPE_WINT_T])
  AC_REQUIRE([AC_TYPE_SIZE_T])
  AC_CHECK_TYPES(ptrdiff_t)
  AC_REQUIRE([gt_AC_TYPE_INTMAX_T])
])

# Prerequisites of lib/vasnprintf.c.
AC_DEFUN([gl_PREREQ_VASNPRINTF],
[
  AC_REQUIRE([AC_FUNC_ALLOCA])
  AC_REQUIRE([gl_AC_TYPE_LONG_LONG])
  AC_REQUIRE([gt_TYPE_LONGDOUBLE])
  AC_REQUIRE([gt_TYPE_WCHAR_T])
  AC_REQUIRE([gt_TYPE_WINT_T])
  AC_CHECK_FUNCS(snprintf wcslen)
])

# Prerequisites of lib/asnprintf.c.
AC_DEFUN([gl_PREREQ_ASNPRINTF],
[
])
