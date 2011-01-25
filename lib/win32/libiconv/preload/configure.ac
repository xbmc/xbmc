dnl Copyright (C) 1999-2009 Free Software Foundation, Inc.
dnl This file is part of the GNU LIBICONV Library.
dnl
dnl The GNU LIBICONV Library is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU Library General Public License as
dnl published by the Free Software Foundation; either version 2 of the
dnl License, or (at your option) any later version.
dnl
dnl The GNU LIBICONV Library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Library General Public License for more details.
dnl
dnl You should have received a copy of the GNU Library General Public License
dnl along with the GNU LIBICONV Library; see the file COPYING.LIB.  If not,
dnl write to the Free Software Foundation, Inc., 51 Franklin Street,
dnl Fifth Floor, Boston, MA 02110-1301, USA.

AC_PREREQ([2.60])

PACKAGE=libiconv
VERSION=0

AC_INIT([configure.ac])
AC_CONFIG_AUX_DIR([../build-aux])
AC_PROG_MAKE_SET
AC_SUBST([PACKAGE])
AC_SUBST([VERSION])

dnl           checks for basic programs

AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL

dnl           check for host type

AC_CANONICAL_HOST

dnl           checks for installer options

gl_RELOCATABLE_LIBRARY

dnl           checks for programs

AC_PROG_LN_S

dnl           checks for UNIX variants that set DEFS

AC_USE_SYSTEM_EXTENSIONS

dnl           checks for compiler output filename suffixes

AC_OBJEXT
AC_EXEEXT

dnl           check for build configuration

dnl Here we need to build shared object files, regardless whether the user
dnl wants to the libiconv library itself to be installed shared or static or
dnl both.
enable_shared=yes
enable_static=no
LT_INIT([win32-dll])
gl_VISIBILITY

dnl           check whether OS supports plug libraries

case "$host_os" in
  linux* | solaris* | osf*)
    PLUGLIB="preloadable_libiconv.so" ;;
  *)
    PLUGLIB=""
esac
AC_SUBST([PLUGLIB])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
