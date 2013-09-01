# autobuild.m4 serial 7
dnl Copyright (C) 2004, 2006, 2007, 2008, 2009, 2010 Free Software Foundation,
dnl Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Simon Josefsson

# Usage: AB_INIT([MODE]).
AC_DEFUN([AB_INIT],
[
  AC_REQUIRE([AC_CANONICAL_BUILD])
  AC_REQUIRE([AC_CANONICAL_HOST])

  if test -z "$AB_PACKAGE"; then
    AB_PACKAGE=${PACKAGE_NAME:-$PACKAGE}
  fi
  AC_MSG_NOTICE([autobuild project... $AB_PACKAGE])

  if test -z "$AB_VERSION"; then
    AB_VERSION=${PACKAGE_VERSION:-$VERSION}
  fi
  AC_MSG_NOTICE([autobuild revision... $AB_VERSION])

  hostname=`hostname`
  if test "$hostname"; then
    AC_MSG_NOTICE([autobuild hostname... $hostname])
  fi

  ifelse([$1],[],,[AC_MSG_NOTICE([autobuild mode... $1])])

  date=`TZ=UTC0 date +%Y%m%dT%H%M%SZ`
  if test "$?" != 0; then
    date=`date`
  fi
  if test "$date"; then
    AC_MSG_NOTICE([autobuild timestamp... $date])
  fi
])
