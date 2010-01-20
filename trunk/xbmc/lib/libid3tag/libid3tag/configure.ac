dnl -*- m4 -*-
dnl
dnl libid3tag - ID3 tag manipulation library
dnl Copyright (C) 2000-2004 Underbit Technologies, Inc.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
dnl
AC_REVISION([$Id: configure.ac,v 1.12 2004/01/23 23:22:46 rob Exp $])dnl

dnl Process this file with autoconf to produce a configure script.

AC_INIT([ID3 Tag], [0.15.1b], [support@underbit.com], [libid3tag])
AC_PREREQ(2.53)

AC_CONFIG_SRCDIR([id3tag.h])

AM_INIT_AUTOMAKE

AM_CONFIG_HEADER([config.h])

dnl System type.

AC_CANONICAL_HOST

dnl Checks for programs.

AC_PROG_CC

if test "$GCC" = yes
then
    case "$host" in
	*-*-mingw*)
	    case "$build" in
		*-*-cygwin*)
		    CPPFLAGS="$CPPFLAGS -mno-cygwin"
		    LDFLAGS="$LDFLAGS -mno-cygwin"
		    ;;
	    esac
    esac

dnl    case "$host" in
dnl	*-*-cygwin* | *-*-mingw*)
dnl	    LDFLAGS="$LDFLAGS -no-undefined -mdll"
dnl	    ;;
dnl    esac
fi

dnl Support for libtool.

dnl AC_DISABLE_SHARED
dnl AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

AC_SUBST(LIBTOOL_DEPS)

dnl Compiler options.

arch=""
debug=""
optimize=""
profile=""

set -- $CFLAGS
CFLAGS=""

if test "$GCC" = yes
then
    CFLAGS="-Wall"
fi

while test $# -gt 0
do
    case "$1" in
	-Wall)
	    if test "$GCC" = yes
	    then
		:
	    else
		CFLAGS="$CFLAGS $1"
	    fi
	    shift
	    ;;
	-g)
	    debug="-g"
	    shift
	    ;;
	-mno-cygwin)
	    shift
	    ;;
	-m*)
	    arch="$arch $1"
	    shift
	    ;;
	-fomit-frame-pointer)
	    shift
	    ;;
	-O*|-f*)
	    optimize="$1"
	    shift
	    ;;
	*)
	    CFLAGS="$CFLAGS $1"
	    shift
	    ;;
    esac
done

dnl Checks for header files.

AC_HEADER_STDC
AC_CHECK_HEADERS(assert.h unistd.h)

AC_CHECK_HEADER(zlib.h, [], [
	AC_MSG_ERROR([zlib.h was not found
*** You must first install zlib (libz) before you can build this package.
*** If zlib is already installed, you may need to use the CPPFLAGS
*** environment variable to specify its installed location, e.g. -I<dir>.])
])

dnl Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST
AC_C_INLINE

dnl Checks for library functions.

AC_CHECK_FUNCS(ftruncate)

AC_CHECK_LIB(z, compress2, [], [
	AC_MSG_ERROR([libz was not found
*** You must first install zlib (libz) before you can build this package.
*** If zlib is already installed, you may need to use the LDFLAGS
*** environment variable to specify its installed location, e.g. -L<dir>.])
])

dnl handle --enable and --disable options

AC_CACHE_SAVE

AC_MSG_CHECKING([whether to enable profiling])
AC_ARG_ENABLE(profiling, AC_HELP_STRING([--enable-profiling],
			 [generate profiling code]),
[
    case "$enableval" in
	yes) profile="-pg" ;;
    esac
])
AC_MSG_RESULT(${enable_profiling-no})

AC_MSG_CHECKING([whether to enable debugging])
AC_ARG_ENABLE(debugging, AC_HELP_STRING([--enable-debugging],
			 [enable diagnostic debugging support])
AC_HELP_STRING([--disable-debugging],
	       [do not enable debugging and use more optimization]),
[
    case "$enableval" in
	yes)
	    AC_DEFINE(DEBUG, 1,
		[Define to enable diagnostic debugging support.])
	    optimize=""
	    ;;
	no)
	    if test -n "$profile"
	    then
    AC_MSG_ERROR(--enable-profiling and --disable-debugging are incompatible)
	    fi

	    AC_DEFINE(NDEBUG, 1,
		[Define to disable debugging assertions.])
	    debug=""
	    if test "$GCC" = yes
	    then
		optimize="$optimize -fomit-frame-pointer"
	    fi
	    ;;
    esac
])
AC_MSG_RESULT(${enable_debugging-default})
AM_CONDITIONAL(DEBUG, test ${enable_debugging-default} = yes)

dnl Create output files.

test -n "$arch"     && CFLAGS="$CFLAGS $arch"
test -n "$debug"    && CFLAGS="$CFLAGS $debug"
test -n "$optimize" && CFLAGS="$CFLAGS $optimize"
test -n "$profile"  && CFLAGS="$CFLAGS $profile" LDFLAGS="$LDFLAGS $profile"

dnl LTLIBOBJS=`echo "$LIBOBJS" | sed -e 's/\.o/.lo/g'`
dnl AC_SUBST(LTLIBOBJS)

AC_CONFIG_FILES([Makefile msvc++/Makefile  \
	libid3tag.list])
AC_OUTPUT
