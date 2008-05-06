AC_PREREQ(2.60a)
AC_INIT(libass, 0.9.4)
AM_INIT_AUTOMAKE
AM_PROG_LIBTOOL
AC_CONFIG_SRCDIR([test/test.cpp])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP

# Checks for libraries.
AC_CHECK_LIB([png], [png_create_write_struct], AC_DEFINE(HAVE_LIBPNG, 1, [use libpng]))
AC_SEARCH_LIBS([iconv_open], [iconv], AC_DEFINE(USE_ICONV, 1, [use iconv]))
AC_CHECK_LIB([enca], [enca_analyser_alloc], [AC_DEFINE(HAVE_ENCA, 1, [use enca])  LIBS="$LIBS -lenca"])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([inttypes.h stdint.h stdlib.h string.h sys/time.h unistd.h iconv.h enca.h])

PKG_CHECK_MODULES([FONTCONFIG], fontconfig >= 2.2.0, [
    CFLAGS="$CFLAGS $FONTCONFIG_CFLAGS"
    LIBS="$LIBS $FONTCONFIG_LIBS"
    AC_DEFINE(HAVE_FONTCONFIG, 1, [found fontconfig via pkg-config])
    ], [true])

PKG_CHECK_MODULES([FREETYPE], freetype2 >= 9.6.3, [
    CFLAGS="$CFLAGS $FREETYPE_CFLAGS"
    LIBS="$LIBS $FREETYPE_LIBS"
    AC_DEFINE(HAVE_FREETYPE, 1, [found freetype2 via pkg-config])
    ])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([memset mkdir strcasecmp strchr strdup strerror strstr strtol])

AC_CONFIG_FILES([Makefile libass/Makefile test/Makefile libass.pc])
AC_OUTPUT
