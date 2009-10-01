dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.54)
AC_INIT([libmpeg2],[0.5.1],[])

AC_CONFIG_SRCDIR([src/mpeg2dec.c])
AC_CONFIG_AUX_DIR(.auto)
AC_CONFIG_FILES([Makefile include/Makefile test/Makefile
    doc/Makefile src/Makefile libmpeg2/Makefile libmpeg2/convert/Makefile
    libvo/Makefile vc++/Makefile
    libmpeg2/libmpeg2.pc libmpeg2/convert/libmpeg2convert.pc])
AC_CONFIG_HEADERS([include/config.h])
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([1.5 -Wall foreign])
AM_MAINTAINER_MODE

dnl Checks for compiler
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AM_PROG_AS

dnl Checks for headers. We do this before the CC-specific section because
dnl autoconf generates tests for generic headers before the first header test.
AC_CHECK_HEADERS([sys/time.h time.h sys/timeb.h io.h])

dnl CC-specific flags
AC_SUBST([OPT_CFLAGS])
AC_SUBST([ARCH_OPT_CFLAGS])
if test x"$CC" = x"checkergcc"; then
    enable_sdl=no
elif test x"$GCC" = x"yes"; then

    dnl GCC-specific flags

    dnl -Wall
    dnl -Werror moved to the end to not disturb the configure script
    TRY_CFLAGS="$OPT_CFLAGS -Wall -Wpointer-arith -Wcast-align -Wcast-qual -Wstrict-prototypes -Wshadow -Waggregate-return -Wmissing-prototypes -Wnested-externs -Wsign-compare"
    AC_TRY_CFLAGS([$TRY_CFLAGS $CFLAGS],[OPT_CFLAGS="$TRY_CFLAGS"])

    dnl -O3
    changequote(<<,>>)
    TRY_CFLAGS=`echo "$OPT_CFLAGS $CFLAGS"|sed "s/-O[0-9]*//g"`
    changequote([,])
    TRY_CFLAGS="$TRY_CFLAGS -O3"
    AC_TRY_CFLAGS([$TRY_CFLAGS],[OPT_CFLAGS="$TRY_CFLAGS"])

    AC_ARG_ENABLE([debug],
	[  --enable-debug          debug mode configuration])
    if test x"$enable_debug" = x"yes"; then
	AC_DEFINE([DEBUG],,[debug mode configuration])
    else
	dnl -fomit-frame-pointer
	TRY_CFLAGS="$OPT_CFLAGS -fomit-frame-pointer"
	AC_TRY_CFLAGS([$TRY_CFLAGS $CFLAGS],[OPT_CFLAGS="$TRY_CFLAGS"])
    fi

    dnl -fno-common
    TRY_CFLAGS="$OPT_CFLAGS -fno-common"
    AC_TRY_CFLAGS([$TRY_CFLAGS $CFLAGS],[OPT_CFLAGS="$TRY_CFLAGS"])

    dnl arch-specific flags
    arm_conditional=false
    case "$host" in
    i?86-* | k?-* | x86_64-* | amd64-*)
	AC_DEFINE([ARCH_X86],,[x86 architecture])
	case "$host" in
	i386-*) TRY_CFLAGS="$OPT_CFLAGS -mcpu=i386";;
	i486-*) TRY_CFLAGS="$OPT_CFLAGS -mcpu=i486";;
	i586-*) TRY_CFLAGS="$OPT_CFLAGS -mcpu=pentium";;
	i686-*) TRY_CFLAGS="$OPT_CFLAGS -mcpu=pentiumpro";;
	k6-*)   TRY_CFLAGS="$OPT_CFLAGS -mcpu=k6";;
	esac
	AC_TRY_CFLAGS([$TRY_CFLAGS $CFLAGS],[OPT_CFLAGS="$TRY_CFLAGS"]);;
    ppc-* | powerpc-*)
	have_altivec=no
	for TRY_CFLAGS in "-mpim-altivec -force_cpusubtype_ALL" -faltivec -maltivec -fvec; do
	    AC_TRY_CFLAGS([$OPT_CFLAGS $TRY_CFLAGS $CFLAGS],
		[save_CFLAGS="$CFLAGS"
		 CFLAGS="$OPT_CFLAGS $TRY_CFLAGS $CFLAGS"
		 AC_MSG_CHECKING([if <altivec.h> is needed])
		 AC_TRY_COMPILE([],
		    [typedef vector int t;
		     vec_ld(0, (unsigned char *)0);],
		    [have_altivec=yes; AC_MSG_RESULT(no)],
		    [AC_TRY_COMPILE([#include <altivec.h>],
			[typedef vector int t; vec_ld(0, (unsigned char *)0);],
			[AC_DEFINE([HAVE_ALTIVEC_H],,
			    [Define to 1 if you have the <altivec.h> header.])
			 have_altivec=yes; AC_MSG_RESULT(yes)],
			[AC_MSG_RESULT(unsupported)])])
		 CFLAGS="$save_CFLAGS"])
	    if test "$have_altivec" = "yes"; then
		ARCH_OPT_CFLAGS="$TRY_CFLAGS"
		AC_DEFINE([ARCH_PPC],,[ppc architecture])
		break
	    fi
	done;;
    sparc-* | sparc64-*)
	AC_DEFINE([ARCH_SPARC],,[sparc architecture])
	TRY_CFLAGS="$OPT_CFLAGS -mcpu=ultrasparc -mvis"
	AC_TRY_CFLAGS([$TRY_CFLAGS $CFLAGS],[OPT_CFLAGS="$TRY_CFLAGS"]);;
    alpha*)
	AC_DEFINE([ARCH_ALPHA],,[alpha architecture]);;
    arm*)
	arm_conditional=:
	AC_DEFINE([ARCH_ARM],,[ARM architecture]);;
    esac
