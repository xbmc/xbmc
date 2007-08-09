dnl FTGL_PROG_CXX()
dnl Check the build plataform and try to use the native compiler
dnl
AC_DEFUN(FTGL_PROG_CXX,
[dnl
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

dnl I really don't know how to handle the cross-compiling case
if test "$build" = "$host" ; then
    case "$build" in
        *-*-irix*)
            if test -z "$CXX" ; then
                CXX=CC
            fi
            if test -z "$CC" ; then
                CC=cc
            fi
            if test x$CXX = xCC -a -z "$CXXFLAGS" ; then
                # It might be worthwhile to move this out of here, say
                # EXTRA_CXXFLAGS.  Forcing -n32 might cause trouble, too.
                CXXFLAGS="-LANG:std -n32 -woff 1201 -O3"
            fi
        ;;
    esac
fi

AC_PROG_CXX
])
# Configure paths for FreeType2
# Marcelo Magallon 2001-10-26, based on gtk.m4 by Owen Taylor

dnl AC_CHECK_FT2([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for FreeType2, and define FT2_CFLAGS and FT2_LIBS
dnl
AC_DEFUN(AC_CHECK_FT2,
[dnl
dnl Get the cflags and libraries from the freetype-config script
dnl
AC_ARG_WITH(freetype-prefix,
[  --with-ft-prefix=PFX      Prefix where FreeType is installed (optional)],
            ft_config_prefix="$withval", ft_config_prefix="")
AC_ARG_WITH(freetype-exec-prefix,
[  --with-ft-exec-prefix=PFX Exec prefix where FreeType is installed (optional)],
            ft_config_exec_prefix="$withval", ft_config_exec_prefix="")
AC_ARG_ENABLE(freetypetest, [  --disable-freetypetest  Do not try to compile and run a test FreeType program],[],
              enable_fttest=yes)

if test x$ft_config_exec_prefix != x ; then
  ft_config_args="$ft_config_args --exec-prefix=$ft_config_exec_prefix"
  if test x${FT2_CONFIG+set} != xset ; then
    FT2_CONFIG=$ft_config_exec_prefix/bin/freetype-config
  fi
fi
if test x$ft_config_prefix != x ; then
  ft_config_args="$ft_config_args --prefix=$ft_config_prefix"
  if test x${FT2_CONFIG+set} != xset ; then
    FT2_CONFIG=$ft_config_prefix/bin/freetype-config
  fi
fi
AC_PATH_PROG(FT2_CONFIG, freetype-config, no)

min_ft_version=ifelse([$1], ,6.1.0,$1)
AC_MSG_CHECKING(for FreeType - version >= $min_ft_version)
no_ft=""
if test "$FT2_CONFIG" = "no" ; then
  no_ft=yes
else
  FT2_CFLAGS=`$FT2_CONFIG $ft_config_args --cflags`
  FT2_LIBS=`$FT2_CONFIG $ft_config_args --libs`
  ft_config_version=`$FT2_CONFIG $ft_config_args --version`
  ft_config_major_version=`echo $ft_config_version | cut -d . -f 1`
  ft_config_minor_version=`echo $ft_config_version | cut -d . -f 2`
  ft_config_micro_version=`echo $ft_config_version | cut -d . -f 3`
  ft_min_major_version=`echo $min_ft_version | cut -d . -f 1`
  ft_min_minor_version=`echo $min_ft_version | cut -d . -f 2`
  ft_min_micro_version=`echo $min_ft_version | cut -d . -f 3`
  if test "x$enable_fttest" = "xyes" ; then
    ft_config_version=`expr $ft_config_major_version \* 10000 + $ft_config_minor_version \* 100 + $ft_config_micro_version`
    ft_min_version=`expr $ft_min_major_version \* 10000 + $ft_min_minor_version \* 100 + $ft_min_micro_version`
    if test $ft_config_version -lt $ft_min_version ; then
      ifelse([$3], , :, [$3])
    else
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $FT2_CFLAGS"
      LIBS="$FT2_LIBS $LIBS"
dnl
dnl Sanity checks the results of freetype-config to some extent
dnl
      AC_TRY_RUN([
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
    FT_Library library;
    FT_Error error;

    error = FT_Init_FreeType( &library );

    if ( error )
    {
        return 1;
    } else {
        FT_Done_FreeType( library );
        return 0;
    }
}
],, no_ft=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
      CFLAGS="$ac_save_CFLAGS"
      LIBS="$ac_save_LIBS"
    fi             # test $ft_config_version -lt $ft_min_version
  fi               # test "x$enable_fttest" = "xyes"
fi                 # test "$FT2_CONFIG" = "no"
if test "x$no_ft" = x ; then
   AC_MSG_RESULT(yes)
   ifelse([$2], , :, [$2])     
else
   AC_MSG_RESULT(no)
   if test "$FT2_CONFIG" = "no" ; then
     echo "*** The freetype-config script installed by FT2 could not be found"
     echo "*** If FT2 was installed in PREFIX, make sure PREFIX/bin is in"
     echo "*** your path, or set the FT2_CONFIG environment variable to the"
     echo "*** full path to freetype-config."
   else
     echo "*** The FreeType test program failed to run.  If your system uses"
     echo "*** shared libraries and they are installed outside the normal"
     echo "*** system library path, make sure the variable LD_LIBRARY_PATH"
     echo "*** (or whatever is appropiate for your system) is correctly set."
   fi
   FT2_CFLAGS=""
   FT2_LIBS=""
   ifelse([$3], , :, [$3])
fi
AC_SUBST(FT2_CFLAGS)
AC_SUBST(FT2_LIBS)
])
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
