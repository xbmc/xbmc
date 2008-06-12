## Scanf tests.  This file is in public domain.
##
## ye_FUNC_SCANF_MODIF_LONG_LONG
## Defines:
## SCANF_MODIF_LONG_LONG to format modifier string used for long long conversion
AC_DEFUN([ye_FUNC_SCANF_MODIF_SIZE_T],
[AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_HEADER_STDC])dnl
AC_REQUIRE([AC_C_CONST])dnl
dnl

dnl Check whether scanf accepts `z' modifier (size_t) to integer conversion
dnl specified in C99.
AC_CACHE_CHECK([for scanf size_t conversion modifier],
  yeti_cv_func_scanf_modif_size_t,
  AC_TRY_RUN([#ifdef STDC_HEADERS
    #include <stdlib.h>
    #endif
    int main(void) {
    size_t x;
    if (sscanf("123456789", "%zd\n", &x) == 1 && x == 123456789) return 0; else return 1;
    }],
    yeti_cv_func_scanf_modif_size_t=z,
    yeti_cv_func_scanf_modif_size_t=,))
AC_DEFINE_UNQUOTED(SCANF_MODIF_SIZE_T,
  "$yeti_cv_func_scanf_modif_size_t",
  [Define to scanf() format needed for size_t conversion.])
])

## ye_FUNC_SCANF_MODIF_LONG_LONG
## Defines:
## SCANF_MODIF_LONG_LONG to format modifier string used for size_t conversion
AC_DEFUN([ye_FUNC_SCANF_MODIF_LONG_LONG],
[AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_HEADER_STDC])dnl
AC_REQUIRE([AC_C_CONST])dnl
dnl

dnl Find out which long long modifier scanf accepts.
AC_CACHE_CHECK([for scanf long long conversion modifier],
  yeti_cv_func_scanf_modif_long_long,
  AC_TRY_RUN([#ifdef STDC_HEADERS
    #include <stdlib.h>
    #endif
    int main(void) {
    long long int x;
    if (sscanf("102030405060708090", "%lld\n", &x) == 1 && x == 102030405060708090LL) return 0; else return 1;
    }],
    yeti_cv_func_scanf_modif_long_long=ll,
    yeti_cv_func_scanf_modif_long_long=,)
  if test -z "$yeti_cv_func_scanf_modif_long_long"; then
    AC_TRY_RUN([#ifdef STDC_HEADERS
    #include <stdlib.h>
    #endif
    int main(void) {
    long long int x;
    if (sscanf("102030405060708090", "%Ld\n", &x) == 1 && x == 102030405060708090LL) return 0; else return 1;
    }],
    yeti_cv_func_scanf_modif_long_long=L,
    yeti_cv_func_scanf_modif_long_long=,)
  fi
  if test -z "$yeti_cv_func_scanf_modif_long_long"; then
    AC_TRY_RUN([#ifdef STDC_HEADERS
    #include <stdlib.h>
    #endif
    int main(void) {
    long long int x;
    if (sscanf("102030405060708090", "%qd\n", &x) == 1 && x == 102030405060708090LL) return 0; else return 1;
    }],
    yeti_cv_func_scanf_modif_long_long=q,
    yeti_cv_func_scanf_modif_long_long=,)
  fi)
AC_DEFINE_UNQUOTED(SCANF_MODIF_LONG_LONG,
  "$yeti_cv_func_scanf_modif_long_long",
  [Define to scanf() format needed for long long conversion.])
])
