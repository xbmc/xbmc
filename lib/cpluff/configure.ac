dnl Process this file with autoconf to produce a configure script.

dnl Copyright 2007 Johannes Lehtinen
dnl This configure.ac script is free software; Johannes Lehtinen gives
dnl unlimited permission to copy, distribute and modify it.

dnl Version information
dnl -------------------

dnl Version information
define(CP_M4_VERSION_MAJOR, [0])
define(CP_M4_VERSION_MINOR, [1])
define(CP_M4_VERSION_REV, [3])
define(CP_M4_RELEASE_VERSION, CP_M4_VERSION_MAJOR.CP_M4_VERSION_MINOR.CP_M4_VERSION_REV)

dnl Backwards compatibility information
define(CP_M4_ABI_COMPATIBILITY, [0.1])

dnl Library version information
define(CP_M4_C_LIB_VERSION, [0:3:0])
define(CP_M4_CXX_LIB_VERSION, [0:0:0])

# Autoconf initialization
# -----------------------
AC_INIT([C-Pluff], CP_M4_RELEASE_VERSION, [johannes.lehtinen@iki.fi], [cpluff])
AC_COPYRIGHT([Copyright 2007 Johannes Lehtinen
This configure script is free software; Johannes Lehtinen gives unlimited
permission to copy, distribute and modify it.])
AC_CONFIG_SRCDIR([libcpluff/cpluff.h])
AC_CONFIG_AUX_DIR([auxliary])
AC_CONFIG_HEADERS([config.h])

# Version information
# -------------------
CP_VERSION_MAJOR=CP_M4_VERSION_MAJOR
CP_VERSION_MINOR=CP_M4_VERSION_MINOR
AC_DEFINE([CP_ABI_COMPATIBILITY], "CP_M4_ABI_COMPATIBILITY", [The earliest ABI compatible version or undefined])
CP_C_LIB_VERSION=CP_M4_C_LIB_VERSION
CP_CXX_LIB_VERSION=CP_M4_CXX_LIB_VERSION
AC_SUBST([CP_VERSION_MAJOR])
AC_SUBST([CP_VERSION_MINOR])
AC_SUBST([CP_C_LIB_VERSION])
AC_SUBST([CP_CXX_LIB_VERSION])

# Automake initialization
# -----------------------
AM_INIT_AUTOMAKE([foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.16.1])

# C/C++ compiler settings
# -------------------
AC_LANG([C])
AC_PROG_CC
if test -z "$CC"; then
  AC_MSG_ERROR([C compiler was not found (required)])
fi
AC_PROG_CC_C_O
AC_C_CONST
AC_LIBTOOL_DLOPEN
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

# Other programs
# --------------
AC_PROG_LN_S

# For config.h
CPPFLAGS="$CPPFLAGS -I\$(top_builddir)"

# For cpluff.h and cpluffdef.h
CPPFLAGS="$CPPFLAGS -I\$(top_builddir)/libcpluff -I\$(top_srcdir)/libcpluff"

# Substitute variables for libcpluff and non-libcpluff components
AC_SUBST([LIBS_LIBCPLUFF])
AC_SUBST([LIBS_OTHER])

# Define shared library extension
# -------------------------------
AC_DEFINE_UNQUOTED([CP_SHREXT], ["$shrext_cmds"], [Shared library extension])

# Thread support checks
# ---------------------

# Check the options
AC_ARG_ENABLE([threads], AS_HELP_STRING([--enable-threads@<:@=TYPE@:>@], [enable multi-threading support (supported TYPEs are "Posix" and "Windows", default is to check support in this order)]))
case "$enable_threads" in
  yes)
  	# Use the default auto detection
  	enable_threads=''
  	;;
  ''|no|Posix|Windows)
  	# Ok, legal value, no action needed
  	;;
  *)
  	AC_MSG_ERROR([$enable_threads threads not supported])
  	;;
esac

