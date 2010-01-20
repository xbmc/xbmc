#serial 1

AC_PREREQ(2.50)

# The EILSEQ errno value ought to be defined in <errno.h>, according to
# ISO C 99 and POSIX.  But some systems (like SunOS 4) don't define it,
# and some systems (like BSD/OS) define it in <wchar.h> not <errno.h>.

# Define EILSEQ as a C macro and as a substituted macro in such a way that
# 1. on all systems, after inclusion of <errno.h>, EILSEQ is usable,
# 2. on systems where EILSEQ is defined elsewhere, we use the same numeric
#    value.

AC_DEFUN([AC_EILSEQ],
[
  AC_REQUIRE([AC_PROG_CC])dnl

  dnl Check for any extra headers that could define EILSEQ.
  AC_CHECK_HEADERS(wchar.h)

  AC_CACHE_CHECK([for EILSEQ], ac_cv_decl_EILSEQ, [
    AC_EGREP_CPP(yes,[
#include <errno.h>
#ifdef EILSEQ
yes
#endif
      ], have_eilseq=1)
    if test -n "$have_eilseq"; then
      dnl EILSEQ exists in <errno.h>. Don't need to define EILSEQ ourselves.
      ac_cv_decl_EILSEQ=yes
    else
      AC_EGREP_CPP(yes,[
#include <errno.h>
#if HAVE_WCHAR_H
#include <wchar.h>
#endif
#ifdef EILSEQ
yes
#endif
        ], have_eilseq=1)
      if test -n "$have_eilseq"; then
        dnl EILSEQ exists in some other system header.
        dnl Define it to the same value.
        _AC_COMPUTE_INT([EILSEQ], ac_cv_decl_EILSEQ, [
#include <errno.h>
#if HAVE_WCHAR_H
#include <wchar.h>
#endif
/* The following two lines are a workaround against an autoconf-2.52 bug.  */
#include <stdio.h>
#include <stdlib.h>
])
      else
        dnl EILSEQ isn't defined by the system. Define EILSEQ ourselves, but
        dnl don't define it as EINVAL, because iconv() callers want to
        dnl distinguish EINVAL and EILSEQ.
        ac_cv_decl_EILSEQ=ENOENT
      fi
    fi
  ])
  if test "$ac_cv_decl_EILSEQ" != yes; then
    AC_DEFINE_UNQUOTED([EILSEQ], [$ac_cv_decl_EILSEQ],
                       [Define as good substitute value for EILSEQ.])
    EILSEQ="$ac_cv_decl_EILSEQ"
    AC_SUBST(EILSEQ)
  fi
])
