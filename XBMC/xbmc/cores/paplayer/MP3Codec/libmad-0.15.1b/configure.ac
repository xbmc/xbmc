dnl -*- m4 -*-
dnl
dnl libmad - MPEG audio decoder library
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
AC_REVISION([$Id: configure.ac,v 1.9 2004/01/23 09:41:32 rob Exp $])dnl

dnl Process this file with autoconf to produce a configure script.

AC_INIT([MPEG Audio Decoder], [0.15.1b], [support@underbit.com], [libmad])
AC_PREREQ(2.53)

AC_CONFIG_SRCDIR([decoder.h])

AM_INIT_AUTOMAKE

AM_CONFIG_HEADER([config.h])

dnl System type.

AC_CANONICAL_HOST

dnl Checks for programs.

AC_PROG_CC
AM_PROG_AS

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
	-O2)
	    optimize="-O"
	    shift
	    ;;
	-fomit-frame-pointer)
	    shift
	    ;;
	-O*|-f*)
	    optimize="$optimize $1"
	    shift
	    ;;
	*)
	    CFLAGS="$CFLAGS $1"
	    shift
	    ;;
    esac
done

if test "$GCC" = yes
then
    if test -z "$arch"
    then
	case "$host" in
	    i386-*)           ;;
	    i?86-*)           arch="-march=i486" ;;
	    arm*-empeg-*)     arch="-march=armv4 -mtune=strongarm1100" ;;
	    armv4*-*)         arch="-march=armv4 -mtune=strongarm" ;;
	    powerpc-*)        ;;
	    mips*-agenda-*)   arch="-mcpu=vr4100" ;;
	    mips*-luxsonor-*) arch="-mips1 -mcpu=r3000 -Wa,-m4010" ;;
	esac
    fi

    case "$optimize" in
	-O|"-O "*)
	    optimize="-O"
	    optimize="$optimize -fforce-mem"
	    optimize="$optimize -fforce-addr"
	    : #x optimize="$optimize -finline-functions"
	    : #- optimize="$optimize -fstrength-reduce"
	    optimize="$optimize -fthread-jumps"
	    optimize="$optimize -fcse-follow-jumps"
	    optimize="$optimize -fcse-skip-blocks"
	    : #x optimize="$optimize -frerun-cse-after-loop"
	    : #x optimize="$optimize -frerun-loop-opt"
	    : #x optimize="$optimize -fgcse"
	    optimize="$optimize -fexpensive-optimizations"
	    optimize="$optimize -fregmove"
	    : #* optimize="$optimize -fdelayed-branch"
	    : #x optimize="$optimize -fschedule-insns"
	    optimize="$optimize -fschedule-insns2"
	    : #? optimize="$optimize -ffunction-sections"
	    : #? optimize="$optimize -fcaller-saves"
	    : #> optimize="$optimize -funroll-loops"
	    : #> optimize="$optimize -funroll-all-loops"
	    : #x optimize="$optimize -fmove-all-movables"
	    : #x optimize="$optimize -freduce-all-givs"
	    : #? optimize="$optimize -fstrict-aliasing"
	    : #* optimize="$optimize -fstructure-noalias"

	    case "$host" in
		arm*-*)
		    optimize="$optimize -fstrength-reduce"
		    ;;
		mips*-*)
		    optimize="$optimize -fstrength-reduce"
		    optimize="$optimize -finline-functions"
		    ;;
		i?86-*)
		    optimize="$optimize -fstrength-reduce"
		    ;;
		powerpc-apple-*)
		    # this triggers an internal compiler error with gcc2
		    : #optimize="$optimize -fstrength-reduce"

		    # this is really only beneficial with gcc3
		    : #optimize="$optimize -finline-functions"
		    ;;
		*)
		    # this sometimes provokes bugs in gcc 2.95.2
		    : #optimize="$optimize -fstrength-reduce"
		    ;;
	    esac
	    ;;
    esac
fi

case "$host" in
    mips*-agenda-*)
	AC_DEFINE(HAVE_MADD16_ASM, 1,
	    [Define if your MIPS CPU supports a 2-operand MADD16 instruction.])
	;;
    mips*-luxsonor-*)
	AC_DEFINE(HAVE_MADD_ASM, 1,
	    [Define if your MIPS CPU supports a 2-operand MADD instruction.])
	;;
esac

dnl Checks for header files.

AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(assert.h limits.h unistd.h sys/types.h fcntl.h errno.h)

dnl Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN
AC_TYPE_PID_T

AC_CHECK_SIZEOF(int, 2)
AC_CHECK_SIZEOF(long, 4)
AC_CHECK_SIZEOF(long long, 8)

dnl Checks for library functions.

AC_CHECK_FUNCS(waitpid fcntl pipe fork)

dnl Other options.

AC_SUBST(FPM)
AC_SUBST(ASO)
AC_SUBST(ASO_OBJS)

dnl handle --enable and --disable options

AC_CACHE_SAVE

AC_MSG_CHECKING(whether to optimize for speed or for accuracy)

AC_ARG_ENABLE(speed, AC_HELP_STRING([--enable-speed],
		     [optimize for speed over accuracy]),
[
    case "$enableval" in
	yes)
	    optimize_for="speed"
	    AC_DEFINE(OPT_SPEED, 1,
		[Define to optimize for speed over accuracy.])
	    ;;
    esac
])

