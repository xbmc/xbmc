# mbstate_t.m4 serial 12
dnl Copyright (C) 2000-2002, 2008, 2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# From Paul Eggert.

# BeOS 5 has <wchar.h> but does not define mbstate_t,
# so you can't declare an object of that type.
# Check for this incompatibility with Standard C.

# AC_TYPE_MBSTATE_T
# -----------------
AC_DEFUN([AC_TYPE_MBSTATE_T],
[
   AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS]) dnl for HP-UX 11.11

   AC_CACHE_CHECK([for mbstate_t], [ac_cv_type_mbstate_t],
     [AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
	   [AC_INCLUDES_DEFAULT[
#	    include <wchar.h>]],
	   [[mbstate_t x; return sizeof x;]])],
	[ac_cv_type_mbstate_t=yes],
	[ac_cv_type_mbstate_t=no])])
   if test $ac_cv_type_mbstate_t = yes; then
     AC_DEFINE([HAVE_MBSTATE_T], [1],
	       [Define to 1 if <wchar.h> declares mbstate_t.])
   else
     AC_DEFINE([mbstate_t], [int],
	       [Define to a type if <wchar.h> does not define.])
   fi
])
