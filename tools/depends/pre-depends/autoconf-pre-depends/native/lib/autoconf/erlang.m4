# This file is part of Autoconf.                       -*- Autoconf -*-
# Erlang/OTP language support.
# Copyright (C) 2006, 2008, 2009, 2010 Free Software Foundation, Inc.

# This file is part of Autoconf.  This program is free
# software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# Under Section 7 of GPL version 3, you are granted additional
# permissions described in the Autoconf Configure Script Exception,
# version 3.0, as published by the Free Software Foundation.
#
# You should have received a copy of the GNU General Public License
# and a copy of the Autoconf Configure Script Exception along with
# this program; see the files COPYINGv3 and COPYING.EXCEPTION
# respectively.  If not, see <http://www.gnu.org/licenses/>.

# Written by Romain Lenglet.


# Table of Contents:
#
# 0. Utility macros
#
# 1. Language selection
#    and routines to produce programs in a given language.
#
# 2. Producing programs in a given language.
#
# 3. Looking for a compiler
#    And possibly the associated preprocessor.



## ------------------- ##
## 0. Utility macros.  ##
## ------------------- ##


# AC_ERLANG_PATH_ERLC([VALUE-IF-NOT-FOUND], [PATH])
# -------------------------------------------------
AC_DEFUN([AC_ERLANG_PATH_ERLC],
[AC_ARG_VAR([ERLC], [Erlang/OTP compiler command [autodetected]])dnl
if test -n "$ERLC"; then
    AC_MSG_CHECKING([for erlc])
    AC_MSG_RESULT([$ERLC])
else
    AC_PATH_TOOL(ERLC, erlc, [$1], [$2])
fi
AC_ARG_VAR([ERLCFLAGS], [Erlang/OTP compiler flags [none]])dnl
])


# AC_ERLANG_NEED_ERLC([PATH])
# ---------------------------
AC_DEFUN([AC_ERLANG_NEED_ERLC],
[AC_ERLANG_PATH_ERLC([not found], [$1])
if test "$ERLC" = "not found"; then
    AC_MSG_ERROR([Erlang/OTP compiler (erlc) not found but required])
fi
])


# AC_ERLANG_PATH_ERL([VALUE-IF-NOT-FOUND], [PATH])
# ------------------------------------------------
AC_DEFUN([AC_ERLANG_PATH_ERL],
[AC_ARG_VAR([ERL], [Erlang/OTP interpreter command [autodetected]])dnl
if test -n "$ERL"; then
    AC_MSG_CHECKING([for erl])
    AC_MSG_RESULT([$ERL])
else
    AC_PATH_TOOL(ERL, erl, [$1], [$2])[]dnl
fi
])


# AC_ERLANG_NEED_ERL([PATH])
# --------------------------
AC_DEFUN([AC_ERLANG_NEED_ERL],
[AC_ERLANG_PATH_ERL([not found], [$1])
if test "$ERL" = "not found"; then
    AC_MSG_ERROR([Erlang/OTP interpreter (erl) not found but required])
fi
])



## ----------------------- ##
## 1. Language selection.  ##
## ----------------------- ##


# AC_LANG(Erlang)
# ---------------
AC_LANG_DEFINE([Erlang], [erl], [ERL], [ERLC], [],
[ac_ext=erl
ac_compile='$ERLC $ERLCFLAGS -b beam conftest.$ac_ext >&AS_MESSAGE_LOG_FD'
ac_link='$ERLC $ERLCFLAGS -b beam conftest.$ac_ext >&AS_MESSAGE_LOG_FD && echo "[#]!/bin/sh" > conftest$ac_exeext && AS_ECHO(["\"$ERL\" -run conftest start -run init stop -noshell"]) >> conftest$ac_exeext && chmod +x conftest$ac_exeext'
])



# AC_LANG_ERLANG
# --------------
AU_DEFUN([AC_LANG_ERLANG], [AC_LANG(Erlang)])



## ----------------------- ##
## 2. Producing programs.  ##
## ----------------------- ##


# AC_LANG_PROGRAM(Erlang)([PROLOGUE], [BODY])
# -------------------------------------------
m4_define([AC_LANG_PROGRAM(Erlang)],
[[-module(conftest).
-export([start/0]).]]
[$1
start() ->
$2
.
])


# _AC_LANG_NULL_PROGRAM(Erlang)
# -----------------------------
# Produce source that does nothing.
m4_define([_AC_LANG_NULL_PROGRAM(Erlang)],
[AC_LANG_PROGRAM([], [halt(0)])])


