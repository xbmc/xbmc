dnl Process this file with autoconf to produce a configure script.

dnl Copyright 2007 Johannes Lehtinen
dnl This configure.ac script is free software; Johannes Lehtinen gives
dnl unlimited permission to copy, distribute and modify it.

# Autoconf initialization
# -----------------------
AC_INIT([C-Pluff examples], [unspecified], [johannes.lehtinen@iki.fi], [cpluff-examples])
AC_COPYRIGHT([Copyright 2007 Johannes Lehtinen
This configure script is free software; Johannes Lehtinen gives unlimited
permission to copy, distribute and modify it.])
AC_CONFIG_SRCDIR([cpfile/README.txt])
AC_CONFIG_AUX_DIR([auxliary])

# Automake initialization
# -----------------------
AM_INIT_AUTOMAKE([foreign])

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

# Check for C-Pluff loader
# ------------------------
AC_CHECK_PROG([CPLUFF_LOADER], [cpluff-loader], [cpluff-loader])
if test -z "$CPLUFF_LOADER"; then
  AC_MSG_ERROR([C-Pluff Loader, cpluff-loader, not found])
fi

# Check for C-Pluff headers and library
# -------------------------------------
AC_CHECK_HEADER([cpluff.h],, AC_MSG_ERROR([C-Pluff header files not found]))
AC_CHECK_LIB([cpluff], [cp_init],, AC_MSG_ERROR([C-Pluff libraries not found]))

# Check for stat/lstat functions
# ------------------------------
AC_CHECK_FUNCS([stat lstat])

# Output Makefiles
# ----------------
AC_CONFIG_FILES([Makefile
cpfile/Makefile
cpfile/cpfile
cpfile/plugins/Makefile
cpfile/plugins/cext/Makefile
cpfile/plugins/core/Makefile
cpfile/plugins/extension/Makefile
cpfile/plugins/special/Makefile
])
AC_OUTPUT
