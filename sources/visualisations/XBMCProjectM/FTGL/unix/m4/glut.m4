dnl FTGL_CHECK_GLUT()
dnl Check for GLUT development environment
dnl
AC_DEFUN([FTGL_CHECK_GLUT],
[dnl
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_PATH_X])dnl
AC_REQUIRE([AC_PATH_XTRA])dnl
AC_REQUIRE([FTGL_CHECK_GL])dnl

AC_ARG_WITH([--with-glut-inc],
    AC_HELP_STRING([--with-glut-inc=DIR],[Directory where GL/glut.h is installed (optional)]))
AC_ARG_WITH([--with-glut-lib],
    AC_HELP_STRING([--with-glut-lib=DIR],[Directory where GLUT libraries are installed (optional)]))

AC_LANG_SAVE
AC_LANG_C

GLUT_SAVE_CPPFLAGS="$CPPFLAGS"
GLUT_SAVE_LIBS="$LIBS"

if test "x$no_x" != xyes ; then
    GLUT_CFLAGS="$X_CFLAGS"
    GLUT_X_LIBS="$X_PRE_LIBS $X_LIBS -lX11 -lXext -lXmu $X_EXTRA_LIBS"
fi

if test "x$with_glut_inc" != "xnone" ; then
    if test -d "$with_glut_inc" ; then
        GLUT_CFLAGS="-I$with_glut_inc"
    else
        GLUT_CFLAGS="$with_glut_inc"
    fi
else
    GLUT_CFLAGS=
fi

CPPFLAGS="$GLUT_CFLAGS"
AC_CHECK_HEADER([GL/glut.h], [HAVE_GLUT=yes], [HAVE_GLUT=no])

if test "x$HAVE_GLUT" = xno ; then
    AC_MSG_WARN([GLUT headers not availabe, example program won't be compiled.])
else

# Check for GLUT libraries

    AC_MSG_CHECKING([for GLUT library])
    if test "x$with_glut_lib" != "x" ; then
        if test -d "$with_glut_lib" ; then
            LIBS="-L$with_glut_lib -lglut"
        else
            LIBS="$with_glut_lib"
        fi
    else
        LIBS="-lglut"
    fi
    AC_LINK_IFELSE(
        [AC_LANG_CALL([],[glutInit])],
        [HAVE_GLUT=yes],
        [HAVE_GLUT=no])

    if test "x$HAVE_GLUT" = xno ; then
        # Try again with the GL libs
        LIBS="-lglut $GL_LIBS"
        AC_LINK_IFELSE(
            [AC_LANG_CALL([],[glutInit])],
            [HAVE_GLUT=yes],
            [HAVE_GLUT=no])
    fi

    if test "x$HAVE_GLUT" = xno && test "x$GLUT_X_LIBS" != x ; then
        # Try again with the GL and X11 libs
        LIBS="-lglut $GL_LIBS $GLUT_X_LIBS"
        AC_LINK_IFELSE(
            [AC_LANG_CALL([],[glutInit])],
            [HAVE_GLUT=yes],
            [HAVE_GLUT=no])
    fi

    if test "x$HAVE_GLUT" = xyes ; then
        AC_MSG_RESULT([yes])
        GLUT_LIBS=$LIBS
    else
        AC_MSG_RESULT([no])
        AC_MSG_WARN([GLUT libraries not availabe, example program won't be compiled.])
    fi

# End check for GLUT libraries

fi

AC_SUBST(HAVE_GLUT)
AC_SUBST(GLUT_CFLAGS)
AC_SUBST(GLUT_LIBS)
AC_LANG_RESTORE

CPPFLAGS="$GLUT_SAVE_CPPFLAGS"
LIBS="$GLUT_SAVE_LIBS"
GLUT_X_CFLAGS=
GLUT_X_LIBS=
])
