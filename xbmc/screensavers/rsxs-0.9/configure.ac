## Process this file with autoconf to produce a configure script.

AC_INIT([Really Slick XScreenSavers],[0.9],[foonly@users.sourceforge.net],[rsxs])
AC_PREREQ(2.59)
AC_CONFIG_SRCDIR([src/hack.hh])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.7 subdir-objects std-options])
AC_SUBST(RELEASE, 1)

AC_CONFIG_MACRO_DIR(m4)
AC_CONFIG_LIBOBJ_DIR(lib)

AC_GNU_SOURCE

# Checks for programs.
LT_AC_PROG_SED
AC_LANG(C++)
AC_PROG_CXX
#CXXFLAGS=`echo $CXXFLAGS | sed -e s,-O2,-O3,`
AC_PROG_CXXCPP

#if test x"$GXX" = x"yes"; then
#	CXXFLAGS=`echo $CXXFLAGS -Wall`
#fi

AC_PROG_CC
AM_PROG_CC_C_O
gl_EARLY
AC_PROG_RANLIB

# Cut-down version of AC_LIB_LTDL. We don't want the option to install
# libltdl, or to use dlpreopen (we don't want libtool at all).
# Also, we can use gnulib's *_ONCE macros.
AC_DEFUN([MAC_LIB_LTDL], [
	# Some of the below macros have dependencies that are m4_require'd.
	# aclocal doesn't resolve those automatically, so we need to call
	# LT_PATH_LD and LTSUGAR_VERSION to pull in libtool.m4 and ltsugar.m4
	# where the required dependencies are m4_defun'd.
	AC_REQUIRE([LT_PATH_LD])dnl
	AC_REQUIRE([LTSUGAR_VERSION])dnl
	AC_REQUIRE([AC_PROG_CC])dnl
	AC_REQUIRE([AC_HEADER_STDC])dnl
	AC_REQUIRE([AC_HEADER_DIRENT])dnl
	AC_REQUIRE([_LT_AC_CHECK_DLFCN])dnl
	AC_REQUIRE([AC_LTDL_SHLIBEXT])dnl
	AC_REQUIRE([AC_LTDL_SHLIBPATH])dnl
	AC_REQUIRE([AC_LTDL_SYSSEARCHPATH])dnl
	AC_REQUIRE([AC_LTDL_OBJDIR])dnl
	AC_REQUIRE([AC_LTDL_DLLIB])dnl
	AC_REQUIRE([AC_LTDL_SYMBOL_USCORE])dnl
	AC_REQUIRE([AC_LTDL_DLSYM_USCORE])dnl
	AC_REQUIRE([AC_LTDL_SYS_DLOPEN_DEPLIBS])dnl

	AC_CHECK_HEADERS_ONCE([assert.h ctype.h errno.h malloc.h memory.h stdlib.h stdio.h unistd.h])
	AC_CHECK_HEADERS_ONCE([dl.h sys/dl.h dld.h mach-o/dyld.h])
	AC_CHECK_HEADERS_ONCE([string.h strings.h], [break])
	AC_CHECK_FUNCS_ONCE([strchr index], [break])
	AC_CHECK_FUNCS_ONCE([strrchr rindex], [break])
	AC_CHECK_FUNCS_ONCE([memcpy bcopy], [break])
	AC_CHECK_FUNCS_ONCE([memmove strcmp])
	AC_CHECK_FUNCS_ONCE([closedir opendir readdir])
])dnl
MAC_LIB_LTDL

MAC_CXX_BOOL
MAC_CXX_NEW_FOR_SCOPING
MAC_CXX_NAMESPACES
if test "$HAVE_CXX_NAMESPACES" != yes; then
	AC_MSG_ERROR([$CXX does not correctly support namespaces])
fi
MAC_CXX_STD
if test "$HAVE_CXX_STD" != yes; then
	AC_MSG_ERROR([the C++ Standard Template Library is unavailable or non-ISO])
fi
MAC_CXX_SSTREAM

AC_PROG_INSTALL
AC_PROG_LN_S

# Checks for libraries.
gl_INIT
MAC_PKG_X
AC_SUBST(X_CPPFLAGS)
AC_SUBST(X_LIBS)
MAC_PKG_XSCREENSAVER
AM_CONDITIONAL(HACKMODE, test $HAVE_PKG_XSCREENSAVER = yes)
AM_CONDITIONAL(XSCREENSAVER4, test $HAVE_PKG_XSCREENSAVER_4 = yes)
AM_CONDITIONAL(SYMLINKS, test $PKG_XSCREENSAVER_SYMLINKS = yes)
AC_SUBST(xscreensaver, $HAVE_PKG_XSCREENSAVER)
AC_SUBST(defaultdir, $PKG_XSCREENSAVER_DEFAULTDIR)
AC_SUBST(hackdir, $PKG_XSCREENSAVER_HACKDIR)
AC_SUBST(configdir, $PKG_XSCREENSAVER_CONFIGDIR)

