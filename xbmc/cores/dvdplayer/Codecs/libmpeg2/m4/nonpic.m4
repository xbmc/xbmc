dnl AC_LIBTOOL_NON_PIC ([ACTION-IF-WORKS], [ACTION-IF-FAILS])
dnl check for nonbuggy libtool -prefer-non-pic
AC_DEFUN([AC_LIBTOOL_NON_PIC],
    [AC_MSG_CHECKING([if libtool supports -prefer-non-pic flag])
    mkdir ac_test_libtool; cd ac_test_libtool; ac_cv_libtool_non_pic=no
    echo "int g (int i); static int h (int i) {return g (i);} int f (int i) {return h (i);}" >f.c
    echo "int (* hook) (int) = 0; int g (int i) {if (hook) i = hook (i); return i + 1;}" >g.c
    ../libtool --mode=compile $CC $CFLAGS -prefer-non-pic \
                -c f.c >/dev/null 2>&1 && \
        ../libtool --mode=compile $CC $CFLAGS -prefer-non-pic \
                -c g.c >/dev/null 2>&1 && \
        ../libtool --mode=link $CC $CFLAGS -prefer-non-pic -o libfoo.la \
                -rpath / f.lo g.lo >/dev/null 2>&1 &&
        ac_cv_libtool_non_pic=yes
    cd ..; rm -fr ac_test_libtool; AC_MSG_RESULT([$ac_cv_libtool_non_pic])
    if test x"$ac_cv_libtool_non_pic" = x"yes"; then
        ifelse([$1],[],[:],[$1])
    else
        ifelse([$2],[],[:],[$2])
    fi])

