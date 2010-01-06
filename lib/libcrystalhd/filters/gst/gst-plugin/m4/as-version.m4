dnl as-version.m4 0.1.0

dnl autostars m4 macro for versioning

dnl Thomas Vander Stichele <thomas at apestaart dot org>

dnl $Id: as-version.m4,v 1.2 2004-09-17 22:18:03 leroutier Exp $

dnl AS_VERSION(PACKAGE, PREFIX, MAJOR, MINOR, MICRO, NANO,
dnl            ACTION-IF-NO-NANO, [ACTION-IF-NANO])

dnl example
dnl AS_VERSION(gstreamer, GST_VERSION, 0, 3, 2,)
dnl for a 0.3.2 release version

dnl this macro
dnl - defines [$PREFIX]_MAJOR, MINOR and MICRO
dnl - if NANO is empty, then we're in release mode, else in cvs/dev mode
dnl - defines [$PREFIX], VERSION, and [$PREFIX]_RELEASE
dnl - executes the relevant action
dnl - AC_SUBST's PACKAGE, VERSION, [$PREFIX] and [$PREFIX]_RELEASE
dnl   as well as the little ones
dnl - doesn't call AM_INIT_AUTOMAKE anymore because it prevents
dnl   maintainer mode from running ok
dnl
dnl don't forget to put #undef [$2] and [$2]_RELEASE in acconfig.h
dnl if you use acconfig.h

AC_DEFUN([AS_VERSION],
[
  PACKAGE=[$1]
  [$2]_MAJOR=[$3]
  [$2]_MINOR=[$4]
  [$2]_MICRO=[$5]
  NANO=[$6]
  [$2]_NANO=$NANO
  if test "x$NANO" = "x" || test "x$NANO" = "x0";
  then
      AC_MSG_NOTICE(configuring [$1] for release)
      VERSION=[$3].[$4].[$5]
      [$2]_RELEASE=1
      dnl execute action
      ifelse([$7], , :, [$7])
  else
      AC_MSG_NOTICE(configuring [$1] for development with nano $NANO)
      VERSION=[$3].[$4].[$5].$NANO
      [$2]_RELEASE=0.`date +%Y%m%d.%H%M%S`
      dnl execute action
      ifelse([$8], , :, [$8])
  fi

  [$2]=$VERSION
  AC_DEFINE_UNQUOTED([$2], "$[$2]", [Define the version])
  AC_SUBST([$2])
  AC_DEFINE_UNQUOTED([$2]_RELEASE, "$[$2]_RELEASE", [Define the release version])
  AC_SUBST([$2]_RELEASE)

  AC_SUBST([$2]_MAJOR)
  AC_SUBST([$2]_MINOR)
  AC_SUBST([$2]_MICRO)
  AC_SUBST([$2]_NANO)
  AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Define the package name])
  AC_SUBST(PACKAGE)
  AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Define the version])
  AC_SUBST(VERSION)
])
