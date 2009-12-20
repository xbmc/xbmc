## Several iconv tests.  This file is in public domain.
## N.B.: Following test relies on files iconvcap.c and has to compile and
##       run it.
## Defines:
## HAVE_GOOD_ICONV when have iconv_open(), iconv.h and it's usable
## ICONV_IS_TRANSITIVE when it seems no triangulation is needed.
## CONVERTER_LIBS (adds library when needed)
## Creates file iconvenc.h (to be included only if defined(HAVE_ICONV))---it
##   would be cleaner to put all the definitions to config.h, but then
##   (1) we would have to check for every charset separately
##   (2) config.h would get polluted by dozens of charset aliases
## Note we need tools/iconvenc.null
AC_DEFUN([ye_CHECK_FUNC_ICONV_USABLE],
[AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_HEADER_STDC])dnl
AC_REQUIRE([AC_C_CONST])dnl
dnl

dnl Use standard iconv test
AM_ICONV
CONVERTER_LIBS="$CONVERTER_LIBS $LIBICONV"

dnl Compile iconvcap.c and run it to determine what encodings iconv actually
dnl knows an under what names. This is not needed with GNU iconv. HAVE_ICONV
dnl is finally defined _only_ if iconv prove at least some minimal reasonable
dnl capabilities.
libiconv_ok="$am_cv_func_iconv"
if test "$libiconv_ok" = yes; then
  AC_MSG_CHECKING([whether iconv implementation is usable])
  if $CC -o iconvcap$ac_exeext $CFLAGS $CPPFLAGS $LDFLAGS $srcdir/iconvcap.c $LIBS $CONVERTER_LIBS 1>&5 2>&5 && test -s ./iconvcap$ac_exeext 2>&5; then
    if ./iconvcap 2>&5 >iconvenc.h; then
      libiconv_ok=yes
    else
      libiconv_ok=no
    fi
  else
    libiconv_ok=no
  fi
  AC_MSG_RESULT($libiconv_ok)
  if test "$libiconv_ok" = yes; then
    AC_DEFINE(HAVE_GOOD_ICONV,1,[Define if you have the UNIX98 iconv functions.])
    AC_CACHE_CHECK([whether iconv is transitive],
      yeti_cv_lib_c_iconv_transitive,
      if ./iconvcap iconvenc.h 2>&5; then
        yeti_cv_lib_c_iconv_transitive=yes
      else
        yeti_cv_lib_c_iconv_transitive=no
      fi)
    if test "$yeti_cv_lib_c_iconv_transitive" = yes; then
      AC_DEFINE(ICONV_IS_TRANSITIVE,1,[Define if iconv is transitive.])
    else
      AC_MSG_WARN([A non-transitive but otherwise usable iconv implementation
                    was found.  This beast was believed to be mythical.
                    Please send your system specs to the maintainer.])
    fi
  else
    echo >iconvenc.h
  fi
else
  libiconv_ok=no
fi

if test "$libiconv_ok" != "yes"; then
  cat $srcdir/tools/iconvenc.null >iconvenc.h
fi])

