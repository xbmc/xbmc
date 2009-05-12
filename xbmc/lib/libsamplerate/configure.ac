# Copyright (C) 2002-2008 Erik de Castro Lopo (erikd AT mega-nerd DOT com).

dnl Require autoconf version >= 2.57)
AC_PREREQ(2.57)

AC_INIT([libsamplerate],[0.1.7],[erikd@mega-nerd.com])

# Put config stuff in Cfg.
AC_CONFIG_AUX_DIR(Cfg)

AC_CONFIG_SRCDIR([src/samplerate.c])
AC_CANONICAL_TARGET([])

AC_CONFIG_MACRO_DIR([M4])

AM_INIT_AUTOMAKE($PACKAGE_NAME,$PACKAGE_VERSION)
AC_CONFIG_HEADER(src/config.h)


dnl Add parameters for aclocal
AC_SUBST(ACLOCAL_AMFLAGS, "-I M4")

AC_LANG([C])

#------------------------------------------------------------------------------------
# Rules for library version information:
#
#  1. Start with version information of `0:0:0' for each libtool library.
#  2. Update the version information only immediately before a public release of
#     your software. More frequent updates are unnecessary, and only guarantee
#     that the current interface number gets larger faster.
#  3. If the library source code has changed at all since the last update, then
#     increment revision (`c:r:a' becomes `c:r+1:a').
#  4. If any interfaces have been added, removed, or changed since the last update,
#     increment current, and set revision to 0.
#  5. If any interfaces have been added since the last public release, then increment
#     age.
#  6. If any interfaces have been removed since the last public release, then set age
#     to 0.

SHARED_VERSION_INFO="1:7:1"

AC_PROG_CC
AM_PROG_CC_C_O

# Do not check for F77.
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])dnl

AM_PROG_LIBTOOL

AC_PROG_INSTALL

AC_PROG_MAKE_SET
AC_PROG_LN_S
AC_PROG_MKDIR_P

if test "x-$build_os-$host_os-$target_os" = x-mingw32-mingw32-mingw32 ; then
	AC_CHECK_PROG(DLLTOOL, dlltool, dlltool)
	AC_CHECK_PROG(DLLWRAP, dllwrap, dllwrap)
	AC_CHECK_PROG(PEXPORTS, pexports, pexports)
	fi

AC_HEADER_STDC

AC_CHECK_HEADERS(stdint.h sys/times.h)

#====================================================================================
# Couple of initializations here. Fill in real values later.

SHLIB_VERSION_ARG=""

#====================================================================================
# Finished checking, handle options.

AC_ARG_ENABLE(gcc-werror,
	AC_HELP_STRING([--enable-gcc-werror], [enable -Werror in all Makefiles]),
	ac_arg_gcc_werror=yes, ac_arg_gcc_werror=no)

AC_ARG_ENABLE(gcc-pipe,
		AC_HELP_STRING([--disable-gcc-pipe], [disable gcc -pipe option ]),
		ac_arg_gcc_pipe="N", ac_arg_gcc_pipe="Y")

AC_ARG_ENABLE(gcc-opt,
		AC_HELP_STRING([--disable-gcc-opt], [disable gcc optimisations ]),
		ac_arg_gcc_opt="N", ac_arg_gcc_opt="Y")

AC_ARG_ENABLE(fftw,
		AC_HELP_STRING([--disable-fftw], [disable usage of FFTW ]),
		ac_arg_fftw="N", ac_arg_fftw="Y")

AC_ARG_ENABLE(cpu-clip,
		AC_HELP_STRING([--disable-cpu-clip], [disable tricky cpu specific clipper]),
		ac_arg_cpu_clip="N", ac_arg_cpu_clip="Y")

#====================================================================================
# Check types and their sizes.

AC_CHECK_SIZEOF(int,0)
AC_CHECK_SIZEOF(long,0)
AC_CHECK_SIZEOF(float,4)
AC_CHECK_SIZEOF(double,8)

#====================================================================================
# Determine endian-ness of target processor.

AC_C_FIND_ENDIAN

AC_DEFINE_UNQUOTED(CPU_IS_BIG_ENDIAN, ${ac_cv_c_big_endian},
        [Target processor is big endian.])