AC_ARG_ENABLE(accuracy, AC_HELP_STRING([--enable-accuracy],
			[optimize for accuracy over speed]),
[
    case "$enableval" in
	yes)
	    if test "$optimize_for" = "speed"
	    then
		optimize_for="both"
	    else
		optimize_for="accuracy"
	    fi
	    AC_DEFINE(OPT_ACCURACY, 1,
		[Define to optimize for accuracy over speed.])
	    ;;
    esac
])

AC_MSG_RESULT(${optimize_for-default})

if test "$optimize_for" = "both"
then
    AC_MSG_ERROR(cannot optimize for both speed and accuracy)
fi

AC_MSG_CHECKING(for architecture-specific fixed-point math routines)
AC_ARG_ENABLE(fpm, AC_HELP_STRING([--enable-fpm=ARCH],
		   [use ARCH-specific fixed-point math routines
		    (one of: intel, arm, mips, sparc, ppc, 64bit, default)]),
[
    case "$enableval" in
	yes)                             ;;
	no|default|approx) FPM="DEFAULT" ;;
	intel|i?86)        FPM="INTEL"   ;;
	arm)               FPM="ARM"     ;;
	mips)              FPM="MIPS"    ;;
	sparc)             FPM="SPARC"   ;;
	ppc|powerpc)       FPM="PPC"     ;;
	64bit)             FPM="64BIT"   ;;
	float)             FPM="FLOAT"   ;;
	*)
	    AC_MSG_RESULT(failed)
	    AC_MSG_ERROR([bad --enable-fpm option])
	    ;;
    esac
])

if test -z "$FPM" && test "$GCC" = yes
then
    case "$host" in
	i?86-*)     FPM="INTEL"  ;;
	arm*-*)     FPM="ARM"    ;;
	mips*-*)    FPM="MIPS"   ;;
	sparc*-*)   FPM="SPARC"  ;;
	powerpc*-*) FPM="PPC"    ;;
	# FIXME: need to test for 64-bit long long...
    esac
fi

AC_MSG_RESULT(${FPM=DEFAULT})

if test "$FPM" = "DEFAULT"
then
    AC_MSG_WARN([default fixed-point math will yield limited accuracy])
fi

FPM="-DFPM_$FPM"

AC_ARG_ENABLE(sso, AC_HELP_STRING([--enable-sso],
		   [use subband synthesis optimization]),
[
    case "$enableval" in
	yes)
	    AC_DEFINE(OPT_SSO, 1,
    [Define to enable a fast subband synthesis approximation optimization.])
	    ;;
    esac
])

AC_ARG_ENABLE(aso, AC_HELP_STRING([--disable-aso],
		   [disable architecture-specific optimizations]),
    [], [enable_aso=yes])

if test "$enable_aso" = yes
then
    case "$host" in
	i?86-*)
	    : #ASO="$ASO -DASO_INTERLEAVE1"
	    ASO="$ASO -DASO_ZEROCHECK"
	    : #not yet #ASO="$ASO -DASO_SYNTH"
	    : #not yet #ASO_OBJS="synth_mmx.lo"
	    ;;
	arm*-*)
	    ASO="$ASO -DASO_INTERLEAVE1"
	    ASO="$ASO -DASO_IMDCT"
	    ASO_OBJS="imdct_l_arm.lo"
	    ;;
	mips*-*)
	    ASO="$ASO -DASO_INTERLEAVE2"
	    ASO="$ASO -DASO_ZEROCHECK"
	    ;;
    esac
fi

AC_MSG_CHECKING(for ISO/IEC interpretation)
AC_ARG_ENABLE(strict-iso, AC_HELP_STRING([--enable-strict-iso],
			  [use strict ISO/IEC interpretations]),
[
    case "$enableval" in
	yes)
	    AC_DEFINE(OPT_STRICT, 1,
    [Define to influence a strict interpretation of the ISO/IEC standards,
     even if this is in opposition with best accepted practices.])
	    interpretation="strict"
	    ;;
    esac
])
AC_MSG_RESULT(${interpretation-best accepted practices})

AC_MSG_CHECKING(whether to enable profiling)
AC_ARG_ENABLE(profiling, AC_HELP_STRING([--enable-profiling],
			 [generate profiling code]),
[
    case "$enableval" in
	yes) profile="-pg" ;;
    esac
])
AC_MSG_RESULT(${enable_profiling-no})

AC_MSG_CHECKING(whether to enable debugging)
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

AC_MSG_CHECKING(whether to enable experimental code)
AC_ARG_ENABLE(experimental, AC_HELP_STRING([--enable-experimental],
			    [enable experimental code]),
[
    case "$enableval" in
	yes)
	    AC_DEFINE(EXPERIMENTAL, 1,
		[Define to enable experimental code.])
	    ;;
    esac
])
AC_MSG_RESULT(${enable_experimental-no})

dnl Create output files.

test -n "$arch"     && CFLAGS="$CFLAGS $arch"
test -n "$debug"    && CFLAGS="$CFLAGS $debug"
test -n "$optimize" && CFLAGS="$CFLAGS $optimize"
test -n "$profile"  && CFLAGS="$CFLAGS $profile" LDFLAGS="$LDFLAGS $profile"

dnl LTLIBOBJS=`echo "$LIBOBJS" | sed -e 's/\.o/.lo/g'`
dnl AC_SUBST(LTLIBOBJS)

AC_CONFIG_FILES([Makefile msvc++/Makefile  \
	libmad.list])
AC_OUTPUT
