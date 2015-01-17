# serial 5  -*- Autoconf -*-
# Enable extensions on systems that normally disable them.

# Copyright (C) 2003, 2006-2008 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This definition of AC_USE_SYSTEM_EXTENSIONS is stolen from CVS
# Autoconf.  Perhaps we can remove this once we can assume Autoconf
# 2.62 or later everywhere, but since CVS Autoconf mutates rapidly
# enough in this area it's likely we'll need to redefine
# AC_USE_SYSTEM_EXTENSIONS for quite some time.

# AC_USE_SYSTEM_EXTENSIONS
# ------------------------
# Enable extensions on systems that normally disable them,
# typically due to standards-conformance issues.
# Remember that #undef in AH_VERBATIM gets replaced with #define by
# AC_DEFINE.  The goal here is to define all known feature-enabling
# macros, then, if reports of conflicts are made, disable macros that
# cause problems on some platforms (such as __EXTENSIONS__).
AC_DEFUN([AC_USE_SYSTEM_EXTENSIONS],
[AC_BEFORE([$0], [AC_COMPILE_IFELSE])dnl
AC_BEFORE([$0], [AC_RUN_IFELSE])dnl

  AC_CHECK_HEADER([minix/config.h], [MINIX=yes], [MINIX=])
  if test "$MINIX" = yes; then
    AC_DEFINE([_POSIX_SOURCE], [1],
      [Define to 1 if you need to in order for `stat' and other
       things to work.])
    AC_DEFINE([_POSIX_1_SOURCE], [2],
      [Define to 2 if the system does not provide POSIX.1 features
       except with this defined.])
    AC_DEFINE([_MINIX], [1],
      [Define to 1 if on MINIX.])
  fi

  AH_VERBATIM([__EXTENSIONS__],
[/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# undef _ALL_SOURCE
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# undef _GNU_SOURCE
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# undef _POSIX_PTHREAD_SEMANTICS
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# undef _TANDEM_SOURCE
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# undef __EXTENSIONS__
#endif
])
  AC_CACHE_CHECK([whether it is safe to define __EXTENSIONS__],
    [ac_cv_safe_to_define___extensions__],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([[
#	  define __EXTENSIONS__ 1
	  ]AC_INCLUDES_DEFAULT])],
       [ac_cv_safe_to_define___extensions__=yes],
       [ac_cv_safe_to_define___extensions__=no])])
  test $ac_cv_safe_to_define___extensions__ = yes &&
    AC_DEFINE([__EXTENSIONS__])
  AC_DEFINE([_ALL_SOURCE])
  AC_DEFINE([_GNU_SOURCE])
  AC_DEFINE([_POSIX_PTHREAD_SEMANTICS])
  AC_DEFINE([_TANDEM_SOURCE])
])# AC_USE_SYSTEM_EXTENSIONS

# gl_USE_SYSTEM_EXTENSIONS
# ------------------------
# Enable extensions on systems that normally disable them,
# typically due to standards-conformance issues.
AC_DEFUN([gl_USE_SYSTEM_EXTENSIONS],
  [AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])])
