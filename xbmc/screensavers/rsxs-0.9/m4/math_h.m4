# math_h.m4 serial 56
dnl Copyright (C) 2007-2011 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_MATH_H],
[
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  gl_CHECK_NEXT_HEADERS([math.h])
  AC_REQUIRE([AC_C_INLINE])

  AC_CACHE_CHECK([whether NAN macro works], [gl_cv_header_math_nan_works],
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[@%:@include <math.h>]],
      [[/* Solaris 10 has a broken definition of NAN.  Other platforms
        fail to provide NAN, or provide it only in C99 mode; this
        test only needs to fail when NAN is provided but wrong.  */
         float f = 1.0f;
#ifdef NAN
         f = NAN;
#endif
         return f == 0;]])],
      [gl_cv_header_math_nan_works=yes],
      [gl_cv_header_math_nan_works=no])])
  if test $gl_cv_header_math_nan_works = no; then
    REPLACE_NAN=1
  fi
  AC_CACHE_CHECK([whether HUGE_VAL works], [gl_cv_header_math_huge_val_works],
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[@%:@include <math.h>]],
      [[/* Solaris 10 has a broken definition of HUGE_VAL.  */
         double d = HUGE_VAL;
         return d == 0;]])],
      [gl_cv_header_math_huge_val_works=yes],
      [gl_cv_header_math_huge_val_works=no])])
  if test $gl_cv_header_math_huge_val_works = no; then
    REPLACE_HUGE_VAL=1
  fi

  dnl Check for declarations of anything we want to poison if the
  dnl corresponding gnulib module is not in use.
  gl_WARN_ON_USE_PREPARE([[#include <math.h>]],
    [acosf acosl asinf asinl atanf atanl
     ceilf ceill copysign copysignf copysignl cosf cosl coshf
     expf expl fabsf floorf floorl fma fmaf fmal fmodf frexpf frexpl
     ldexpf ldexpl logb logf logl log10f modff powf
     rint rintf rintl round roundf roundl sinf sinl sinhf sqrtf sqrtl
     tanf tanl tanhf trunc truncf truncl])
])

AC_DEFUN([gl_MATH_MODULE_INDICATOR],
[
  dnl Use AC_REQUIRE here, so that the default settings are expanded once only.
  AC_REQUIRE([gl_MATH_H_DEFAULTS])
  gl_MODULE_INDICATOR_SET_VARIABLE([$1])
  dnl Define it also as a C macro, for the benefit of the unit tests.
  gl_MODULE_INDICATOR_FOR_TESTS([$1])
])