# _AC_LANG_IO_PROGRAM(Erlang)
# ---------------------------
# Produce source that performs I/O.
m4_define([_AC_LANG_IO_PROGRAM(Erlang)],
[AC_LANG_PROGRAM([], [dnl
   ReturnValue = case file:write_file("conftest.out", "") of
       {error, _} -> 1;
       ok -> 0
   end,
   halt(ReturnValue)])])


## -------------------------------------------- ##
## 3. Looking for Compilers and Preprocessors.  ##
## -------------------------------------------- ##


# AC_LANG_PREPROC(Erlang)
# -----------------------
# Find the Erlang preprocessor.  Must be AC_DEFUN'd to be AC_REQUIRE'able.
AC_DEFUN([AC_LANG_PREPROC(Erlang)],
[m4_warn([syntax],
	 [$0: No preprocessor defined for ]_AC_LANG)])

# AC_LANG_COMPILER(Erlang)
# ------------------------
# Find the Erlang compiler.  Must be AC_DEFUN'd to be AC_REQUIRE'able.
AC_DEFUN([AC_LANG_COMPILER(Erlang)],
[AC_REQUIRE([AC_ERLANG_PATH_ERLC])])


# AC_ERLANG_CHECK_LIB(LIBRARY, [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ----------------------------------------------------------------------
# Macro for checking if an Erlang library is installed, and to
# determine its version.
AC_DEFUN([AC_ERLANG_CHECK_LIB],
[AC_REQUIRE([AC_ERLANG_PATH_ERLC])[]dnl
AC_REQUIRE([AC_ERLANG_PATH_ERL])[]dnl
AC_CACHE_CHECK([for Erlang/OTP '$1' library subdirectory],
    [ac_cv_erlang_lib_dir_$1],
    [AC_LANG_PUSH(Erlang)[]dnl
     AC_RUN_IFELSE(
	[AC_LANG_PROGRAM([], [dnl
	    ReturnValue = case code:lib_dir("[$1]") of
	    {error, bad_name} ->
		file:write_file("conftest.out", "not found\n"),
		1;
	    LibDir ->
		file:write_file("conftest.out", LibDir),
		0
	    end,
	    halt(ReturnValue)])],
	[ac_cv_erlang_lib_dir_$1=`cat conftest.out`
	 rm -f conftest.out],
	[if test ! -f conftest.out; then
	     AC_MSG_FAILURE([test Erlang program execution failed])
	 else
	     ac_cv_erlang_lib_dir_$1="not found"
	     rm -f conftest.out
	 fi])
     AC_LANG_POP(Erlang)[]dnl
    ])
AC_CACHE_CHECK([for Erlang/OTP '$1' library version],
    [ac_cv_erlang_lib_ver_$1],
    [AS_IF([test "$ac_cv_erlang_lib_dir_$1" = "not found"],
	[ac_cv_erlang_lib_ver_$1="not found"],
	[ac_cv_erlang_lib_ver_$1=`AS_ECHO(["$ac_cv_erlang_lib_dir_$1"]) | sed -n -e 's,^.*-\([[^/-]]*\)$,\1,p'`])[]dnl
    ])
AC_SUBST([ERLANG_LIB_DIR_$1], [$ac_cv_erlang_lib_dir_$1])
AC_SUBST([ERLANG_LIB_VER_$1], [$ac_cv_erlang_lib_ver_$1])
AS_IF([test "$ac_cv_erlang_lib_dir_$1" = "not found"], [$3], [$2])
])# AC_ERLANG_CHECK_LIB


# AC_ERLANG_SUBST_ROOT_DIR
# ------------------------
# Determines the Erlang/OTP root directory.
AC_DEFUN([AC_ERLANG_SUBST_ROOT_DIR],
[AC_REQUIRE([AC_ERLANG_NEED_ERLC])[]dnl
AC_REQUIRE([AC_ERLANG_NEED_ERL])[]dnl
AC_CACHE_CHECK([for Erlang/OTP root directory],
    [ac_cv_erlang_root_dir],
    [AC_LANG_PUSH(Erlang)[]dnl
     AC_RUN_IFELSE(
	[AC_LANG_PROGRAM([], [dnl
	    RootDir = code:root_dir(),
	    file:write_file("conftest.out", RootDir),
	    ReturnValue = 0,
	    halt(ReturnValue)])],
	[ac_cv_erlang_root_dir=`cat conftest.out`
	 rm -f conftest.out],
	[rm -f conftest.out
	 AC_MSG_FAILURE([test Erlang program execution failed])])
     AC_LANG_POP(Erlang)[]dnl
    ])
AC_SUBST([ERLANG_ROOT_DIR], [$ac_cv_erlang_root_dir])
])# AC_ERLANG_SUBST_ROOT_DIR


