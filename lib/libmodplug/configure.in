dnl Process this file with autoconf to produce a configure script.
AC_INIT
AC_PREREQ([2.60])
AC_CONFIG_SRCDIR([Makefile.am])

AM_INIT_AUTOMAKE(libmodplug, 0.8.7)
AC_CONFIG_HEADERS([src/config.h])

dnl Checks for programs.
dnl  I am disabling static libraries here because otherwise libtool insists on
dnl  compiling everything twice -- once with and once without -fPIC.  Pisses me
dnl  off.  Just do everything with -fPIC, damnit!  Compiling everything twice
dnl  probably wastes more cycles than not using -fPIC saves.
AC_DISABLE_STATIC
AM_DISABLE_STATIC
AC_PROG_CC
AC_PROG_CXX
AC_LANG([C++])
AC_C_BIGENDIAN     #this will cause problems when cross-compiling...
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AC_HEADER_STDC
AC_CHECK_HEADERS([inttypes.h stdint.h malloc.h])
AC_CHECK_FUNCS(setenv)

CXXFLAGS="$CXXFLAGS -fno-exceptions -Wall -ffast-math -D_REENTRANT"
# CXXFLAGS="-fno-exceptions -g -Wall -ffast-math -D_REENTRANT `glib-config --cflags`"

AC_CANONICAL_HOST
case "$host" in
	*mingw32* )
		MINGW32=yes
		;;
esac
AM_CONDITIONAL([MINGW32], [test "x$MINGW32" = "xyes"])

case ${target_os} in
	*sun* | *solaris*)
		CXXFLAGS="$CXXFLAGS -fpermissive"
		;;
esac

# portable types. requires autoconf 2.60
# `configure' will check if these are defined in system headers.
# if not, it will auto-detect and define them in `config.h'
AC_TYPE_INT8_T
AC_TYPE_UINT8_T
AC_TYPE_INT16_T
AC_TYPE_UINT16_T
AC_TYPE_INT32_T
AC_TYPE_UINT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT64_T


AC_CONFIG_FILES([Makefile
          src/Makefile
	  libmodplug.pc])
AC_OUTPUT
