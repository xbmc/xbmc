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
