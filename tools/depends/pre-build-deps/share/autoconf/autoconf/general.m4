# This file is part of Autoconf.                       -*- Autoconf -*-
# Parameterized macros.
m4_define([_AC_COPYRIGHT_YEARS], [
Copyright (C) 1992, 1993, 1994, 1995, 1996, 1998, 1999, 2000, 2001,
2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010 Free Software
Foundation, Inc.
])

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

# Written by David MacKenzie, with help from
# Franc,ois Pinard, Karl Berry, Richard Pixley, Ian Lance Taylor,
# Roland McGrath, Noah Friedman, david d zuhn, and many others.


## ---------------- ##
## The diversions.  ##
## ---------------- ##


# We heavily use m4's diversions both for the initializations and for
# required macros (see AC_REQUIRE), because in both cases we have to
# issue high in `configure' something which is discovered late.
#
# KILL is only used to suppress output.
#
# The layers of `configure'.  We let m4 undivert them by itself, when
# it reaches the end of `configure.ac'.
#
# - BINSH
#   #! /bin/sh
# - HEADER-REVISION
#   Sent by AC_REVISION
# - HEADER-COMMENT
#   Purpose of the script.
# - HEADER-COPYRIGHT
#   Copyright notice(s)
# - M4SH-INIT
#   Initialization of bottom layers.
#
# - DEFAULTS
#   early initializations (defaults)
# - PARSE_ARGS
#   initialization code, option handling loop.
#
# - HELP_BEGIN
#   Handling `configure --help'.
# - HELP_CANON
#   Help msg for AC_CANONICAL_*
# - HELP_ENABLE
#   Help msg from AC_ARG_ENABLE.
# - HELP_WITH
#   Help msg from AC_ARG_WITH.
# - HELP_VAR
#   Help msg from AC_ARG_VAR.
# - HELP_VAR_END
#   A small paragraph on the use of the variables.
# - HELP_END
#   Tail of the handling of --help.
#
# - VERSION_BEGIN
#   Head of the handling of --version.
# - VERSION_FSF
#   FSF copyright notice for --version.
# - VERSION_USER
#   User copyright notice for --version.
# - VERSION_END
#   Tail of the handling of --version.
#
# - SHELL_FN
#   Shell functions.
#
# - INIT_PREPARE
#   Tail of initialization code.
#
# - BODY
#   the tests and output code
#


# _m4_divert(DIVERSION-NAME)
# --------------------------
# Convert a diversion name into its number.  Otherwise, return
# DIVERSION-NAME which is supposed to be an actual diversion number.
# Of course it would be nicer to use m4_case here, instead of zillions
# of little macros, but it then takes twice longer to run `autoconf'!
#
# From M4sugar:
#    -1. KILL
# 10000. GROW
#
# From M4sh:
#    0. BINSH
#    1. HEADER-REVISION
#    2. HEADER-COMMENT
#    3. HEADER-COPYRIGHT
#    4. M4SH-INIT
# 1000. BODY
m4_define([_m4_divert(DEFAULTS)],        10)
m4_define([_m4_divert(PARSE_ARGS)],      20)

m4_define([_m4_divert(HELP_BEGIN)],     100)
m4_define([_m4_divert(HELP_CANON)],     101)
m4_define([_m4_divert(HELP_ENABLE)],    102)
m4_define([_m4_divert(HELP_WITH)],      103)
m4_define([_m4_divert(HELP_VAR)],       104)
m4_define([_m4_divert(HELP_VAR_END)],   105)
m4_define([_m4_divert(HELP_END)],       106)

m4_define([_m4_divert(VERSION_BEGIN)],  200)
m4_define([_m4_divert(VERSION_FSF)],    201)
m4_define([_m4_divert(VERSION_USER)],   202)
m4_define([_m4_divert(VERSION_END)],    203)

m4_define([_m4_divert(SHELL_FN)],       250)

m4_define([_m4_divert(INIT_PREPARE)],   300)



# AC_DIVERT_PUSH(DIVERSION-NAME)
# AC_DIVERT_POP
# ------------------------------
m4_copy([m4_divert_push],[AC_DIVERT_PUSH])
m4_copy([m4_divert_pop], [AC_DIVERT_POP])



## ------------------------------------ ##
## Defining/requiring Autoconf macros.  ##
## ------------------------------------ ##


# AC_DEFUN(NAME, EXPANSION)
# AC_DEFUN_ONCE(NAME, EXPANSION)
# AC_BEFORE(THIS-MACRO-NAME, CALLED-MACRO-NAME)
# AC_REQUIRE(STRING)
# AC_PROVIDE(MACRO-NAME)
# AC_PROVIDE_IFELSE(MACRO-NAME, IF-PROVIDED, IF-NOT-PROVIDED)
# -----------------------------------------------------------
m4_copy([m4_defun],       [AC_DEFUN])
m4_copy([m4_defun_once],  [AC_DEFUN_ONCE])
m4_copy([m4_before],      [AC_BEFORE])
m4_copy([m4_require],     [AC_REQUIRE])
m4_copy([m4_provide],     [AC_PROVIDE])
m4_copy([m4_provide_if],  [AC_PROVIDE_IFELSE])


# AC_OBSOLETE(THIS-MACRO-NAME, [SUGGESTION])
# ------------------------------------------
m4_define([AC_OBSOLETE],
[AC_DIAGNOSE([obsolete], [$1 is obsolete$2])])



## ----------------------------- ##
## Implementing shell functions. ##
## ----------------------------- ##


# AC_REQUIRE_SHELL_FN(NAME-TO-CHECK, COMMENT, BODY, [DIVERSION = SHELL_FN]
# ------------------------------------------------------------------------
# Same as AS_REQUIRE_SHELL_FN except that the default diversion comes
# later in the script (speeding up configure --help and --version).
AC_DEFUN([AC_REQUIRE_SHELL_FN],
[AS_REQUIRE_SHELL_FN([$1], [$2], [$3], m4_default_quoted([$4], [SHELL_FN]))])



## ----------------------------- ##
## Implementing Autoconf loops.  ##
## ----------------------------- ##


