# stdio_h.m4 serial 14
dnl Copyright (C) 2007-2008 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_STDIO_H],
[
  AC_REQUIRE([gl_STDIO_H_DEFAULTS])
  gl_CHECK_NEXT_HEADERS([stdio.h])
  dnl No need to create extra modules for these functions. Everyone who uses
  dnl <stdio.h> likely needs them.
  GNULIB_FPRINTF=1
  GNULIB_PRINTF=1
  GNULIB_VFPRINTF=1
  GNULIB_VPRINTF=1
  GNULIB_FPUTC=1
  GNULIB_PUTC=1
  GNULIB_PUTCHAR=1
  GNULIB_FPUTS=1
  GNULIB_PUTS=1
  GNULIB_FWRITE=1
  dnl This ifdef is just an optimization, to avoid performing a configure
  dnl check whose result is not used. It does not make the test of
  dnl GNULIB_STDIO_H_SIGPIPE or GNULIB_SIGPIPE redundant.
  m4_ifdef([gl_SIGNAL_SIGPIPE], [
    gl_SIGNAL_SIGPIPE
    if test $gl_cv_header_signal_h_SIGPIPE != yes; then
      REPLACE_STDIO_WRITE_FUNCS=1
      AC_LIBOBJ([stdio-write])
    fi
  ])
])

AC_DEFUN([gl_STDIO_MODULE_INDICATOR],
[
  dnl Use AC_REQUIRE here, so that the default settings are expanded once only.
  AC_REQUIRE([gl_STDIO_H_DEFAULTS])
  GNULIB_[]m4_translit([$1],[abcdefghijklmnopqrstuvwxyz./-],[ABCDEFGHIJKLMNOPQRSTUVWXYZ___])=1
])

