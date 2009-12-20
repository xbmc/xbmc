## `Small' checks for typedefs and variables.  This file is in public domain.

## program_invocation_short_name test.
## Defines:
## HAVE_PROGRAM_INVOCATION_SHORT_NAME when program_invocation_short_name is
##   defined in errno.h
AC_DEFUN([ye_CHECK_VAR_PROGRAM_INVOCATION_SHORT_NAME],
[AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AM_PROG_CC_STDC])dnl
dnl

dnl Check for program_invocation_short_name (present on GNU systems only?)
AC_CACHE_CHECK([for program_invocation_short_name],
  yeti_cv_lib_c_program_invocation_short_name,
  AC_TRY_LINK([#include <errno.h>],
    [if (!program_invocation_short_name) return 1;],
    yeti_cv_lib_c_program_invocation_short_name=yes,
    yeti_cv_lib_c_program_invocation_short_name=no))
if test "$yeti_cv_lib_c_program_invocation_short_name" = yes; then
  AC_DEFINE(HAVE_PROGRAM_INVOCATION_SHORT_NAME,1,[Define if you have program_invocation_short_name variable.])
fi])
