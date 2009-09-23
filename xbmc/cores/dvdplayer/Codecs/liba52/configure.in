dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.52)
AC_INIT
AC_CONFIG_SRCDIR([src/a52dec.c])
AC_CONFIG_AUX_DIR(autotools)
AC_CONFIG_FILES([Makefile autotools/Makefile include/Makefile test/Makefile
    doc/Makefile src/Makefile liba52/Makefile libao/Makefile vc++/Makefile])
AM_INIT_AUTOMAKE([a52dec],[0.7.4])
AM_CONFIG_HEADER(include/config.h)
AM_MAINTAINER_MODE
AC_CANONICAL_HOST

dnl Checks for compiler
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL

if test x"$GCC" = x"yes"; then

    dnl GCC-specific flags - try to optimize them sometime
    dnl -Wall -Werror moved to the end to not disturb the configure script

    dnl -O3
    changequote(<<,>>)
    OPT_CFLAGS=`echo "$CFLAGS"|sed "s/-O[0-9]*//g"`
    changequote([,])
    OPT_CFLAGS="$OPT_CFLAGS -O3"
    AC_TRY_CFLAGS([$OPT_CFLAGS],[CFLAGS=$OPT_CFLAGS])

    dnl -fomit-frame-pointer
    OPT_CFLAGS="$CFLAGS -fomit-frame-pointer"
    AC_TRY_CFLAGS([$OPT_CFLAGS],[CFLAGS=$OPT_CFLAGS])

    dnl arch-specific flags
    case "$host" in
    i?86-* | k?-*)
	case "$host" in
	i386-*) OPT_CFLAGS="$CFLAGS -mcpu=i386";;
	i486-*) OPT_CFLAGS="$CFLAGS -mcpu=i486";;
	i586-*) OPT_CFLAGS="$CFLAGS -mcpu=pentium";;
	i686-*) OPT_CFLAGS="$CFLAGS -mcpu=pentiumpro";;
	k6-*)   OPT_CFLAGS="$CFLAGS -mcpu=k6";;
	esac
	AC_TRY_CFLAGS([$OPT_CFLAGS],[CFLAGS=$OPT_CFLAGS]);;
    sparc-* | sparc64-*)
	OPT_CFLAGS="$CFLAGS -mtune=ultrasparc"
	AC_TRY_CFLAGS([$OPT_CFLAGS],[CFLAGS=$OPT_CFLAGS]);;
    mips-sgi-irix6.*)	dnl do we need to be that specific ?
	OPT_CFLAGS="$CFLAGS -mabi=64"
	AC_TRY_CFLAGS([$OPT_CFLAGS],[CFLAGS=$OPT_CFLAGS]);;
    esac
elif test x"$CC" = x"tcc" -a x"`$CC -version 2>&1 | grep TenDRA`" != x""; then
    dnl TenDRA portability checking compiler
    TENDRA=yes
    CFLAGS="-Xp -Yansi -f`pwd`/include/tendra.h -DELIDE_CODE"
    enable_mlib=no
    enable_oss=no
    enable_solaris_audio=no
elif test x"$CC" = x"icc" -a x"`$CC -V 2>&1 | grep Intel`" != x""; then
    dnl Intel C++ compiler
    CFLAGS="-g -O3 -unroll -ip"
else
    dnl non-gcc flags - we probably need exact configuration triplets here.
    case "$host" in
    mips-sgi-irix6.*)
	OPT_CFLAGS="$CFLAGS -64"
	AC_TRY_CFLAGS([$OPT_CFLAGS],[CFLAGS=$OPT_CFLAGS]);;
    sparc-sun-solaris*)
	OPT_CFLAGS="$CFLAGS -xCC -fast -xO5"
	AC_TRY_CFLAGS([$OPT_CFLAGS],[CFLAGS=$OPT_CFLAGS]);;
    esac
fi

dnl Checks for libtool - this must be done after we set cflags
AC_DISABLE_SHARED
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

dnl Checks for libraries.

dnl Checks for header files.
INCLUDES='-I$(top_srcdir)/include -I$(top_builddir)/include'
AC_SUBST([INCLUDES])
AC_CHECK_HEADERS([sys/time.h sys/timeb.h io.h])
AC_CHECK_GENERATE_INTTYPES([include])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_ALWAYS_INLINE
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_C_BIGENDIAN

dnl Checks for library functions.
AC_CHECK_FUNCS([memalign gettimeofday ftime])
AC_TYPE_SIGNAL

builtin([include],[liba52/configure.incl])
builtin([include],[libao/configure.incl])

AC_ARG_ENABLE([warnings],
    [  --enable-warnings       treat warnings as errors])
if test x"$enable_warnings" = x"yes" -a x"$GCC" = x"yes"; then
    dnl compiler warnings
    OPT_CFLAGS="$CFLAGS -Wall -Werror"
    AC_TRY_CFLAGS([$OPT_CFLAGS],[CFLAGS=$OPT_CFLAGS])
elif test x"$TENDRA" = x"yes"; then
    dnl TenDRA portability checking compiler
    CFLAGS="$CFLAGS -DTenDRA_check"
fi

builtin([include],[src/configure.incl])

AC_C_ATTRIBUTE_ALIGNED

AC_OUTPUT
