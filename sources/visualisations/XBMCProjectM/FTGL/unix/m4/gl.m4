dnl FTGL_CHECK_GL()
dnl Check for OpenGL development environment and GLU >= 1.2
dnl
AC_DEFUN([FTGL_CHECK_GL],
[dnl
AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([AC_PATH_X])
AC_REQUIRE([AC_PATH_XTRA])

AC_ARG_WITH([--with-gl-inc],
    AC_HELP_STRING([--with-gl-inc=DIR],[Directory where GL/gl.h is installed]))
AC_ARG_WITH([--with-gl-lib],
    AC_HELP_STRING([--with-gl-lib=DIR],[Directory where OpenGL libraries are installed]))

AC_LANG_SAVE
AC_LANG_C

GL_SAVE_CPPFLAGS="$CPPFLAGS"
GL_SAVE_LIBS="$LIBS"

if test "x$no_x" != xyes ; then
    GL_CFLAGS="$X_CFLAGS"
    GL_X_LIBS="$X_PRE_LIBS $X_LIBS -lX11 -lXext -lXmu $X_EXTRA_LIBS"
fi

if test "x$with_gl_inc" != "xnone" ; then
    if test -d "$with_gl_inc" ; then
        GL_CFLAGS="-I$with_gl_inc"
    else
        GL_CFLAGS="$with_gl_inc"
    fi
else
    GL_CFLAGS=
fi

CPPFLAGS="$GL_CFLAGS"
AC_CHECK_HEADER([GL/gl.h], [], [AC_MSG_ERROR(GL/gl.h is needed, please specify its location with --with-gl-inc.  If this still fails, please contact mmagallo@debian.org, include the string FTGL somewhere in the subject line and provide a copy of the config.log file that was left behind.)])

AC_MSG_CHECKING([for GL library])
if test "x$with_gl_lib" != "x" ; then
    if test -d "$with_gl_lib" ; then
        LIBS="-L$with_gl_lib -lGL"
    else
        LIBS="$with_gl_lib"
    fi
else
    LIBS="-lGL"
fi
AC_LINK_IFELSE([AC_LANG_CALL([],[glBegin])],[HAVE_GL=yes], [HAVE_GL=no])
if test "x$HAVE_GL" = xno ; then
    if test "x$GL_X_LIBS" != x ; then
        LIBS="-lGL $GL_X_LIBS"
        AC_LINK_IFELSE([AC_LANG_CALL([],[glBegin])],[HAVE_GL=yes], [HAVE_GL=no])
    fi
fi
if test "x$HAVE_GL" = xyes ; then
    AC_MSG_RESULT([yes])
    GL_LIBS=$LIBS
else
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([GL library could not be found, please specify its location with --with-gl-lib.  If this still fails, please contact mmagallo@debian.org, include the string FTGL somewhere in the subject line and provide a copy of the config.log file that was left behind.])
fi

AC_CHECK_HEADER([GL/glu.h])
AC_MSG_CHECKING([for GLU version >= 1.2])
AC_TRY_COMPILE([#include <GL/glu.h>], [
#if !defined(GLU_VERSION_1_2)
#error GLU too old
#endif
               ],
               [AC_MSG_RESULT([yes])],
               [AC_MSG_RESULT([no])
                AC_MSG_ERROR([GLU >= 1.2 is needed to compile this library])
               ])

AC_MSG_CHECKING([for GLU library])
LIBS="-lGLU $GL_LIBS"
AC_LINK_IFELSE([AC_LANG_CALL([],[gluNewTess])],[HAVE_GLU=yes], [HAVE_GLU=no])
if test "x$HAVE_GLU" = xno ; then
    if test "x$GL_X_LIBS" != x ; then
        LIBS="-lGLU $GL_LIBS $GL_X_LIBS"
        AC_LINK_IFELSE([AC_LANG_CALL([],[gluNewTess])],[HAVE_GLU=yes], [HAVE_GLU=no])
    fi
fi
if test "x$HAVE_GLU" = xyes ; then
    AC_MSG_RESULT([yes])
    GL_LIBS="$LIBS"
else
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([GLU library could not be found, please specify its location with --with-gl-lib.  If this still fails, please contact mmagallo@debian.org, include the string FTGL somewhere in the subject line and provide a copy of the config.log file that was left behind.])
fi

AC_SUBST(GL_CFLAGS)
AC_SUBST(GL_LIBS)

CPPFLAGS="$GL_SAVE_CPPFLAGS"
LIBS="$GL_SAVE_LIBS"
AC_LANG_RESTORE
GL_X_LIBS=""
])