# AU::AC_FOREACH(VARIABLE, LIST, EXPRESSION)
# ------------------------------------------
AU_DEFUN([AC_FOREACH], [[m4_foreach_w($@)]])
AC_DEFUN([AC_FOREACH], [m4_foreach_w($@)dnl
AC_DIAGNOSE([obsolete], [The macro `AC_FOREACH' is obsolete.
You should run autoupdate.])])



## ----------------------------------- ##
## Helping macros to display strings.  ##
## ----------------------------------- ##


# AU::AC_HELP_STRING(LHS, RHS, [COLUMN])
# --------------------------------------
AU_ALIAS([AC_HELP_STRING], [AS_HELP_STRING])



## ---------------------------------------------- ##
## Information on the package being Autoconf'ed.  ##
## ---------------------------------------------- ##


# It is suggested that the macros in this section appear before
# AC_INIT in `configure.ac'.  Nevertheless, this is just stylistic,
# and from the implementation point of view, AC_INIT *must* be expanded
# beforehand: it puts data in diversions which must appear before the
# data provided by the macros of this section.

# The solution is to require AC_INIT in each of these macros.  AC_INIT
# has the needed magic so that it can't be expanded twice.

# _AC_INIT_LITERAL(STRING)
# ------------------------
# Reject STRING if it contains newline, or if it cannot be used as-is
# in single-quoted strings, double-quoted strings, and quoted and
# unquoted here-docs.
m4_define([_AC_INIT_LITERAL],
[m4_if(m4_index(m4_translit([[$1]], [
""], ['']), ['])AS_LITERAL_HEREDOC_IF([$1], [-]), [-1-], [],
  [m4_warn([syntax], [AC_INIT: not a literal: $1])])])

# _AC_INIT_PACKAGE(PACKAGE-NAME, VERSION, BUG-REPORT, [TARNAME], [URL])
# ---------------------------------------------------------------------
m4_define([_AC_INIT_PACKAGE],
[_AC_INIT_LITERAL([$1])
_AC_INIT_LITERAL([$2])
_AC_INIT_LITERAL([$3])
m4_ifndef([AC_PACKAGE_NAME],
	  [m4_define([AC_PACKAGE_NAME],     [$1])])
m4_ifndef([AC_PACKAGE_TARNAME],
	  [m4_define([AC_PACKAGE_TARNAME],
		     m4_default([$4],
				[m4_bpatsubst(m4_tolower(m4_bpatsubst([[$1]],
								     [GNU ])),
				 [[^_abcdefghijklmnopqrstuvwxyz0123456789]],
				 [-])]))])
m4_ifndef([AC_PACKAGE_VERSION],
	  [m4_define([AC_PACKAGE_VERSION],   [$2])])
m4_ifndef([AC_PACKAGE_STRING],
	  [m4_define([AC_PACKAGE_STRING],    [$1 $2])])
m4_ifndef([AC_PACKAGE_BUGREPORT],
	  [m4_define([AC_PACKAGE_BUGREPORT], [$3])])
m4_ifndef([AC_PACKAGE_URL],
	  [m4_define([AC_PACKAGE_URL],
  m4_if([$5], [], [m4_if(m4_index([$1], [GNU ]), [0],
	  [[http://www.gnu.org/software/]m4_defn([AC_PACKAGE_TARNAME])[/]])],
	[[$5]]))])
])


# AC_COPYRIGHT(TEXT, [VERSION-DIVERSION = VERSION_USER],
#              [FILTER = m4_newline])
# ------------------------------------------------------
# Emit TEXT, a copyright notice, in the top of `configure' and in
# --version output.  Macros in TEXT are evaluated once.  Process
# the --version output through FILTER (m4_newline, m4_do, and
# m4_copyright_condense are common filters).
m4_define([AC_COPYRIGHT],
[AS_COPYRIGHT([$1])[]]dnl
[m4_divert_text(m4_default_quoted([$2], [VERSION_USER]),
[m4_default([$3], [m4_newline])([$1])])])# AC_COPYRIGHT


# AC_REVISION(REVISION-INFO)
# --------------------------
# The second quote in the translit is just to cope with font-lock-mode
# which sees the opening of a string.
m4_define([AC_REVISION],
[m4_divert_text([HEADER-REVISION],
		[@%:@ From __file__ m4_translit([$1], [$""]).])dnl
])




## ---------------------------------------- ##
## Requirements over the Autoconf version.  ##
## ---------------------------------------- ##


# AU::AC_PREREQ(VERSION)
# ----------------------
# Update this `AC_PREREQ' statement to require the current version of
# Autoconf.  But fail if ever this autoupdate is too old.
#
# Note that `m4_defn([m4_PACKAGE_VERSION])' below are expanded before
# calling `AU_DEFUN', i.e., it is hard coded.  Otherwise it would be
# quite complex for autoupdate to import the value of
# `m4_PACKAGE_VERSION'.  We could `AU_DEFUN' `m4_PACKAGE_VERSION', but
# this would replace all its occurrences with the current version of
# Autoconf, which is certainly not what the user intended.
AU_DEFUN([AC_PREREQ],
[m4_version_prereq([$1])[]dnl
[AC_PREREQ(]]m4_dquote(m4_dquote(m4_defn([m4_PACKAGE_VERSION])))[[)]])


# AC_PREREQ(VERSION)
# ------------------
# Complain and exit if the Autoconf version is less than VERSION.
m4_undefine([AC_PREREQ])
m4_copy([m4_version_prereq], [AC_PREREQ])


# AC_AUTOCONF_VERSION
# -------------------
# The current version of Autoconf parsing this file.
m4_copy([m4_PACKAGE_VERSION], [AC_AUTOCONF_VERSION])





## ---------------- ##
## Initialization.  ##
## ---------------- ##


# All the following macros are used by AC_INIT.  Ideally, they should
# be presented in the order in which they are output.  Please, help us
# sorting it, or at least, don't augment the entropy.


# _AC_INIT_NOTICE
# ---------------
# Provide useful headers; override the HEADER-COMMENT created by M4sh.
m4_define([_AC_INIT_NOTICE],
[m4_cleardivert([HEADER-COMMENT])]dnl
[m4_divert_text([HEADER-COMMENT],
[@%:@ Guess values for system-dependent variables and create Makefiles.
@%:@ Generated by m4_PACKAGE_STRING[]dnl
m4_ifset([AC_PACKAGE_STRING], [ for AC_PACKAGE_STRING]).])

m4_ifset([AC_PACKAGE_BUGREPORT],
	 [m4_divert_text([HEADER-COMMENT],
			 [@%:@
@%:@ Report bugs to <AC_PACKAGE_BUGREPORT>.])])
])


# _AC_INIT_COPYRIGHT
# ------------------
# We dump to VERSION_FSF to make sure we are inserted before the
# user copyrights, and after the setup of the --version handling.
m4_define([_AC_INIT_COPYRIGHT],
[AC_COPYRIGHT(m4_defn([_AC_COPYRIGHT_YEARS]), [VERSION_FSF], [
m4_copyright_condense])dnl
AC_COPYRIGHT(
[This configure script is free software; the Free Software Foundation
gives unlimited permission to copy, distribute and modify it.],
	     [VERSION_FSF], [m4_echo])])


# File Descriptors
# ----------------
# Set up the file descriptors used by `configure'.
# File descriptor usage:
# 0 standard input (/dev/null)
# 1 file creation
# 2 errors and warnings
# AS_MESSAGE_LOG_FD compiler messages saved in config.log
# AS_MESSAGE_FD checking for... messages and results
# AS_ORIGINAL_STDIN_FD original standard input (still open)
#
# stdin is /dev/null because checks that run programs may
# inadvertently run interactive ones, which would stop configuration
# until someone typed an EOF.
m4_define([AS_MESSAGE_FD], 6)
m4_define([AS_ORIGINAL_STDIN_FD], 7)
# That's how they used to be named.
AU_ALIAS([AC_FD_CC],  [AS_MESSAGE_LOG_FD])
AU_ALIAS([AC_FD_MSG], [AS_MESSAGE_FD])


# _AC_INIT_DEFAULTS
# -----------------
# Values which defaults can be set from `configure.ac'.
# `/bin/machine' is used in `glibcbug'.  The others are used in config.*
m4_define([_AC_INIT_DEFAULTS],
[m4_divert_push([DEFAULTS])dnl

test -n "$DJDIR" || exec AS_ORIGINAL_STDIN_FD<&0 </dev/null
exec AS_MESSAGE_FD>&1

# Name of the host.
# hostname on some systems (SVR3.2, old GNU/Linux) returns a bogus exit status,
# so uname gets run too.
ac_hostname=`(hostname || uname -n) 2>/dev/null | sed 1q`

#
# Initializations.
#
ac_default_prefix=/usr/local
ac_clean_files=
ac_config_libobj_dir=.
LIB@&t@OBJS=
cross_compiling=no
subdirs=
MFLAGS=
MAKEFLAGS=
AC_SUBST([SHELL])dnl
AC_SUBST([PATH_SEPARATOR])dnl

# Identity of this package.
AC_SUBST([PACKAGE_NAME],
	 [m4_ifdef([AC_PACKAGE_NAME],      ['AC_PACKAGE_NAME'])])dnl
AC_SUBST([PACKAGE_TARNAME],
	 [m4_ifdef([AC_PACKAGE_TARNAME],   ['AC_PACKAGE_TARNAME'])])dnl
AC_SUBST([PACKAGE_VERSION],
	 [m4_ifdef([AC_PACKAGE_VERSION],   ['AC_PACKAGE_VERSION'])])dnl
AC_SUBST([PACKAGE_STRING],
	 [m4_ifdef([AC_PACKAGE_STRING],    ['AC_PACKAGE_STRING'])])dnl
AC_SUBST([PACKAGE_BUGREPORT],
	 [m4_ifdef([AC_PACKAGE_BUGREPORT], ['AC_PACKAGE_BUGREPORT'])])dnl
AC_SUBST([PACKAGE_URL],
	 [m4_ifdef([AC_PACKAGE_URL],       ['AC_PACKAGE_URL'])])dnl

m4_divert_pop([DEFAULTS])dnl
m4_wrap_lifo([m4_divert_text([DEFAULTS],
[ac_subst_vars='m4_set_dump([_AC_SUBST_VARS], m4_newline)'
ac_subst_files='m4_ifdef([_AC_SUBST_FILES], [m4_defn([_AC_SUBST_FILES])])'
ac_user_opts='
enable_option_checking
m4_ifdef([_AC_USER_OPTS], [m4_defn([_AC_USER_OPTS])
])'
m4_ifdef([_AC_PRECIOUS_VARS],
  [_AC_ARG_VAR_STORE[]dnl
   _AC_ARG_VAR_VALIDATE[]dnl
   ac_precious_vars='m4_defn([_AC_PRECIOUS_VARS])'])
m4_ifdef([_AC_LIST_SUBDIRS],
  [ac_subdirs_all='m4_defn([_AC_LIST_SUBDIRS])'])dnl
])])dnl
])# _AC_INIT_DEFAULTS


# AC_PREFIX_DEFAULT(PREFIX)
# -------------------------
AC_DEFUN([AC_PREFIX_DEFAULT],
[m4_divert_text([DEFAULTS], [ac_default_prefix=$1])])


# AC_PREFIX_PROGRAM(PROGRAM)
# --------------------------
# Guess the value for the `prefix' variable by looking for
# the argument program along PATH and taking its parent.
# Example: if the argument is `gcc' and we find /usr/local/gnu/bin/gcc,
# set `prefix' to /usr/local/gnu.
# This comes too late to find a site file based on the prefix,
# and it might use a cached value for the path.
# No big loss, I think, since most configures don't use this macro anyway.
AC_DEFUN([AC_PREFIX_PROGRAM],
[if test "x$prefix" = xNONE; then
dnl We reimplement AC_MSG_CHECKING (mostly) to avoid the ... in the middle.
  _AS_ECHO_N([checking for prefix by ])
  AC_PATH_PROG(ac_prefix_program, [$1])
  if test -n "$ac_prefix_program"; then
    prefix=`AS_DIRNAME(["$ac_prefix_program"])`
    prefix=`AS_DIRNAME(["$prefix"])`
  fi
fi
])# AC_PREFIX_PROGRAM


# AC_CONFIG_SRCDIR([UNIQUE-FILE-IN-SOURCE-DIR])
# ---------------------------------------------
# UNIQUE-FILE-IN-SOURCE-DIR is a file name unique to this package,
# relative to the directory that configure is in, which we can look
# for to find out if srcdir is correct.
AC_DEFUN([AC_CONFIG_SRCDIR],
[m4_divert_text([DEFAULTS], [ac_unique_file="$1"])])


# _AC_INIT_DIRCHECK
# -----------------
# Set ac_pwd, and sanity-check it and the source and installation directories.
#
# (This macro is AC_REQUIREd by _AC_INIT_SRCDIR, so it has to be AC_DEFUNed.)
#
AC_DEFUN([_AC_INIT_DIRCHECK],
[m4_divert_push([PARSE_ARGS])dnl

ac_pwd=`pwd` && test -n "$ac_pwd" &&
ac_ls_di=`ls -di .` &&
ac_pwd_ls_di=`cd "$ac_pwd" && ls -di .` ||
  AC_MSG_ERROR([working directory cannot be determined])
test "X$ac_ls_di" = "X$ac_pwd_ls_di" ||
  AC_MSG_ERROR([pwd does not report name of working directory])

m4_divert_pop([PARSE_ARGS])dnl
])# _AC_INIT_DIRCHECK

# _AC_INIT_SRCDIR
# ---------------
# Compute `srcdir' based on `$ac_unique_file'.
#
# (We have to AC_DEFUN it, since we use AC_REQUIRE.)
#
AC_DEFUN([_AC_INIT_SRCDIR],
[AC_REQUIRE([_AC_INIT_DIRCHECK])dnl
m4_divert_push([PARSE_ARGS])dnl

# Find the source files, if location was not specified.
if test -z "$srcdir"; then
  ac_srcdir_defaulted=yes
  # Try the directory containing this script, then the parent directory.
  ac_confdir=`AS_DIRNAME(["$as_myself"])`
  srcdir=$ac_confdir
  if test ! -r "$srcdir/$ac_unique_file"; then
    srcdir=..
  fi
else
  ac_srcdir_defaulted=no
fi
if test ! -r "$srcdir/$ac_unique_file"; then
  test "$ac_srcdir_defaulted" = yes && srcdir="$ac_confdir or .."
  AC_MSG_ERROR([cannot find sources ($ac_unique_file) in $srcdir])
fi
ac_msg="sources are in $srcdir, but \`cd $srcdir' does not work"
ac_abs_confdir=`(
	cd "$srcdir" && test -r "./$ac_unique_file" || AC_MSG_ERROR([$ac_msg])
	pwd)`
# When building in place, set srcdir=.
if test "$ac_abs_confdir" = "$ac_pwd"; then
  srcdir=.
fi
# Remove unnecessary trailing slashes from srcdir.
# Double slashes in file names in object file debugging info
# mess up M-x gdb in Emacs.
case $srcdir in
*/) srcdir=`expr "X$srcdir" : 'X\(.*[[^/]]\)' \| "X$srcdir" : 'X\(.*\)'`;;
esac
m4_divert_pop([PARSE_ARGS])dnl
])# _AC_INIT_SRCDIR


# _AC_INIT_PARSE_ARGS
# -------------------
m4_define([_AC_INIT_PARSE_ARGS],
[m4_divert_push([PARSE_ARGS])dnl

# Initialize some variables set by options.
ac_init_help=
ac_init_version=false
ac_unrecognized_opts=
ac_unrecognized_sep=
# The variables have the same names as the options, with
# dashes changed to underlines.
cache_file=/dev/null
AC_SUBST(exec_prefix, NONE)dnl
no_create=
no_recursion=
AC_SUBST(prefix, NONE)dnl
program_prefix=NONE
program_suffix=NONE
AC_SUBST(program_transform_name, [s,x,x,])dnl
silent=
site=
srcdir=
verbose=
x_includes=NONE
x_libraries=NONE

# Installation directory options.
# These are left unexpanded so users can "make install exec_prefix=/foo"
# and all the variables that are supposed to be based on exec_prefix
# by default will actually change.
# Use braces instead of parens because sh, perl, etc. also accept them.
# (The list follows the same order as the GNU Coding Standards.)
AC_SUBST([bindir],         ['${exec_prefix}/bin'])dnl
AC_SUBST([sbindir],        ['${exec_prefix}/sbin'])dnl
AC_SUBST([libexecdir],     ['${exec_prefix}/libexec'])dnl
AC_SUBST([datarootdir],    ['${prefix}/share'])dnl
AC_SUBST([datadir],        ['${datarootdir}'])dnl
AC_SUBST([sysconfdir],     ['${prefix}/etc'])dnl
AC_SUBST([sharedstatedir], ['${prefix}/com'])dnl
AC_SUBST([localstatedir],  ['${prefix}/var'])dnl
AC_SUBST([includedir],     ['${prefix}/include'])dnl
AC_SUBST([oldincludedir],  ['/usr/include'])dnl
AC_SUBST([docdir],         [m4_ifset([AC_PACKAGE_TARNAME],
				     ['${datarootdir}/doc/${PACKAGE_TARNAME}'],
				     ['${datarootdir}/doc/${PACKAGE}'])])dnl
AC_SUBST([infodir],        ['${datarootdir}/info'])dnl
AC_SUBST([htmldir],        ['${docdir}'])dnl
AC_SUBST([dvidir],         ['${docdir}'])dnl
AC_SUBST([pdfdir],         ['${docdir}'])dnl
AC_SUBST([psdir],          ['${docdir}'])dnl
AC_SUBST([libdir],         ['${exec_prefix}/lib'])dnl
AC_SUBST([localedir],      ['${datarootdir}/locale'])dnl
AC_SUBST([mandir],         ['${datarootdir}/man'])dnl

ac_prev=
ac_dashdash=
for ac_option
do
  # If the previous option needs an argument, assign it.
  if test -n "$ac_prev"; then
    eval $ac_prev=\$ac_option
    ac_prev=
    continue
  fi

  case $ac_option in
  *=?*) ac_optarg=`expr "X$ac_option" : '[[^=]]*=\(.*\)'` ;;
  *=)   ac_optarg= ;;
  *)    ac_optarg=yes ;;
  esac

  # Accept the important Cygnus configure options, so we can diagnose typos.

  case $ac_dashdash$ac_option in
  --)
    ac_dashdash=yes ;;

  -bindir | --bindir | --bindi | --bind | --bin | --bi)
    ac_prev=bindir ;;
  -bindir=* | --bindir=* | --bindi=* | --bind=* | --bin=* | --bi=*)
    bindir=$ac_optarg ;;

  -build | --build | --buil | --bui | --bu)
    ac_prev=build_alias ;;
  -build=* | --build=* | --buil=* | --bui=* | --bu=*)
    build_alias=$ac_optarg ;;

  -cache-file | --cache-file | --cache-fil | --cache-fi \
  | --cache-f | --cache- | --cache | --cach | --cac | --ca | --c)
    ac_prev=cache_file ;;
  -cache-file=* | --cache-file=* | --cache-fil=* | --cache-fi=* \
  | --cache-f=* | --cache-=* | --cache=* | --cach=* | --cac=* | --ca=* | --c=*)
    cache_file=$ac_optarg ;;

  --config-cache | -C)
    cache_file=config.cache ;;

  -datadir | --datadir | --datadi | --datad)
    ac_prev=datadir ;;
  -datadir=* | --datadir=* | --datadi=* | --datad=*)
    datadir=$ac_optarg ;;

  -datarootdir | --datarootdir | --datarootdi | --datarootd | --dataroot \
  | --dataroo | --dataro | --datar)
    ac_prev=datarootdir ;;
  -datarootdir=* | --datarootdir=* | --datarootdi=* | --datarootd=* \
  | --dataroot=* | --dataroo=* | --dataro=* | --datar=*)
    datarootdir=$ac_optarg ;;

  _AC_INIT_PARSE_ENABLE([disable])

  -docdir | --docdir | --docdi | --doc | --do)
    ac_prev=docdir ;;
  -docdir=* | --docdir=* | --docdi=* | --doc=* | --do=*)
    docdir=$ac_optarg ;;

  -dvidir | --dvidir | --dvidi | --dvid | --dvi | --dv)
    ac_prev=dvidir ;;
  -dvidir=* | --dvidir=* | --dvidi=* | --dvid=* | --dvi=* | --dv=*)
    dvidir=$ac_optarg ;;

  _AC_INIT_PARSE_ENABLE([enable])

  -exec-prefix | --exec_prefix | --exec-prefix | --exec-prefi \
  | --exec-pref | --exec-pre | --exec-pr | --exec-p | --exec- \
  | --exec | --exe | --ex)
    ac_prev=exec_prefix ;;
  -exec-prefix=* | --exec_prefix=* | --exec-prefix=* | --exec-prefi=* \
  | --exec-pref=* | --exec-pre=* | --exec-pr=* | --exec-p=* | --exec-=* \
  | --exec=* | --exe=* | --ex=*)
    exec_prefix=$ac_optarg ;;

  -gas | --gas | --ga | --g)
    # Obsolete; use --with-gas.
    with_gas=yes ;;

  -help | --help | --hel | --he | -h)
    ac_init_help=long ;;
  -help=r* | --help=r* | --hel=r* | --he=r* | -hr*)
    ac_init_help=recursive ;;
  -help=s* | --help=s* | --hel=s* | --he=s* | -hs*)
    ac_init_help=short ;;

  -host | --host | --hos | --ho)
    ac_prev=host_alias ;;
  -host=* | --host=* | --hos=* | --ho=*)
    host_alias=$ac_optarg ;;

  -htmldir | --htmldir | --htmldi | --htmld | --html | --htm | --ht)
    ac_prev=htmldir ;;
  -htmldir=* | --htmldir=* | --htmldi=* | --htmld=* | --html=* | --htm=* \
  | --ht=*)
    htmldir=$ac_optarg ;;

  -includedir | --includedir | --includedi | --included | --include \
  | --includ | --inclu | --incl | --inc)
    ac_prev=includedir ;;
  -includedir=* | --includedir=* | --includedi=* | --included=* | --include=* \
  | --includ=* | --inclu=* | --incl=* | --inc=*)
    includedir=$ac_optarg ;;

  -infodir | --infodir | --infodi | --infod | --info | --inf)
    ac_prev=infodir ;;
  -infodir=* | --infodir=* | --infodi=* | --infod=* | --info=* | --inf=*)
    infodir=$ac_optarg ;;

  -libdir | --libdir | --libdi | --libd)
    ac_prev=libdir ;;
  -libdir=* | --libdir=* | --libdi=* | --libd=*)
    libdir=$ac_optarg ;;

  -libexecdir | --libexecdir | --libexecdi | --libexecd | --libexec \
  | --libexe | --libex | --libe)
    ac_prev=libexecdir ;;
  -libexecdir=* | --libexecdir=* | --libexecdi=* | --libexecd=* | --libexec=* \
  | --libexe=* | --libex=* | --libe=*)
    libexecdir=$ac_optarg ;;

  -localedir | --localedir | --localedi | --localed | --locale)
    ac_prev=localedir ;;
  -localedir=* | --localedir=* | --localedi=* | --localed=* | --locale=*)
    localedir=$ac_optarg ;;

  -localstatedir | --localstatedir | --localstatedi | --localstated \
  | --localstate | --localstat | --localsta | --localst | --locals)
    ac_prev=localstatedir ;;
  -localstatedir=* | --localstatedir=* | --localstatedi=* | --localstated=* \
  | --localstate=* | --localstat=* | --localsta=* | --localst=* | --locals=*)
    localstatedir=$ac_optarg ;;

  -mandir | --mandir | --mandi | --mand | --man | --ma | --m)
    ac_prev=mandir ;;
  -mandir=* | --mandir=* | --mandi=* | --mand=* | --man=* | --ma=* | --m=*)
    mandir=$ac_optarg ;;

  -nfp | --nfp | --nf)
    # Obsolete; use --without-fp.
    with_fp=no ;;

  -no-create | --no-create | --no-creat | --no-crea | --no-cre \
  | --no-cr | --no-c | -n)
    no_create=yes ;;

  -no-recursion | --no-recursion | --no-recursio | --no-recursi \
  | --no-recurs | --no-recur | --no-recu | --no-rec | --no-re | --no-r)
    no_recursion=yes ;;

  -oldincludedir | --oldincludedir | --oldincludedi | --oldincluded \
  | --oldinclude | --oldinclud | --oldinclu | --oldincl | --oldinc \
  | --oldin | --oldi | --old | --ol | --o)
    ac_prev=oldincludedir ;;
  -oldincludedir=* | --oldincludedir=* | --oldincludedi=* | --oldincluded=* \
  | --oldinclude=* | --oldinclud=* | --oldinclu=* | --oldincl=* | --oldinc=* \
  | --oldin=* | --oldi=* | --old=* | --ol=* | --o=*)
    oldincludedir=$ac_optarg ;;

  -prefix | --prefix | --prefi | --pref | --pre | --pr | --p)
    ac_prev=prefix ;;
  -prefix=* | --prefix=* | --prefi=* | --pref=* | --pre=* | --pr=* | --p=*)
    prefix=$ac_optarg ;;

  -program-prefix | --program-prefix | --program-prefi | --program-pref \
  | --program-pre | --program-pr | --program-p)
    ac_prev=program_prefix ;;
  -program-prefix=* | --program-prefix=* | --program-prefi=* \
  | --program-pref=* | --program-pre=* | --program-pr=* | --program-p=*)
    program_prefix=$ac_optarg ;;

  -program-suffix | --program-suffix | --program-suffi | --program-suff \
  | --program-suf | --program-su | --program-s)
    ac_prev=program_suffix ;;
  -program-suffix=* | --program-suffix=* | --program-suffi=* \
  | --program-suff=* | --program-suf=* | --program-su=* | --program-s=*)
    program_suffix=$ac_optarg ;;

  -program-transform-name | --program-transform-name \
  | --program-transform-nam | --program-transform-na \
  | --program-transform-n | --program-transform- \
  | --program-transform | --program-transfor \
  | --program-transfo | --program-transf \
  | --program-trans | --program-tran \
  | --progr-tra | --program-tr | --program-t)
    ac_prev=program_transform_name ;;
  -program-transform-name=* | --program-transform-name=* \
  | --program-transform-nam=* | --program-transform-na=* \
  | --program-transform-n=* | --program-transform-=* \
  | --program-transform=* | --program-transfor=* \
  | --program-transfo=* | --program-transf=* \
  | --program-trans=* | --program-tran=* \
  | --progr-tra=* | --program-tr=* | --program-t=*)
    program_transform_name=$ac_optarg ;;

  -pdfdir | --pdfdir | --pdfdi | --pdfd | --pdf | --pd)
    ac_prev=pdfdir ;;
  -pdfdir=* | --pdfdir=* | --pdfdi=* | --pdfd=* | --pdf=* | --pd=*)
    pdfdir=$ac_optarg ;;

  -psdir | --psdir | --psdi | --psd | --ps)
    ac_prev=psdir ;;
  -psdir=* | --psdir=* | --psdi=* | --psd=* | --ps=*)
    psdir=$ac_optarg ;;

  -q | -quiet | --quiet | --quie | --qui | --qu | --q \
  | -silent | --silent | --silen | --sile | --sil)
    silent=yes ;;

  -sbindir | --sbindir | --sbindi | --sbind | --sbin | --sbi | --sb)
    ac_prev=sbindir ;;
  -sbindir=* | --sbindir=* | --sbindi=* | --sbind=* | --sbin=* \
  | --sbi=* | --sb=*)
    sbindir=$ac_optarg ;;

  -sharedstatedir | --sharedstatedir | --sharedstatedi \
  | --sharedstated | --sharedstate | --sharedstat | --sharedsta \
  | --sharedst | --shareds | --shared | --share | --shar \
  | --sha | --sh)
    ac_prev=sharedstatedir ;;
  -sharedstatedir=* | --sharedstatedir=* | --sharedstatedi=* \
  | --sharedstated=* | --sharedstate=* | --sharedstat=* | --sharedsta=* \
  | --sharedst=* | --shareds=* | --shared=* | --share=* | --shar=* \
  | --sha=* | --sh=*)
    sharedstatedir=$ac_optarg ;;

  -site | --site | --sit)
    ac_prev=site ;;
  -site=* | --site=* | --sit=*)
    site=$ac_optarg ;;

  -srcdir | --srcdir | --srcdi | --srcd | --src | --sr)
    ac_prev=srcdir ;;
  -srcdir=* | --srcdir=* | --srcdi=* | --srcd=* | --src=* | --sr=*)
    srcdir=$ac_optarg ;;

  -sysconfdir | --sysconfdir | --sysconfdi | --sysconfd | --sysconf \
  | --syscon | --sysco | --sysc | --sys | --sy)
    ac_prev=sysconfdir ;;
  -sysconfdir=* | --sysconfdir=* | --sysconfdi=* | --sysconfd=* | --sysconf=* \
  | --syscon=* | --sysco=* | --sysc=* | --sys=* | --sy=*)
    sysconfdir=$ac_optarg ;;

  -target | --target | --targe | --targ | --tar | --ta | --t)
    ac_prev=target_alias ;;
  -target=* | --target=* | --targe=* | --targ=* | --tar=* | --ta=* | --t=*)
    target_alias=$ac_optarg ;;

  -v | -verbose | --verbose | --verbos | --verbo | --verb)
    verbose=yes ;;

  -version | --version | --versio | --versi | --vers | -V)
    ac_init_version=: ;;

  _AC_INIT_PARSE_ENABLE([with])

  _AC_INIT_PARSE_ENABLE([without])

  --x)
    # Obsolete; use --with-x.
    with_x=yes ;;

  -x-includes | --x-includes | --x-include | --x-includ | --x-inclu \
  | --x-incl | --x-inc | --x-in | --x-i)
    ac_prev=x_includes ;;
  -x-includes=* | --x-includes=* | --x-include=* | --x-includ=* | --x-inclu=* \
  | --x-incl=* | --x-inc=* | --x-in=* | --x-i=*)
    x_includes=$ac_optarg ;;

  -x-libraries | --x-libraries | --x-librarie | --x-librari \
  | --x-librar | --x-libra | --x-libr | --x-lib | --x-li | --x-l)
    ac_prev=x_libraries ;;
  -x-libraries=* | --x-libraries=* | --x-librarie=* | --x-librari=* \
  | --x-librar=* | --x-libra=* | --x-libr=* | --x-lib=* | --x-li=* | --x-l=*)
    x_libraries=$ac_optarg ;;

  -*) AC_MSG_ERROR([unrecognized option: `$ac_option'
Try `$[0] --help' for more information])
    ;;

  *=*)
    ac_envvar=`expr "x$ac_option" : 'x\([[^=]]*\)='`
    # Reject names that are not valid shell variable names.
    case $ac_envvar in #(
      '' | [[0-9]]* | *[[!_$as_cr_alnum]]* )
      AC_MSG_ERROR([invalid variable name: `$ac_envvar']) ;;
    esac
    eval $ac_envvar=\$ac_optarg
    export $ac_envvar ;;

  *)
    # FIXME: should be removed in autoconf 3.0.
    AC_MSG_WARN([you should use --build, --host, --target])
    expr "x$ac_option" : "[.*[^-._$as_cr_alnum]]" >/dev/null &&
      AC_MSG_WARN([invalid host type: $ac_option])
    : "${build_alias=$ac_option} ${host_alias=$ac_option} ${target_alias=$ac_option}"
    ;;

  esac
done

if test -n "$ac_prev"; then
  ac_option=--`echo $ac_prev | sed 's/_/-/g'`
  AC_MSG_ERROR([missing argument to $ac_option])
fi

if test -n "$ac_unrecognized_opts"; then
  case $enable_option_checking in
    no) ;;
    fatal) AC_MSG_ERROR([unrecognized options: $ac_unrecognized_opts]) ;;
    *)     AC_MSG_WARN( [unrecognized options: $ac_unrecognized_opts]) ;;
  esac
fi

# Check all directory arguments for consistency.
for ac_var in	exec_prefix prefix bindir sbindir libexecdir datarootdir \
		datadir sysconfdir sharedstatedir localstatedir includedir \
		oldincludedir docdir infodir htmldir dvidir pdfdir psdir \
		libdir localedir mandir
do
  eval ac_val=\$$ac_var
  # Remove trailing slashes.
  case $ac_val in
    */ )
      ac_val=`expr "X$ac_val" : 'X\(.*[[^/]]\)' \| "X$ac_val" : 'X\(.*\)'`
      eval $ac_var=\$ac_val;;
  esac
  # Be sure to have absolute directory names.
  case $ac_val in
    [[\\/$]]* | ?:[[\\/]]* )  continue;;
    NONE | '' ) case $ac_var in *prefix ) continue;; esac;;
  esac
  AC_MSG_ERROR([expected an absolute directory name for --$ac_var: $ac_val])
done

# There might be people who depend on the old broken behavior: `$host'
# used to hold the argument of --host etc.
# FIXME: To remove some day.
build=$build_alias
host=$host_alias
target=$target_alias

# FIXME: To remove some day.
if test "x$host_alias" != x; then
  if test "x$build_alias" = x; then
    cross_compiling=maybe
    AC_MSG_WARN([if you wanted to set the --build type, don't use --host.
    If a cross compiler is detected then cross compile mode will be used])
  elif test "x$build_alias" != "x$host_alias"; then
    cross_compiling=yes
  fi
fi

ac_tool_prefix=
test -n "$host_alias" && ac_tool_prefix=$host_alias-

test "$silent" = yes && exec AS_MESSAGE_FD>/dev/null

m4_divert_pop([PARSE_ARGS])dnl
])# _AC_INIT_PARSE_ARGS


