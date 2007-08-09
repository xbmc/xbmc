#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT(libprojectM, 0.98, psperl@sourceforge.net)
AC_CONFIG_SRCDIR([src/projectM.cc])
AC_CONFIG_HEADER([config.h])
AC_PROG_LIBTOOL

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_INSTALL

AC_PROG_CC
AC_PROG_CXX

AX_CHECK_GL

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS([malloc.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([floor gettimeofday memset pow setenv sqrt strcasecmp strchr strstr strtol strverscmp])

PKG_CHECK_MODULES(FTGL, ftgl >= 2.0.0, [libft="yes"], [libft="no"])
if test x$libft = xno; then
	AC_MSG_ERROR([*** ftgl not found.
			libftgl is needed to build this package./])
fi
AC_SUBST(FTGL_LIBS)
AC_SUBST(FTGL_CFLAGS)



AC_CONFIG_FILES([Makefile
                 src/Makefile
                 libprojectM.pc])
AC_OUTPUT
