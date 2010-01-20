## GNU recode library test.  This file is in public domain.
## Defines:
## HAVE_LIBRECODE when have recode_new_outer() and recode.h
## LIBS (adds library when needed)
AC_DEFUN([ye_CHECK_LIB_RECODE],
[AC_REQUIRE([AC_PROG_CC])dnl
dnl

dnl Test for librecode.
dnl Braindead librecode depends on symbol program_name defined in main program
dnl this makes the test braindead too.  In header file test, we have to use
dnl a whole load of fakes, since it depends e.g. on bool and FILE defined.
AC_ARG_WITH(librecode,
  [  --with-librecode@<:@=DIR@:>@  look for librecode in DIR/lib and DIR/include @<:@auto@:>@],
  [case "$withval" in
    yes|auto) WANT_LIBRECODE=1 ;;
    no)  WANT_LIBRECODE=0 ;;
    *)   WANT_LIBRECODE=1 ; yeti_librecode_CPPFLAGS="-I$withval/include" ; yeti_librecode_LDFLAGS="-L$withval/lib" ;;
    esac],
  [WANT_LIBRECODE=1])

if test "$WANT_LIBRECODE" = 1; then
  yeti_save_LIBS="$LIBS"
  yeti_save_CPPFLAGS="$CPPFLAGS"
  yeti_save_LDFLAGS="$LDFLAGS"
  LIBS="$LIBS -lrecode"
  CPPFLAGS="$CPPFLAGS $yeti_librecode_CPPFLAGS"
  LDFLAGS="$LDFLAGS $yeti_librecode_LDFLAGS"
  AC_CACHE_CHECK([for recode_new_outer in librecode],
    yeti_cv_lib_recode_new_outer,
    AC_TRY_LINK([char* program_name = "";],
      [recode_new_outer(0);],
      yeti_cv_lib_recode_new_outer=yes,
      yeti_cv_lib_recode_new_outer=no))
  librecode_ok="$yeti_cv_lib_recode_new_outer";
  if test "$librecode_ok" = yes; then
    AC_CHECK_HEADER(recode.h,
      librecode_ok=yes,
      librecode_ok=no,
      [#define bool int
       #define size_t int
       #define FILE void])
  fi
  if test "$librecode_ok" = yes; then
    AC_CHECK_HEADER(recodext.h,
      librecode_ok=yes,
      librecode_ok=no,
      [#define bool int
       #define size_t int
       #define FILE void])
  fi
  if test "$librecode_ok" = yes; then
    AC_DEFINE(HAVE_LIBRECODE,1,[Define if you have the recode library (-lrecode).])
    CONVERTER_LIBS="$CONVERTER_LIBS -lrecode"
  fi
  LIBS="$yeti_save_LIBS"
else
  librecode_ok=no
fi

if test "$librecode_ok" != "yes"; then
  if test "$WANT_LIBRECODE" = 1; then
    CPPFLAGS="$yeti_save_CPPFLAGS"
    LDFLAGS="$yeti_save_LDFLAGS"
  fi
fi])