# _AC_INIT_PARSE_ENABLE(OPTION-NAME)
# ----------------------------------
# A trivial front-end for _AC_INIT_PARSE_ENABLE2.
#
m4_define([_AC_INIT_PARSE_ENABLE],
[m4_bmatch([$1], [^with],
	   [_AC_INIT_PARSE_ENABLE2([$1], [with])],
	   [_AC_INIT_PARSE_ENABLE2([$1], [enable])])])


# _AC_INIT_PARSE_ENABLE2(OPTION-NAME, POSITIVE-NAME)
# --------------------------------------------------
# Handle an `--enable' or a `--with' option.
#
# OPTION-NAME is `enable', `disable', `with', or `without'.
# POSITIVE-NAME is the corresponding positive variant, i.e. `enable' or `with'.
#
# Positive variant of the option is recognized by the condition
#	OPTION-NAME == POSITIVE-NAME .
#
m4_define([_AC_INIT_PARSE_ENABLE2],
[-$1-* | --$1-*)
    ac_useropt=`expr "x$ac_option" : 'x-*$1-\(m4_if([$1], [$2], [[[^=]]], [.])*\)'`
    # Reject names that are not valid shell variable names.
    expr "x$ac_useropt" : "[.*[^-+._$as_cr_alnum]]" >/dev/null &&
      AC_MSG_ERROR(
	[invalid ]m4_if([$2], [with], [package], [feature])[ name: $ac_useropt])
    ac_useropt_orig=$ac_useropt
    ac_useropt=`AS_ECHO(["$ac_useropt"]) | sed 's/[[-+.]]/_/g'`
    case $ac_user_opts in
      *"
"$2_$ac_useropt"
"*) ;;
      *) ac_unrecognized_opts="$ac_unrecognized_opts$ac_unrecognized_sep--$1-$ac_useropt_orig"
	 ac_unrecognized_sep=', ';;
    esac
    eval $2_$ac_useropt=m4_if([$1], [$2], [\$ac_optarg], [no]) ;;dnl
])


# _AC_INIT_HELP
# -------------
# Handle the `configure --help' message.
m4_define([_AC_INIT_HELP],
[m4_divert_push([HELP_BEGIN])dnl

#
# Report the --help message.
#
if test "$ac_init_help" = "long"; then
  # Omit some internal or obsolete options to make the list less imposing.
  # This message is too long to be a string in the A/UX 3.1 sh.
  cat <<_ACEOF
\`configure' configures m4_ifset([AC_PACKAGE_STRING],
			[AC_PACKAGE_STRING],
			[this package]) to adapt to many kinds of systems.

Usage: $[0] [[OPTION]]... [[VAR=VALUE]]...

[To assign environment variables (e.g., CC, CFLAGS...), specify them as
VAR=VALUE.  See below for descriptions of some of the useful variables.

Defaults for the options are specified in brackets.

Configuration:
  -h, --help              display this help and exit
      --help=short        display options specific to this package
      --help=recursive    display the short help of all the included packages
  -V, --version           display version information and exit
  -q, --quiet, --silent   do not print \`checking ...' messages
      --cache-file=FILE   cache test results in FILE [disabled]
  -C, --config-cache      alias for \`--cache-file=config.cache'
  -n, --no-create         do not create output files
      --srcdir=DIR        find the sources in DIR [configure dir or \`..']

Installation directories:
]AS_HELP_STRING([--prefix=PREFIX],
  [install architecture-independent files in PREFIX [$ac_default_prefix]])
AS_HELP_STRING([--exec-prefix=EPREFIX],
  [install architecture-dependent files in EPREFIX [PREFIX]])[

By default, \`make install' will install all the files in
\`$ac_default_prefix/bin', \`$ac_default_prefix/lib' etc.  You can specify
an installation prefix other than \`$ac_default_prefix' using \`--prefix',
for instance \`--prefix=\$HOME'.

For better control, use the options below.

Fine tuning of the installation directories:
  --bindir=DIR            user executables [EPREFIX/bin]
  --sbindir=DIR           system admin executables [EPREFIX/sbin]
  --libexecdir=DIR        program executables [EPREFIX/libexec]
  --sysconfdir=DIR        read-only single-machine data [PREFIX/etc]
  --sharedstatedir=DIR    modifiable architecture-independent data [PREFIX/com]
  --localstatedir=DIR     modifiable single-machine data [PREFIX/var]
  --libdir=DIR            object code libraries [EPREFIX/lib]
  --includedir=DIR        C header files [PREFIX/include]
  --oldincludedir=DIR     C header files for non-gcc [/usr/include]
  --datarootdir=DIR       read-only arch.-independent data root [PREFIX/share]
  --datadir=DIR           read-only architecture-independent data [DATAROOTDIR]
  --infodir=DIR           info documentation [DATAROOTDIR/info]
  --localedir=DIR         locale-dependent data [DATAROOTDIR/locale]
  --mandir=DIR            man documentation [DATAROOTDIR/man]
]AS_HELP_STRING([--docdir=DIR],
  [documentation root ]@<:@DATAROOTDIR/doc/m4_ifset([AC_PACKAGE_TARNAME],
    [AC_PACKAGE_TARNAME], [PACKAGE])@:>@)[
  --htmldir=DIR           html documentation [DOCDIR]
  --dvidir=DIR            dvi documentation [DOCDIR]
  --pdfdir=DIR            pdf documentation [DOCDIR]
  --psdir=DIR             ps documentation [DOCDIR]
_ACEOF

  cat <<\_ACEOF]
m4_divert_pop([HELP_BEGIN])dnl
dnl The order of the diversions here is
dnl - HELP_BEGIN
dnl   which may be extended by extra generic options such as with X or
dnl   AC_ARG_PROGRAM.  Displayed only in long --help.
dnl
dnl - HELP_CANON
dnl   Support for cross compilation (--build, --host and --target).
dnl   Display only in long --help.
dnl
dnl - HELP_ENABLE
dnl   which starts with the trailer of the HELP_BEGIN, HELP_CANON section,
dnl   then implements the header of the non generic options.
dnl
dnl - HELP_WITH
dnl
dnl - HELP_VAR
dnl
dnl - HELP_VAR_END
dnl
dnl - HELP_END
dnl   initialized below, in which we dump the trailer (handling of the
dnl   recursion for instance).
m4_divert_push([HELP_ENABLE])dnl
_ACEOF
fi

if test -n "$ac_init_help"; then
m4_ifset([AC_PACKAGE_STRING],
[  case $ac_init_help in
     short | recursive ) echo "Configuration of AC_PACKAGE_STRING:";;
   esac])
  cat <<\_ACEOF
m4_divert_pop([HELP_ENABLE])dnl
m4_divert_push([HELP_END])dnl

Report bugs to m4_ifset([AC_PACKAGE_BUGREPORT], [<AC_PACKAGE_BUGREPORT>],
  [the package provider]).dnl
m4_ifdef([AC_PACKAGE_NAME], [m4_ifset([AC_PACKAGE_URL], [
AC_PACKAGE_NAME home page: <AC_PACKAGE_URL>.])dnl
m4_if(m4_index(m4_defn([AC_PACKAGE_NAME]), [GNU ]), [0], [
General help using GNU software: <http://www.gnu.org/gethelp/>.])])
_ACEOF
ac_status=$?
fi

if test "$ac_init_help" = "recursive"; then
  # If there are subdirs, report their specific --help.
  for ac_dir in : $ac_subdirs_all; do test "x$ac_dir" = x: && continue
    test -d "$ac_dir" ||
      { cd "$srcdir" && ac_pwd=`pwd` && srcdir=. && test -d "$ac_dir"; } ||
      continue
    _AC_SRCDIRS(["$ac_dir"])
    cd "$ac_dir" || { ac_status=$?; continue; }
    # Check for guested configure.
    if test -f "$ac_srcdir/configure.gnu"; then
      echo &&
      $SHELL "$ac_srcdir/configure.gnu" --help=recursive
    elif test -f "$ac_srcdir/configure"; then
      echo &&
      $SHELL "$ac_srcdir/configure" --help=recursive
    else
      AC_MSG_WARN([no configuration information is in $ac_dir])
    fi || ac_status=$?
    cd "$ac_pwd" || { ac_status=$?; break; }
  done
fi

test -n "$ac_init_help" && exit $ac_status
m4_divert_pop([HELP_END])dnl
])# _AC_INIT_HELP


# _AC_INIT_VERSION
# ----------------
# Handle the `configure --version' message.
m4_define([_AC_INIT_VERSION],
[m4_divert_text([VERSION_BEGIN],
[if $ac_init_version; then
  cat <<\_ACEOF
m4_ifset([AC_PACKAGE_NAME], [AC_PACKAGE_NAME ])configure[]dnl
m4_ifset([AC_PACKAGE_VERSION], [ AC_PACKAGE_VERSION])
generated by m4_PACKAGE_STRING])
m4_divert_text([VERSION_END],
[_ACEOF
  exit
fi])dnl
])# _AC_INIT_VERSION


# _AC_INIT_CONFIG_LOG
# -------------------
# Initialize the config.log file descriptor and write header to it.
m4_define([_AC_INIT_CONFIG_LOG],
[m4_divert_text([INIT_PREPARE],
[m4_define([AS_MESSAGE_LOG_FD], 5)dnl
cat >config.log <<_ACEOF
This file contains any messages produced by compilers while
running configure, to aid debugging if configure makes a mistake.

It was created by m4_ifset([AC_PACKAGE_NAME], [AC_PACKAGE_NAME ])dnl
$as_me[]m4_ifset([AC_PACKAGE_VERSION], [ AC_PACKAGE_VERSION]), which was
generated by m4_PACKAGE_STRING.  Invocation command line was

  $ $[0] $[@]

_ACEOF
exec AS_MESSAGE_LOG_FD>>config.log
AS_UNAME >&AS_MESSAGE_LOG_FD

cat >&AS_MESSAGE_LOG_FD <<_ACEOF


m4_text_box([Core tests.])

_ACEOF
])])# _AC_INIT_CONFIG_LOG


# _AC_INIT_PREPARE
# ----------------
# Called by AC_INIT to build the preamble of the `configure' scripts.
# 1. Trap and clean up various tmp files.
# 2. Set up the fd and output files
# 3. Remember the options given to `configure' for `config.status --recheck'.
# 4. Initiates confdefs.h
# 5. Loads site and cache files
m4_define([_AC_INIT_PREPARE],
[m4_divert_push([INIT_PREPARE])dnl

# Keep a trace of the command line.
# Strip out --no-create and --no-recursion so they do not pile up.
# Strip out --silent because we don't want to record it for future runs.
# Also quote any args containing shell meta-characters.
# Make two passes to allow for proper duplicate-argument suppression.
ac_configure_args=
ac_configure_args0=
ac_configure_args1=
ac_must_keep_next=false
for ac_pass in 1 2
do
  for ac_arg
  do
    case $ac_arg in
    -no-create | --no-c* | -n | -no-recursion | --no-r*) continue ;;
    -q | -quiet | --quiet | --quie | --qui | --qu | --q \
    | -silent | --silent | --silen | --sile | --sil)
      continue ;;
    *\'*)
      ac_arg=`AS_ECHO(["$ac_arg"]) | sed "s/'/'\\\\\\\\''/g"` ;;
    esac
    case $ac_pass in
    1) AS_VAR_APPEND([ac_configure_args0], [" '$ac_arg'"]) ;;
    2)
      AS_VAR_APPEND([ac_configure_args1], [" '$ac_arg'"])