AC_DEFUN([gl_STDIO_H_DEFAULTS],
[
  GNULIB_FPRINTF=0;              AC_SUBST([GNULIB_FPRINTF])
  GNULIB_FPRINTF_POSIX=0;        AC_SUBST([GNULIB_FPRINTF_POSIX])
  GNULIB_PRINTF=0;               AC_SUBST([GNULIB_PRINTF])
  GNULIB_PRINTF_POSIX=0;         AC_SUBST([GNULIB_PRINTF_POSIX])
  GNULIB_SNPRINTF=0;             AC_SUBST([GNULIB_SNPRINTF])
  GNULIB_SPRINTF_POSIX=0;        AC_SUBST([GNULIB_SPRINTF_POSIX])
  GNULIB_VFPRINTF=0;             AC_SUBST([GNULIB_VFPRINTF])
  GNULIB_VFPRINTF_POSIX=0;       AC_SUBST([GNULIB_VFPRINTF_POSIX])
  GNULIB_VPRINTF=0;              AC_SUBST([GNULIB_VPRINTF])
  GNULIB_VPRINTF_POSIX=0;        AC_SUBST([GNULIB_VPRINTF_POSIX])
  GNULIB_VSNPRINTF=0;            AC_SUBST([GNULIB_VSNPRINTF])
  GNULIB_VSPRINTF_POSIX=0;       AC_SUBST([GNULIB_VSPRINTF_POSIX])
  GNULIB_VASPRINTF=0;            AC_SUBST([GNULIB_VASPRINTF])
  GNULIB_OBSTACK_PRINTF=0;       AC_SUBST([GNULIB_OBSTACK_PRINTF])
  GNULIB_OBSTACK_PRINTF_POSIX=0; AC_SUBST([GNULIB_OBSTACK_PRINTF_POSIX])
  GNULIB_FOPEN=0;                AC_SUBST([GNULIB_FOPEN])
  GNULIB_FREOPEN=0;              AC_SUBST([GNULIB_FREOPEN])
  GNULIB_FSEEK=0;                AC_SUBST([GNULIB_FSEEK])
  GNULIB_FSEEKO=0;               AC_SUBST([GNULIB_FSEEKO])
  GNULIB_FTELL=0;                AC_SUBST([GNULIB_FTELL])
  GNULIB_FTELLO=0;               AC_SUBST([GNULIB_FTELLO])
  GNULIB_FFLUSH=0;               AC_SUBST([GNULIB_FFLUSH])
  GNULIB_FCLOSE=0;               AC_SUBST([GNULIB_FCLOSE])
  GNULIB_FPUTC=0;                AC_SUBST([GNULIB_FPUTC])
  GNULIB_PUTC=0;                 AC_SUBST([GNULIB_PUTC])
  GNULIB_PUTCHAR=0;              AC_SUBST([GNULIB_PUTCHAR])
  GNULIB_FPUTS=0;                AC_SUBST([GNULIB_FPUTS])
  GNULIB_PUTS=0;                 AC_SUBST([GNULIB_PUTS])
  GNULIB_FWRITE=0;               AC_SUBST([GNULIB_FWRITE])
  GNULIB_GETDELIM=0;             AC_SUBST([GNULIB_GETDELIM])
  GNULIB_GETLINE=0;              AC_SUBST([GNULIB_GETLINE])
  GNULIB_PERROR=0;               AC_SUBST([GNULIB_PERROR])
  GNULIB_STDIO_H_SIGPIPE=0;      AC_SUBST([GNULIB_STDIO_H_SIGPIPE])
  dnl Assume proper GNU behavior unless another module says otherwise.
  REPLACE_STDIO_WRITE_FUNCS=0;   AC_SUBST([REPLACE_STDIO_WRITE_FUNCS])
  REPLACE_FPRINTF=0;             AC_SUBST([REPLACE_FPRINTF])
  REPLACE_VFPRINTF=0;            AC_SUBST([REPLACE_VFPRINTF])
  REPLACE_PRINTF=0;              AC_SUBST([REPLACE_PRINTF])
  REPLACE_VPRINTF=0;             AC_SUBST([REPLACE_VPRINTF])
  REPLACE_SNPRINTF=0;            AC_SUBST([REPLACE_SNPRINTF])
  HAVE_DECL_SNPRINTF=1;          AC_SUBST([HAVE_DECL_SNPRINTF])
  REPLACE_VSNPRINTF=0;           AC_SUBST([REPLACE_VSNPRINTF])
  HAVE_DECL_VSNPRINTF=1;         AC_SUBST([HAVE_DECL_VSNPRINTF])
  REPLACE_SPRINTF=0;             AC_SUBST([REPLACE_SPRINTF])
  REPLACE_VSPRINTF=0;            AC_SUBST([REPLACE_VSPRINTF])
  HAVE_VASPRINTF=1;              AC_SUBST([HAVE_VASPRINTF])
  REPLACE_VASPRINTF=0;           AC_SUBST([REPLACE_VASPRINTF])
  HAVE_DECL_OBSTACK_PRINTF=1;    AC_SUBST([HAVE_DECL_OBSTACK_PRINTF])
  REPLACE_OBSTACK_PRINTF=0;      AC_SUBST([REPLACE_OBSTACK_PRINTF])
  REPLACE_FOPEN=0;               AC_SUBST([REPLACE_FOPEN])
  REPLACE_FREOPEN=0;             AC_SUBST([REPLACE_FREOPEN])
  HAVE_FSEEKO=1;                 AC_SUBST([HAVE_FSEEKO])
  REPLACE_FSEEKO=0;              AC_SUBST([REPLACE_FSEEKO])
  REPLACE_FSEEK=0;               AC_SUBST([REPLACE_FSEEK])
  HAVE_FTELLO=1;                 AC_SUBST([HAVE_FTELLO])
  REPLACE_FTELLO=0;              AC_SUBST([REPLACE_FTELLO])
  REPLACE_FTELL=0;               AC_SUBST([REPLACE_FTELL])
  REPLACE_FFLUSH=0;              AC_SUBST([REPLACE_FFLUSH])
  REPLACE_FCLOSE=0;              AC_SUBST([REPLACE_FCLOSE])
  HAVE_DECL_GETDELIM=1;          AC_SUBST([HAVE_DECL_GETDELIM])
  HAVE_DECL_GETLINE=1;           AC_SUBST([HAVE_DECL_GETLINE])
  REPLACE_GETLINE=0;             AC_SUBST([REPLACE_GETLINE])
  REPLACE_PERROR=0;              AC_SUBST([REPLACE_PERROR])
])

dnl Code shared by fseeko and ftello.  Determine if large files are supported,
dnl but stdin does not start as a large file by default.
AC_DEFUN([gl_STDIN_LARGE_OFFSET],
  [
    AC_CACHE_CHECK([whether stdin defaults to large file offsets],
      [gl_cv_var_stdin_large_offset],
      [AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>]],
[[#if defined __SL64 && defined __SCLE /* cygwin */
  /* Cygwin 1.5.24 and earlier fail to put stdin in 64-bit mode, making
     fseeko/ftello needlessly fail.  This bug was fixed in 1.5.25, and
     it is easier to do a version check than building a runtime test.  */
# include <cygwin/version.h>
# if CYGWIN_VERSION_DLL_COMBINED < CYGWIN_VERSION_DLL_MAKE_COMBINED (1005, 25)
  choke me
# endif
#endif]])],
	[gl_cv_var_stdin_large_offset=yes],
	[gl_cv_var_stdin_large_offset=no])])
])
