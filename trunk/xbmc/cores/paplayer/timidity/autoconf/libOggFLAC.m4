# Configure paths for libOggFLAC
# "Inspired" by ogg.m4

dnl AM_PATH_LIBOGGFLAC([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libOggFLAC, and define LIBOGGFLAC_CFLAGS and LIBOGGFLAC_LIBS
dnl
AC_DEFUN(AM_PATH_LIBOGGFLAC,
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(libOggFLAC,[  --with-libOggFLAC=PFX   Prefix where libOggFLAC is installed (optional)], libOggFLAC_prefix="$withval", libOggFLAC_prefix="")
AC_ARG_WITH(libOggFLAC-libraries,[  --with-libOggFLAC-libraries=DIR   Directory where libOggFLAC library is installed (optional)], libOggFLAC_libraries="$withval", libOggFLAC_libraries="")
AC_ARG_WITH(libOggFLAC-includes,[  --with-libOggFLAC-includes=DIR   Directory where libOggFLAC header files are installed (optional)], libOggFLAC_includes="$withval", libOggFLAC_includes="")
AC_ARG_ENABLE(libOggFLACtest, [  --disable-libOggFLACtest       Do not try to compile and run a test libOggFLAC program],, enable_libOggFLACtest=yes)

  if test "x$libOggFLAC_libraries" != "x" ; then
    LIBOGGFLAC_LIBS="-L$libOggFLAC_libraries"
  elif test "x$libOggFLAC_prefix" != "x" ; then
    LIBOGGFLAC_LIBS="-L$libOggFLAC_prefix/lib"
  elif test "x$prefix" != "xNONE" ; then
    LIBOGGFLAC_LIBS="-L$prefix/lib"
  fi

  LIBOGGFLAC_LIBS="$LIBOGGFLAC_LIBS -lOggFLAC -lFLAC -lm"

  if test "x$libOggFLAC_includes" != "x" ; then
    LIBOGGFLAC_CFLAGS="-I$libOggFLAC_includes"
  elif test "x$libOggFLAC_prefix" != "x" ; then
    LIBOGGFLAC_CFLAGS="-I$libOggFLAC_prefix/include"
  elif test "$prefix" != "xNONE"; then
    LIBOGGFLAC_CFLAGS="-I$prefix/include"
  fi

  AC_MSG_CHECKING(for libOggFLAC)
  no_libOggFLAC=""


  if test "x$enable_libOggFLACtest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_CXXFLAGS="$CXXFLAGS"
    ac_save_LIBS="$LIBS"
    CFLAGS="$CFLAGS $LIBOGGFLAC_CFLAGS"
    CXXFLAGS="$CXXFLAGS $LIBOGGFLAC_CFLAGS"
    LIBS="$LIBS $LIBOGGFLAC_LIBS"
dnl
dnl Now check if the installed libOggFLAC is sufficiently new.
dnl
      rm -f conf.libOggFLACtest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OggFLAC/stream_decoder.h>

int main ()
{
  system("touch conf.libOggFLACtest");
  return 0;
}

],, no_libOggFLAC=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_libOggFLAC" = "x" ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.libOggFLACtest ; then
       :
     else
       echo "*** Could not run libOggFLAC test program, checking why..."
       CFLAGS="$CFLAGS $LIBOGGFLAC_CFLAGS"
       LIBS="$LIBS $LIBOGGFLAC_LIBS"
       AC_TRY_LINK([
#include <stdio.h>
#include <OggFLAC/stream_decoder.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding libOggFLAC or finding the wrong"
       echo "*** version of libOggFLAC. If it is not finding libOggFLAC, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means libOggFLAC was incorrectly installed"
       echo "*** or that you have moved libOggFLAC since it was installed. In the latter case, you"
       echo "*** may want to edit the libOggFLAC-config script: $LIBOGGFLAC_CONFIG" ])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
     LIBOGGFLAC_CFLAGS=""
     LIBOGGFLAC_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(LIBOGGFLAC_CFLAGS)
  AC_SUBST(LIBOGGFLAC_LIBS)
  rm -f conf.libOggFLACtest
])
