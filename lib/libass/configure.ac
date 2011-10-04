AC_INIT(libass, 0.10.0)
AM_INIT_AUTOMAKE
AC_CONFIG_MACRO_DIR([m4])
# Disable C++/Fortran checks
define([AC_LIBTOOL_LANG_CXX_CONFIG], [:])
define([AC_LIBTOOL_LANG_F77_CONFIG], [:])
AC_PROG_LIBTOOL
AC_CONFIG_SRCDIR([libass/ass.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_C_O

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_CHECK_HEADERS([inttypes.h stdint.h stdlib.h string.h sys/time.h unistd.h iconv.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([mkdir strcasecmp strdup strtol])

# Checks for libraries.
AC_SEARCH_LIBS([iconv_open], [iconv], AC_DEFINE(CONFIG_ICONV, 1, [use iconv]))
AC_CHECK_LIB([m], [fabs])

# Check for libraries via pkg-config
AC_ARG_ENABLE([test], AS_HELP_STRING([--enable-test],
    [enable test program (requires libpng) @<:@default=no@:>@]))
AC_ARG_ENABLE([enca], AS_HELP_STRING([--disable-enca],
    [disable enca (charset autodetect) support @<:@default=check@:>@]))
AC_ARG_ENABLE([fontconfig], AS_HELP_STRING([--disable-fontconfig],
    [disable fontconfig support @<:@default=enabled@:>@]))
AC_ARG_ENABLE([harfbuzz], AS_HELP_STRING([--disable-harfbuzz],
    [disable HarfBuzz support @<:@default=check@:>@]))

PKG_CHECK_MODULES([FREETYPE], freetype2 >= 9.10.3, [
    CFLAGS="$CFLAGS $FREETYPE_CFLAGS"
    LIBS="$LIBS $FREETYPE_LIBS"
    AC_DEFINE(CONFIG_FREETYPE, 1, [found freetype2 via pkg-config])
    ])

PKG_CHECK_MODULES([FRIBIDI], fribidi >= 0.19.0, [
    CFLAGS="$CFLAGS $FRIBIDI_CFLAGS"
    LIBS="$LIBS $FRIBIDI_LIBS"
    AC_DEFINE(CONFIG_FRIBIDI, 1, [found fribidi via pkg-config])
    ])

if test x$enable_fontconfig != xno; then
PKG_CHECK_MODULES([FONTCONFIG], fontconfig >= 2.4.2, [
    CFLAGS="$CFLAGS $FONTCONFIG_CFLAGS"
    LIBS="$LIBS $FONTCONFIG_LIBS"
    AC_DEFINE(CONFIG_FONTCONFIG, 1, [found fontconfig via pkg-config])
	fontconfig=true
    ])
fi

if test x$enable_harfbuzz != xno; then
PKG_CHECK_MODULES([HARFBUZZ], harfbuzz >= 0.7.0, [
    CFLAGS="$CFLAGS $HARFBUZZ_CFLAGS"
    LIBS="$LIBS $HARFBUZZ_LIBS"
    AC_DEFINE(CONFIG_HARFBUZZ, 1, [found harfbuzz-ng via pkg-config])
	harfbuzz=true
    ], [harfbuzz=false])
fi

if test x$enable_enca != xno; then
PKG_CHECK_MODULES([ENCA], enca, [
    CFLAGS="$CFLAGS $ENCA_CFLAGS"
    LIBS="$LIBS $ENCA_LIBS"
    AC_DEFINE(CONFIG_ENCA, 1, [found enca via pkg-config])
	enca=true
	], [enca=false])
fi

libpng=false
if test x$enable_test = xyes; then
PKG_CHECK_MODULES([LIBPNG], libpng >= 1.2.0, [
    CFLAGS="$CFLAGS $LIBPNG_CFLAGS"
    AC_DEFINE(CONFIG_LIBPNG, 1, [found libpng via pkg-config])
    libpng=true])
fi

AM_CONDITIONAL([HAVE_LIBPNG], [test x$libpng = xtrue])

# add libraries/packages to pkg-config for static linking
pkg_libs="-lm"
pkg_requires="freetype2 >= 9.6.3"
pkg_requires="fribidi >= 0.19.0, ${pkg_requires}"
if test x$enca = xtrue; then
    pkg_requires="enca, ${pkg_requires}"
fi
if test x$fontconfig = xtrue; then
    pkg_requires="fontconfig >= 2.2.0, ${pkg_requires}"
fi
if test x$harfbuzz = xtrue; then
    pkg_requires="harfbuzz >= 0.7.0, ${pkg_requires}"
fi

AC_SUBST([PKG_LIBS_DEFAULT], [$(test x$enable_shared = xno && echo ${pkg_libs})])
AC_SUBST([PKG_REQUIRES_DEFAULT], [$(test x$enable_shared = xno && echo ${pkg_requires})])
AC_SUBST([PKG_LIBS_PRIVATE], [$(test x$enable_shared = xno || echo ${pkg_libs})])
AC_SUBST([PKG_REQUIRES_PRIVATE], [$(test x$enable_shared = xno || echo ${pkg_requires})])

# Setup output beautifier.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_FILES([Makefile libass/Makefile test/Makefile libass.pc])
AC_OUTPUT