elif test x"$CC" = x"tendracc"; then
    dnl TenDRA portability checking compiler
    TENDRA=yes
    OPT_CFLAGS="-Xp -Yansi -f`pwd`/include/tendra.h -DELIDE_CODE"
    no_x=yes
    enable_sdl=no
elif test x"$CC" = x"icc" -a x"`$CC -V 2>&1 | grep Intel`" != x""; then
    dnl Intel C++ compiler
    OPT_CFLAGS="-g -O3 -unroll -ip"
else
    dnl non-gcc flags - we probably need exact configuration triplets here.
    case "$host" in
    sparc-sun-solaris*)
	TRY_CFLAGS="$OPT_CFLAGS -xCC -fast -xO5"
	AC_TRY_CFLAGS([$TRY_CFLAGS $CFLAGS],[OPT_CFLAGS="$TRY_CFLAGS"]);;
    esac
fi

AM_CONDITIONAL(ARCH_ARM, ${arm_conditional})

dnl Checks for libtool - this must be done after we set cflags
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

dnl Checks for libraries.

dnl Checks for header files.
AM_CPPFLAGS='-I$(top_srcdir)/include -I$(top_builddir)/include'
AC_SUBST([AM_CPPFLAGS])
AC_HEADER_TIME
AC_CHECK_TYPES([struct timeval], [], [],
    [#include <sys/time.h>
    #include <time.h>])
AC_CHECK_GENERATE_INTTYPES([include])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_ALWAYS_INLINE
AC_C_RESTRICT
AC_C_BUILTIN_EXPECT
AC_C_BIGENDIAN
AC_C_VOLATILE
AC_TYPE_SIZE_T
AC_TYPE_SIGNAL
AC_SYS_LARGEFILE

dnl Checks for library functions.
AC_CHECK_FUNCS([gettimeofday ftime])

case "$target" in
  dnl avoid -fPIC on 32-bit x86 platforms
  i?86-*|k?-*)
    AC_LIBTOOL_NON_PIC([LIBMPEG2_CFLAGS="$LIBMPEG2_CFLAGS -prefer-non-pic"])
  ;;
esac
AC_SUBST([LIBMPEG2_CFLAGS])

dnl check for cpudetect
AC_ARG_ENABLE([accel-detect],
    [  --disable-accel-detect  make a version without accel detection code])
if test x"$enable_accel_detect" != x"no"; then
    AC_DEFINE([ACCEL_DETECT],,[autodetect accelerations])
fi

dnl check for X11
AC_PATH_XTRA
if test x"$no_x" != x"yes"; then
    dnl check for Xshm
    AC_CHECK_LIB([Xext],[XShmCreateImage],
        [AC_DEFINE([LIBVO_X11],,[libvo X11 support])
        LIBVO_CFLAGS="$LIBVO_CFLAGS $X_CFLAGS"
        LIBVO_LIBS="$LIBVO_LIBS $X_PRE_LIBS $X_LIBS $X_EXTRA_LIBS -lXext -lX11"
        dnl check for Xv
        AC_CHECK_LIB([Xv],[XvShmCreateImage],
            [AC_DEFINE([LIBVO_XV],,[libvo Xv support])
            LIBVO_LIBS="$LIBVO_LIBS -lXv"],,
            [$X_PRE_LIBS $X_LIBS $X_EXTRA_LIBS -lXext -lX11])],,
        [$X_PRE_LIBS $X_LIBS $X_EXTRA_LIBS -lX11])
fi

dnl check for DirectX
AC_ARG_ENABLE([directx],
    [  --enable-directx=DIR    use Win32 DirectX headers in DIR])
case $enable_directx in
    ""|yes)     AC_CHECK_HEADER([ddraw.h],
                    [AC_DEFINE([LIBVO_DX],,[libvo DirectX support])
                    LIBVO_LIBS="$LIBVO_LIBS -lgdi32"]);;
    no)         ;;
    *)          if test -f "$enable_directx/ddraw.h"; then
                    AC_DEFINE([LIBVO_DX],,[libvo DirectX support])
                    LIBVO_CFLAGS="$LIBVO_CFLAGS -I$enable_directx"
                    LIBVO_LIBS="$LIBVO_LIBS -lgdi32"
                else
                    AC_MSG_ERROR([Cannot find $enable_directx/ddraw.h])
                fi;;
