# Configure paths for libpopt, based on m4's part of gnome
# (c) 2002, 2004 Herbert Valerio Riedel

dnl AM_PATH_LIBPOPT([ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
dnl Test for libpopt, sets LIBPOPT_{CFLAGS,LIBS}
dnl alas there's no easy way to check the version available

AC_DEFUN([AM_PATH_LIBPOPT], [

AC_ARG_WITH(libpopt-prefix,[  --with-libpopt-prefix=PFX    Prefix where libpopt is installed (optional)],
	    libpopt_prefix="$withval", libpopt_prefix="")

  if test x$libpopt_prefix != x ; then
    LIBPOPT_CFLAGS="-I$libpopt_prefix/include"
    LIBPOPT_LIBS="-L$libpopt_prefix/lib -lpopt"
  else
    LIBPOPT_CFLAGS=""
    LIBPOPT_LIBS="-lpopt"
  fi
  
  AC_MSG_CHECKING(for libpopt library)

dnl save CFLAGS and LIBS
  ac_save_CFLAGS="$CFLAGS"
  ac_save_LIBS="$LIBS"
  CFLAGS="$CFLAGS $LIBPOPT_CFLAGS"
  LIBS="$LIBPOPT_LIBS $LIBS"
  
dnl now check whether the installed libpopt is usable
  rm -f conf.glibtest
  AC_TRY_RUN([
#include <popt.h>

int
main(int argc, const char *argv[])
{
  char *s;
  const struct poptOption options[] = {
    POPT_AUTOHELP
    { NULL, 0, 0, NULL, 0, NULL, NULL },
    {"foo", 'f', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &s, NULL, 
     "test doc", "FILE"},
  };

  poptContext context = poptGetContext("popt-test", argc, argv, options, 0);
  poptSetOtherOptionHelp (context, "[OPTION...] <argument...>");
  poptGetNextOpt(context);

  return 0;
}
],, no_popt=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
  CFLAGS="$ac_save_CFLAGS"
  LIBS="$ac_save_LIBS"

dnl handle test result
  if test "x$no_popt" = x ; then
    AC_MSG_RESULT(yes)
    ifelse([$1], , :, [$1])
  else
    AC_MSG_RESULT(no or not new enough - need libpopt 1.7 or greater)
    LIBPOPT_CFLAGS=""
    LIBPOPT_LIBS=""
    ifelse([$2], , :, [$2])
  fi

  AC_SUBST(LIBPOPT_CFLAGS)
  AC_SUBST(LIBPOPT_LIBS)

])

dnl EOF