# AC_ERLANG_SUBST_LIB_DIR
# -----------------------
AC_DEFUN([AC_ERLANG_SUBST_LIB_DIR],
[AC_REQUIRE([AC_ERLANG_NEED_ERLC])[]dnl
AC_REQUIRE([AC_ERLANG_NEED_ERL])[]dnl
AC_CACHE_CHECK([for Erlang/OTP library base directory],
    [ac_cv_erlang_lib_dir],
    [AC_LANG_PUSH(Erlang)[]dnl
     AC_RUN_IFELSE(
	[AC_LANG_PROGRAM([], [dnl
	    LibDir = code:lib_dir(),
	    file:write_file("conftest.out", LibDir),
	    ReturnValue = 0,
	    halt(ReturnValue)])],
	[ac_cv_erlang_lib_dir=`cat conftest.out`
	 rm -f conftest.out],
	[rm -f conftest.out
	 AC_MSG_FAILURE([test Erlang program execution failed])])
     AC_LANG_POP(Erlang)[]dnl
    ])
AC_SUBST([ERLANG_LIB_DIR], [$ac_cv_erlang_lib_dir])
])# AC_ERLANG_SUBST_LIB_DIR


# AC_ERLANG_SUBST_INSTALL_LIB_DIR
# -------------------------------
# Directories for installing Erlang/OTP packages are separated from the
# directories determined by running the Erlang/OTP installation that is used
# for building.
AC_DEFUN([AC_ERLANG_SUBST_INSTALL_LIB_DIR],
[AC_MSG_CHECKING([for Erlang/OTP library installation base directory])
AC_ARG_VAR([ERLANG_INSTALL_LIB_DIR],
    [Erlang/OTP library installation base directory [LIBDIR/erlang/lib]])
if test -n "$ERLANG_INSTALL_LIB_DIR"; then
    AC_MSG_RESULT([$ERLANG_INSTALL_LIB_DIR])
else
    AC_SUBST([ERLANG_INSTALL_LIB_DIR], ['${libdir}/erlang/lib'])
    AC_MSG_RESULT([$libdir/erlang/lib])
fi
])# AC_ERLANG_SUBST_INSTALL_LIB_DIR


# AC_ERLANG_SUBST_INSTALL_LIB_SUBDIR(PACKAGE_TARNAME, PACKAGE_VERSION)
# --------------------------------------------------------------------
AC_DEFUN([AC_ERLANG_SUBST_INSTALL_LIB_SUBDIR],
[AC_REQUIRE([AC_ERLANG_SUBST_INSTALL_LIB_DIR])[]dnl
AC_MSG_CHECKING([for Erlang/OTP '$1' library installation subdirectory])
AC_ARG_VAR([ERLANG_INSTALL_LIB_DIR_$1],
    [Erlang/OTP '$1' library installation subdirectory
       [ERLANG_INSTALL_LIB_DIR/$1-$2]])
if test -n "$ERLANG_INSTALL_LIB_DIR_$1"; then
    AC_MSG_RESULT([$ERLANG_INSTALL_LIB_DIR_$1])
else
    AC_SUBST([ERLANG_INSTALL_LIB_DIR_$1], ['${ERLANG_INSTALL_LIB_DIR}/$1-$2'])
    AC_MSG_RESULT([$ERLANG_INSTALL_LIB_DIR/$1-$2])
fi
])# AC_ERLANG_SUBST_INSTALL_LIB_SUBDIR


# AC_ERLANG_SUBST_ERTS_VER
# ------------------------
# Determines the Erlang runtime system version.
AC_DEFUN([AC_ERLANG_SUBST_ERTS_VER],
[AC_REQUIRE([AC_ERLANG_NEED_ERLC])[]dnl
AC_REQUIRE([AC_ERLANG_NEED_ERL])[]dnl
AC_CACHE_CHECK([for Erlang/OTP ERTS version],
    [ac_cv_erlang_erts_ver],
    [AC_LANG_PUSH([Erlang])[]dnl
     AC_RUN_IFELSE(
	[AC_LANG_PROGRAM([], [dnl
	    Version = erlang:system_info(version),
	    file:write_file("conftest.out", Version),
	    ReturnValue = 0,
	    halt(ReturnValue)])],
	[ac_cv_erlang_erts_ver=`cat conftest.out`
	 rm -f conftest.out],
	[rm -f conftest.out
	 AC_MSG_FAILURE([test Erlang program execution failed])])
     AC_LANG_POP([Erlang])[]dnl
    ])
AC_SUBST([ERLANG_ERTS_VER], [$ac_cv_erlang_erts_ver])
])# AC_ERLANG_SUBST_ERTS_VER