# Only check for different thread types if threading not disabled
cp_threads=''
if test "$enable_threads" != no; then

  # Check for Posix thread support
  if test -z "$cp_threads" && \
    ( test -z "$enable_threads" || test "$enable_threads" = Posix ); then
    AC_MSG_CHECKING([for Posix threads])
    AC_LINK_IFELSE(
[AC_LANG_SOURCE([#include <pthread.h>

int main(int argc, char *argv[]) {
	pthread_mutex_t mutex;
	
	pthread_mutex_init(&mutex, NULL);
	return 0;
}
])], [AC_MSG_RESULT([yes])
cp_threads=Posix], AC_MSG_RESULT([no]))
  fi
  
  # Check for Windows thread suppport
  if test -z "$cp_threads" && \
    ( test -z "$enable_threads" || test "$enable_threads" = Windows ); then
    AC_MSG_CHECKING([for Windows threads])
    AC_LINK_IFELSE(
[AC_LANG_SOURCE([#include <windows.h>

int main(int argc, char *argv[]) {
  CreateMutex(NULL, FALSE, NULL);
  return 0;
}
])], [AC_MSG_RESULT([yes])
cp_threads=Windows], AC_MSG_RESULT([no]))
  fi
  
  # Check if we got the desired thread support
  if test -n "$enable_threads" && test "$enable_threads" != "$cp_threads"; then
    AC_MSG_ERROR([$enable_threads threads not detected])
  fi
  
fi

if test -z "$cp_threads" && test "$enable_threads" != no; then
  AC_MSG_WARN([multi-threading support not detected])
fi

if test -n "$cp_threads"; then
  	AC_DEFINE_UNQUOTED([CP_THREADS], ["$cp_threads"], [Multi-threading support type])
fi
AM_CONDITIONAL([POSIX_THREADS], test "$cp_threads" = Posix)
AM_CONDITIONAL([WINDOWS_THREADS], test "$cp_threads" = Windows)

# Check for the dlopen mechanism (Posix dlopen or GNU Libtool libltdl)
# --------------------------------------------------------------------
AC_ARG_WITH([dlopen],
  AS_HELP_STRING([--with-dlopen],
    [use the Posix dlopen facility]))
AC_ARG_WITH([libltdl],
  AS_HELP_STRING([--with-libltdl],
    [use the GNU Libtool libltdl]))
if test "$with_dlopen" = yes && test "$with_libltdl" = yes; then
  AC_MSG_ERROR([Can not use both Posix dlopen and GNU Libtool libltdl])
fi
dlmechanism=none
if test "$with_dlopen" != no && test "$with_libltdl" != yes; then
  AC_CHECK_HEADER([dlfcn.h],
    AC_CHECK_LIB([dl], [dlopen],
      [LIBS_LIBCPLUFF="-ldl $LIBS_LIBCPLUFF"; dlmechanism=dlopen],
	AC_CHECK_LIB([c], [dlopen], [dlmechanism=dlopen])))
fi
if test "$dlmechanism" = none && test "$with_libltdl" != no && test "$with_dlopen" != yes; then  
  AC_CHECK_HEADER([ltdl.h],
    AC_CHECK_LIB([ltdl], [lt_dlopen],
      [LIBS_LIBCPLUFF="-lltdl $LIBS_LIBCPLUFF"; dlmechanism=libltdl]))
fi
case "$dlmechanism" in
  dlopen)
    AC_DEFINE([DLOPEN_POSIX], [], [Define to use Posix dlopen])
      ;;
  libltdl)
    AC_DEFINE([DLOPEN_LIBTOOL], [], [Define to use GNU Libtool libltdl])
    ;;
  *)
    AC_MSG_ERROR([Either the Posix dlopen facility or GNU Libtool libltdl is required])
    ;;
esac

# Check for Expat XML parsing library
# -----------------------------------
AC_CHECK_HEADER([expat.h],, AC_MSG_ERROR([Expat header file is required]))
AC_CHECK_LIB([expat], [XML_ParseBuffer], [LIBS_LIBCPLUFF="-lexpat $LIBS_LIBCPLUFF"], AC_MSG_ERROR([Expat library is required]))

# Check for the GNU Readline Library
# ----------------------------------
AC_ARG_WITH([readline],
  AS_HELP_STRING([--with-readline],
    [use the GNU Readline Library]))
have_readline=no
LIB_READLINE=
if test "$with_readline" != no; then
  AC_CHECK_HEADER([readline/readline.h],
    AC_CHECK_LIB([readline], [add_history], [LIB_READLINE="-lreadline"; have_readline=yes]))
  if test "$with_readline" = yes && test "$have_readline" != yes; then
    AC_MSG_ERROR([GNU readline requested but headers or library not found])
  fi
fi
AC_SUBST([LIB_READLINE])
AM_CONDITIONAL([HAVE_READLINE], test "$have_readline" = yes)