esac

dnl check for SDL
AC_ARG_ENABLE([sdl],[  --disable-sdl           make a version not using SDL])
if test x"$enable_sdl" != x"no"; then
    AC_CHECK_PROG([SDLCONFIG],[sdl-config],[yes])
    if test x"$SDLCONFIG" = x"yes"; then
        AC_DEFINE([LIBVO_SDL],,[libvo SDL support])
        LIBVO_CFLAGS="$LIBVO_CFLAGS `sdl-config --cflags`"
        LIBVO_LIBS="$LIBVO_LIBS `sdl-config --libs`"
    fi
fi

AC_SUBST([LIBVO_CFLAGS])
AC_SUBST([LIBVO_LIBS])

AC_ARG_ENABLE([warnings],
    [  --enable-warnings       treat warnings as errors])
if test x"$enable_warnings" = x"yes" -a x"$GCC" = x"yes"; then
    dnl compiler warnings
    TRY_CFLAGS="$OPT_CFLAGS -Werror"
    AC_TRY_CFLAGS([$TRY_CFLAGS $CFLAGS],[OPT_CFLAGS="$TRY_CFLAGS"])
elif test x"$TENDRA" = x"yes"; then
    dnl TenDRA portability checking compiler
    OPT_CFLAGS="$OPT_CFLAGS -DTenDRA_check"
fi

MPEG2DEC_CFLAGS="$OPT_CFLAGS"

AC_ARG_ENABLE([gprof],[  --enable-gprof          make a version using gprof])
if test x"$enable_gprof" = x"yes"; then
    AC_DEFINE([MPEG2DEC_GPROF],,[mpeg2dec profiling])
    MPEG2DEC_CFLAGS=`echo "$MPEG2DEC_CFLAGS"|sed "s/-fomit-frame-pointer//g"`
    MPEG2DEC_CFLAGS="$MPEG2DEC_CFLAGS -p"
fi

AC_SUBST([MPEG2DEC_CFLAGS])

AC_C_ATTRIBUTE_ALIGNED

AC_OUTPUT
