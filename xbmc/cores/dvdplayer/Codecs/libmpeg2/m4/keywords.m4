dnl AC_C_RESTRICT
dnl Do nothing if the compiler accepts the restrict keyword.
dnl Otherwise define restrict to __restrict__ or __restrict if one of
dnl those work, otherwise define restrict to be empty.
AC_DEFUN([AC_C_RESTRICT],
    [AC_MSG_CHECKING([for restrict])
    ac_cv_c_restrict=no
    for ac_kw in restrict __restrict__ __restrict; do
        AC_TRY_COMPILE([],[char * $ac_kw p;],[ac_cv_c_restrict=$ac_kw; break])
    done
    AC_MSG_RESULT([$ac_cv_c_restrict])
    case $ac_cv_c_restrict in
        restrict) ;;
        no)     AC_DEFINE([restrict],,
                    [Define as `__restrict' if that's what the C compiler calls
                    it, or to nothing if it is not supported.]) ;;
        *)      AC_DEFINE_UNQUOTED([restrict],$ac_cv_c_restrict) ;;
    esac])

dnl AC_C_BUILTIN_EXPECT
dnl Check whether compiler understands __builtin_expect.
AC_DEFUN([AC_C_BUILTIN_EXPECT],
    [AC_CACHE_CHECK([for __builtin_expect],[ac_cv_builtin_expect],
        [cat > conftest.c <<EOF
#line __oline__ "configure"
int foo (int a)
{
    a = __builtin_expect (a, 10);
    return a == 10 ? 0 : 1;
}
EOF
        if AC_TRY_COMMAND([${CC-cc} $CFLAGS -nostdlib -nostartfiles
            -o conftest conftest.c -lgcc >&AC_FD_CC]); then
            ac_cv_builtin_expect=yes
        else
            ac_cv_builtin_expect=no
        fi
        rm -f conftest*])
    if test x"$ac_cv_builtin_expect" = x"yes"; then
        AC_DEFINE(HAVE_BUILTIN_EXPECT,,
            [Define if you have the `__builtin_expect' function.])
    fi])

dnl AC_C_ALWAYS_INLINE
dnl Define inline to something appropriate, including the new always_inline
dnl attribute from gcc 3.1
AC_DEFUN([AC_C_ALWAYS_INLINE],
    [AC_C_INLINE
    if test x"$GCC" = x"yes" -a x"$ac_cv_c_inline" = x"inline"; then
        AC_MSG_CHECKING([for always_inline])
        SAVE_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS -Wall -Werror"
        AC_TRY_COMPILE([],
            [__attribute__ ((__always_inline__)) void f (void);
            #ifdef __cplusplus
            42 = 42;    // obviously illegal - we want c++ to fail here
            #endif],
            [ac_cv_always_inline=yes],[ac_cv_always_inline=no])
        CFLAGS="$SAVE_CFLAGS"
        AC_MSG_RESULT([$ac_cv_always_inline])
        if test x"$ac_cv_always_inline" = x"yes"; then
            AC_DEFINE_UNQUOTED([inline],[__attribute__ ((__always_inline__))])
        fi
    fi])

dnl AC_C_ATTRIBUTE_ALIGNED
dnl define ATTRIBUTE_ALIGNED_MAX to the maximum alignment if this is supported
AC_DEFUN([AC_C_ATTRIBUTE_ALIGNED],
    [SAV_CFLAGS=$CFLAGS;
    if test x"$GCC" = xyes; then CFLAGS="$CFLAGS -Werror"; fi
    AC_CACHE_CHECK([__attribute__ ((aligned ())) support],
        [ac_cv_c_attribute_aligned],
        [ac_cv_c_attribute_aligned=0
        for ac_cv_c_attr_align_try in 2 4 8 16 32 64; do
            AC_TRY_COMPILE([],
                [static struct s {
                    char a;
                    char b __attribute__ ((aligned($ac_cv_c_attr_align_try)));
                } S = {0, 0};
                switch (1) {
                    case 0:
                    case (long)(&((struct s *)0)->b) == $ac_cv_c_attr_align_try:
                        return 0;
                }
                return (long)&S;],
                [ac_cv_c_attribute_aligned=$ac_cv_c_attr_align_try])
        done])
    if test x"$ac_cv_c_attribute_aligned" != x"0"; then
        AC_DEFINE_UNQUOTED([ATTRIBUTE_ALIGNED_MAX],
            [$ac_cv_c_attribute_aligned],[maximum supported data alignment])
    fi
    CFLAGS=$SAV_CFLAGS])