AC_DEFINE_UNQUOTED(CPU_IS_LITTLE_ENDIAN, ${ac_cv_c_little_endian},
        [Target processor is little endian.])

#====================================================================================
# Check for functions.

AC_CHECK_FUNCS(malloc calloc free memcpy memmove alarm signal)

AC_CHECK_LIB([m],floor)
AC_CHECK_FUNCS(floor ceil fmod)

AC_CHECK_SIGNAL(SIGALRM)

AC_C99_FUNC_LRINT
AC_C99_FUNC_LRINTF
# AC_C99_FUNC_LLRINT  Don't need this (yet?).

case "x$ac_cv_c99_lrint$ac_cv_c99_lrintf" in
	xyesyes)
		;;
	*)
		AC_MSG_WARN([[*** Missing C99 standard functions lrint() and lrintf().]])
		AC_MSG_WARN([[*** This may cause benign compiler warnings on some systems (ie Solaris).]])
		;;
	esac

#====================================================================================
# Determine if the processor can do clipping on float to int conversions.

if test x$ac_arg_cpu_clip = "xY" ; then
	AC_C_CLIP_MODE
else
	echo "checking processor clipping capabilities... disabled"
	ac_cv_c_clip_positive=0
	ac_cv_c_clip_negative=0
	fi

AC_DEFINE_UNQUOTED(CPU_CLIPS_POSITIVE, ${ac_cv_c_clip_positive},
	[Target processor clips on positive float to int conversion.])
AC_DEFINE_UNQUOTED(CPU_CLIPS_NEGATIVE, ${ac_cv_c_clip_negative},
	[Target processor clips on negative float to int conversion.])

#====================================================================================
# Check for libsndfile which is required for the test and example programs.

AC_ARG_ENABLE(sndfile,
	AC_HELP_STRING([--disable-sndfile], [disable support for sndfile (default=autodetect)]),
	[ enable_sndfile=$enableval ], [ enable_sndfile=yes ])

# Check for pkg-config outside the if statement.
PKG_PROG_PKG_CONFIG

if test "x$enable_sndfile" = "xyes"; then
	PKG_CHECK_MODULES(SNDFILE, sndfile >= 1.0.6, ac_cv_sndfile=1, ac_cv_sndfile=0)
else
	ac_cv_sndfile=0
fi

AC_DEFINE_UNQUOTED([HAVE_SNDFILE],$ac_cv_sndfile,[Set to 1 if you have libsndfile.])

if test x$ac_cv_sndfile = x1 ; then
	ac_cv_sndfile=yes
	HAVE_SNDFILE=1
else
	ac_cv_sndfile=no
	HAVE_SNDFILE=0
	fi

#====================================================================================
# Check for libfftw3 which is required for the test and example programs.

if test $ac_arg_fftw = "Y" ; then
	PKG_CHECK_MODULES(FFTW3, fftw3 >= 0.15.0, ac_cv_fftw3=1, ac_cv_fftw3=0)
	AC_DEFINE_UNQUOTED([HAVE_FFTW3],$ac_cv_fftw3,[Set to 1 if you have libfftw3.])

	if test x$ac_cv_fftw3 = x1 ; then
		ac_cv_fftw3=yes
		HAVE_FFTW3=1
		fi
	fi

if test x$ac_cv_fftw3 != xyes ; then
	ac_cv_fftw3=no
	HAVE_FFTW3=0
	fi

#====================================================================================
# GCC stuff.