MAC_PKG_OPENGL
if test "$HAVE_PKG_OPENGL" != yes; then
	AC_MSG_ERROR([can't compile without OpenGL support])
fi
AC_SUBST(OPENGL_LIBS)

HAVE_SOUND=no
USE_DLOPEN=no
SOUND_CPPFLAGS=
SOUND_CXXFLAGS=
SOUND_LDFLAGS=
SOUND_LIBS=
AC_ARG_ENABLE(sound,
	AS_HELP_STRING(--disable-sound,disable OGG sound file support))
m4_divert_once([HELP_ENABLE],
	AS_HELP_STRING(--enable-sound=dlopen,use OGG sound file support at runtime))dnl
if test "$enable_sound" != "no"; then
	MAC_PKG_OPENAL
	if test "$HAVE_PKG_OPENAL" = yes; then
		MAC_PKG_VORBISFILE
		if test "$HAVE_PKG_VORBISFILE" = yes; then
			HAVE_SOUND=yes
			SOUND_CPPFLAGS="$OPENAL_CPPFLAGS $VORBISFILE_CPPFLAGS"
			SOUND_CPPFLAGS="$OPENAL_CXXFLAGS $VORBISFILE_CXXFLAGS"
			if test "$enable_sound" = "dlopen"; then
				AC_DEFINE(USE_DLOPEN, 1, [Define to 1 if run-time dynamic linking should be used.])
				USE_DLOPEN=yes
			else
				SOUND_LDFLAGS="$OPENAL_LDFLAGS $VORBISFILE_LDFLAGS"
				SOUND_LIBS="$OPENAL_LIBS $VORBISFILE_LIBS"
			fi
			AC_DEFINE(HAVE_SOUND, 1, [Define to 1 if OGG sound support is available.])
		fi
	fi
fi
AC_SUBST(HAVE_SOUND)
AC_SUBST(SOUND_CPPFLAGS)
AC_SUBST(SOUND_CXXFLAGS)
AC_SUBST(SOUND_LDFLAGS)
AC_SUBST(SOUND_LIBS)
AM_CONDITIONAL(SOUND, test "$HAVE_SOUND" = yes)
AM_CONDITIONAL(DLOPEN, test "$USE_DLOPEN" = yes)

HAVE_IMAGE=no
IMAGE_CPPFLAGS=
IMAGE_CXXFLAGS=
IMAGE_LDFLAGS=
IMAGE_LIBS=
AC_ARG_ENABLE(image,
	AS_HELP_STRING(--disable-image,disable PNG image file support))
if test "$enable_image" != "no"; then
	MAC_PKG_PNG
	if test "$HAVE_PKG_PNG" = yes; then
		case "$LIBPNG_SUPPORTS_SETJMP" in
		no)
			AC_MSG_ERROR([need a version of libpng with setjmp support])
			;;
		unknown)
			AC_MSG_WARN([assuming libpng has setjmp support])
			;;
		esac
		HAVE_IMAGE=yes
		IMAGE_CPPFLAGS="$PNG_CPPFLAGS"
		IMAGE_CXXFLAGS="$PNG_CXXFLAGS"
		IMAGE_LDFLAGS="$PNG_LDFLAGS"
		IMAGE_LIBS="$PNG_LIBS"
		AC_DEFINE(HAVE_IMAGE, 1, [Define to 1 if PNG image support is available.])
	fi
fi
AC_SUBST(HAVE_IMAGE)
AC_SUBST(IMAGE_CPPFLAGS)
AC_SUBST(IMAGE_CXXFLAGS)
AC_SUBST(IMAGE_LDFLAGS)
AC_SUBST(IMAGE_LIBS)
AM_CONDITIONAL(IMAGE, test "$HAVE_IMAGE" = yes)

# Create a hackdir that is relative to bindir
adl_COMPUTE_RELATIVE_PATHS([bindir:hackdir:hackreldir])
AC_SUBST(hackreldir)

# Checks for header files.
AC_CHECK_HEADERS_ONCE([setjmp.h sys/types.h sys/select.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS_ONCE([setjmp select], [], [
	AC_MSG_ERROR([your environment is missing a necessary library function])
])

AC_ARG_ENABLE(debug,
	AS_HELP_STRING(--enable-debug, [include debug code]))
case "$enable_debug" in
no|"")
	AC_DEFINE(NDEBUG, 1, [Define to 1 if debugging is disabled])
	;;
*)
	;;
esac

AC_ARG_ENABLE(profile,
	AS_HELP_STRING(--enable-profile, [enable profiling]))
