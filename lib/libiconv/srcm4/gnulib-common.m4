# gnulib-common.m4 serial 11
dnl Copyright (C) 2007-2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# gl_COMMON
# is expanded unconditionally through gnulib-tool magic.
AC_DEFUN([gl_COMMON], [
  dnl Use AC_REQUIRE here, so that the code is expanded once only.
  AC_REQUIRE([gl_00GNULIB])
  AC_REQUIRE([gl_COMMON_BODY])
])
AC_DEFUN([gl_COMMON_BODY], [
  AH_VERBATIM([isoc99_inline],
[/* Work around a bug in Apple GCC 4.0.1 build 5465: In C99 mode, it supports
   the ISO C 99 semantics of 'extern inline' (unlike the GNU C semantics of
   earlier versions), but does not display it by setting __GNUC_STDC_INLINE__.
   __APPLE__ && __MACH__ test for MacOS X.
   __APPLE_CC__ tests for the Apple compiler and its version.
   __STDC_VERSION__ tests for the C99 mode.  */
#if defined __APPLE__ && defined __MACH__ && __APPLE_CC__ >= 5465 && !defined __cplusplus && __STDC_VERSION__ >= 199901L && !defined __GNUC_STDC_INLINE__
# define __GNUC_STDC_INLINE__ 1
#endif])
  AH_VERBATIM([unused_parameter],
[/* Define as a marker that can be attached to function parameter declarations
   for parameters that are not used.  This helps to reduce warnings, such as
   from GCC -Wunused-parameter.  */
#if __GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
# define _UNUSED_PARAMETER_ __attribute__ ((__unused__))
#else
# define _UNUSED_PARAMETER_
#endif
])
])

# gl_MODULE_INDICATOR([modulename])
# defines a C macro indicating the presence of the given module.
AC_DEFUN([gl_MODULE_INDICATOR],
[
  AC_DEFINE([GNULIB_]translit([$1],[abcdefghijklmnopqrstuvwxyz./-],[ABCDEFGHIJKLMNOPQRSTUVWXYZ___]), [1],
    [Define to 1 when using the gnulib module ]$1[.])
])

# m4_foreach_w
# is a backport of autoconf-2.59c's m4_foreach_w.
# Remove this macro when we can assume autoconf >= 2.60.
m4_ifndef([m4_foreach_w],
  [m4_define([m4_foreach_w],
    [m4_foreach([$1], m4_split(m4_normalize([$2]), [ ]), [$3])])])

# AC_PROG_MKDIR_P
# is a backport of autoconf-2.60's AC_PROG_MKDIR_P.
# Remove this macro when we can assume autoconf >= 2.60.
m4_ifdef([AC_PROG_MKDIR_P], [], [
  AC_DEFUN_ONCE([AC_PROG_MKDIR_P],
    [AC_REQUIRE([AM_PROG_MKDIR_P])dnl defined by automake
     MKDIR_P='$(mkdir_p)'
     AC_SUBST([MKDIR_P])])])

# AC_C_RESTRICT
# This definition overrides the AC_C_RESTRICT macro from autoconf 2.60..2.61,
# so that mixed use of GNU C and GNU C++ and mixed use of Sun C and Sun C++
# works.
# This definition can be removed once autoconf >= 2.62 can be assumed.
AC_DEFUN([AC_C_RESTRICT],
[AC_CACHE_CHECK([for C/C++ restrict keyword], [ac_cv_c_restrict],
  [ac_cv_c_restrict=no
   # The order here caters to the fact that C++ does not require restrict.
   for ac_kw in __restrict __restrict__ _Restrict restrict; do
     AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
      [[typedef int * int_ptr;
	int foo (int_ptr $ac_kw ip) {
	return ip[0];
       }]],
      [[int s[1];
	int * $ac_kw t = s;
	t[0] = 0;
	return foo(t)]])],
      [ac_cv_c_restrict=$ac_kw])
     test "$ac_cv_c_restrict" != no && break
   done
  ])
 AH_VERBATIM([restrict],
[/* Define to the equivalent of the C99 'restrict' keyword, or to
   nothing if this is not supported.  Do not define if restrict is
   supported directly.  */
#undef restrict
/* Work around a bug in Sun C++: it does not support _Restrict, even
   though the corresponding Sun C compiler does, which causes
   "#define restrict _Restrict" in the previous line.  Perhaps some future
   version of Sun C++ will work with _Restrict; if so, it'll probably
   define __RESTRICT, just as Sun C does.  */
#if defined __SUNPRO_CC && !defined __RESTRICT
# define _Restrict
#endif])
 case $ac_cv_c_restrict in
   restrict) ;;
   no) AC_DEFINE([restrict], []) ;;
   *)  AC_DEFINE_UNQUOTED([restrict], [$ac_cv_c_restrict]) ;;
 esac
])

# gl_BIGENDIAN
# is like AC_C_BIGENDIAN, except that it can be AC_REQUIREd.
# Note that AC_REQUIRE([AC_C_BIGENDIAN]) does not work reliably because some
# macros invoke AC_C_BIGENDIAN with arguments.
AC_DEFUN([gl_BIGENDIAN],
[
  AC_C_BIGENDIAN
])

# gl_CACHE_VAL_SILENT(cache-id, command-to-set-it)
# is like AC_CACHE_VAL(cache-id, command-to-set-it), except that it does not
# output a spurious "(cached)" mark in the midst of other configure output.
# This macro should be used instead of AC_CACHE_VAL when it is not surrounded
# by an AC_MSG_CHECKING/AC_MSG_RESULT pair.
AC_DEFUN([gl_CACHE_VAL_SILENT],
[
  saved_as_echo_n="$as_echo_n"
  as_echo_n=':'
  AC_CACHE_VAL([$1], [$2])
  as_echo_n="$saved_as_echo_n"
])