# Link non-library parts with the C-Pluff library
# -----------------------------------------------
LIBS_OTHER="\$(top_builddir)/libcpluff/libcpluff.la $LIBS_OTHER"

# Check for stat/lstat functions
# ------------------------------
AC_CHECK_FUNCS([stat lstat])

# Debugging support
# -----------------
AC_ARG_ENABLE([debug],
  AS_HELP_STRING([--enable-assertions], [enable assertion checks for debugging]))
if ! test "$enable_assertions" = yes; then
  CPPFLAGS="$CPPFLAGS -DNDEBUG"
fi
AC_ARG_ENABLE([gcc-warnings],
  AS_HELP_STRING([--enable-gcc-warnings],
    [enable default set of GCC compiler warnings]))
if test "$enable_gcc_warnings" = yes; then
  CFLAGS="$CFLAGS -Wall -pedantic -std=gnu99"
fi

# File name separator character
# -----------------------------
AC_MSG_CHECKING([which file name separator to use])
case "$host" in
  *-*-mingw32* | *-*-windows*)
  	cp_fnamesep='\\'
  	;;
  *)
  	cp_fnamesep='/'
  	;;
esac
AC_MSG_RESULT(['$cp_fnamesep'])
AC_DEFINE_UNQUOTED([CP_FNAMESEP_CHAR], ['$cp_fnamesep'], [File name separator character])
AC_DEFINE_UNQUOTED([CP_FNAMESEP_STR], ["$cp_fnamesep"], [File name separator string])

# Substitute C-Pluff loader for examples
# --------------------------------------
CPLUFF_LOADER="$bindir/cpluff-loader"
AC_SUBST(CPLUFF_LOADER)

# Output Makefiles
# ----------------
AC_CONFIG_FILES([Makefile
libcpluff/Makefile
libcpluff/cpluffdef.h
libcpluff/docsrc/Makefile
libcpluff/docsrc/Doxyfile-ref
libcpluff/docsrc/Doxyfile-impl
loader/Makefile
console/Makefile
po/Makefile.in
doc/Makefile
doc/img/Makefile
docsrc/Makefile
test/Makefile
test/plugins-source/Makefile
test/plugins-source/callbackcounter/Makefile
test/plugins-source/symuser/Makefile
test/plugins-source/symprovider/Makefile
examples/Makefile
examples/cpfile/Makefile
examples/cpfile/cpfile
examples/cpfile/plugins/Makefile
examples/cpfile/plugins/core/Makefile
examples/cpfile/plugins/special/Makefile
examples/cpfile/plugins/extension/Makefile
examples/cpfile/plugins/cext/Makefile])
AC_OUTPUT


# Print configuration information
# -------------------------------
AC_MSG_NOTICE([-----------------------------------------------------------])
AC_MSG_NOTICE([C-Pluff configuration])
AC_MSG_NOTICE([  release version:                $PACKAGE_VERSION])
if test -n "$cp_threads"; then
  val="yes ($cp_threads)"
else
  val=no
fi
AC_MSG_NOTICE([  multi-threading support:        $val])
AC_MSG_NOTICE([  dlopening mechanism:            $dlmechanism])
val=no
test "$USE_NLS" = no || val="yes (gettext)"
AC_MSG_NOTICE([  localization support:           $val])
AC_MSG_NOTICE([  use GNU readline library:       $have_readline])
if test "$enable_assertions" = yes; then
  val=yes
else
  val=no;
fi
AC_MSG_NOTICE([  assertion checks for debugging: $val])
AC_MSG_NOTICE([  file name separator character:  '$cp_fnamesep'])
AC_MSG_NOTICE([  compiler and linker settings:])
AC_MSG_NOTICE([    CC='$CC'])
AC_MSG_NOTICE([    CPPFLAGS='$CPPFLAGS'])
AC_MSG_NOTICE([    CFLAGS='$CFLAGS'])
AC_MSG_NOTICE([    LDFLAGS='$LDFLAGS'])
AC_MSG_NOTICE([    LIBS='$LIBS'])
AC_MSG_NOTICE([    LIBS_LIBCPLUFF='$LIBS_LIBCPLUFF'])
AC_MSG_NOTICE([    LIBS_OTHER='$LIBS_OTHER'])
AC_MSG_NOTICE([    LIB_READLINE='$LIB_READLINE'])
AC_MSG_NOTICE([    LTLIBINTL='$LTLIBINTL'])
AC_MSG_NOTICE([-----------------------------------------------------------])