dnl If trying to remove duplicates, be sure to (i) keep the *last*
dnl value (e.g. --prefix=1 --prefix=2 --prefix=1 might keep 2 only),
dnl and (ii) not to strip long options (--prefix foo --prefix bar might
dnl give --prefix foo bar).
      if test $ac_must_keep_next = true; then
	ac_must_keep_next=false # Got value, back to normal.
      else
	case $ac_arg in
dnl Use broad patterns, as arguments that would have already made configure
dnl exit don't matter.
	  *=* | --config-cache | -C | -disable-* | --disable-* \
	  | -enable-* | --enable-* | -gas | --g* | -nfp | --nf* \
	  | -q | -quiet | --q* | -silent | --sil* | -v | -verb* \
	  | -with-* | --with-* | -without-* | --without-* | --x)
	    case "$ac_configure_args0 " in
	      "$ac_configure_args1"*" '$ac_arg' "* ) continue ;;
	    esac
	    ;;
	  -* ) ac_must_keep_next=true ;;
	esac
      fi
      AS_VAR_APPEND([ac_configure_args], [" '$ac_arg'"])
      ;;
    esac
  done
done
AS_UNSET(ac_configure_args0)
AS_UNSET(ac_configure_args1)

# When interrupted or exit'd, cleanup temporary files, and complete
# config.log.  We remove comments because anyway the quotes in there
# would cause problems or look ugly.
# WARNING: Use '\'' to represent an apostrophe within the trap.
# WARNING: Do not start the trap code with a newline, due to a FreeBSD 4.0 bug.
trap 'exit_status=$?
  # Save into config.log some information that might help in debugging.
  {
    echo

    AS_BOX([Cache variables.])
    echo
    m4_bpatsubsts(m4_defn([_AC_CACHE_DUMP]),
		  [^ *\(#.*\)?
],                [],
		  ['], ['\\''])
    echo

    AS_BOX([Output variables.])
    echo
    for ac_var in $ac_subst_vars
    do
      eval ac_val=\$$ac_var
      case $ac_val in
      *\'\''*) ac_val=`AS_ECHO(["$ac_val"]) | sed "s/'\''/'\''\\\\\\\\'\'''\''/g"`;;
      esac
      AS_ECHO(["$ac_var='\''$ac_val'\''"])
    done | sort
    echo

    if test -n "$ac_subst_files"; then
      AS_BOX([File substitutions.])
      echo
      for ac_var in $ac_subst_files
      do
	eval ac_val=\$$ac_var
	case $ac_val in
	*\'\''*) ac_val=`AS_ECHO(["$ac_val"]) | sed "s/'\''/'\''\\\\\\\\'\'''\''/g"`;;
	esac
	AS_ECHO(["$ac_var='\''$ac_val'\''"])
      done | sort
      echo
    fi

    if test -s confdefs.h; then
      AS_BOX([confdefs.h.])
      echo
      cat confdefs.h
      echo
    fi
    test "$ac_signal" != 0 &&
      AS_ECHO(["$as_me: caught signal $ac_signal"])
    AS_ECHO(["$as_me: exit $exit_status"])
  } >&AS_MESSAGE_LOG_FD
  rm -f core *.core core.conftest.* &&
    rm -f -r conftest* confdefs* conf$[$]* $ac_clean_files &&
    exit $exit_status
' 0
for ac_signal in 1 2 13 15; do
  trap 'ac_signal='$ac_signal'; AS_EXIT([1])' $ac_signal
done
ac_signal=0

# confdefs.h avoids OS command line length limits that DEFS can exceed.
rm -f -r conftest* confdefs.h

dnl AIX cpp loses on an empty file, NextStep 3.3 (patch 3) loses on a file
dnl containing less than 14 bytes (including the newline).
AS_ECHO(["/* confdefs.h */"]) > confdefs.h

# Predefined preprocessor variables.
AC_DEFINE_UNQUOTED([PACKAGE_NAME], ["$PACKAGE_NAME"],
		   [Define to the full name of this package.])dnl
AC_DEFINE_UNQUOTED([PACKAGE_TARNAME], ["$PACKAGE_TARNAME"],
		   [Define to the one symbol short name of this package.])dnl
AC_DEFINE_UNQUOTED([PACKAGE_VERSION], ["$PACKAGE_VERSION"],
		   [Define to the version of this package.])dnl
AC_DEFINE_UNQUOTED([PACKAGE_STRING], ["$PACKAGE_STRING"],
		   [Define to the full name and version of this package.])dnl
AC_DEFINE_UNQUOTED([PACKAGE_BUGREPORT], ["$PACKAGE_BUGREPORT"],
		   [Define to the address where bug reports for this package
		    should be sent.])dnl
AC_DEFINE_UNQUOTED([PACKAGE_URL], ["$PACKAGE_URL"],
		   [Define to the home page for this package.])

# Let the site file select an alternate cache file if it wants to.
AC_SITE_LOAD
AC_CACHE_LOAD
m4_divert_pop([INIT_PREPARE])dnl
])# _AC_INIT_PREPARE


# AU::AC_INIT([UNIQUE-FILE-IN-SOURCE-DIR])
# ----------------------------------------
# This macro is used only for Autoupdate.
AU_DEFUN([AC_INIT],
[m4_ifval([$2], [[AC_INIT($@)]],
	  [m4_ifval([$1],
[[AC_INIT]
AC_CONFIG_SRCDIR([$1])], [[AC_INIT]])])[]dnl
])


