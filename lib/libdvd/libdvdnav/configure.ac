dnl --------------------------------------------------------------
dnl Configure.ac for libdvdnav
dnl --------------------------------------------------------------

dnl --------------------------------------------------------------
dnl Require autoconf version 2.53
dnl --------------------------------------------------------------
AC_PREREQ([2.53])

dnl --------------------------------------------------------------
dnl Making releases: dvdnav_sub += 1; change DVDNAV_LT_* accordingly
dnl
dnl These are defined in m4 so they can be passed to AC_INIT
dnl --------------------------------------------------------------
m4_define([dvdnav_major], [4])
m4_define([dvdnav_minor], [2])
m4_define([dvdnav_sub],   [1])
m4_define([dvdnav_pre],   [])

AC_INIT([libdvdnav], [dvdnav_major.dvdnav_minor.dvdnav_sub[]dvdnav_pre])
AC_CONFIG_SRCDIR([src/dvdnav.c])
AM_INIT_AUTOMAKE([1.6])

dnl create a config.h file (Automake will add -DHAVE_CONFIG_H)
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_HOST

DVDNAV_MAJOR="dvdnav_major"
DVDNAV_MINOR="dvdnav_minor"
DVDNAV_SUB="dvdnav_sub"
DVDNAV_PRE="dvdnav_pre"

AC_SUBST(DVDNAV_MAJOR)
AC_SUBST(DVDNAV_MINOR)
AC_SUBST(DVDNAV_SUB)

dnl The libtool version numbers (DVDNAV_LT_*); Don't even think about faking this!
dnl 
dnl immediately before every release do:
dnl ===================================
dnl if (the interface is totally unchanged from previous release)
dnl    DVDNAV_LT_REVISION ++;
dnl else { /* interfaces have been added, removed or changed */
dnl    DVDNAV_LT_REVISION = 0;
dnl    DVDNAV_LT_CURRENT ++;
dnl    if (any interfaces have been _added_ since last release)
dnl       AGE ++;
dnl    if (any interfaces have been _removed_ or _incompatibly changed_)
dnl       AGE = 0;
dnl }
dnl 
dnl If you want to know more about what you are doing, here are some details:
dnl  * DVDNAV_LT_CURRENT is the current API version
dnl  * DVDNAV_LT_REVISION is an internal revision number which is increased when the API
dnl    itself did not change
dnl  * DVDNAV_LT_AGE is the number of previous API versions still supported by this library
dnl  * libtool has its own numbering scheme, because local library numbering schemes
dnl    are platform dependent
dnl  * in Linux, the library will be named
dnl    libname.so.(DVDNAV_LT_CURRENT - DVDNAV_LT_AGE).DVDNAV_LT_AGE.DVDNAV_LT_REVISION

DVDNAV_LT_CURRENT=5
DVDNAV_LT_AGE=1
DVDNAV_LT_REVISION=2

AC_SUBST(DVDNAV_LT_CURRENT)
AC_SUBST(DVDNAV_LT_AGE)
AC_SUBST(DVDNAV_LT_REVISION)

dnl --------------------------------------------------------------
dnl Make possible to build for another arch.
dnl --------------------------------------------------------------
if test x$DVDNAV_BUILD != "x"; then
  AC_MSG_RESULT(*** build forced to $DVDNAV_BUILD ***)
  build=$DVDNAV_BUILD
  host=$DVDNAV_BUILD
else
  check_athlon=yes
fi

dnl --------------------------------------------------------------
dnl Checks for programs.
dnl --------------------------------------------------------------
dnl Save CFLAGS, AC_ISC_POSIX set some unwanted default CFLAGS
saved_CFLAGS="$CFLAGS"
AC_ISC_POSIX
CFLAGS="$saved_CFLAGS"
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_LN_S

dnl --------------------------------------------------------------
dnl Libtool
dnl --------------------------------------------------------------
dnl LT_PREREQ only available in libtool-2.2+
dnl LT_PREREQ([1.4.0])
AC_LIBTOOL_DLOPEN
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)
if ${CONFIG_SHELL} ./libtool --features | grep "enable static" >/dev/null; then
  STATIC="-static"
else
  STATIC=
fi
AC_SUBST(STATIC)

dnl --------------------------------------------------------------
dnl Checks for header files.
dnl --------------------------------------------------------------
AC_HEADER_STDC
AC_CHECK_HEADER(unistd.h)
AC_CHECK_HEADER(string.h)

