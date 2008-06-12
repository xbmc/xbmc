## Check for math library.  Taken from libtool.m4.
## Defines:
## LIBM
## LIBS (adds library when needed)
AC_DEFUN([ye_CHECK_LIBM],
[AC_REQUIRE([AC_CANONICAL_HOST])dnl

case "$target" in
NONE) yeti_libm_target="$host" ;;
*) yeti_libm_target="$target" ;;
esac

LIBM=
case "$yeti_libm_target" in
*-*-beos* | *-*-cygwin*)
  # These system don't have libm
  ;;
*-ncr-sysv4.3*)
  AC_CHECK_LIB(mw, _mwvalidcheckl, LIBM="-lmw")
  AC_CHECK_LIB(m, sqrt, LIBM="$LIBM -lm")
  ;;
*)
  AC_CHECK_LIB(m, sqrt, LIBM="-lm")
  ;;
esac
AC_SUBST(LIBM)
LIBS="$LIBS $LIBM"])