# AC_INIT([PACKAGE, VERSION, [BUG-REPORT], [TARNAME], [URL])
# ----------------------------------------------------------
# Include the user macro files, prepare the diversions, and output the
# preamble of the `configure' script.
#
# If BUG-REPORT is omitted, do without (unless the user previously
# defined the m4 macro AC_PACKAGE_BUGREPORT).  If TARNAME is omitted,
# use PACKAGE to seed it.  If URL is omitted, use
# `http://www.gnu.org/software/TARNAME/' if PACKAGE begins with `GNU',
# otherwise, do without.
#
# Note that the order is important: first initialize, then set the
# AC_CONFIG_SRCDIR.
m4_define([AC_INIT],
[# Forbidden tokens and exceptions.
m4_pattern_forbid([^_?A[CHUM]_])
m4_pattern_forbid([_AC_])
m4_pattern_forbid([^LIBOBJS$],
		  [do not use LIBOBJS directly, use AC_LIBOBJ (see section `AC_LIBOBJ vs LIBOBJS'])
# Actually reserved by M4sh.
m4_pattern_allow([^AS_FLAGS$])
AS_INIT[]dnl
AS_PREPARE[]dnl
m4_divert_push([KILL])
m4_ifval([$2], [_AC_INIT_PACKAGE($@)])
_AC_INIT_DEFAULTS
_AC_INIT_PARSE_ARGS
_AC_INIT_DIRCHECK
_AC_INIT_SRCDIR
_AC_INIT_HELP
_AC_INIT_VERSION
_AC_INIT_CONFIG_LOG
_AC_INIT_PREPARE
_AC_INIT_NOTICE
_AC_INIT_COPYRIGHT
m4_divert_text([SHELL_FN], [
m4_text_box([Autoconf initialization.])])
m4_divert_pop
m4_ifval([$2], , [m4_ifval([$1], [AC_CONFIG_SRCDIR([$1])])])dnl
dnl
dnl Substitute for predefined variables.
AC_SUBST([DEFS])dnl
AC_SUBST([ECHO_C])dnl
AC_SUBST([ECHO_N])dnl
AC_SUBST([ECHO_T])dnl
AC_SUBST([LIBS])dnl
_AC_ARG_VAR_PRECIOUS([build_alias])AC_SUBST([build_alias])dnl
_AC_ARG_VAR_PRECIOUS([host_alias])AC_SUBST([host_alias])dnl
_AC_ARG_VAR_PRECIOUS([target_alias])AC_SUBST([target_alias])dnl
dnl
AC_LANG_PUSH(C)
])




## ------------------------------------------------------------- ##
## Selecting optional features, working with optional software.  ##
## ------------------------------------------------------------- ##

# AC_PRESERVE_HELP_ORDER
# ----------------------
# Emit help strings in the order given, rather than grouping all --enable-FOO
# and all --with-BAR.
AC_DEFUN([AC_PRESERVE_HELP_ORDER],
[m4_divert_once([HELP_ENABLE], [[
Optional Features and Packages:
  --disable-option-checking  ignore unrecognized --enable/--with options
  --disable-FEATURE       do not include FEATURE (same as --enable-FEATURE=no)
  --enable-FEATURE[=ARG]  include FEATURE [ARG=yes]
  --with-PACKAGE[=ARG]    use PACKAGE [ARG=yes]
  --without-PACKAGE       do not use PACKAGE (same as --with-PACKAGE=no)]])
m4_define([_m4_divert(HELP_ENABLE)],    _m4_divert(HELP_WITH))
])# AC_PRESERVE_HELP_ORDER

# _AC_ENABLE_IF(OPTION, FEATURE, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# -------------------------------------------------------------------
# Common code for AC_ARG_ENABLE and AC_ARG_WITH.
# OPTION is either "enable" or "with".
#
m4_define([_AC_ENABLE_IF],
[@%:@ Check whether --$1-$2 was given.
_AC_ENABLE_IF_ACTION([$1], m4_translit([$2], [-+.], [___]), [$3], [$4])
])

m4_define([_AC_ENABLE_IF_ACTION],
[m4_append_uniq([_AC_USER_OPTS], [$1_$2], [
])dnl
AS_IF([test "${$1_$2+set}" = set], [$1val=$$1_$2; $3], [$4])dnl
])

# AC_ARG_ENABLE(FEATURE, HELP-STRING, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ------------------------------------------------------------------------
AC_DEFUN([AC_ARG_ENABLE],
[AC_PROVIDE_IFELSE([AC_PRESERVE_HELP_ORDER],
[],
[m4_divert_once([HELP_ENABLE], [[
Optional Features:
  --disable-option-checking  ignore unrecognized --enable/--with options
  --disable-FEATURE       do not include FEATURE (same as --enable-FEATURE=no)
  --enable-FEATURE[=ARG]  include FEATURE [ARG=yes]]])])dnl
m4_divert_once([HELP_ENABLE], [$2])dnl
_AC_ENABLE_IF([enable], [$1], [$3], [$4])dnl
])# AC_ARG_ENABLE


AU_DEFUN([AC_ENABLE],
[AC_ARG_ENABLE([$1], [  --enable-$1], [$2], [$3])])


# AC_ARG_WITH(PACKAGE, HELP-STRING, ACTION-IF-TRUE, [ACTION-IF-FALSE])
# --------------------------------------------------------------------
AC_DEFUN([AC_ARG_WITH],
[AC_PROVIDE_IFELSE([AC_PRESERVE_HELP_ORDER],
[],
[m4_divert_once([HELP_WITH], [[
Optional Packages:
  --with-PACKAGE[=ARG]    use PACKAGE [ARG=yes]
  --without-PACKAGE       do not use PACKAGE (same as --with-PACKAGE=no)]])])
m4_divert_once([HELP_WITH], [$2])dnl
_AC_ENABLE_IF([with], [$1], [$3], [$4])dnl
])# AC_ARG_WITH

AU_DEFUN([AC_WITH],
[AC_ARG_WITH([$1], [  --with-$1], [$2], [$3])])

# AC_DISABLE_OPTION_CHECKING
# --------------------------
AC_DEFUN([AC_DISABLE_OPTION_CHECKING],
[m4_divert_once([DEFAULTS], [enable_option_checking=no])
])# AC_DISABLE_OPTION_CHECKING


## ----------------------------------------- ##
## Remembering variables for reconfiguring.  ##
## ----------------------------------------- ##


# AC_ARG_VAR(VARNAME, DOCUMENTATION)
# ----------------------------------
# Register VARNAME as a precious variable, and document it in
# `configure --help' (but only once).
AC_DEFUN([AC_ARG_VAR],
[m4_divert_once([HELP_VAR], [[
Some influential environment variables:]])dnl
m4_divert_once([HELP_VAR_END], [[
Use these variables to override the choices made by `configure' or to help
it to find libraries and programs with nonstandard names/locations.]])dnl
m4_expand_once([m4_divert_text([HELP_VAR],
			       [AS_HELP_STRING([$1], [$2], [              ])])],
	       [$0($1)])dnl
AC_SUBST([$1])dnl
_AC_ARG_VAR_PRECIOUS([$1])dnl
])# AC_ARG_VAR


# _AC_ARG_VAR_PRECIOUS(VARNAME)
# -----------------------------
# Declare VARNAME is precious.
m4_define([_AC_ARG_VAR_PRECIOUS],
[m4_append_uniq([_AC_PRECIOUS_VARS], [$1], [
])dnl
])


# _AC_ARG_VAR_STORE
# -----------------
# We try to diagnose when precious variables have changed.  To do this,
# make two early snapshots (after the option processing to take
# explicit variables into account) of those variables: one (ac_env_)
# which represents the current run, and a second (ac_cv_env_) which,
# at the first run, will be saved in the cache.  As an exception to
# the cache mechanism, its loading will override these variables (non
# `ac_cv_env_' cache value are only set when unset).
#
# In subsequent runs, after having loaded the cache, compare
# ac_cv_env_foo against ac_env_foo.  See _AC_ARG_VAR_VALIDATE.
m4_define([_AC_ARG_VAR_STORE],
[m4_divert_text([PARSE_ARGS],
[for ac_var in $ac_precious_vars; do
  eval ac_env_${ac_var}_set=\${${ac_var}+set}
  eval ac_env_${ac_var}_value=\$${ac_var}
  eval ac_cv_env_${ac_var}_set=\${${ac_var}+set}
  eval ac_cv_env_${ac_var}_value=\$${ac_var}
done])dnl
])


# _AC_ARG_VAR_VALIDATE
# --------------------
# The precious variables are saved twice at the beginning of
# configure.  E.g., PRECIOUS is saved as `ac_env_PRECIOUS_set' and
# `ac_env_PRECIOUS_value' on the one hand and `ac_cv_env_PRECIOUS_set'
# and `ac_cv_env_PRECIOUS_value' on the other hand.
#
# Now the cache has just been loaded, so `ac_cv_env_' represents the
# content of the cached values, while `ac_env_' represents that of the
# current values.
#
# So we check that `ac_env_' and `ac_cv_env_' are consistent.  If
# they aren't, die.
m4_define([_AC_ARG_VAR_VALIDATE],
[m4_divert_text([INIT_PREPARE],
[# Check that the precious variables saved in the cache have kept the same
# value.
ac_cache_corrupted=false
for ac_var in $ac_precious_vars; do
  eval ac_old_set=\$ac_cv_env_${ac_var}_set
  eval ac_new_set=\$ac_env_${ac_var}_set
  eval ac_old_val=\$ac_cv_env_${ac_var}_value
  eval ac_new_val=\$ac_env_${ac_var}_value
  case $ac_old_set,$ac_new_set in
    set,)
      AS_MESSAGE([error: `$ac_var' was set to `$ac_old_val' in the previous run], 2)
      ac_cache_corrupted=: ;;
    ,set)
      AS_MESSAGE([error: `$ac_var' was not set in the previous run], 2)
      ac_cache_corrupted=: ;;
    ,);;
    *)
      if test "x$ac_old_val" != "x$ac_new_val"; then
	# differences in whitespace do not lead to failure.
	ac_old_val_w=`echo x $ac_old_val`
	ac_new_val_w=`echo x $ac_new_val`
	if test "$ac_old_val_w" != "$ac_new_val_w"; then
	  AS_MESSAGE([error: `$ac_var' has changed since the previous run:], 2)
	  ac_cache_corrupted=:
	else
	  AS_MESSAGE([warning: ignoring whitespace changes in `$ac_var' since the previous run:], 2)
	  eval $ac_var=\$ac_old_val
	fi
	AS_MESSAGE([  former value:  `$ac_old_val'], 2)
	AS_MESSAGE([  current value: `$ac_new_val'], 2)
      fi;;
  esac
  # Pass precious variables to config.status.
  if test "$ac_new_set" = set; then
    case $ac_new_val in
    *\'*) ac_arg=$ac_var=`AS_ECHO(["$ac_new_val"]) | sed "s/'/'\\\\\\\\''/g"` ;;
    *) ac_arg=$ac_var=$ac_new_val ;;
    esac
    case " $ac_configure_args " in
      *" '$ac_arg' "*) ;; # Avoid dups.  Use of quotes ensures accuracy.
      *) AS_VAR_APPEND([ac_configure_args], [" '$ac_arg'"]) ;;
    esac
  fi
done
if $ac_cache_corrupted; then
  AS_MESSAGE([error: in `$ac_pwd':], 2)
  AS_MESSAGE([error: changes in the environment can compromise the build], 2)
  AS_ERROR([run `make distclean' and/or `rm $cache_file' and start over])
fi])dnl
])# _AC_ARG_VAR_VALIDATE





## ---------------------------- ##
## Transforming program names.  ##
## ---------------------------- ##


# AC_ARG_PROGRAM
# --------------
# This macro is expanded only once, to avoid that `foo' ends up being
# installed as `ggfoo'.
AC_DEFUN_ONCE([AC_ARG_PROGRAM],
[dnl Document the options.
m4_divert_push([HELP_BEGIN])dnl

Program names:
  --program-prefix=PREFIX            prepend PREFIX to installed program names
  --program-suffix=SUFFIX            append SUFFIX to installed program names
  --program-transform-name=PROGRAM   run sed PROGRAM on installed program names
m4_divert_pop([HELP_BEGIN])dnl
test "$program_prefix" != NONE &&
  program_transform_name="s&^&$program_prefix&;$program_transform_name"
# Use a double $ so make ignores it.
test "$program_suffix" != NONE &&
  program_transform_name="s&\$&$program_suffix&;$program_transform_name"
# Double any \ or $.
# By default was `s,x,x', remove it if useless.
[ac_script='s/[\\$]/&&/g;s/;s,x,x,$//']
program_transform_name=`AS_ECHO(["$program_transform_name"]) | sed "$ac_script"`
])# AC_ARG_PROGRAM





## ------------------------- ##
## Finding auxiliary files.  ##
## ------------------------- ##


# AC_CONFIG_AUX_DIR(DIR)
# ----------------------
# Find install-sh, config.sub, config.guess, and Cygnus configure
# in directory DIR.  These are auxiliary files used in configuration.
# DIR can be either absolute or relative to $srcdir.
AC_DEFUN([AC_CONFIG_AUX_DIR],
[AC_CONFIG_AUX_DIRS($1 "$srcdir"/$1)])


# AC_CONFIG_AUX_DIR_DEFAULT
# -------------------------
# The default is `$srcdir' or `$srcdir/..' or `$srcdir/../..'.
# There's no need to call this macro explicitly; just AC_REQUIRE it.
AC_DEFUN([AC_CONFIG_AUX_DIR_DEFAULT],
[AC_CONFIG_AUX_DIRS("$srcdir" "$srcdir/.." "$srcdir/../..")])


# AC_CONFIG_AUX_DIRS(DIR ...)
# ---------------------------
# Internal subroutine.
# Search for the configuration auxiliary files in directory list $1.
# We look only for install-sh, so users of AC_PROG_INSTALL
# do not automatically need to distribute the other auxiliary files.
AC_DEFUN([AC_CONFIG_AUX_DIRS],
[ac_aux_dir=
for ac_dir in $1; do
  if test -f "$ac_dir/install-sh"; then
    ac_aux_dir=$ac_dir
    ac_install_sh="$ac_aux_dir/install-sh -c"
    break
  elif test -f "$ac_dir/install.sh"; then
    ac_aux_dir=$ac_dir
    ac_install_sh="$ac_aux_dir/install.sh -c"
    break
  elif test -f "$ac_dir/shtool"; then
    ac_aux_dir=$ac_dir
    ac_install_sh="$ac_aux_dir/shtool install -c"
    break
  fi
done
if test -z "$ac_aux_dir"; then
  AC_MSG_ERROR([cannot find install-sh, install.sh, or shtool in $1])
fi

# These three variables are undocumented and unsupported,
# and are intended to be withdrawn in a future Autoconf release.
# They can cause serious problems if a builder's source tree is in a directory
# whose full name contains unusual characters.
ac_config_guess="$SHELL $ac_aux_dir/config.guess"  # Please don't use this var.
ac_config_sub="$SHELL $ac_aux_dir/config.sub"  # Please don't use this var.
ac_configure="$SHELL $ac_aux_dir/configure"  # Please don't use this var.

AC_PROVIDE([AC_CONFIG_AUX_DIR_DEFAULT])dnl
])# AC_CONFIG_AUX_DIRS




## ------------------------ ##
## Finding aclocal macros.  ##
## ------------------------ ##


# AC_CONFIG_MACRO_DIR(DIR)
# ------------------------
# Declare directory containing additional macros for aclocal.
AC_DEFUN([AC_CONFIG_MACRO_DIR], [])



## --------------------- ##
## Requiring aux files.  ##
## --------------------- ##

# AC_REQUIRE_AUX_FILE(FILE)
# -------------------------
# This macro does nothing, it's a hook to be read with `autoconf --trace'.
# It announces FILE is required in the auxdir.
m4_define([AC_REQUIRE_AUX_FILE],
[AS_LITERAL_WORD_IF([$1], [],
	       [m4_fatal([$0: requires a literal argument])])])



## ----------------------------------- ##
## Getting the canonical system type.  ##
## ----------------------------------- ##

# The inputs are:
#    configure --host=HOST --target=TARGET --build=BUILD
#
# The rules are:
# 1. Build defaults to the current platform, as determined by config.guess.
# 2. Host defaults to build.
# 3. Target defaults to host.


# _AC_CANONICAL_SPLIT(THING)
# --------------------------
# Generate the variables THING, THING_{alias cpu vendor os}.
m4_define([_AC_CANONICAL_SPLIT],
[case $ac_cv_$1 in
*-*-*) ;;
*) AC_MSG_ERROR([invalid value of canonical $1]);;
esac
AC_SUBST([$1], [$ac_cv_$1])dnl
ac_save_IFS=$IFS; IFS='-'
set x $ac_cv_$1
shift
AC_SUBST([$1_cpu], [$[1]])dnl
AC_SUBST([$1_vendor], [$[2]])dnl
shift; shift
[# Remember, the first character of IFS is used to create $]*,
# except with old shells:
$1_os=$[*]
IFS=$ac_save_IFS
case $$1_os in *\ *) $1_os=`echo "$$1_os" | sed 's/ /-/g'`;; esac
AC_SUBST([$1_os])dnl
])# _AC_CANONICAL_SPLIT


# AC_CANONICAL_BUILD
# ------------------
AC_DEFUN_ONCE([AC_CANONICAL_BUILD],
[AC_REQUIRE([AC_CONFIG_AUX_DIR_DEFAULT])dnl
AC_REQUIRE_AUX_FILE([config.sub])dnl
AC_REQUIRE_AUX_FILE([config.guess])dnl
m4_divert_once([HELP_CANON],
[[
System types:
  --build=BUILD     configure for building on BUILD [guessed]]])dnl
# Make sure we can run config.sub.
$SHELL "$ac_aux_dir/config.sub" sun4 >/dev/null 2>&1 ||
  AC_MSG_ERROR([cannot run $SHELL $ac_aux_dir/config.sub])

AC_CACHE_CHECK([build system type], [ac_cv_build],
[ac_build_alias=$build_alias
test "x$ac_build_alias" = x &&
  ac_build_alias=`$SHELL "$ac_aux_dir/config.guess"`
test "x$ac_build_alias" = x &&
  AC_MSG_ERROR([cannot guess build type; you must specify one])
ac_cv_build=`$SHELL "$ac_aux_dir/config.sub" $ac_build_alias` ||
  AC_MSG_ERROR([$SHELL $ac_aux_dir/config.sub $ac_build_alias failed])
])
_AC_CANONICAL_SPLIT(build)
])# AC_CANONICAL_BUILD


# AC_CANONICAL_HOST
# -----------------
AC_DEFUN_ONCE([AC_CANONICAL_HOST],
[AC_REQUIRE([AC_CANONICAL_BUILD])dnl
m4_divert_once([HELP_CANON],
[[  --host=HOST       cross-compile to build programs to run on HOST [BUILD]]])dnl
AC_CACHE_CHECK([host system type], [ac_cv_host],
[if test "x$host_alias" = x; then
  ac_cv_host=$ac_cv_build
else
  ac_cv_host=`$SHELL "$ac_aux_dir/config.sub" $host_alias` ||
    AC_MSG_ERROR([$SHELL $ac_aux_dir/config.sub $host_alias failed])
fi
])
_AC_CANONICAL_SPLIT([host])
])# AC_CANONICAL_HOST


# AC_CANONICAL_TARGET
# -------------------
AC_DEFUN_ONCE([AC_CANONICAL_TARGET],
[AC_REQUIRE([AC_CANONICAL_HOST])dnl
AC_BEFORE([$0], [AC_ARG_PROGRAM])dnl
m4_divert_once([HELP_CANON],
[[  --target=TARGET   configure for building compilers for TARGET [HOST]]])dnl
AC_CACHE_CHECK([target system type], [ac_cv_target],
[if test "x$target_alias" = x; then
  ac_cv_target=$ac_cv_host
else
  ac_cv_target=`$SHELL "$ac_aux_dir/config.sub" $target_alias` ||
    AC_MSG_ERROR([$SHELL $ac_aux_dir/config.sub $target_alias failed])
fi
])
_AC_CANONICAL_SPLIT([target])

# The aliases save the names the user supplied, while $host etc.
# will get canonicalized.
test -n "$target_alias" &&
  test "$program_prefix$program_suffix$program_transform_name" = \
    NONENONEs,x,x, &&
  program_prefix=${target_alias}-[]dnl
])# AC_CANONICAL_TARGET


AU_ALIAS([AC_CANONICAL_SYSTEM], [AC_CANONICAL_TARGET])


# AU::AC_VALIDATE_CACHED_SYSTEM_TUPLE([CMD])
# ------------------------------------------
# If the cache file is inconsistent with the current host,
# target and build system types, execute CMD or print a default
# error message.  Now handled via _AC_ARG_VAR_PRECIOUS.
AU_DEFUN([AC_VALIDATE_CACHED_SYSTEM_TUPLE], [])


## ---------------------- ##
## Caching test results.  ##
## ---------------------- ##


# AC_SITE_LOAD
# ------------
# Look for site- or system-specific initialization scripts.
m4_define([AC_SITE_LOAD],
[# Prefer an explicitly selected file to automatically selected ones.
ac_site_file1=NONE
ac_site_file2=NONE
if test -n "$CONFIG_SITE"; then
  # We do not want a PATH search for config.site.
  case $CONFIG_SITE in @%:@((
    -*)  ac_site_file1=./$CONFIG_SITE;;
    */*) ac_site_file1=$CONFIG_SITE;;
    *)   ac_site_file1=./$CONFIG_SITE;;
  esac
elif test "x$prefix" != xNONE; then
  ac_site_file1=$prefix/share/config.site
  ac_site_file2=$prefix/etc/config.site
else
  ac_site_file1=$ac_default_prefix/share/config.site
  ac_site_file2=$ac_default_prefix/etc/config.site
fi
for ac_site_file in "$ac_site_file1" "$ac_site_file2"
do
  test "x$ac_site_file" = xNONE && continue
  if test /dev/null != "$ac_site_file" && test -r "$ac_site_file"; then
    AC_MSG_NOTICE([loading site script $ac_site_file])
    sed 's/^/| /' "$ac_site_file" >&AS_MESSAGE_LOG_FD
    . "$ac_site_file" \
      || AC_MSG_FAILURE([failed to load site script $ac_site_file])
  fi
done
])


# AC_CACHE_LOAD
# -------------
m4_define([AC_CACHE_LOAD],
[if test -r "$cache_file"; then
  # Some versions of bash will fail to source /dev/null (special files
  # actually), so we avoid doing that.  DJGPP emulates it as a regular file.
  if test /dev/null != "$cache_file" && test -f "$cache_file"; then
    AC_MSG_NOTICE([loading cache $cache_file])
    case $cache_file in
      [[\\/]]* | ?:[[\\/]]* ) . "$cache_file";;
      *)                      . "./$cache_file";;
    esac
  fi
else
  AC_MSG_NOTICE([creating cache $cache_file])
  >$cache_file
fi
])# AC_CACHE_LOAD


# _AC_CACHE_DUMP
# --------------
# Dump the cache to stdout.  It can be in a pipe (this is a requirement).
m4_define([_AC_CACHE_DUMP],
[# The following way of writing the cache mishandles newlines in values,
# but we know of no workaround that is simple, portable, and efficient.
# So, we kill variables containing newlines.
# Ultrix sh set writes to stderr and can't be redirected directly,
# and sets the high bit in the cache file unless we assign to the vars.
(
  for ac_var in `(set) 2>&1 | sed -n ['s/^\([a-zA-Z_][a-zA-Z0-9_]*\)=.*/\1/p']`; do
    eval ac_val=\$$ac_var
    case $ac_val in #(
    *${as_nl}*)
      case $ac_var in #(
      *_cv_*) AC_MSG_WARN([cache variable $ac_var contains a newline]) ;;
      esac
      case $ac_var in #(
      _ | IFS | as_nl) ;; #(
      BASH_ARGV | BASH_SOURCE) eval $ac_var= ;; #(
      *) AS_UNSET([$ac_var]) ;;
      esac ;;
    esac
  done

  (set) 2>&1 |
    case $as_nl`(ac_space=' '; set) 2>&1` in #(
    *${as_nl}ac_space=\ *)
      # `set' does not quote correctly, so add quotes: double-quote
      # substitution turns \\\\ into \\, and sed turns \\ into \.
      sed -n \
	["s/'/'\\\\''/g;
	  s/^\\([_$as_cr_alnum]*_cv_[_$as_cr_alnum]*\\)=\\(.*\\)/\\1='\\2'/p"]
      ;; #(
    *)
      # `set' quotes correctly as required by POSIX, so do not add quotes.
      sed -n ["/^[_$as_cr_alnum]*_cv_[_$as_cr_alnum]*=/p"]
      ;;
    esac |
    sort
)dnl
])# _AC_CACHE_DUMP


# AC_CACHE_SAVE
# -------------
# Save the cache.
# Allow a site initialization script to override cache values.
m4_define([AC_CACHE_SAVE],
[cat >confcache <<\_ACEOF
# This file is a shell script that caches the results of configure
# tests run on this system so they can be shared between configure
# scripts and configure runs, see configure's option --config-cache.
# It is not useful on other systems.  If it contains results you don't
# want to keep, you may remove or edit it.
#
# config.status only pays attention to the cache file if you give it
# the --recheck option to rerun configure.
#
# `ac_cv_env_foo' variables (set or unset) will be overridden when
# loading this file, other *unset* `ac_cv_foo' will be assigned the
# following values.

_ACEOF

_AC_CACHE_DUMP() |
  sed ['
     /^ac_cv_env_/b end
     t clear
     :clear
     s/^\([^=]*\)=\(.*[{}].*\)$/test "${\1+set}" = set || &/
     t end
     s/^\([^=]*\)=\(.*\)$/\1=${\1=\2}/
     :end'] >>confcache
if diff "$cache_file" confcache >/dev/null 2>&1; then :; else
  if test -w "$cache_file"; then
    if test "x$cache_file" != "x/dev/null"; then
      AC_MSG_NOTICE([updating cache $cache_file])
      if test ! -f "$cache_file" || test -h "$cache_file"; then
	cat confcache >"$cache_file"
      else
dnl Try to update the cache file atomically even on different mount points;
dnl at the same time, avoid filename limitation issues in the common case.
        case $cache_file in #(
        */* | ?:*)
	  mv -f confcache "$cache_file"$$ &&
	  mv -f "$cache_file"$$ "$cache_file" ;; #(
        *)
	  mv -f confcache "$cache_file" ;;
	esac
      fi
    fi
  else
    AC_MSG_NOTICE([not updating unwritable cache $cache_file])
  fi
fi
rm -f confcache[]dnl
])# AC_CACHE_SAVE


# AC_CACHE_VAL(CACHE-ID, COMMANDS-TO-SET-IT)
# ------------------------------------------
# The name of shell var CACHE-ID must contain `_cv_' in order to get saved.
# Should be dnl'ed.  Try to catch common mistakes.
m4_defun([AC_CACHE_VAL],
[AS_LITERAL_WORD_IF([$1], [m4_if(m4_index(m4_quote($1), [_cv_]), [-1],
			    [AC_DIAGNOSE([syntax],
[$0($1, ...): suspicious cache-id, must contain _cv_ to be cached])])])dnl
m4_if(m4_index([$2], [AC_DEFINE]), [-1], [],
      [AC_DIAGNOSE([syntax],
[$0($1, ...): suspicious presence of an AC_DEFINE in the second argument, ]dnl
[where no actions should be taken])])dnl
m4_if(m4_index([$2], [AC_SUBST]), [-1], [],
      [AC_DIAGNOSE([syntax],
[$0($1, ...): suspicious presence of an AC_SUBST in the second argument, ]dnl
[where no actions should be taken])])dnl
AS_VAR_SET_IF([$1],
	      [_AS_ECHO_N([(cached) ])],
	      [$2])
])


# AC_CACHE_CHECK(MESSAGE, CACHE-ID, COMMANDS)
# -------------------------------------------
# Do not call this macro with a dnl right behind.
m4_defun([AC_CACHE_CHECK],
[AC_MSG_CHECKING([$1])
AC_CACHE_VAL([$2], [$3])dnl
AS_LITERAL_WORD_IF([$2],
	      [AC_MSG_RESULT([$$2])],
	      [AS_VAR_COPY([ac_res], [$2])
	       AC_MSG_RESULT([$ac_res])])dnl
])

# _AC_CACHE_CHECK_INT(MESSAGE, CACHE-ID, EXPRESSION,
#                     [PROLOGUE = DEFAULT-INCLUDES], [IF-FAILS])
# --------------------------------------------------------------
AC_DEFUN([_AC_CACHE_CHECK_INT],
[AC_CACHE_CHECK([$1], [$2],
   [AC_COMPUTE_INT([$2], [$3], [$4], [$5])])
])# _AC_CACHE_CHECK_INT



## ---------------------- ##
## Defining CPP symbols.  ##
## ---------------------- ##


# AC_DEFINE_TRACE_LITERAL(LITERAL-CPP-SYMBOL)
# -------------------------------------------
# Used by --trace to collect the list of AC_DEFINEd macros.
m4_define([AC_DEFINE_TRACE_LITERAL],
[m4_pattern_allow([^$1$])dnl
AS_IDENTIFIER_IF([$1], [],
  [m4_warn([syntax], [AC_DEFINE: not an identifier: $1])])dnl
])# AC_DEFINE_TRACE_LITERAL


# AC_DEFINE_TRACE(CPP-SYMBOL)
# ---------------------------
# This macro is a wrapper around AC_DEFINE_TRACE_LITERAL which filters
# out non literal symbols.  CPP-SYMBOL must not include any parameters.
m4_define([AC_DEFINE_TRACE],
[AS_LITERAL_WORD_IF([$1], [AC_DEFINE_TRACE_LITERAL(_m4_expand([$1]))])])


# AC_DEFINE(VARIABLE, [VALUE], [DESCRIPTION])
# -------------------------------------------
# Set VARIABLE to VALUE, verbatim, or 1.  Remember the value
# and if VARIABLE is affected the same VALUE, do nothing, else
# die.  The third argument is used by autoheader.
m4_define([AC_DEFINE], [_AC_DEFINE_Q([_$0], $@)])

# _AC_DEFINE(STRING)
# ------------------
# Append the pre-expanded STRING and a newline to confdefs.h, as if by
# a quoted here-doc.
m4_define([_AC_DEFINE],
[AS_ECHO(["AS_ESCAPE([[$1]])"]) >>confdefs.h])


# AC_DEFINE_UNQUOTED(VARIABLE, [VALUE], [DESCRIPTION])
# ----------------------------------------------------
# Similar, but perform shell substitutions $ ` \ once on VALUE, as
# in an unquoted here-doc.
m4_define([AC_DEFINE_UNQUOTED], [_AC_DEFINE_Q([_$0], $@)])

# _AC_DEFINE_UNQUOTED(STRING)
# ---------------------------
# Append the pre-expanded STRING and a newline to confdefs.h, as if
# with an unquoted here-doc, but avoiding a fork in the common case of
# no backslash, no command substitution, no complex variable
# substitution, and no quadrigraphs.
m4_define([_AC_DEFINE_UNQUOTED],
[m4_if(m4_bregexp([$1], [\\\|`\|\$(\|\${\|@]), [-1],
       [AS_ECHO(["AS_ESCAPE([$1], [""])"]) >>confdefs.h],
       [cat >>confdefs.h <<_ACEOF
[$1]
_ACEOF])])


# _AC_DEFINE_Q(MACRO, VARIABLE, [VALUE], [DESCRIPTION])
# -----------------------------------------------------
# Internal function that performs common elements of AC_DEFINE{,_UNQUOTED}.
# MACRO must take one argument, which is the fully expanded string to
# append to confdefs.h as if by a possibly-quoted here-doc.
#
# m4_index is roughly 5 to 8 times faster than m4_bpatsubst, so we use
# m4_format rather than regex to grab prefix up to first ().  AC_name
# is defined with over-quotation, so that we can avoid m4_defn; this
# is only safe because the name should not contain $.
#
# Guarantee a match in m4_index, so as to avoid a bug with precision
# -1 in m4_format in older m4.
m4_define([_AC_DEFINE_Q],
[m4_pushdef([AC_name], m4_format([[[%.*s]]], m4_index([$2(], [(]), [$2]))]dnl
[AC_DEFINE_TRACE(AC_name)]dnl
[m4_cond([m4_index([$3], [
])], [-1], [],
	[m4_bregexp([[$3]], [[^\\]
], [-])], [], [],
	[m4_warn([syntax], [AC_DEFINE]m4_if([$1], [_AC_DEFINE], [],
  [[_UNQUOTED]])[: `$3' is not a valid preprocessor define value])])]dnl
[m4_ifval([$4], [AH_TEMPLATE(AC_name, [$4])
])_m4_popdef([AC_name])]dnl
[$1(m4_expand([[@%:@define] $2 ]m4_if([$#], 2, 1,
  [$3], [], [/**/], [[$3]])))
])



## -------------------------- ##
## Setting output variables.  ##
## -------------------------- ##


# AC_SUBST_TRACE(VARIABLE)
# ------------------------
# This macro is used with --trace to collect the list of substituted variables.
m4_define([AC_SUBST_TRACE])


# AC_SUBST(VARIABLE, [VALUE])
# ---------------------------
# Create an output variable from a shell VARIABLE.  If VALUE is given
# assign it to VARIABLE.  Use `""' if you want to set VARIABLE to an
# empty value, not an empty second argument.
#
m4_define([AC_SUBST],
[AS_IDENTIFIER_IF([$1], [],
  [m4_fatal([$0: `$1' is not a valid shell variable name])])]dnl
[AC_SUBST_TRACE([$1])]dnl
[m4_pattern_allow([^$1$])]dnl
[m4_ifvaln([$2], [$1=$2])[]]dnl
[m4_set_add([_AC_SUBST_VARS], [$1])])# AC_SUBST


# AC_SUBST_FILE(VARIABLE)
# -----------------------
# Read the comments of the preceding macro.
m4_define([AC_SUBST_FILE],
[m4_pattern_allow([^$1$])dnl
m4_append_uniq([_AC_SUBST_FILES], [$1], [
])])



## --------------------------------------- ##
## Printing messages at autoconf runtime.  ##
## --------------------------------------- ##

# In fact, I think we should promote the use of m4_warn and m4_fatal
# directly.  This will also avoid to some people to get it wrong
# between AC_FATAL and AC_MSG_ERROR.


# AC_DIAGNOSE(CATEGORY, MESSAGE)
# AC_FATAL(MESSAGE, [EXIT-STATUS])
# --------------------------------
m4_define([AC_DIAGNOSE], [m4_warn($@)])
m4_define([AC_FATAL],    [m4_fatal($@)])


# AC_WARNING(MESSAGE)
# -------------------
# Report a MESSAGE to the user of autoconf if `-W' or `-W all' was
# specified.
m4_define([AC_WARNING],
[AC_DIAGNOSE([syntax], [$1])])




## ---------------------------------------- ##
## Printing messages at configure runtime.  ##
## ---------------------------------------- ##


# AC_MSG_CHECKING(FEATURE)
# ------------------------
m4_define([AC_MSG_CHECKING],
[{ _AS_ECHO_LOG([checking $1])
_AS_ECHO_N([checking $1... ]); }dnl
])


# AC_MSG_RESULT(RESULT)
# ---------------------
m4_define([AC_MSG_RESULT],
[{ _AS_ECHO_LOG([result: $1])
_AS_ECHO([$1]); }dnl
])


# AC_MSG_WARN(PROBLEM)
# AC_MSG_NOTICE(STRING)
# AC_MSG_ERROR(ERROR, [EXIT-STATUS = 1])
# AC_MSG_FAILURE(ERROR, [EXIT-STATUS = 1])
# ----------------------------------------
m4_copy([AS_WARN],    [AC_MSG_WARN])
m4_copy([AS_MESSAGE], [AC_MSG_NOTICE])
m4_copy([AS_ERROR],   [AC_MSG_ERROR])
m4_define([AC_MSG_FAILURE],
[{ AS_MESSAGE([error: in `$ac_pwd':], 2)
AC_MSG_ERROR([$1
See `config.log' for more details], [$2]); }])


# _AC_MSG_LOG_CONFTEST
# --------------------
m4_define([_AC_MSG_LOG_CONFTEST],
[AS_ECHO(["$as_me: failed program was:"]) >&AS_MESSAGE_LOG_FD
sed 's/^/| /' conftest.$ac_ext >&AS_MESSAGE_LOG_FD
])


# AU::AC_CHECKING(FEATURE)
# ------------------------
AU_DEFUN([AC_CHECKING],
[AS_MESSAGE([checking $1...])])


# AU::AC_MSG_RESULT_UNQUOTED(RESULT)
# ----------------------------------
# No escaping, so it performed also backtick substitution.
AU_DEFUN([AC_MSG_RESULT_UNQUOTED],
[_AS_ECHO_UNQUOTED([$as_me:${as_lineno-$LINENO}: result: $1], AS_MESSAGE_LOG_FD)
_AS_ECHO_UNQUOTED([$1])[]dnl
])


# AU::AC_VERBOSE(STRING)
# ----------------------
AU_ALIAS([AC_VERBOSE], [AC_MSG_RESULT])






## ---------------------------- ##
## Compiler-running mechanics.  ##
## ---------------------------- ##


# _AC_RUN_LOG(COMMAND, LOG-COMMANDS)
# ----------------------------------
# Eval COMMAND, save the exit status in ac_status, and log it.  The return
# code is 0 if COMMAND succeeded, so that it can be used directly in AS_IF
# constructs.
AC_DEFUN([_AC_RUN_LOG],
[{ { $2; } >&AS_MESSAGE_LOG_FD
  ($1) 2>&AS_MESSAGE_LOG_FD
  ac_status=$?
  _AS_ECHO_LOG([\$? = $ac_status])
  test $ac_status = 0; }])


# _AC_RUN_LOG_STDERR(COMMAND, LOG-COMMANDS)
# -----------------------------------------
# Run COMMAND, save its stderr into conftest.err, save the exit status
# in ac_status, and log it.  Don't forget to clean up conftest.err after
# use.
# Note that when tracing, most shells will leave the traces in stderr
# starting with "+": that's what this macro tries to address.
# The return code is 0 if COMMAND succeeded, so that it can be used directly
# in AS_IF constructs.
AC_DEFUN([_AC_RUN_LOG_STDERR],
[{ { $2; } >&AS_MESSAGE_LOG_FD
  ($1) 2>conftest.err
  ac_status=$?
  if test -s conftest.err; then
    grep -v '^ *+' conftest.err >conftest.er1
    cat conftest.er1 >&AS_MESSAGE_LOG_FD
    mv -f conftest.er1 conftest.err
  fi
  _AS_ECHO_LOG([\$? = $ac_status])
  test $ac_status = 0; }])


# _AC_RUN_LOG_LIMIT(COMMAND, LOG-COMMANDS, [LINES])
# -------------------------------------------------
# Like _AC_RUN_LOG, but only log LINES lines from stderr,
# defaulting to 10 lines.
AC_DEFUN([_AC_RUN_LOG_LIMIT],
[{ { $2; } >&AS_MESSAGE_LOG_FD
  ($1) 2>conftest.err
  ac_status=$?
  if test -s conftest.err; then
    sed 'm4_default([$3], [10])a\
... rest of stderr output deleted ...
         m4_default([$3], [10])q' conftest.err >conftest.er1
    cat conftest.er1 >&AS_MESSAGE_LOG_FD
  fi
  rm -f conftest.er1 conftest.err
  _AS_ECHO_LOG([\$? = $ac_status])
  test $ac_status = 0; }])


# _AC_DO_ECHO(COMMAND)
# --------------------
# Echo COMMAND.  This is designed to be used just before evaluating COMMAND.
AC_DEFUN([_AC_DO_ECHO],
[m4_if([$1], [$ac_try], [], [ac_try="$1"
])]dnl
dnl If the string contains '\"', '`', or '\\', then just echo it rather
dnl than expanding it.  This is a hack, but it is safer, while also
dnl typically expanding simple substrings like '$CC', which is what we want.
dnl
dnl Much of this macro body is quoted, to work around misuses like
dnl `AC_CHECK_FUNC(sigblock, , AC_CHECK_LIB(bsd, sigblock))',
dnl which underquotes the 3rd arg and would misbehave if we didn't quote here.
dnl The "(($ac_try" instead of $ac_try avoids problems with even-worse
dnl underquoting misuses, such as
dnl `AC_CHECK_FUNC(foo, , AC_CHECK_LIB(a, foo, , AC_CHECK_LIB(b, foo)))'.
dnl We normally wouldn't bother with this kind of workaround for invalid code
dnl but this change was put in just before Autoconf 2.60 and we wanted to
dnl minimize the integration hassle.
[[case "(($ac_try" in
  *\"* | *\`* | *\\*) ac_try_echo=\$ac_try;;
  *) ac_try_echo=$ac_try;;
esac
eval ac_try_echo="\"\$as_me:${as_lineno-$LINENO}: $ac_try_echo\""]
AS_ECHO(["$ac_try_echo"])])

# _AC_DO(COMMAND)
# ---------------
# Eval COMMAND, save the exit status in ac_status, and log it.
# For internal use only.
AC_DEFUN([_AC_DO],
[_AC_RUN_LOG([eval "$1"],
	     [_AC_DO_ECHO([$1])])])


# _AC_DO_STDERR(COMMAND)
# ----------------------
# Like _AC_RUN_LOG_STDERR, but eval (instead of running) COMMAND.
AC_DEFUN([_AC_DO_STDERR],
[_AC_RUN_LOG_STDERR([eval "$1"],
		    [_AC_DO_ECHO([$1])])])


# _AC_DO_VAR(VARIABLE)
# --------------------
# Evaluate "$VARIABLE", which should be a valid shell command.
# The purpose of this macro is to write "configure:123: command line"
# into config.log for every test run.
AC_DEFUN([_AC_DO_VAR],
[_AC_DO([$$1])])


# _AC_DO_TOKENS(COMMAND)
# ----------------------
# Like _AC_DO_VAR, but execute COMMAND instead, where COMMAND is a series of
# tokens of the shell command language.
AC_DEFUN([_AC_DO_TOKENS],
[{ ac_try='$1'
  _AC_DO([$ac_try]); }])


# _AC_DO_LIMIT(COMMAND, [LINES])
# ------------------------------
# Like _AC_DO, but limit the amount of stderr lines logged to LINES.
# For internal use only.
AC_DEFUN([_AC_DO_LIMIT],
[_AC_RUN_LOG_LIMIT([eval "$1"],
		   [_AC_DO_ECHO([$1])], [$2])])


# _AC_EVAL(COMMAND)
# -----------------
# Eval COMMAND, save the exit status in ac_status, and log it.
# Unlike _AC_DO, this macro mishandles quoted arguments in some cases.
# It is present only for backward compatibility with previous Autoconf versions.
AC_DEFUN([_AC_EVAL],
[_AC_RUN_LOG([eval $1],
	     [eval echo "\"\$as_me\":${as_lineno-$LINENO}: \"$1\""])])


# _AC_EVAL_STDERR(COMMAND)
# ------------------------
# Like _AC_RUN_LOG_STDERR, but eval (instead of running) COMMAND.
# Unlike _AC_DO_STDERR, this macro mishandles quoted arguments in some cases.
# It is present only for backward compatibility with previous Autoconf versions.
AC_DEFUN([_AC_EVAL_STDERR],
[_AC_RUN_LOG_STDERR([eval $1],
		    [eval echo "\"\$as_me\":${as_lineno-$LINENO}: \"$1\""])])


# AC_TRY_EVAL(VARIABLE)
# ---------------------
# Evaluate $VARIABLE, which should be a valid shell command.
# The purpose of this macro is to write "configure:123: command line"
# into config.log for every test run.
#
# The AC_TRY_EVAL and AC_TRY_COMMAND macros are dangerous and
# undocumented, and should not be used.
# They may be removed or their API changed in a future release.
# Autoconf itself no longer uses these two macros; they are present
# only for backward compatibility with previous versions of Autoconf.
# Not every shell command will work due to problems with eval
# and quoting, and the rules for exactly what does work are tricky.
# Worse, due to double-expansion during evaluation, arbitrary unintended
# shell commands could be executed in some situations.
AC_DEFUN([AC_TRY_EVAL],
[_AC_EVAL([$$1])])


# AC_TRY_COMMAND(COMMAND)
# -----------------------
# Like AC_TRY_EVAL, but execute COMMAND instead, where COMMAND is a series of
# tokens of the shell command language.
# This macro should not be used; see the comments under AC_TRY_EVAL for why.
AC_DEFUN([AC_TRY_COMMAND],
[{ ac_try='$1'
  _AC_EVAL([$ac_try]); }])


# AC_RUN_LOG(COMMAND)
# -------------------
AC_DEFUN([AC_RUN_LOG],
[_AC_RUN_LOG([$1],
	     [AS_ECHO(["$as_me:${as_lineno-$LINENO}: AS_ESCAPE([$1])"])])])




## ------------------------ ##
## Examining declarations.  ##
## ------------------------ ##


# _AC_PREPROC_IFELSE_BODY
# -----------------------
# Shell function body for _AC_PREPROC_IFELSE.
m4_define([_AC_PREPROC_IFELSE_BODY],
[  AS_LINENO_PUSH([$[]1])
  AS_IF([_AC_DO_STDERR([$ac_cpp conftest.$ac_ext]) > conftest.i && {
	 test -z "$ac_[]_AC_LANG_ABBREV[]_preproc_warn_flag$ac_[]_AC_LANG_ABBREV[]_werror_flag" ||
	 test ! -s conftest.err
       }],
    [ac_retval=0],
    [_AC_MSG_LOG_CONFTEST
    ac_retval=1])
  AS_LINENO_POP
  AS_SET_STATUS([$ac_retval])
])# _AC_PREPROC_IFELSE_BODY


# _AC_PREPROC_IFELSE(PROGRAM, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ----------------------------------------------------------------
# Try to preprocess PROGRAM.
#
# This macro can be used during the selection of a preprocessor.
# eval is necessary to expand ac_cpp.
AC_DEFUN([_AC_PREPROC_IFELSE],
[AC_REQUIRE_SHELL_FN([ac_fn_]_AC_LANG_ABBREV[_try_cpp],
  [AS_FUNCTION_DESCRIBE([ac_fn_]_AC_LANG_ABBREV[_try_cpp], [LINENO],
    [Try to preprocess conftest.$ac_ext, and return whether this succeeded.])],
  [$0_BODY])]dnl
[m4_ifvaln([$1], [AC_LANG_CONFTEST([$1])])]dnl
[AS_IF([ac_fn_[]_AC_LANG_ABBREV[]_try_cpp "$LINENO"], [$2], [$3])
rm -f conftest.err conftest.i[]m4_ifval([$1], [ conftest.$ac_ext])[]dnl
])# _AC_PREPROC_IFELSE

# AC_PREPROC_IFELSE(PROGRAM, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ---------------------------------------------------------------
# Try to preprocess PROGRAM.  Requires that the preprocessor for the
# current language was checked for, hence do not use this macro in macros
# looking for a preprocessor.
AC_DEFUN([AC_PREPROC_IFELSE],
[AC_LANG_PREPROC_REQUIRE()dnl
_AC_PREPROC_IFELSE($@)])


# AC_TRY_CPP(INCLUDES, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ---------------------------------------------------------
# AC_TRY_CPP is used to check whether particular header files exist.
# (But it actually tests whether INCLUDES produces no CPP errors.)
#
# INCLUDES are not defaulted and are double quoted.
AU_DEFUN([AC_TRY_CPP],
[AC_PREPROC_IFELSE([AC_LANG_SOURCE([[$1]])], [$2], [$3])])


# AC_EGREP_CPP(PATTERN, PROGRAM,
#              [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ------------------------------------------------------
# Because this macro is used by AC_PROG_GCC_TRADITIONAL, which must
# come early, it is not included in AC_BEFORE checks.
AC_DEFUN([AC_EGREP_CPP],
[AC_LANG_PREPROC_REQUIRE()dnl
AC_REQUIRE([AC_PROG_EGREP])dnl
AC_LANG_CONFTEST([AC_LANG_SOURCE([[$2]])])
AS_IF([dnl eval is necessary to expand ac_cpp.
dnl Ultrix and Pyramid sh refuse to redirect output of eval, so use subshell.
(eval "$ac_cpp conftest.$ac_ext") 2>&AS_MESSAGE_LOG_FD |
dnl Quote $1 to prevent m4 from eating character classes
  $EGREP "[$1]" >/dev/null 2>&1],
  [$3],
  [$4])
rm -f conftest*
])# AC_EGREP_CPP


# AC_EGREP_HEADER(PATTERN, HEADER-FILE,
#                 [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ---------------------------------------------------------
AC_DEFUN([AC_EGREP_HEADER],
[AC_EGREP_CPP([$1],
[#include <$2>
], [$3], [$4])])




## ------------------ ##
## Examining syntax.  ##
## ------------------ ##

# _AC_COMPILE_IFELSE_BODY
# -----------------------
# Shell function body for _AC_COMPILE_IFELSE.
m4_define([_AC_COMPILE_IFELSE_BODY],
[  AS_LINENO_PUSH([$[]1])
  rm -f conftest.$ac_objext
  AS_IF([_AC_DO_STDERR($ac_compile) && {
	 test -z "$ac_[]_AC_LANG_ABBREV[]_werror_flag" ||
	 test ! -s conftest.err
       } && test -s conftest.$ac_objext],
      [ac_retval=0],
      [_AC_MSG_LOG_CONFTEST
	ac_retval=1])
  AS_LINENO_POP
  AS_SET_STATUS([$ac_retval])
])# _AC_COMPILE_IFELSE_BODY


# _AC_COMPILE_IFELSE(PROGRAM, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ----------------------------------------------------------------
# Try to compile PROGRAM.
# This macro can be used during the selection of a compiler.
AC_DEFUN([_AC_COMPILE_IFELSE],
[AC_REQUIRE_SHELL_FN([ac_fn_]_AC_LANG_ABBREV[_try_compile],
  [AS_FUNCTION_DESCRIBE([ac_fn_]_AC_LANG_ABBREV[_try_compile], [LINENO],
    [Try to compile conftest.$ac_ext, and return whether this succeeded.])],
  [$0_BODY])]dnl
[m4_ifvaln([$1], [AC_LANG_CONFTEST([$1])])]dnl
[AS_IF([ac_fn_[]_AC_LANG_ABBREV[]_try_compile "$LINENO"], [$2], [$3])
rm -f core conftest.err conftest.$ac_objext[]m4_ifval([$1], [ conftest.$ac_ext])[]dnl
])# _AC_COMPILE_IFELSE


# AC_COMPILE_IFELSE(PROGRAM, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ---------------------------------------------------------------
# Try to compile PROGRAM.  Requires that the compiler for the current
# language was checked for, hence do not use this macro in macros looking
# for a compiler.
AC_DEFUN([AC_COMPILE_IFELSE],
[AC_LANG_COMPILER_REQUIRE()dnl
_AC_COMPILE_IFELSE($@)])


# AC_TRY_COMPILE(INCLUDES, FUNCTION-BODY,
#                [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ---------------------------------------------------
AU_DEFUN([AC_TRY_COMPILE],
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[$1]], [[$2]])], [$3], [$4])])



## --------------------- ##
## Examining libraries.  ##
## --------------------- ##


# _AC_LINK_IFELSE_BODY
# --------------------
# Shell function body for _AC_LINK_IFELSE.
m4_define([_AC_LINK_IFELSE_BODY],
[  AS_LINENO_PUSH([$[]1])
  rm -f conftest.$ac_objext conftest$ac_exeext
  AS_IF([_AC_DO_STDERR($ac_link) && {
	 test -z "$ac_[]_AC_LANG_ABBREV[]_werror_flag" ||
	 test ! -s conftest.err
       } && test -s conftest$ac_exeext && {
	 test "$cross_compiling" = yes ||
	 AS_TEST_X([conftest$ac_exeext])
       }],
      [ac_retval=0],
      [_AC_MSG_LOG_CONFTEST
	ac_retval=1])
  # Delete the IPA/IPO (Inter Procedural Analysis/Optimization) information
  # created by the PGI compiler (conftest_ipa8_conftest.oo), as it would
  # interfere with the next link command; also delete a directory that is
  # left behind by Apple's compiler.  We do this before executing the actions.
  rm -rf conftest.dSYM conftest_ipa8_conftest.oo
  AS_LINENO_POP
  AS_SET_STATUS([$ac_retval])
])# _AC_LINK_IFELSE_BODY


# _AC_LINK_IFELSE(PROGRAM, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# -------------------------------------------------------------
# Try to link PROGRAM.
# This macro can be used during the selection of a compiler.
#
# Test that resulting file is executable; see the problem reported by mwoehlke
# in <http://lists.gnu.org/archive/html/bug-coreutils/2006-10/msg00048.html>.
# But skip the test when cross-compiling, to prevent problems like the one
# reported by Chris Johns in
# <http://lists.gnu.org/archive/html/autoconf/2007-03/msg00085.html>.
#
AC_DEFUN([_AC_LINK_IFELSE],
[AC_REQUIRE_SHELL_FN([ac_fn_]_AC_LANG_ABBREV[_try_link],
  [AS_FUNCTION_DESCRIBE([ac_fn_]_AC_LANG_ABBREV[_try_link], [LINENO],
    [Try to link conftest.$ac_ext, and return whether this succeeded.])],
  [$0_BODY])]dnl
[m4_ifvaln([$1], [AC_LANG_CONFTEST([$1])])]dnl
[AS_IF([ac_fn_[]_AC_LANG_ABBREV[]_try_link "$LINENO"], [$2], [$3])
rm -f core conftest.err conftest.$ac_objext \
    conftest$ac_exeext[]m4_ifval([$1], [ conftest.$ac_ext])[]dnl
])# _AC_LINK_IFELSE


# AC_LINK_IFELSE(PROGRAM, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ------------------------------------------------------------
# Try to link PROGRAM.  Requires that the compiler for the current
# language was checked for, hence do not use this macro in macros looking
# for a compiler.
AC_DEFUN([AC_LINK_IFELSE],
[AC_LANG_COMPILER_REQUIRE()dnl
_AC_LINK_IFELSE($@)])


# AC_TRY_LINK(INCLUDES, FUNCTION-BODY,
#             [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ------------------------------------------------
# Contrarily to AC_LINK_IFELSE, this macro double quote its first two args.
AU_DEFUN([AC_TRY_LINK],
[AC_LINK_IFELSE([AC_LANG_PROGRAM([[$1]], [[$2]])], [$3], [$4])])


# AC_COMPILE_CHECK(ECHO-TEXT, INCLUDES, FUNCTION-BODY,
#                  ACTION-IF-TRUE, [ACTION-IF-FALSE])
# ---------------------------------------------------
AU_DEFUN([AC_COMPILE_CHECK],
[m4_ifvaln([$1], [AC_MSG_CHECKING([for $1])])dnl
AC_LINK_IFELSE([AC_LANG_PROGRAM([[$2]], [[$3]])], [$4], [$5])])




## ------------------------------- ##
## Checking for runtime features.  ##
## ------------------------------- ##


# _AC_RUN_IFELSE_BODY
# -------------------
# Shell function body for _AC_RUN_IFELSE.
m4_define([_AC_RUN_IFELSE_BODY],
[  AS_LINENO_PUSH([$[]1])
  AS_IF([_AC_DO_VAR(ac_link) && _AC_DO_TOKENS(./conftest$ac_exeext)],
      [ac_retval=0],
      [AS_ECHO(["$as_me: program exited with status $ac_status"]) >&AS_MESSAGE_LOG_FD
       _AC_MSG_LOG_CONFTEST
       ac_retval=$ac_status])
  rm -rf conftest.dSYM conftest_ipa8_conftest.oo
  AS_LINENO_POP
  AS_SET_STATUS([$ac_retval])
])# _AC_RUN_IFELSE_BODY


# _AC_RUN_IFELSE(PROGRAM, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ------------------------------------------------------------
# Compile, link, and run.
# This macro can be used during the selection of a compiler.
# We also remove conftest.o as if the compilation fails, some compilers
# don't remove it.  We remove gmon.out and bb.out, which may be
# created during the run if the program is built with profiling support.
AC_DEFUN([_AC_RUN_IFELSE],
[AC_REQUIRE_SHELL_FN([ac_fn_]_AC_LANG_ABBREV[_try_run],
  [AS_FUNCTION_DESCRIBE([ac_fn_]_AC_LANG_ABBREV[_try_run], [LINENO],
    [Try to link conftest.$ac_ext, and return whether this succeeded.
     Assumes that executables *can* be run.])],
  [$0_BODY])]dnl
[m4_ifvaln([$1], [AC_LANG_CONFTEST([$1])])]dnl
[AS_IF([ac_fn_[]_AC_LANG_ABBREV[]_try_run "$LINENO"], [$2], [$3])
rm -f core *.core core.conftest.* gmon.out bb.out conftest$ac_exeext \
  conftest.$ac_objext conftest.beam[]m4_ifval([$1], [ conftest.$ac_ext])[]dnl
])# _AC_RUN_IFELSE

# AC_RUN_IFELSE(PROGRAM,
#               [ACTION-IF-TRUE], [ACTION-IF-FALSE],
#               [ACTION-IF-CROSS-COMPILING = RUNTIME-ERROR])
# ----------------------------------------------------------
# Compile, link, and run. Requires that the compiler for the current
# language was checked for, hence do not use this macro in macros looking
# for a compiler.
AC_DEFUN([AC_RUN_IFELSE],
[AC_LANG_COMPILER_REQUIRE()dnl
m4_ifval([$4], [],
	 [AC_DIAGNOSE([cross],
		     [$0 called without default to allow cross compiling])])dnl
AS_IF([test "$cross_compiling" = yes],
  [m4_default([$4],
	   [AC_MSG_FAILURE([cannot run test program while cross compiling])])],
  [_AC_RUN_IFELSE($@)])
])


# AC_TRY_RUN(PROGRAM,
#            [ACTION-IF-TRUE], [ACTION-IF-FALSE],
#            [ACTION-IF-CROSS-COMPILING = RUNTIME-ERROR])
# -------------------------------------------------------
AU_DEFUN([AC_TRY_RUN],
[AC_RUN_IFELSE([AC_LANG_SOURCE([[$1]])], [$2], [$3], [$4])])



## ------------------------------------- ##
## Checking for the existence of files.  ##
## ------------------------------------- ##

# AC_CHECK_FILE(FILE, [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# -------------------------------------------------------------
#
# Check for the existence of FILE.
AC_DEFUN([AC_CHECK_FILE],
[AC_DIAGNOSE([cross],
	     [cannot check for file existence when cross compiling])dnl
AS_VAR_PUSHDEF([ac_File], [ac_cv_file_$1])dnl
AC_CACHE_CHECK([for $1], [ac_File],
[test "$cross_compiling" = yes &&
  AC_MSG_ERROR([cannot check for file existence when cross compiling])
if test -r "$1"; then
  AS_VAR_SET([ac_File], [yes])
else
  AS_VAR_SET([ac_File], [no])
fi])
AS_VAR_IF([ac_File], [yes], [$2], [$3])
AS_VAR_POPDEF([ac_File])dnl
])# AC_CHECK_FILE


# _AC_CHECK_FILES(FILE)
# ---------------------
# Helper to AC_CHECK_FILES, which generates two of the three arguments
# to AC_CHECK_FILE based on FILE.
m4_define([_AC_CHECK_FILES],
[[$1], [AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_$1]), [1],
  [Define to 1 if you have the file `$1'.])]])


# AC_CHECK_FILES(FILE..., [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# -----------------------------------------------------------------
# For each word in the whitespace-separated FILE list, perform either
# ACTION-IF-FOUND or ACTION-IF-NOT-FOUND.  For files that exist, also
# provide the preprocessor variable HAVE_FILE.
AC_DEFUN([AC_CHECK_FILES],
[m4_map_args_w([$1], [AC_CHECK_FILE(_$0(], [)[$2], [$3])])])


## ------------------------------- ##
## Checking for declared symbols.  ##
## ------------------------------- ##


# _AC_CHECK_DECL_BODY
# -------------------
# Shell function body for AC_CHECK_DECL.
m4_define([_AC_CHECK_DECL_BODY],
[  AS_LINENO_PUSH([$[]1])
  [as_decl_name=`echo $][2|sed 's/ *(.*//'`]
  [as_decl_use=`echo $][2|sed -e 's/(/((/' -e 's/)/) 0&/' -e 's/,/) 0& (/g'`]
  AC_CACHE_CHECK([whether $as_decl_name is declared], [$[]3],
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([$[]4],
[@%:@ifndef $[]as_decl_name
@%:@ifdef __cplusplus
  (void) $[]as_decl_use;
@%:@else
  (void) $[]as_decl_name;
@%:@endif
@%:@endif
])],
		   [AS_VAR_SET([$[]3], [yes])],
		   [AS_VAR_SET([$[]3], [no])])])
  AS_LINENO_POP
])# _AC_CHECK_DECL_BODY

# AC_CHECK_DECL(SYMBOL,
#               [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#               [INCLUDES = DEFAULT-INCLUDES])
# -------------------------------------------------------
# Check whether SYMBOL (a function, variable, or constant) is declared.
AC_DEFUN([AC_CHECK_DECL],
[AC_REQUIRE_SHELL_FN([ac_fn_]_AC_LANG_ABBREV[_check_decl],
  [AS_FUNCTION_DESCRIBE([ac_fn_]_AC_LANG_ABBREV[_check_decl],
    [LINENO SYMBOL VAR INCLUDES],
    [Tests whether SYMBOL is declared in INCLUDES, setting cache variable
     VAR accordingly.])],
  [_$0_BODY])]dnl
[AS_VAR_PUSHDEF([ac_Symbol], [ac_cv_have_decl_$1])]dnl
[ac_fn_[]_AC_LANG_ABBREV[]_check_decl ]dnl
["$LINENO" "$1" "ac_Symbol" "AS_ESCAPE([AC_INCLUDES_DEFAULT([$4])], [""])"
AS_VAR_IF([ac_Symbol], [yes], [$2], [$3])
AS_VAR_POPDEF([ac_Symbol])dnl
])# AC_CHECK_DECL


# _AC_CHECK_DECLS(SYMBOL, ACTION-IF_FOUND, ACTION-IF-NOT-FOUND,
#                 INCLUDES)
# -------------------------------------------------------------
# Helper to AC_CHECK_DECLS, which generates the check for a single
# SYMBOL with INCLUDES, performs the AC_DEFINE, then expands
# ACTION-IF-FOUND or ACTION-IF-NOT-FOUND.
m4_define([_AC_CHECK_DECLS],
[AC_CHECK_DECL([$1], [ac_have_decl=1], [ac_have_decl=0], [$4])]dnl
[AC_DEFINE_UNQUOTED(AS_TR_CPP(m4_bpatsubst(HAVE_DECL_[$1],[ *(.*])),
  [$ac_have_decl],
  [Define to 1 if you have the declaration of `$1',
   and to 0 if you don't.])]dnl
[m4_ifvaln([$2$3], [AS_IF([test $ac_have_decl = 1], [$2], [$3])])])

# AC_CHECK_DECLS(SYMBOLS,
#                [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#                [INCLUDES = DEFAULT-INCLUDES])
# --------------------------------------------------------
# Defines HAVE_DECL_SYMBOL to 1 if declared, 0 otherwise.  See the
# documentation for a detailed explanation of this difference with
# other AC_CHECK_*S macros.  SYMBOLS is an m4 list.
AC_DEFUN([AC_CHECK_DECLS],
[m4_map_args_sep([_$0(], [, [$2], [$3], [$4])], [], $1)])


# _AC_CHECK_DECL_ONCE(SYMBOL)
# ---------------------------
# Check for a single SYMBOL once.
m4_define([_AC_CHECK_DECL_ONCE],
[AC_DEFUN([_AC_Check_Decl_$1], [_AC_CHECK_DECLS([$1])])]dnl
[AC_REQUIRE([_AC_Check_Decl_$1])])

# AC_CHECK_DECLS_ONCE(SYMBOLS)
# ----------------------------
# Like AC_CHECK_DECLS(SYMBOLS), but do it at most once.
AC_DEFUN([AC_CHECK_DECLS_ONCE],
[m4_map_args_sep([_AC_CHECK_DECL_ONCE(], [)], [], $1)])



## ---------------------------------- ##
## Replacement of library functions.  ##
## ---------------------------------- ##


# AC_CONFIG_LIBOBJ_DIR(DIRNAME)
# -----------------------------
# Announce LIBOBJ replacement files are in $top_srcdir/DIRNAME.
AC_DEFUN_ONCE([AC_CONFIG_LIBOBJ_DIR],
[m4_divert_text([DEFAULTS], [ac_config_libobj_dir=$1])])


# AC_LIBSOURCE(FILE-NAME)
# -----------------------
# Announce we might need the file `FILE-NAME'.
m4_define([AC_LIBSOURCE], [])


# AC_LIBSOURCES([FILE-NAME1, ...])
# --------------------------------
# Announce we might need these files.
AC_DEFUN([AC_LIBSOURCES],
[m4_map_args([AC_LIBSOURCE], $1)])


# _AC_LIBOBJ(FILE-NAME-NOEXT, ACTION-IF-INDIR)
# --------------------------------------------
# We need `FILE-NAME-NOEXT.o', save this into `LIBOBJS'.
m4_define([_AC_LIBOBJ],
[case " $LIB@&t@OBJS " in
  *" $1.$ac_objext "* ) ;;
  *) AC_SUBST([LIB@&t@OBJS], ["$LIB@&t@OBJS $1.$ac_objext"]) ;;
esac
])


# AC_LIBOBJ(FILE-NAME-NOEXT)
# --------------------------
# We need `FILE-NAME-NOEXT.o', save this into `LIBOBJS'.
AC_DEFUN([AC_LIBOBJ],
[_AC_LIBOBJ([$1])]dnl
[AS_LITERAL_WORD_IF([$1], [AC_LIBSOURCE([$1.c])],
  [AC_DIAGNOSE([syntax], [$0($1): you should use literals])])])


# _AC_LIBOBJS_NORMALIZE
# ---------------------
# Clean up LIBOBJS and LTLIBOBJS so that they work with 1. ac_objext,
# 2. Automake's ANSI2KNR, 3. Libtool, 4. combination of the three.
# Used with AC_CONFIG_COMMANDS_PRE.
AC_DEFUN([_AC_LIBOBJS_NORMALIZE],
[ac_libobjs=
ac_ltlibobjs=
m4_ifndef([AM_C_PROTOTYPES], [U=
])dnl
for ac_i in : $LIB@&t@OBJS; do test "x$ac_i" = x: && continue
  # 1. Remove the extension, and $U if already installed.
  ac_script='s/\$U\././;s/\.o$//;s/\.obj$//'
  ac_i=`AS_ECHO(["$ac_i"]) | sed "$ac_script"`
  # 2. Prepend LIBOBJDIR.  When used with automake>=1.10 LIBOBJDIR
  #    will be set to the directory where LIBOBJS objects are built.
  AS_VAR_APPEND([ac_libobjs], [" \${LIBOBJDIR}$ac_i\$U.$ac_objext"])
  AS_VAR_APPEND([ac_ltlibobjs], [" \${LIBOBJDIR}$ac_i"'$U.lo'])
done
AC_SUBST([LIB@&t@OBJS], [$ac_libobjs])
AC_SUBST([LTLIBOBJS], [$ac_ltlibobjs])
])


## ----------------------------------- ##
## Checking compiler characteristics.  ##
## ----------------------------------- ##


# _AC_COMPUTE_INT_COMPILE(EXPRESSION, VARIABLE, PROLOGUE, [IF-SUCCESS],
#                         [IF-FAILURE])
# ---------------------------------------------------------------------
# Compute the integer EXPRESSION and store the result in the VARIABLE.
# Works OK if cross compiling, but assumes twos-complement arithmetic.
m4_define([_AC_COMPUTE_INT_COMPILE],
[# Depending upon the size, compute the lo and hi bounds.
_AC_COMPILE_IFELSE([AC_LANG_BOOL_COMPILE_TRY([$3], [($1) >= 0])],
 [ac_lo=0 ac_mid=0
  while :; do
    _AC_COMPILE_IFELSE([AC_LANG_BOOL_COMPILE_TRY([$3], [($1) <= $ac_mid])],
		       [ac_hi=$ac_mid; break],
		       [AS_VAR_ARITH([ac_lo], [$ac_mid + 1])
			if test $ac_lo -le $ac_mid; then
			  ac_lo= ac_hi=
			  break
			fi
			AS_VAR_ARITH([ac_mid], [2 '*' $ac_mid + 1])])
  done],
[AC_COMPILE_IFELSE([AC_LANG_BOOL_COMPILE_TRY([$3], [($1) < 0])],
 [ac_hi=-1 ac_mid=-1
  while :; do
    _AC_COMPILE_IFELSE([AC_LANG_BOOL_COMPILE_TRY([$3], [($1) >= $ac_mid])],
		       [ac_lo=$ac_mid; break],
		       [AS_VAR_ARITH([ac_hi], ['(' $ac_mid ')' - 1])
			if test $ac_mid -le $ac_hi; then
			  ac_lo= ac_hi=
			  break
			fi
			AS_VAR_ARITH([ac_mid], [2 '*' $ac_mid])])
  done],
 [ac_lo= ac_hi=])])
# Binary search between lo and hi bounds.
while test "x$ac_lo" != "x$ac_hi"; do
  AS_VAR_ARITH([ac_mid], ['(' $ac_hi - $ac_lo ')' / 2 + $ac_lo])
  _AC_COMPILE_IFELSE([AC_LANG_BOOL_COMPILE_TRY([$3], [($1) <= $ac_mid])],
		     [ac_hi=$ac_mid],
		     [AS_VAR_ARITH([ac_lo], ['(' $ac_mid ')' + 1])])
done
case $ac_lo in @%:@((
?*) AS_VAR_SET([$2], [$ac_lo]); $4 ;;
'') $5 ;;
esac[]dnl
])# _AC_COMPUTE_INT_COMPILE


# _AC_COMPUTE_INT_RUN(EXPRESSION, VARIABLE, PROLOGUE, [IF-SUCCESS],
#                     [IF-FAILURE])
# -----------------------------------------------------------------
# Store the evaluation of the integer EXPRESSION in VARIABLE.
#
# AC_LANG_INT_SAVE intentionally does not end the file in a newline, so
# we must add one to make it a text file before passing it to read.
m4_define([_AC_COMPUTE_INT_RUN],
[_AC_RUN_IFELSE([AC_LANG_INT_SAVE([$3], [$1])],
		[echo >>conftest.val; read $2 <conftest.val; $4], [$5])
rm -f conftest.val
])# _AC_COMPUTE_INT_RUN


# _AC_COMPUTE_INT_BODY
# --------------------
# Shell function body for AC_COMPUTE_INT.
m4_define([_AC_COMPUTE_INT_BODY],
[  AS_LINENO_PUSH([$[]1])
  if test "$cross_compiling" = yes; then
    _AC_COMPUTE_INT_COMPILE([$[]2], [$[]3], [$[]4],
			    [ac_retval=0], [ac_retval=1])
  else
    _AC_COMPUTE_INT_RUN([$[]2], [$[]3], [$[]4],
			[ac_retval=0], [ac_retval=1])
  fi
  AS_LINENO_POP
  AS_SET_STATUS([$ac_retval])
])# _AC_COMPUTE_INT_BODY

# AC_COMPUTE_INT(VARIABLE, EXPRESSION, PROLOGUE, [IF-FAILS])
# ----------------------------------------------------------
# Store into the shell variable VARIABLE the value of the integer C expression
# EXPRESSION.  The value should fit in an initializer in a C variable of type
# `signed long'.  If no PROLOGUE are specified, the default includes are used.
# IF-FAILS is evaluated if the value cannot be found (which includes the
# case of cross-compilation, if EXPRESSION is not computable at compile-time.
AC_DEFUN([AC_COMPUTE_INT],
[AC_LANG_COMPILER_REQUIRE()]dnl
[AC_REQUIRE_SHELL_FN([ac_fn_]_AC_LANG_ABBREV[_compute_int],
  [AS_FUNCTION_DESCRIBE([ac_fn_]_AC_LANG_ABBREV[_compute_int],
    [LINENO EXPR VAR INCLUDES],
    [Tries to find the compile-time value of EXPR in a program that includes
     INCLUDES, setting VAR accordingly.  Returns whether the value could
     be computed])],
    [_$0_BODY])]dnl
[AS_IF([ac_fn_[]_AC_LANG_ABBREV[]_compute_int "$LINENO" "$2" "$1" ]dnl
       ["AS_ESCAPE([$3], [""])"],
       [], [$4])
])# AC_COMPUTE_INT

# _AC_COMPUTE_INT(EXPRESSION, VARIABLE, PROLOGUE, [IF-FAILS])
# -----------------------------------------------------------
# FIXME: this private interface was used by several packages.
# Give them time to transition to AC_COMPUTE_INT and then delete this one.
AC_DEFUN([_AC_COMPUTE_INT],
[AC_COMPUTE_INT([$2], [$1], [$3], [$4])
AC_DIAGNOSE([obsolete],
[The macro `_AC_COMPUTE_INT' is obsolete and will be deleted in a
future version or Autoconf.  Hence, it is suggested that you use
instead the public AC_COMPUTE_INT macro.  Note that the arguments are
slightly different between the two.])dnl
])# _AC_COMPUTE_INT
