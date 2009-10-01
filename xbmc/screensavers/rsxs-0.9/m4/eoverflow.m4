# eoverflow.m4 serial 1
dnl Copyright (C) 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Bruno Haible.

# The EOVERFLOW errno value ought to be defined in <errno.h>, according to
# POSIX.  But some systems (like AIX 3) don't define it, and some systems
# (like OSF/1) define it when _XOPEN_SOURCE_EXTENDED is defined.

# Define EOVERFLOW as a C macro and as a substituted macro in such a way that
# 1. on all systems, after inclusion of <errno.h>, EOVERFLOW is usable,
# 2. on systems where EOVERFLOW is defined elsewhere, we use the same numeric
#    value.

AC_DEFUN([gl_EOVERFLOW],
[
  AC_REQUIRE([AC_PROG_CC])dnl

  AC_CACHE_CHECK([for EOVERFLOW], ac_cv_decl_EOVERFLOW, [
    AC_EGREP_CPP(yes,[
#include <errno.h>
#ifdef EOVERFLOW
yes
#endif
      ], have_eoverflow=1)
    if test -n "$have_eoverflow"; then
      dnl EOVERFLOW exists in <errno.h>. Don't need to define EOVERFLOW ourselves.
      ac_cv_decl_EOVERFLOW=yes
    else
      AC_EGREP_CPP(yes,[
#define _XOPEN_SOURCE_EXTENDED 1
#include <errno.h>
#ifdef EOVERFLOW
yes
#endif
        ], have_eoverflow=1)
      if test -n "$have_eoverflow"; then
        dnl EOVERFLOW exists but is hidden.
        dnl Define it to the same value.
        _AC_COMPUTE_INT([EOVERFLOW], ac_cv_decl_EOVERFLOW, [
#define _XOPEN_SOURCE_EXTENDED 1
#include <errno.h>
/* The following two lines are a workaround against an autoconf-2.52 bug.  */
#include <stdio.h>
#include <stdlib.h>
])
      else
        dnl EOVERFLOW isn't defined by the system. Define EOVERFLOW ourselves, but
        dnl don't define it as EINVAL, because snprintf() callers want to
        dnl distinguish EINVAL and EOVERFLOW.
        ac_cv_decl_EOVERFLOW=E2BIG
      fi
    fi
  ])
  if test "$ac_cv_decl_EOVERFLOW" != yes; then
    AC_DEFINE_UNQUOTED([EOVERFLOW], [$ac_cv_decl_EOVERFLOW],
                       [Define as good substitute value for EOVERFLOW.])
    EOVERFLOW="$ac_cv_decl_EOVERFLOW"
    AC_SUBST(EOVERFLOW)
  fi
])
