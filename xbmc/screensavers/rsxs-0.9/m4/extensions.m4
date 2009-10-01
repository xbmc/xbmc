# Enable extensions on systems that normally disable them.

# Copyright (C) 2003, 2006 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This file is only needed in autoconf <= 2.59.  Newer versions of autoconf
# have a macro AC_USE_SYSTEM_EXTENSIONS with identical semantics.

# gl_USE_SYSTEM_EXTENSIONS
# ------------------------
# Enable extensions on systems that normally disable them,
# typically due to standards-conformance issues.
AC_DEFUN([gl_USE_SYSTEM_EXTENSIONS], [
  AC_BEFORE([$0], [AC_COMPILE_IFELSE])
  AC_BEFORE([$0], [AC_RUN_IFELSE])

  AC_REQUIRE([AC_GNU_SOURCE])
  AC_REQUIRE([AC_AIX])
  AC_REQUIRE([AC_MINIX])

  AH_VERBATIM([__EXTENSIONS__],
[/* Enable extensions on Solaris.  */
#ifndef __EXTENSIONS__
# undef __EXTENSIONS__
#endif
#ifndef _POSIX_PTHREAD_SEMANTICS
# undef _POSIX_PTHREAD_SEMANTICS
#endif])
  AC_CACHE_CHECK([whether it is safe to define __EXTENSIONS__],
    [ac_cv_safe_to_define___extensions__],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([
	  #define __EXTENSIONS__ 1
	  AC_INCLUDES_DEFAULT])],
       [ac_cv_safe_to_define___extensions__=yes],
       [ac_cv_safe_to_define___extensions__=no])])
  test $ac_cv_safe_to_define___extensions__ = yes &&
    AC_DEFINE([__EXTENSIONS__])
  AC_DEFINE([_POSIX_PTHREAD_SEMANTICS])
])
