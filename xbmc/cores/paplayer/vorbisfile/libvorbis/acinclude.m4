# acinclude.m4
# all .m4 files needed that might not be installed go here

# Configure paths for libogg
# Jack Moffitt <jack@icecast.org> 10-21-2000
# Shamelessly stolen from Owen Taylor and Manish Singh

dnl XIPH_PATH_OGG([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libogg, and define OGG_CFLAGS and OGG_LIBS
dnl
AC_DEFUN([XIPH_PATH_OGG],
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(ogg,[  --with-ogg=PFX   Prefix where libogg is installed (optional)], ogg_prefix="$withval", ogg_prefix="")
AC_ARG_WITH(ogg-libraries,[  --with-ogg-libraries=DIR   Directory where libogg library is installed (optional)], ogg_libraries="$withval", ogg_libraries="")
AC_ARG_WITH(ogg-includes,[  --with-ogg-includes=DIR   Directory where libogg header files are installed (optional)], ogg_includes="$withval", ogg_includes="")
AC_ARG_ENABLE(oggtest, [  --disable-oggtest       Do not try to compile and run a test Ogg program],, enable_oggtest=yes)

  if test "x$ogg_libraries" != "x" ; then
    OGG_LIBS="-L$ogg_libraries"
  elif test "x$ogg_prefix" != "x" ; then
    OGG_LIBS="-L$ogg_prefix/lib"
  elif test "x$prefix" != "xNONE" ; then
    OGG_LIBS="-L$prefix/lib"
  fi

  OGG_LIBS="$OGG_LIBS -logg"

  if test "x$ogg_includes" != "x" ; then
    OGG_CFLAGS="-I$ogg_includes"
  elif test "x$ogg_prefix" != "x" ; then
    OGG_CFLAGS="-I$ogg_prefix/include"
  elif test "x$prefix" != "xNONE"; then
    OGG_CFLAGS="-I$prefix/include"
  fi

  AC_MSG_CHECKING(for Ogg)
  no_ogg=""


  if test "x$enable_oggtest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $OGG_CFLAGS"
    LIBS="$LIBS $OGG_LIBS"
dnl
dnl Now check if the installed Ogg is sufficiently new.
dnl
      rm -f conf.oggtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>

int main ()
{
  system("touch conf.oggtest");
  return 0;
}

],, no_ogg=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_ogg" = "x" ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.oggtest ; then
       :
     else
       echo "*** Could not run Ogg test program, checking why..."
       CFLAGS="$CFLAGS $OGG_CFLAGS"
       LIBS="$LIBS $OGG_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <ogg/ogg.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding Ogg or finding the wrong"
       echo "*** version of Ogg. If it is not finding Ogg, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means Ogg was incorrectly installed"
       echo "*** or that you have moved Ogg since it was installed. In the latter case, you"
       echo "*** may want to edit the ogg-config script: $OGG_CONFIG" ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     OGG_CFLAGS=""
     OGG_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(OGG_CFLAGS)
  AC_SUBST(OGG_LIBS)
  rm -f conf.oggtest
])

dnl PKG_CHECK_MODULES(GSTUFF, gtk+-2.0 >= 1.3 glib = 1.3.4, action-if, action-not)
dnl defines GSTUFF_LIBS, GSTUFF_CFLAGS, see pkg-config man page
dnl also defines GSTUFF_PKG_ERRORS on error
AC_DEFUN([PKG_CHECK_MODULES], [
  succeeded=no

  if test -z "$PKG_CONFIG"; then
    AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  fi

  if test "$PKG_CONFIG" = "no" ; then
     echo "*** The pkg-config script could not be found. Make sure it is"
     echo "*** in your path, or set the PKG_CONFIG environment variable"
     echo "*** to the full path to pkg-config."
     echo "*** Or see http://www.freedesktop.org/software/pkgconfig to get pkg-config."
  else
     PKG_CONFIG_MIN_VERSION=0.9.0
     if $PKG_CONFIG --atleast-pkgconfig-version $PKG_CONFIG_MIN_VERSION; then
        AC_MSG_CHECKING(for $2)

        if $PKG_CONFIG --exists "$2" ; then
            AC_MSG_RESULT(yes)
            succeeded=yes

            AC_MSG_CHECKING($1_CFLAGS)
            $1_CFLAGS=`$PKG_CONFIG --cflags "$2"`
            AC_MSG_RESULT($$1_CFLAGS)

            AC_MSG_CHECKING($1_LIBS)
            $1_LIBS=`$PKG_CONFIG --libs "$2"`
            AC_MSG_RESULT($$1_LIBS)
        else
            $1_CFLAGS=""
            $1_LIBS=""
            ## If we have a custom action on failure, don't print errors, but 
            ## do set a variable so people can do so.
            $1_PKG_ERRORS=`$PKG_CONFIG --errors-to-stdout --print-errors "$2"`
            ifelse([$4], ,echo $$1_PKG_ERRORS,)
        fi

        AC_SUBST($1_CFLAGS)
        AC_SUBST($1_LIBS)
     else
        echo "*** Your version of pkg-config is too old. You need version $PKG_CONFIG_MIN_VERSION or newer."
        echo "*** See http://www.freedesktop.org/software/pkgconfig"
     fi
  fi

  if test $succeeded = yes; then
     ifelse([$3], , :, [$3])
  else
     ifelse([$4], , AC_MSG_ERROR([Library requirements ($2) not met; consider adjusting the PKG_CONFIG_PATH environment variable if your libraries are in a nonstandard prefix so pkg-config can find them.]), [$4])
  fi
])