dnl --------------------------------------------------------------
dnl Checks for typedefs, structures, and compiler characteristics.
dnl --------------------------------------------------------------
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
dnl AC_CHECK_TYPES([ptrdiff_t])
AC_C_BIGENDIAN

dnl -------------------------------------------------------------
dnl Check for basic *nix fonction that we may emulate on windows.
dnl -------------------------------------------------------------
case $host in
  *mingw32* | *cygwin*)
    AC_CHECK_FUNCS(gettimeofday)
    ;;
  *)
    ;;
esac

dnl ---------------------------------------------
dnl threads
dnl ---------------------------------------------
case $host in
  *-*-freebsd*)
    THREAD_LIBS="-L/usr/local/lib -pthread"
    THREAD_CFLAGS="-I/usr/local/include -D_THREAD_SAFE"
    CFLAGS="$THREAD_CFLAGS $CFLAGS"
    ;;
  *mingw32* | *cygwin*)
    ;;
  *)
    AC_CHECK_LIB(pthread, pthread_create,
	[THREAD_LIBS="-lpthread"],
	AC_MSG_ERROR(pthread needed))
    ;;
esac
AC_SUBST(THREAD_LIBS)
AC_SUBST(THREAD_CFLAGS)

dnl ---------------------------------------------
dnl dynamic linker
dnl ---------------------------------------------
case $host in
  *mingw32*)
    CFLAGS="-idirafter \$(top_srcdir)/msvc/include $CFLAGS"
    LDFLAGS="-no-undefined $LDFLAGS"
    ;;
  *cygwin*)
    LDFLAGS="-no-undefined $LDFLAGS"
    ;;
  *os2*)
    LDFLAGS="-no-undefined -Zbin-files $LDFLAGS"
    ;;
  *)
    AC_CHECK_LIB(c, dlopen,
	         DYNAMIC_LD_LIBS="",
	         AC_CHECK_LIB(dl, dlopen,
             	              DYNAMIC_LD_LIBS="-ldl",
	                      AC_MSG_ERROR(dynamic linker needed)))
    AC_SUBST(DYNAMIC_LD_LIBS)
    ;;
esac

dnl ---------------------------------------------
dnl libdvdread
dnl ---------------------------------------------
AC_ARG_WITH([dvdread-config],
  [AS_HELP_STRING([--with-dvdread-config=PROG],
    [dvdread-config program to use @<:@default=from PATH@:>@])],
  [DVDREAD_CONFIG="$withval"],
  [dnl User didn't specify program, search PATH
  AC_PATH_PROG([DVDREAD_CONFIG], [dvdread-config], [no])
  test "x$DVDREAD_CONFIG" = xno && \
    AC_MSG_ERROR([dvdread-config required to link with libdvdread])
  ])
DVDREAD_CFLAGS=`$DVDREAD_CONFIG --cflags` || \
  AC_MSG_ERROR([Could not get libdvdread CFLAGS from $DVDREAD_CONFIG])
DVDREAD_LIBS=`$DVDREAD_CONFIG --libs` || \
  AC_MSG_ERROR([Could not get libdvdread LIBS from $DVDREAD_CONFIG])
AC_SUBST([DVDREAD_CFLAGS])
AC_SUBST([DVDREAD_LIBS])

dnl ---------------------------------------------
dnl cflags
dnl ---------------------------------------------
dnl Common cflags for all platforms
CFLAGS="-O3 -Wall -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE $CFLAGS"
DEBUG_CFLAGS="-g -DDEBUG $CFLAGS"

AC_SUBST(DEBUG_CFLAGS)

dnl ---------------------------------------------
dnl Check for doxygen (dynamic documentation generator)
dnl ---------------------------------------------
AC_CHECK_PROG(DOXYGEN, doxygen, doxygen, no)

dnl ---------------------------------------------
dnl Some include paths ( !!! DO NOT REMOVE !!! )
dnl ---------------------------------------------
INCLUDES='-I$(top_srcdir) $(DVDNAV_CFLAGS)'
AC_SUBST(INCLUDES)

dnl ---------------------------------------------
dnl Output configuration files
dnl ---------------------------------------------
AC_OUTPUT([
Makefile 
src/Makefile
src/vm/Makefile
misc/Makefile
misc/dvdnav-config
misc/dvdnav.pc
misc/dvdnavmini.pc
misc/libdvdnav.spec
misc/relchk.sh
m4/Makefile
doc/Makefile
examples/Makefile
])
