# Configure paths for libFLAC++
# "Inspired" by ogg.m4
# Caller must first run AM_PATH_LIBFLAC

dnl AM_PATH_LIBFLACPP([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libFLAC++, and define LIBFLACPP_CFLAGS, LIBFLACPP_LIBS, LIBFLACPP_LIBDIR
dnl
AC_DEFUN([AM_PATH_LIBFLACPP],
[dnl 
dnl Get the cflags and libraries
dnl
AC_ARG_WITH(libFLACPP,[  --with-libFLACPP=PFX   Prefix where libFLAC++ is installed (optional)], libFLACPP_prefix="$withval", libFLACPP_prefix="")
AC_ARG_WITH(libFLACPP-libraries,[  --with-libFLACPP-libraries=DIR   Directory where libFLAC++ library is installed (optional)], libFLACPP_libraries="$withval", libFLACPP_libraries="")
AC_ARG_WITH(libFLACPP-includes,[  --with-libFLACPP-includes=DIR   Directory where libFLAC++ header files are installed (optional)], libFLACPP_includes="$withval", libFLACPP_includes="")
AC_ARG_ENABLE(libFLACPPtest, [  --disable-libFLACPPtest       Do not try to compile and run a test libFLAC++ program],, enable_libFLACPPtest=yes)

  if test "x$libFLACPP_libraries" != "x" ; then
    LIBFLACPP_LIBDIR="$libFLACPP_libraries"
  elif test "x$libFLACPP_prefix" != "x" ; then
    LIBFLACPP_LIBDIR="$libFLACPP_prefix/lib"
  elif test "x$prefix" != "xNONE" ; then
    LIBFLACPP_LIBDIR="$libdir"
  fi

  LIBFLACPP_LIBS="-L$LIBFLACPP_LIBDIR -lFLAC++ $LIBFLAC_LIBS"

  if test "x$libFLACPP_includes" != "x" ; then
    LIBFLACPP_CFLAGS="-I$libFLACPP_includes"
  elif test "x$libFLACPP_prefix" != "x" ; then
    LIBFLACPP_CFLAGS="-I$libFLACPP_prefix/include"
  elif test "$prefix" != "xNONE"; then
    LIBFLACPP_CFLAGS=""
  fi

  LIBFLACPP_CFLAGS="$LIBFLACPP_CFLAGS $LIBFLAC_CFLAGS"

  AC_MSG_CHECKING(for libFLAC++)
  no_libFLACPP=""


  if test "x$enable_libFLACPPtest" = "xyes" ; then
    ac_save_CFLAGS="$CFLAGS"
    ac_save_CXXFLAGS="$CXXFLAGS"
    ac_save_LIBS="$LIBS"
    ac_save_LD_LIBRARY_PATH="$LD_LIBRARY_PATH"
    CFLAGS="$CFLAGS $LIBFLACPP_CFLAGS"
    CXXFLAGS="$CXXFLAGS $LIBFLACPP_CFLAGS"
    LIBS="$LIBS $LIBFLACPP_LIBS"
    LD_LIBRARY_PATH="$LIBFLACPP_LIBDIR:$LIBFLAC_LIBDIR:$LD_LIBRARY_PATH"
dnl
dnl Now check if the installed libFLAC++ is sufficiently new.
dnl
      rm -f conf.libFLAC++test
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <FLAC++/decoder.h>

int main ()
{
  system("touch conf.libFLAC++test");
  return 0;
}

],, no_libFLACPP=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       CXXFLAGS="$ac_save_CXXFLAGS"
       LIBS="$ac_save_LIBS"
       LD_LIBRARY_PATH="$ac_save_LD_LIBRARY_PATH"
  fi

  if test "x$no_libFLACPP" = "x" ; then
     AC_MSG_RESULT(yes)
     ifelse([$1], , :, [$1])     
  else
     AC_MSG_RESULT(no)
     if test -f conf.libFLAC++test ; then
       :
     else
       echo "*** Could not run libFLAC++ test program, checking why..."
       CFLAGS="$CFLAGS $LIBFLACPP_CFLAGS"
       CXXFLAGS="$CXXFLAGS $LIBFLACPP_CFLAGS"
       LIBS="$LIBS $LIBFLACPP_LIBS"
       LD_LIBRARY_PATH="$LIBFLACPP_LIBDIR:$LIBFLAC_LIBDIR:$LD_LIBRARY_PATH"
       AC_TRY_LINK([
#include <stdio.h>
#include <FLAC++/decoder.h>
],     [ return 0; ],
       [ echo "*** The test program compiled, but did not run. This usually means"
       echo "*** that the run-time linker is not finding libFLAC++ or finding the wrong"
       echo "*** version of libFLAC++. If it is not finding libFLAC++, you'll need to set your"
       echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
       echo "*** to the installed location  Also, make sure you have run ldconfig if that"
       echo "*** is required on your system"
       echo "***"
       echo "*** If you have an old version installed, it is best to remove it, although"
       echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
       [ echo "*** The test program failed to compile or link. See the file config.log for the"
       echo "*** exact error that occured. This usually means libFLAC++ was incorrectly installed"
       echo "*** or that you have moved libFLAC++ since it was installed. In the latter case, you"
       echo "*** may want to edit the libFLAC++-config script: $LIBFLACPP_CONFIG" ])
       CFLAGS="$ac_save_CFLAGS"
       CXXFLAGS="$ac_save_CXXFLAGS"
       LIBS="$ac_save_LIBS"
       LD_LIBRARY_PATH="$ac_save_LD_LIBRARY_PATH"
     fi
     LIBFLACPP_CFLAGS=""
     LIBFLACPP_LIBDIR=""
     LIBFLACPP_LIBS=""
     ifelse([$2], , :, [$2])
  fi
  AC_SUBST(LIBFLACPP_CFLAGS)
  AC_SUBST(LIBFLACPP_LIBDIR)
  AC_SUBST(LIBFLACPP_LIBS)
  rm -f conf.libFLAC++test
])