if test $ac_cv_c_compiler_gnu = yes ; then
	CFLAGS="$CFLAGS -std=gnu99 -W -Wstrict-prototypes -Wmissing-prototypes -Wall -Waggregate-return -Wcast-align -Wcast-qual -Wnested-externs -Wshadow -Wpointer-arith"
	# -Wundef -Wbad-function-cast -Wmissing-declarations -Wconversion -Winline"
	if test "$ac_arg_gcc_opt" = "N" ; then
		temp_CFLAGS=`echo $CFLAGS | sed "s/O2/O0/"`
		CFLAGS=$temp_CFLAGS
		AC_MSG_WARN([[*** Compiler optimisations switched off. ***]])
		fi

	# Disable -Wall for Apple Darwin/Rhapsody.
	# System headers on these systems are broken.
	case "$target_os" in
		darwin* | rhapsody*)
			temp_CFLAGS=`echo $CFLAGS | sed "s/-Wall//"`
			CFLAGS=$temp_CFLAGS
			;;
		linux*)
			SHLIB_VERSION_ARG="-Wl,--version-script=Version_script"
			;;
		*)
			;;
		esac
	if test x$ac_arg_gcc_pipe != "xN" ; then
	 	CFLAGS="$CFLAGS -pipe"
	 	fi

	if test x$ac_arg_gcc_werror = "xyes" ; then
		CFLAGS="-Werror $CFLAGS"
		fi

	AC_DEFINE([COMPILER_IS_GCC],1, [Set to 1 if the compile is GNU GCC.])
	GCC_MAJOR_VERSION=`$CC -dumpversion | sed "s/\..*//"`
	AC_DEFINE_UNQUOTED([GCC_MAJOR_VERSION],${GCC_MAJOR_VERSION}, [Major version of GCC or 3 otherwise.])
	fi

#====================================================================================
# Find known target OS.

OS_SPECIFIC_INCLUDES=""
os_is_win32=0

case "$target_os" in
	darwin* | rhapsody*)
		OS_SPECIFIC_INCLUDES="-fpascal-strings -I/Developer/Headers/FlatCarbon"
		OS_SPECIFIC_LINKS="-framework CoreAudio"
		;;
	mingw32*)
		OS_SPECIFIC_LINKS="-lwinmm"
		os_is_win32=1
		;;
	*)
		OS_SPECIFIC_INCLUDES=""
		OS_SPECIFIC_LINKS=""
		;;
	esac

#====================================================================================
# Now use the information from the checking stage.

AC_DEFINE_UNQUOTED(OS_IS_WIN32, ${os_is_win32}, [Set to 1 if compiling for Win32])

AC_SUBST(SHLIB_VERSION_ARG)
AC_SUBST(SHARED_VERSION_INFO)
AC_SUBST(OS_SPECIFIC_INCLUDES)
AC_SUBST(OS_SPECIFIC_LINKS)

AC_SUBST(COMPILER_IS_GCC)
AC_SUBST(GCC_MAJOR_VERSION)

AC_SUBST(HAVE_FFTW3)
AC_SUBST(FFTW3_CFLAGS)
AC_SUBST(FFTW3_LIBS)

AC_SUBST(HAVE_SNDFILE)
AC_SUBST(SNDFILE_CFLAGS)
AC_SUBST(SNDFILE_LIBS)

AC_CONFIG_FILES([Makefile src/Version_script Win32/Makefile Win32/Makefile.mingw \
			src/Makefile examples/Makefile tests/Makefile doc/Makefile \
			libsamplerate.spec samplerate.pc])
AC_OUTPUT

#====================================================================================

AC_MSG_RESULT([
-=-=-=-=-=-=-=-=-=-= Configuration Complete =-=-=-=-=-=-=-=-=-=-=-

  Configuration summary :

    Version : ..................... ${VERSION}
])

if test x$ac_cv_c_compiler_gnu = xyes ; then
	echo -e "  Tools :\n"
	echo "    Compiler is GCC : ............. ${ac_cv_c_compiler_gnu}"
	echo "    GCC major version : ........... ${GCC_MAJOR_VERSION}"
	fi

AC_MSG_RESULT([
  Extra tools required for testing and examples :

    Use FFTW : .................... ${ac_cv_fftw3}])

AC_MSG_RESULT([    Have libsndfile : ............. ${ac_cv_sndfile}
])

AC_MSG_RESULT([  Installation directories :

    Library directory : ........... ${prefix}/lib
    Program directory : ........... ${prefix}/bin
    Pkgconfig directory : ......... ${prefix}/lib/pkgconfig
])

if test x$prefix != "x/usr" ; then
	echo "Compiling some other packages against ${PACKAGE} may require "
	echo -e "the addition of \"${prefix}/lib/pkgconfig\" to the "
	echo -e "PKG_CONFIG_PATH environment variable.\n"
	fi