AC_DEFUN([gl_MATH_H_DEFAULTS],
[
  GNULIB_ACOSF=0;     AC_SUBST([GNULIB_ACOSF])
  GNULIB_ACOSL=0;     AC_SUBST([GNULIB_ACOSL])
  GNULIB_ASINF=0;     AC_SUBST([GNULIB_ASINF])
  GNULIB_ASINL=0;     AC_SUBST([GNULIB_ASINL])
  GNULIB_ATANF=0;     AC_SUBST([GNULIB_ATANF])
  GNULIB_ATANL=0;     AC_SUBST([GNULIB_ATANL])
  GNULIB_ATAN2F=0;    AC_SUBST([GNULIB_ATAN2F])
  GNULIB_CEIL=0;      AC_SUBST([GNULIB_CEIL])
  GNULIB_CEILF=0;     AC_SUBST([GNULIB_CEILF])
  GNULIB_CEILL=0;     AC_SUBST([GNULIB_CEILL])
  GNULIB_COPYSIGN=0;  AC_SUBST([GNULIB_COPYSIGN])
  GNULIB_COPYSIGNF=0; AC_SUBST([GNULIB_COPYSIGNF])
  GNULIB_COPYSIGNL=0; AC_SUBST([GNULIB_COPYSIGNL])
  GNULIB_COSF=0;      AC_SUBST([GNULIB_COSF])
  GNULIB_COSL=0;      AC_SUBST([GNULIB_COSL])
  GNULIB_COSHF=0;     AC_SUBST([GNULIB_COSHF])
  GNULIB_EXPF=0;      AC_SUBST([GNULIB_EXPF])
  GNULIB_EXPL=0;      AC_SUBST([GNULIB_EXPL])
  GNULIB_FABSF=0;     AC_SUBST([GNULIB_FABSF])
  GNULIB_FLOOR=0;     AC_SUBST([GNULIB_FLOOR])
  GNULIB_FLOORF=0;    AC_SUBST([GNULIB_FLOORF])
  GNULIB_FLOORL=0;    AC_SUBST([GNULIB_FLOORL])
  GNULIB_FMA=0;       AC_SUBST([GNULIB_FMA])
  GNULIB_FMAF=0;      AC_SUBST([GNULIB_FMAF])
  GNULIB_FMAL=0;      AC_SUBST([GNULIB_FMAL])
  GNULIB_FMODF=0;     AC_SUBST([GNULIB_FMODF])
  GNULIB_FREXPF=0;    AC_SUBST([GNULIB_FREXPF])
  GNULIB_FREXP=0;     AC_SUBST([GNULIB_FREXP])
  GNULIB_FREXPL=0;    AC_SUBST([GNULIB_FREXPL])
  GNULIB_ISFINITE=0;  AC_SUBST([GNULIB_ISFINITE])
  GNULIB_ISINF=0;     AC_SUBST([GNULIB_ISINF])
  GNULIB_ISNAN=0;     AC_SUBST([GNULIB_ISNAN])
  GNULIB_ISNANF=0;    AC_SUBST([GNULIB_ISNANF])
  GNULIB_ISNAND=0;    AC_SUBST([GNULIB_ISNAND])
  GNULIB_ISNANL=0;    AC_SUBST([GNULIB_ISNANL])
  GNULIB_LDEXPF=0;    AC_SUBST([GNULIB_LDEXPF])
  GNULIB_LDEXPL=0;    AC_SUBST([GNULIB_LDEXPL])
  GNULIB_LOGB=0;      AC_SUBST([GNULIB_LOGB])
  GNULIB_LOGF=0;      AC_SUBST([GNULIB_LOGF])
  GNULIB_LOGL=0;      AC_SUBST([GNULIB_LOGL])
  GNULIB_LOG10F=0;    AC_SUBST([GNULIB_LOG10F])
  GNULIB_MODFF=0;     AC_SUBST([GNULIB_MODFF])
  GNULIB_POWF=0;      AC_SUBST([GNULIB_POWF])
  GNULIB_RINT=0;      AC_SUBST([GNULIB_RINT])
  GNULIB_RINTF=0;     AC_SUBST([GNULIB_RINTF])
  GNULIB_RINTL=0;     AC_SUBST([GNULIB_RINTL])
  GNULIB_ROUND=0;     AC_SUBST([GNULIB_ROUND])
  GNULIB_ROUNDF=0;    AC_SUBST([GNULIB_ROUNDF])
  GNULIB_ROUNDL=0;    AC_SUBST([GNULIB_ROUNDL])
  GNULIB_SIGNBIT=0;   AC_SUBST([GNULIB_SIGNBIT])
  GNULIB_SINF=0;      AC_SUBST([GNULIB_SINF])
  GNULIB_SINL=0;      AC_SUBST([GNULIB_SINL])
  GNULIB_SINHF=0;     AC_SUBST([GNULIB_SINHF])
  GNULIB_SQRTF=0;     AC_SUBST([GNULIB_SQRTF])
  GNULIB_SQRTL=0;     AC_SUBST([GNULIB_SQRTL])
  GNULIB_TANF=0;      AC_SUBST([GNULIB_TANF])
  GNULIB_TANL=0;      AC_SUBST([GNULIB_TANL])
  GNULIB_TANHF=0;     AC_SUBST([GNULIB_TANHF])
  GNULIB_TRUNC=0;     AC_SUBST([GNULIB_TRUNC])
  GNULIB_TRUNCF=0;    AC_SUBST([GNULIB_TRUNCF])
  GNULIB_TRUNCL=0;    AC_SUBST([GNULIB_TRUNCL])
  dnl Assume proper GNU behavior unless another module says otherwise.
  HAVE_ACOSF=1;                AC_SUBST([HAVE_ACOSF])
  HAVE_ACOSL=1;                AC_SUBST([HAVE_ACOSL])
  HAVE_ASINF=1;                AC_SUBST([HAVE_ASINF])
  HAVE_ASINL=1;                AC_SUBST([HAVE_ASINL])
  HAVE_ATANF=1;                AC_SUBST([HAVE_ATANF])
  HAVE_ATANL=1;                AC_SUBST([HAVE_ATANL])
  HAVE_ATAN2F=1;               AC_SUBST([HAVE_ATAN2F])
  HAVE_COPYSIGN=1;             AC_SUBST([HAVE_COPYSIGN])
  HAVE_COPYSIGNF=1;            AC_SUBST([HAVE_COPYSIGNF])
  HAVE_COPYSIGNL=1;            AC_SUBST([HAVE_COPYSIGNL])
  HAVE_COSF=1;                 AC_SUBST([HAVE_COSF])
  HAVE_COSL=1;                 AC_SUBST([HAVE_COSL])
  HAVE_COSHF=1;                AC_SUBST([HAVE_COSHF])
  HAVE_EXPF=1;                 AC_SUBST([HAVE_EXPF])
  HAVE_EXPL=1;                 AC_SUBST([HAVE_EXPL])
  HAVE_FABSF=1;                AC_SUBST([HAVE_FABSF])
  HAVE_FMA=1;                  AC_SUBST([HAVE_FMA])
  HAVE_FMAF=1;                 AC_SUBST([HAVE_FMAF])
  HAVE_FMAL=1;                 AC_SUBST([HAVE_FMAL])
  HAVE_FMODF=1;                AC_SUBST([HAVE_FMODF])
  HAVE_FREXPF=1;               AC_SUBST([HAVE_FREXPF])
  HAVE_ISNANF=1;               AC_SUBST([HAVE_ISNANF])
  HAVE_ISNAND=1;               AC_SUBST([HAVE_ISNAND])
  HAVE_ISNANL=1;               AC_SUBST([HAVE_ISNANL])
  HAVE_LDEXPF=1;               AC_SUBST([HAVE_LDEXPF])
  HAVE_LOGF=1;                 AC_SUBST([HAVE_LOGF])
  HAVE_LOGL=1;                 AC_SUBST([HAVE_LOGL])
  HAVE_LOG10F=1;               AC_SUBST([HAVE_LOG10F])
  HAVE_MODFF=1;                AC_SUBST([HAVE_MODFF])
  HAVE_POWF=1;                 AC_SUBST([HAVE_POWF])
  HAVE_RINT=1;                 AC_SUBST([HAVE_RINT])
  HAVE_RINTF=1;                AC_SUBST([HAVE_RINTF])
  HAVE_RINTL=1;                AC_SUBST([HAVE_RINTL])
  HAVE_SINF=1;                 AC_SUBST([HAVE_SINF])
  HAVE_SINL=1;                 AC_SUBST([HAVE_SINL])
  HAVE_SINHF=1;                AC_SUBST([HAVE_SINHF])
  HAVE_SQRTF=1;                AC_SUBST([HAVE_SQRTF])
  HAVE_SQRTL=1;                AC_SUBST([HAVE_SQRTL])
  HAVE_TANF=1;                 AC_SUBST([HAVE_TANF])
  HAVE_TANL=1;                 AC_SUBST([HAVE_TANL])
  HAVE_TANHF=1;                AC_SUBST([HAVE_TANHF])
  HAVE_DECL_ACOSL=1;           AC_SUBST([HAVE_DECL_ACOSL])
  HAVE_DECL_ASINL=1;           AC_SUBST([HAVE_DECL_ASINL])
  HAVE_DECL_ATANL=1;           AC_SUBST([HAVE_DECL_ATANL])
  HAVE_DECL_CEILF=1;           AC_SUBST([HAVE_DECL_CEILF])
  HAVE_DECL_CEILL=1;           AC_SUBST([HAVE_DECL_CEILL])
  HAVE_DECL_COSL=1;            AC_SUBST([HAVE_DECL_COSL])
  HAVE_DECL_EXPL=1;            AC_SUBST([HAVE_DECL_EXPL])
  HAVE_DECL_FLOORF=1;          AC_SUBST([HAVE_DECL_FLOORF])
  HAVE_DECL_FLOORL=1;          AC_SUBST([HAVE_DECL_FLOORL])
  HAVE_DECL_FREXPL=1;          AC_SUBST([HAVE_DECL_FREXPL])
  HAVE_DECL_LDEXPL=1;          AC_SUBST([HAVE_DECL_LDEXPL])
  HAVE_DECL_LOGB=1;            AC_SUBST([HAVE_DECL_LOGB])
  HAVE_DECL_LOGL=1;            AC_SUBST([HAVE_DECL_LOGL])
  HAVE_DECL_ROUND=1;           AC_SUBST([HAVE_DECL_ROUND])
  HAVE_DECL_ROUNDF=1;          AC_SUBST([HAVE_DECL_ROUNDF])
  HAVE_DECL_ROUNDL=1;          AC_SUBST([HAVE_DECL_ROUNDL])
  HAVE_DECL_SINL=1;            AC_SUBST([HAVE_DECL_SINL])
  HAVE_DECL_SQRTL=1;           AC_SUBST([HAVE_DECL_SQRTL])
  HAVE_DECL_TANL=1;            AC_SUBST([HAVE_DECL_TANL])
  HAVE_DECL_TRUNC=1;           AC_SUBST([HAVE_DECL_TRUNC])
  HAVE_DECL_TRUNCF=1;          AC_SUBST([HAVE_DECL_TRUNCF])
  HAVE_DECL_TRUNCL=1;          AC_SUBST([HAVE_DECL_TRUNCL])
  REPLACE_CEIL=0;              AC_SUBST([REPLACE_CEIL])
  REPLACE_CEILF=0;             AC_SUBST([REPLACE_CEILF])
  REPLACE_CEILL=0;             AC_SUBST([REPLACE_CEILL])
  REPLACE_FLOOR=0;             AC_SUBST([REPLACE_FLOOR])
  REPLACE_FLOORF=0;            AC_SUBST([REPLACE_FLOORF])
  REPLACE_FLOORL=0;            AC_SUBST([REPLACE_FLOORL])
  REPLACE_FMA=0;               AC_SUBST([REPLACE_FMA])
  REPLACE_FMAF=0;              AC_SUBST([REPLACE_FMAF])
  REPLACE_FMAL=0;              AC_SUBST([REPLACE_FMAL])
  REPLACE_FREXPF=0;            AC_SUBST([REPLACE_FREXPF])
  REPLACE_FREXP=0;             AC_SUBST([REPLACE_FREXP])
  REPLACE_FREXPL=0;            AC_SUBST([REPLACE_FREXPL])
  REPLACE_HUGE_VAL=0;          AC_SUBST([REPLACE_HUGE_VAL])
  REPLACE_ISFINITE=0;          AC_SUBST([REPLACE_ISFINITE])
  REPLACE_ISINF=0;             AC_SUBST([REPLACE_ISINF])
  REPLACE_ISNAN=0;             AC_SUBST([REPLACE_ISNAN])
  REPLACE_LDEXPL=0;            AC_SUBST([REPLACE_LDEXPL])
  REPLACE_NAN=0;               AC_SUBST([REPLACE_NAN])
  REPLACE_ROUND=0;             AC_SUBST([REPLACE_ROUND])
  REPLACE_ROUNDF=0;            AC_SUBST([REPLACE_ROUNDF])
  REPLACE_ROUNDL=0;            AC_SUBST([REPLACE_ROUNDL])
  REPLACE_SIGNBIT=0;           AC_SUBST([REPLACE_SIGNBIT])
  REPLACE_SIGNBIT_USING_GCC=0; AC_SUBST([REPLACE_SIGNBIT_USING_GCC])
  REPLACE_TRUNC=0;             AC_SUBST([REPLACE_TRUNC])
  REPLACE_TRUNCF=0;            AC_SUBST([REPLACE_TRUNCF])
  REPLACE_TRUNCL=0;            AC_SUBST([REPLACE_TRUNCL])
])