case "$enable_profile" in
no|"")
	;;
*)
	if test "$GCC" != "yes"; then
		AC_MSG_ERROR([I can only use profiling on GCC])
	fi
	CFLAGS=`echo $CXXFLAGS -pg -fprofile-arcs -ftest-coverage | sed -e s,-O2,-O0,`
	LDFLAGS="$LDFLAGS -pg -fprofile-arcs -ftest-coverage"
	;;
esac

ENABLED_HACKS=
ENABLED_BINARIES=
ENABLED_CONFIGS=
DISABLED_HACKS=

MAC_HACK_ENABLE(cyclone,    [Cyclone])
MAC_HACK_ENABLE(euphoria,   [Euphoria], [
	if test "$HAVE_IMAGE" != yes; then
		enable_euphoria="no  (requires PNG image support)"
		enable_hack=no
	fi
])
MAC_HACK_ENABLE(fieldlines, [Field Lines])
MAC_HACK_ENABLE(flocks,     [Flocks])
MAC_HACK_ENABLE(flux,       [Flux])
MAC_HACK_ENABLE(helios,     [Helios], [
	if test "$HAVE_IMAGE" != yes; then
		enable_helios="no  (requires PNG image support)"
		enable_hack=no
	fi
])
MAC_HACK_ENABLE(hyperspace, [Hyperspace], [
	if test "$HAVE_IMAGE" != yes; then
		enable_hyperspace="no  (requires PNG image support)"
		enable_hack=no
	fi
])
MAC_HACK_ENABLE(lattice,    [Lattice], [
	if test "$HAVE_IMAGE" != yes; then
		enable_lattice="no  (requires PNG image support)"
		enable_hack=no
	fi
])
MAC_HACK_ENABLE(plasma,     [Plasma])
MAC_HACK_ENABLE(skyrocket,  [Skyrocket], [
	if test "$HAVE_IMAGE" != yes; then
		enable_skyrocket="no  (requires PNG image support)"
		enable_hack=no
	else
		if test "$HAVE_SOUND" != yes; then
			enable_skyrocket="yes (but without sound)"
		elif test "$USE_DLOPEN" = yes; then
			enable_skyrocket="yes (if available at run-time)"
		fi
	fi
])
MAC_HACK_ENABLE(solarwinds, [Solar Winds])

AC_SUBST(ENABLED_HACKS)
AC_SUBST(ENABLED_BINARIES)
AC_SUBST(ENABLED_CONFIGS)
AC_SUBST(DISABLED_HACKS)

# Embed stuff in the RPM spec
INSTALL_AWK=scripts/install.awk
UNINSTALL_AWK=scripts/uninstall.awk
VERIFY_AWK=scripts/verify.awk
CHANGELOG=ChangeLog
AC_SUBST_FILE(INSTALL_AWK)
AC_SUBST_FILE(UNINSTALL_AWK)
AC_SUBST_FILE(VERIFY_AWK)
AC_SUBST_FILE(CHANGELOG)

AC_CONFIG_FILES([
	Makefile
	config/Makefile
	lib/Makefile
	libltdl/Makefile
	src/Makefile
	src/cyclone/Makefile
	src/euphoria/Makefile
	src/fieldlines/Makefile
	src/flocks/Makefile
	src/flux/Makefile
	src/helios/Makefile
	src/hyperspace/Makefile
	src/lattice/Makefile
	src/plasma/Makefile
	src/skyrocket/Makefile
	src/solarwinds/Makefile
	rsxs.spec
])
AC_OUTPUT

# Print a pretty summary
echo >&2
echo "====================================" >&2
echo "  XScreenSaver support      :  $HAVE_PKG_XSCREENSAVER" >&2
echo "  PNG image support         :  $HAVE_IMAGE" >&2
echo "  OGG Vorbis sound support  :  $HAVE_SOUND" >&2
echo "------------------------------------" >&2
echo "  Enabled screensavers:" >&2
echo "  - Cyclone                 :  $enable_cyclone" >&2
echo "  - Euphoria                :  $enable_euphoria" >&2
echo "  - Field Lines             :  $enable_fieldlines" >&2
echo "  - Flocks                  :  $enable_flocks" >&2
echo "  - Flux                    :  $enable_flux" >&2
echo "  - Helios                  :  $enable_helios" >&2
echo "  - Hyperspace              :  $enable_hyperspace" >&2
echo "  - Lattice                 :  $enable_lattice" >&2
echo "  - Plasma                  :  $enable_plasma" >&2
echo "  - Skyrocket               :  $enable_skyrocket" >&2
echo "  - Solar Winds             :  $enable_solarwinds" >&2
echo "====================================" >&2
echo >&2
