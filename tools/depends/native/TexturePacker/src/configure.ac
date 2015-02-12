dnl Process this file with autoconf to produce a configure script.
AC_INIT(TexturePacker, 1.0)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR(TexturePacker.cpp)
AM_INIT_AUTOMAKE([foreign])
AC_PROG_CXX
AC_LANG([C++])
AC_C_BIGENDIAN

abs_top_srcdir=${abs_top_srcdir=$(cd $srcdir; pwd)}
KODI_SRC_DIR=${KODI_SRC_DIR:-"${abs_top_srcdir}/../../../../.."}

AC_ARG_ENABLE([static],
  [AS_HELP_STRING([--enable-static],
  [build static TexturePacker (default is no)])],
  [STATIC_FLAG="-static"],
  [STATIC_FLAG=""])


PKG_CHECK_MODULES([PNG], [libpng],
  [INCLUDES="$PNG_CFLAGS"; LIBS="$LIBS $(pkg-config --silence-errors --static --libs libpng)"],
  AC_MSG_ERROR("libpng not found"))

AC_CHECK_HEADER([gif_lib.h],, AC_MSG_ERROR("gif_lib.h not found"))
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <gif_lib.h>]], [[
#if !defined GIFLIB_MAJOR || GIFLIB_MAJOR < 5
#error libgif version < 5;
#endif ]])], [], [AC_MSG_NOTICE([[WARNING: libgif version is unsupported, please consider upgrading to 5.0.5 or higher.]])])

AC_CHECK_LIB([gif],[main],, AC_MSG_ERROR("libgif not found"))
AC_CHECK_HEADER([jpeglib.h],, AC_MSG_ERROR("jpeglib.h not found"))
AC_CHECK_LIB([jpeg],[main],, AC_MSG_ERROR("libjpeg not found"))
AC_CHECK_HEADER([lzo/lzo1x.h],, AC_MSG_ERROR("lzo/lzo1x.h not found"))
AC_CHECK_LIB([lzo2],[main],, AC_MSG_ERROR("liblzo2 not found"))

# we don't compile with default upstream squish, so we use our own one
# even on non unified deps platforms
export PKG_CONFIG_PATH="${abs_top_srcdir}/../../libsquish-native/squish-install/lib/pkgconfig:$PKG_CONFIG_PATH"
PKG_CHECK_MODULES([SQUISH], [squish],
 [CPPFLAGS="$CPPFLAGS $SQUISH_CFLAGS" LIBS="$LIBS $SQUISH_LIBS"],
 AC_MSG_ERROR("libsquish not found"))


AC_SUBST(KODI_SRC_DIR)
AC_SUBST(STATIC_FLAG)
AC_SUBST(EXTRA_DEFINES)

AC_OUTPUT(Makefile)