# gl_LONG_DOUBLE_VS_DOUBLE
# determines whether 'long double' and 'double' have the same representation.
# Sets variable HAVE_SAME_LONG_DOUBLE_AS_DOUBLE to 0 or 1, and defines
# HAVE_SAME_LONG_DOUBLE_AS_DOUBLE accordingly.
# The currently known platforms where this is the case are:
# Linux/HPPA, Minix 3.1.8, AIX 5, AIX 6 and 7 with xlc, MSVC 9.
AC_DEFUN([gl_LONG_DOUBLE_VS_DOUBLE],
[
  AC_CACHE_CHECK([whether long double and double are the same],
    [gl_cv_long_double_equals_double],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([[#include <float.h>]],
          [[typedef int check[sizeof (long double) == sizeof (double)
                              && LDBL_MANT_DIG == DBL_MANT_DIG
                              && LDBL_MAX_EXP == DBL_MAX_EXP
                              && LDBL_MIN_EXP == DBL_MIN_EXP
                              ? 1 : -1];
          ]])],
       [gl_cv_long_double_equals_double=yes],
       [gl_cv_long_double_equals_double=no])
    ])
  if test $gl_cv_long_double_equals_double = yes; then
    AC_DEFINE([HAVE_SAME_LONG_DOUBLE_AS_DOUBLE], [1],
      [Define to 1 if 'long double' and 'double' have the same representation.])
    HAVE_SAME_LONG_DOUBLE_AS_DOUBLE=1
  else
    HAVE_SAME_LONG_DOUBLE_AS_DOUBLE=0
  fi
  AC_SUBST([HAVE_SAME_LONG_DOUBLE_AS_DOUBLE])
])
