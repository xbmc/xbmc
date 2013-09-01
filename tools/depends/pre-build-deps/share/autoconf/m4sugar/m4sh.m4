# This file is part of Autoconf.                          -*- Autoconf -*-
# M4 sugar for common shell constructs.
# Requires GNU M4 and M4sugar.
#
# Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
# 2009, 2010 Free Software Foundation, Inc.

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

# Written by Akim Demaille, Pavel Roskin, Alexandre Oliva, Lars J. Aas
# and many other people.


# We heavily use m4's diversions both for the initializations and for
# required macros, because in both cases we have to issue soon in
# output something which is discovered late.
#
#
# KILL is only used to suppress output.
#
# - BINSH
#   AC_REQUIRE'd #! /bin/sh line
# - HEADER-REVISION
#   RCS keywords etc.
# - HEADER-COMMENT
#   Purpose of the script etc.
# - HEADER-COPYRIGHT
#   Copyright notice(s)
# - M4SH-SANITIZE
#   M4sh's shell setup
# - M4SH-INIT-FN
#   M4sh initialization (shell functions)
# - M4SH-INIT
#   M4sh initialization (detection code)
# - BODY
#   The body of the script.


# _m4_divert(DIVERSION-NAME)
# --------------------------
# Convert a diversion name into its number.  Otherwise, return
# DIVERSION-NAME which is supposed to be an actual diversion number.
# Of course it would be nicer to use m4_case here, instead of zillions
# of little macros, but it then takes twice longer to run `autoconf'!
m4_define([_m4_divert(BINSH)],             0)
m4_define([_m4_divert(HEADER-REVISION)],   1)
m4_define([_m4_divert(HEADER-COMMENT)],    2)
m4_define([_m4_divert(HEADER-COPYRIGHT)],  3)
m4_define([_m4_divert(M4SH-SANITIZE)],     4)
m4_define([_m4_divert(M4SH-INIT-FN)],      5)
m4_define([_m4_divert(M4SH-INIT)],         6)
m4_define([_m4_divert(BODY)],           1000)

# Aaarg.  Yet it starts with compatibility issues...  Libtool wants to
# use NOTICE to insert its own LIBTOOL-INIT stuff.  People should ask
# before diving into our internals :(
m4_copy([_m4_divert(M4SH-INIT)], [_m4_divert(NOTICE)])



## ------------------------- ##
## 1. Sanitizing the shell.  ##
## ------------------------- ##
# Please maintain lexicographic sorting of this section, ignoring leading _.

# AS_BOURNE_COMPATIBLE
# --------------------
# Try to be as Bourne and/or POSIX as possible.
#
# This does not set BIN_SH, due to the problems described in
# <http://lists.gnu.org/archive/html/autoconf-patches/2006-03/msg00081.html>.
# People who need BIN_SH should set it in their environment before invoking
# configure; apparently this would include UnixWare, as described in
# <http://lists.gnu.org/archive/html/bug-autoconf/2006-06/msg00025.html>.
m4_define([AS_BOURNE_COMPATIBLE],
[# Be more Bourne compatible
DUALCASE=1; export DUALCASE # for MKS sh
_$0
])

# _AS_BOURNE_COMPATIBLE
# ---------------------
# This is the part of AS_BOURNE_COMPATIBLE which has to be repeated inside
# each instance.
m4_define([_AS_BOURNE_COMPATIBLE],
[AS_IF([test -n "${ZSH_VERSION+set}" && (emulate sh) >/dev/null 2>&1],
 [emulate sh
  NULLCMD=:
  [#] Pre-4.2 versions of Zsh do word splitting on ${1+"$[@]"}, which
  # is contrary to our usage.  Disable this feature.
  alias -g '${1+"$[@]"}'='"$[@]"'
  setopt NO_GLOB_SUBST],
 [AS_CASE([`(set -o) 2>/dev/null`], [*posix*], [set -o posix])])
])


# _AS_CLEANUP
# -----------
# Expanded as the last thing before m4sugar cleanup begins.  Macros
# may append m4sh cleanup hooks to this as appropriate.
m4_define([_AS_CLEANUP],
[m4_divert_text([M4SH-SANITIZE], [_AS_DETECT_BETTER_SHELL])])


# AS_COPYRIGHT(TEXT)
# ------------------
# Emit TEXT, a copyright notice, as a shell comment near the top of the
# script.  TEXT is evaluated once; to accomplish that, we do not prepend
# `# ' but `@%:@ '.
m4_define([AS_COPYRIGHT],
[m4_divert_text([HEADER-COPYRIGHT],
[m4_bpatsubst([
$1], [^], [@%:@ ])])])


# _AS_DETECT_EXPAND(VAR, SET)
# ---------------------------
# Assign the contents of VAR from the contents of SET, expanded in such
# a manner that VAR can be passed to _AS_RUN.  In order to make
# _AS_LINENO_WORKS operate correctly, we must specially handle the
# first instance of $LINENO within any line being expanded (the first
# instance is important to tests using the current shell, leaving
# remaining instances for tests using a candidate shell).  Bash loses
# track of line numbers if a double quote contains a newline, hence,
# we must piece-meal the assignment of VAR such that $LINENO expansion
# occurs in a single line.
m4_define([_AS_DETECT_EXPAND],
[$1="m4_bpatsubst(m4_dquote(AS_ESCAPE(_m4_expand(m4_set_contents([$2], [
])))), [\\\$LINENO\(.*\)$], [";$1=$$1$LINENO;$1=$$1"\1])"])


# _AS_DETECT_REQUIRED(TEST)
# -------------------------
# Refuse to execute under a shell that does not pass the given TEST.
# Does not do AS_REQUIRE for the better-shell detection code.
#
# M4sh should never require something not required by POSIX, although
# other clients are free to do so.
m4_defun([_AS_DETECT_REQUIRED],
[m4_set_add([_AS_DETECT_REQUIRED_BODY], [$1 || AS_EXIT])])


# _AS_DETECT_SUGGESTED(TEST)
# --------------------------
# Prefer to execute under a shell that passes the given TEST.
# Does not do AS_REQUIRE for the better-shell detection code.
#
# M4sh should never suggest something not required by POSIX, although
# other clients are free to do so.
m4_defun([_AS_DETECT_SUGGESTED],
[m4_set_add([_AS_DETECT_SUGGESTED_BODY], [$1 || AS_EXIT])])


# _AS_DETECT_SUGGESTED_PRUNE(TEST)
# --------------------------------
# If TEST is also a required test, remove it from the set of suggested tests.
m4_define([_AS_DETECT_SUGGESTED_PRUNE],
[m4_set_contains([_AS_DETECT_REQUIRED_BODY], [$1],
		 [m4_set_remove([_AS_DETECT_SUGGESTED_BODY], [$1])])])


# _AS_DETECT_BETTER_SHELL
# -----------------------
# The real workhorse for detecting a shell with the correct
# features.
#
# In previous versions, we prepended /usr/posix/bin to the path, but that
# caused a regression on OpenServer 6.0.0
# <http://lists.gnu.org/archive/html/bug-autoconf/2006-06/msg00017.html>
# and on HP-UX 11.11, see the failure of test 120 in
# <http://lists.gnu.org/archive/html/bug-autoconf/2006-10/msg00003.html>
#
# FIXME: The code should test for the OSF bug described in
# <http://lists.gnu.org/archive/html/autoconf-patches/2006-03/msg00081.html>.
#
# This code is run outside any trap 0 context, hence we can simplify AS_EXIT.
m4_defun([_AS_DETECT_BETTER_SHELL],
dnl Remove any tests from suggested that are also required
[m4_set_map([_AS_DETECT_SUGGESTED_BODY], [_AS_DETECT_SUGGESTED_PRUNE])]dnl
[m4_pushdef([AS_EXIT], [exit m4_default(]m4_dquote([$][1])[, 1)])]dnl
[if test "x$CONFIG_SHELL" = x; then
  as_bourne_compatible="AS_ESCAPE(_m4_expand([_AS_BOURNE_COMPATIBLE]))"
  _AS_DETECT_EXPAND([as_required], [_AS_DETECT_REQUIRED_BODY])
  _AS_DETECT_EXPAND([as_suggested], [_AS_DETECT_SUGGESTED_BODY])
  AS_IF([_AS_RUN(["$as_required"])],
	[as_have_required=yes],
	[as_have_required=no])
  AS_IF([test x$as_have_required = xyes && _AS_RUN(["$as_suggested"])],
    [],
    [_AS_PATH_WALK([/bin$PATH_SEPARATOR/usr/bin$PATH_SEPARATOR$PATH],
      [case $as_dir in @%:@(
	 /*)
	   for as_base in sh bash ksh sh5; do
	     # Try only shells that exist, to save several forks.
	     as_shell=$as_dir/$as_base
	     AS_IF([{ test -f "$as_shell" || test -f "$as_shell.exe"; } &&
		    _AS_RUN(["$as_required"], ["$as_shell"])],
		   [CONFIG_SHELL=$as_shell as_have_required=yes
		   m4_set_empty([_AS_DETECT_SUGGESTED_BODY], [break 2],
		     [AS_IF([_AS_RUN(["$as_suggested"], ["$as_shell"])],
			    [break 2])])])
	   done;;
       esac],
      [AS_IF([{ test -f "$SHELL" || test -f "$SHELL.exe"; } &&
	      _AS_RUN(["$as_required"], ["$SHELL"])],
	     [CONFIG_SHELL=$SHELL as_have_required=yes])])

      AS_IF([test "x$CONFIG_SHELL" != x],
	[# We cannot yet assume a decent shell, so we have to provide a
	# neutralization value for shells without unset; and this also
	# works around shells that cannot unset nonexistent variables.
	# Preserve -v and -x to the replacement shell.
	BASH_ENV=/dev/null
	ENV=/dev/null
	(unset BASH_ENV) >/dev/null 2>&1 && unset BASH_ENV ENV
	export CONFIG_SHELL
	case $- in @%:@ ((((
	  *v*x* | *x*v* ) as_opts=-vx ;;
	  *v* ) as_opts=-v ;;
	  *x* ) as_opts=-x ;;
	  * ) as_opts= ;;
	esac
	exec "$CONFIG_SHELL" $as_opts "$as_myself" ${1+"$[@]"}])

dnl Unfortunately, $as_me isn't available here.
    AS_IF([test x$as_have_required = xno],
      [AS_ECHO(["$[]0: This script requires a shell more modern than all"])
  AS_ECHO(["$[]0: the shells that I found on your system."])
  if test x${ZSH_VERSION+set} = xset ; then
    AS_ECHO(["$[]0: In particular, zsh $ZSH_VERSION has bugs and should"])
    AS_ECHO(["$[]0: be upgraded to zsh 4.3.4 or later."])
  else
    AS_ECHO("m4_text_wrap([Please tell ]_m4_defn([m4_PACKAGE_BUGREPORT])
m4_ifset([AC_PACKAGE_BUGREPORT], [m4_if(_m4_defn([m4_PACKAGE_BUGREPORT]),
_m4_defn([AC_PACKAGE_BUGREPORT]), [], [and _m4_defn([AC_PACKAGE_BUGREPORT])])])
[about your system, including any error possibly output before this message.
Then install a modern shell, or manually run the script under such a
shell if you do have one.], [$[]0: ], [], [62])")
  fi
  AS_EXIT])])
fi
SHELL=${CONFIG_SHELL-/bin/sh}
export SHELL
# Unset more variables known to interfere with behavior of common tools.
CLICOLOR_FORCE= GREP_OPTIONS=
unset CLICOLOR_FORCE GREP_OPTIONS
_m4_popdef([AS_EXIT])])# _AS_DETECT_BETTER_SHELL


# _AS_PREPARE
# -----------
# This macro has a very special status.  Normal use of M4sh relies
# heavily on AS_REQUIRE, so that needed initializations (such as
# _AS_TEST_PREPARE) are performed on need, not on demand.  But
# Autoconf is the first client of M4sh, and for two reasons: configure
# and config.status.  Relying on AS_REQUIRE is of course fine for
# configure, but fails for config.status (which is created by
# configure).  So we need a means to force the inclusion of the
# various _AS_*_PREPARE on top of config.status.  That's basically why
# there are so many _AS_*_PREPARE below, and that's also why it is
# important not to forget some: config.status needs them.
# List any preparations that create shell functions first, then
# topologically sort the others by their dependencies.
#
# Special case: we do not need _AS_LINENO_PREPARE, because the
# parent will have substituted $LINENO for us when processing its
# own invocation of _AS_LINENO_PREPARE.
#
# Special case: the full definition of _AS_ERROR_PREPARE is not output
# unless AS_MESSAGE_LOG_FD is non-empty, although the value of
# AS_MESSAGE_LOG_FD is not relevant.
m4_defun([_AS_PREPARE],
[m4_pushdef([AS_REQUIRE])]dnl
[m4_pushdef([AS_REQUIRE_SHELL_FN], _m4_defn([_AS_REQUIRE_SHELL_FN])
)]dnl
[m4_pushdef([AS_MESSAGE_LOG_FD], [-1])]dnl
[_AS_ERROR_PREPARE
_m4_popdef([AS_MESSAGE_LOG_FD])]dnl
[_AS_EXIT_PREPARE
_AS_UNSET_PREPARE
_AS_VAR_APPEND_PREPARE
_AS_VAR_ARITH_PREPARE

_AS_EXPR_PREPARE
_AS_BASENAME_PREPARE
_AS_DIRNAME_PREPARE
_AS_ME_PREPARE
_AS_CR_PREPARE
_AS_ECHO_N_PREPARE
_AS_LN_S_PREPARE
_AS_MKDIR_P_PREPARE
_AS_TEST_PREPARE
_AS_TR_CPP_PREPARE
_AS_TR_SH_PREPARE
_m4_popdef([AS_REQUIRE], [AS_REQUIRE_SHELL_FN])])

# AS_PREPARE
# ----------
# Output all the M4sh possible initialization into the initialization
# diversion.  We do not use _AS_PREPARE so that the m4_provide symbols for
# AS_REQUIRE and AS_REQUIRE_SHELL_FN are defined properly, and so that
# shell functions are placed in M4SH-INIT-FN.
m4_defun([AS_PREPARE],
[m4_divert_push([KILL])
m4_append_uniq([_AS_CLEANUP],
  [m4_divert_text([M4SH-INIT-FN], [_AS_ERROR_PREPARE[]])])
AS_REQUIRE([_AS_EXPR_PREPARE])
AS_REQUIRE([_AS_BASENAME_PREPARE])
AS_REQUIRE([_AS_DIRNAME_PREPARE])
AS_REQUIRE([_AS_ME_PREPARE])
AS_REQUIRE([_AS_CR_PREPARE])
AS_REQUIRE([_AS_LINENO_PREPARE])
AS_REQUIRE([_AS_ECHO_N_PREPARE])
AS_REQUIRE([_AS_EXIT_PREPARE])
AS_REQUIRE([_AS_LN_S_PREPARE])
AS_REQUIRE([_AS_MKDIR_P_PREPARE])
AS_REQUIRE([_AS_TEST_PREPARE])
AS_REQUIRE([_AS_TR_CPP_PREPARE])
AS_REQUIRE([_AS_TR_SH_PREPARE])
AS_REQUIRE([_AS_UNSET_PREPARE])
AS_REQUIRE([_AS_VAR_APPEND_PREPARE], [], [M4SH-INIT-FN])
AS_REQUIRE([_AS_VAR_ARITH_PREPARE], [], [M4SH-INIT-FN])
m4_divert_pop[]])


# AS_REQUIRE(NAME-TO-CHECK, [BODY-TO-EXPAND = NAME-TO-CHECK],
#            [DIVERSION = M4SH-INIT])
# -----------------------------------------------------------
# BODY-TO-EXPAND is some initialization which must be expanded in the
# given diversion when expanded (required or not).  The expansion
# goes in the named diversion or an earlier one.
#
# Since $2 can be quite large, this is factored for faster execution, giving
# either m4_require([$1], [$2]) or m4_divert_require(desired, [$1], [$2]).
m4_defun([AS_REQUIRE],
[m4_define([_m4_divert_desired], [m4_default_quoted([$3], [M4SH-INIT])])]dnl
[m4_if(m4_eval(_m4_divert_dump - 0 <= _m4_divert(_m4_divert_desired, [-])),
       1, [m4_require(],
	  [m4_divert_require(_m4_divert_desired,]) [$1], [$2])])

# _AS_REQUIRE_SHELL_FN(NAME-TO-CHECK, COMMENT, BODY-TO-EXPAND)
# ------------------------------------------------------------
# Core of AS_REQUIRE_SHELL_FN, but without diversion support.
m4_define([_AS_REQUIRE_SHELL_FN], [
m4_n([$2])$1 ()
{
$3
} @%:@ $1[]])

# AS_REQUIRE_SHELL_FN(NAME-TO-CHECK, COMMENT, BODY-TO-EXPAND,
#                     [DIVERSION = M4SH-INIT-FN])
# -----------------------------------------------------------
# BODY-TO-EXPAND is the body of a shell function to be emitted in the
# given diversion when expanded (required or not).  Unlike other
# xx_REQUIRE macros, BODY-TO-EXPAND is mandatory.  If COMMENT is
# provided (often via AS_FUNCTION_DESCRIBE), it is listed with a
# newline before the function name.
m4_define([AS_REQUIRE_SHELL_FN],
[m4_provide_if([AS_SHELL_FN_$1], [],
[AS_REQUIRE([AS_SHELL_FN_$1],
[m4_provide([AS_SHELL_FN_$1])_$0($@)],
m4_default_quoted([$4], [M4SH-INIT-FN]))])])


# _AS_RUN(TEST, [SHELL])
# ----------------------
# Run TEST under the current shell (if one parameter is used)
# or under the given SHELL, protecting it from syntax errors.
# Set as_run in order to assist _AS_LINENO_WORKS.
m4_define([_AS_RUN],
[m4_ifval([$2], [{ $as_echo "$as_bourne_compatible"$1 | as_run=a $2; }],
		[(eval $1)]) 2>/dev/null])


# _AS_SHELL_FN_WORK
# -----------------
# This is a spy to detect "in the wild" shells that do not support shell
# functions correctly.  It is based on the m4sh.at Autotest testcases.
m4_define([_AS_SHELL_FN_WORK],
[as_fn_return () { (exit [$]1); }
as_fn_success () { as_fn_return 0; }
as_fn_failure () { as_fn_return 1; }
as_fn_ret_success () { return 0; }
as_fn_ret_failure () { return 1; }

exitcode=0
as_fn_success || { exitcode=1; echo as_fn_success failed.; }
as_fn_failure && { exitcode=1; echo as_fn_failure succeeded.; }
as_fn_ret_success || { exitcode=1; echo as_fn_ret_success failed.; }
as_fn_ret_failure && { exitcode=1; echo as_fn_ret_failure succeeded.; }
AS_IF([( set x; as_fn_ret_success y && test x = "[$]1" )], [],
      [exitcode=1; echo positional parameters were not saved.])
test x$exitcode = x0[]])# _AS_SHELL_FN_WORK


# _AS_SHELL_SANITIZE
# ------------------
# This is the prolog that is emitted by AS_INIT and AS_INIT_GENERATED;
# it is executed prior to shell function definitions, hence the
# temporary redefinition of AS_EXIT.
m4_defun([_AS_SHELL_SANITIZE],
[m4_pushdef([AS_EXIT], [exit m4_default(]m4_dquote([$][1])[, 1)])]dnl
[m4_text_box([M4sh Initialization.])

AS_BOURNE_COMPATIBLE
_AS_ECHO_PREPARE
_AS_PATH_SEPARATOR_PREPARE

# IFS
# We need space, tab and new line, in precisely that order.  Quoting is
# there to prevent editors from complaining about space-tab.
# (If _AS_PATH_WALK were called with IFS unset, it would disable word
# splitting by setting IFS to empty value.)
IFS=" ""	$as_nl"

# Find who we are.  Look in the path if we contain no directory separator.
as_myself=
case $[0] in @%:@((
  *[[\\/]]* ) as_myself=$[0] ;;
  *) _AS_PATH_WALK([],
		   [test -r "$as_dir/$[0]" && as_myself=$as_dir/$[0] && break])
     ;;
esac
# We did not find ourselves, most probably we were run as `sh COMMAND'
# in which case we are not to be found in the path.
if test "x$as_myself" = x; then
  as_myself=$[0]
fi
if test ! -f "$as_myself"; then
  AS_ECHO(["$as_myself: error: cannot find myself; rerun with an absolute file name"]) >&2
  AS_EXIT
fi

# Unset variables that we do not need and which cause bugs (e.g. in
# pre-3.0 UWIN ksh).  But do not cause bugs in bash 2.01; the "|| exit 1"
# suppresses any "Segmentation fault" message there.  '((' could
# trigger a bug in pdksh 5.2.14.
for as_var in BASH_ENV ENV MAIL MAILPATH
do eval test x\${$as_var+set} = xset \
  && ( (unset $as_var) || exit 1) >/dev/null 2>&1 && unset $as_var || :
done
PS1='$ '
PS2='> '
PS4='+ '

# NLS nuisances.
LC_ALL=C
export LC_ALL
LANGUAGE=C
export LANGUAGE

# CDPATH.
(unset CDPATH) >/dev/null 2>&1 && unset CDPATH
_m4_popdef([AS_EXIT])])# _AS_SHELL_SANITIZE


# AS_SHELL_SANITIZE
# -----------------
# This is only needed for the sake of Libtool, which screws up royally
# in its usage of M4sh internals.
m4_define([AS_SHELL_SANITIZE],
[_AS_SHELL_SANITIZE
m4_provide_if([AS_INIT], [],
[m4_provide([AS_INIT])
_AS_DETECT_REQUIRED([_AS_SHELL_FN_WORK])
_AS_DETECT_BETTER_SHELL
_AS_UNSET_PREPARE
])])


## ----------------------------- ##
## 2. Wrappers around builtins.  ##
## ----------------------------- ##

# This section is lexicographically sorted.


# AS_CASE(WORD, [PATTERN1], [IF-MATCHED1]...[DEFAULT])
# ----------------------------------------------------
# Expand into
# | case WORD in #(
# |   PATTERN1) IF-MATCHED1 ;; #(
# |   ...
# |   *) DEFAULT ;;
# | esac
# The shell comments are intentional, to work around people who don't
# realize the impacts of using insufficient m4 quoting.  This macro
# always uses : and provides a default case, to work around Solaris
# /bin/sh bugs regarding the exit status.
m4_define([_AS_CASE],
[ [@%:@(]
  $1[)] :
    $2 ;;])
m4_define([_AS_CASE_DEFAULT],
[ [@%:@(]
  *[)] :
    $1 ;;])

m4_defun([AS_CASE],
[case $1 in[]m4_map_args_pair([_$0], [_$0_DEFAULT],
   m4_shift($@m4_if(m4_eval([$# & 1]), [1], [,])))
esac])# AS_CASE


# _AS_EXIT_PREPARE
# ----------------
# Ensure AS_EXIT and AS_SET_STATUS will work.
#
# We cannot simply use "exit N" because some shells (zsh and Solaris sh)
# will not set $? to N while running the code set by "trap 0"
# Some shells fork even for (exit N), so we use a helper function
# to set $? prior to the exit.
# Then there are shells that don't inherit $? correctly into the start of
# a shell function, so we must always be given an argument.
# Other shells don't use `$?' as default for `exit', hence just repeating
# the exit value can only help improving portability.
m4_defun([_AS_EXIT_PREPARE],
[AS_REQUIRE_SHELL_FN([as_fn_set_status],
  [AS_FUNCTION_DESCRIBE([as_fn_set_status], [STATUS],
    [Set $? to STATUS, without forking.])], [  return $[]1])]dnl
[AS_REQUIRE_SHELL_FN([as_fn_exit],
  [AS_FUNCTION_DESCRIBE([as_fn_exit], [STATUS],
    [Exit the shell with STATUS, even in a "trap 0" or "set -e" context.])],
[  set +e
  as_fn_set_status $[1]
  exit $[1]])])#_AS_EXIT_PREPARE


# AS_EXIT([EXIT-CODE = $?])
# -------------------------
# Exit, with status set to EXIT-CODE in the way that it's seen
# within "trap 0", and without interference from "set -e".  If
# EXIT-CODE is omitted, then use $?.
m4_defun([AS_EXIT],
[AS_REQUIRE([_AS_EXIT_PREPARE])[]as_fn_exit m4_ifval([$1], [$1], [$][?])])


# AS_FOR(MACRO, SHELL-VAR, [LIST = "$@"], [BODY = :])
# ---------------------------------------------------
# Expand to a shell loop that assigns SHELL-VAR to each of the
# whitespace-separated entries in LIST (or "$@" if LIST is empty),
# then executes BODY.  BODY may call break to abort the loop, or
# continue to proceed with the next element of LIST.  Requires that
# IFS be set to the normal space-tab-newline.  As an optimization,
# BODY should access MACRO rather than $SHELL-VAR.  Normally, MACRO
# expands to $SHELL-VAR, but if LIST contains only a single element
# that needs no additional shell quoting, then MACRO will expand to
# that element, thus providing a direct value rather than a shell
# variable indirection.
#
# Only use the optimization if LIST can be used without additional
# shell quoting in either a literal or double-quoted context (that is,
# we give up on default IFS chars, parameter expansion, command
# substitution, shell quoting, globs, or quadrigraphs).  Inline the
# m4_defn for speed.
m4_defun([AS_FOR],
[m4_pushdef([$1], m4_if([$3], [], [[$$2]], m4_translit([$3], ]dnl
m4_dquote(_m4_defn([m4_cr_symbols2]))[[%+=:,./-]), [], [[$3]], [[$$2]]))]dnl
[for $2[]m4_ifval([$3], [ in $3])
do :
  $4
done[]_m4_popdef([$1])])


# AS_IF(TEST1, [IF-TRUE1 = :]...[IF-FALSE = :])
# ---------------------------------------------
# Expand into
# | if TEST1; then
# |   IF-TRUE1
# | elif TEST2; then
# |   IF-TRUE2
# [...]
# | else
# |   IF-FALSE
# | fi
# with simplifications if IF-TRUE1 and/or IF-FALSE is empty.
#
m4_define([_AS_IF],
[elif $1; then :
  $2
])
m4_define([_AS_IF_ELSE],
[m4_ifnblank([$1],
[else
  $1
])])

m4_defun([AS_IF],
[if $1; then :
  $2
m4_map_args_pair([_$0], [_$0_ELSE], m4_shift2($@))]dnl
[fi[]])# AS_IF


# AS_SET_STATUS(STATUS)
# ---------------------
# Set the shell status ($?) to STATUS, without forking.
m4_defun([AS_SET_STATUS],
[AS_REQUIRE([_AS_EXIT_PREPARE])[]as_fn_set_status $1])


# _AS_UNSET_PREPARE
# -----------------
# Define $as_unset to execute AS_UNSET, for backwards compatibility
# with older versions of M4sh.
m4_defun([_AS_UNSET_PREPARE],
[AS_FUNCTION_DESCRIBE([as_fn_unset], [VAR], [Portably unset VAR.])
as_fn_unset ()
{
  AS_UNSET([$[1]])
}
as_unset=as_fn_unset])


# AS_UNSET(VAR)
# -------------
# Unset the env VAR, working around shells that do not allow unsetting
# a variable that is not already set.  You should not unset MAIL and
# MAILCHECK, as that triggers a bug in Bash 2.01.
m4_defun([AS_UNSET],
[{ AS_LITERAL_WORD_IF([$1], [], [eval ])$1=; unset $1;}])






## ------------------------------------------ ##
## 3. Error and warnings at the shell level.  ##
## ------------------------------------------ ##


# AS_MESSAGE_FD
# -------------
# Must expand to the fd where messages will be sent.  Defaults to 1,
# although a script may reassign this value and use exec to either
# copy stdout to the new fd, or open the new fd on /dev/null.
m4_define([AS_MESSAGE_FD], [1])

# AS_MESSAGE_LOG_FD
# -----------------
# Must expand to either the empty string (when no logging is
# performed), or to the fd of a log file.  Defaults to empty, although
# a script may reassign this value and use exec to open a log.  When
# not empty, messages to AS_MESSAGE_FD are duplicated to the log,
# along with a LINENO reference.
m4_define([AS_MESSAGE_LOG_FD])


# AS_ORIGINAL_STDIN_FD
# --------------------
# Must expand to the fd of the script's original stdin.  Defaults to
# 0, although the script may reassign this value and use exec to
# shuffle fd's.
m4_define([AS_ORIGINAL_STDIN_FD], [0])


# AS_ESCAPE(STRING, [CHARS = `\"$])
# ---------------------------------
# Add backslash escaping to the CHARS in STRING.  In an effort to
# optimize use of this macro inside double-quoted shell constructs,
# the behavior is intentionally undefined if CHARS is longer than 4
# bytes, or contains bytes outside of the set [`\"$].  However,
# repeated bytes within the set are permissible (AS_ESCAPE([$1], [""])
# being a common way to be nice to syntax highlighting).
#
# Avoid the m4_bpatsubst if there are no interesting characters to escape.
# _AS_ESCAPE bypasses argument defaulting.
m4_define([AS_ESCAPE],
[_$0([$1], m4_if([$2], [], [[`], [\"$]], [m4_substr([$2], [0], [1]), [$2]]))])

# _AS_ESCAPE(STRING, KEY, SET)
# ----------------------------
# Backslash-escape all instances of the single byte KEY or up to four
# bytes in SET occurring in STRING.  Although a character can occur
# multiple times, optimum efficiency occurs when KEY and SET are
# distinct, and when SET does not exceed two bytes.  These particular
# semantics allow for the fewest number of parses of STRING, as well
# as taking advantage of the optimizations in m4 1.4.13+ when
# m4_translit is passed SET of size 2 or smaller.
m4_define([_AS_ESCAPE],
[m4_if(m4_index(m4_translit([[$1]], [$3], [$2$2$2$2]), [$2]), [-1],
       [$0_], [m4_bpatsubst])([$1], [[$2$3]], [\\\&])])
m4_define([_AS_ESCAPE_], [$1])


# _AS_QUOTE(STRING)
# -----------------
# If there are quoted (via backslash) backquotes, output STRING
# literally and warn; otherwise, output STRING with ` and " quoted.
#
# Compatibility glue between the old AS_MSG suite which did not
# quote anything, and the modern suite which quotes the quotes.
# If STRING contains `\\' or `\$', it's modern.
# If STRING contains `\"' or `\`', it's old.
# Otherwise it's modern.
#
# Profiling shows that m4_index is 5 to 8x faster than m4_bregexp.  The
# slower implementation used:
# m4_bmatch([$1],
#	    [\\[\\$]], [$2],
#	    [\\[`"]], [$3],
#	    [$2])
# The current implementation caters to the common case of no backslashes,
# to minimize m4_index expansions (hence the nested if).
m4_define([_AS_QUOTE],
[m4_cond([m4_index([$1], [\])], [-1], [_AS_QUOTE_MODERN],
	 [m4_eval(m4_index(m4_translit([[$1]], [$], [\]), [\\]) >= 0)],
[1], [_AS_QUOTE_MODERN],
	 [m4_eval(m4_index(m4_translit([[$1]], ["], [`]), [\`]) >= 0)],dnl"
[1], [_AS_QUOTE_OLD],
	 [_AS_QUOTE_MODERN])([$1])])

m4_define([_AS_QUOTE_MODERN],
[_AS_ESCAPE([$1], [`], [""])])

m4_define([_AS_QUOTE_OLD],
[m4_warn([obsolete],
   [back quotes and double quotes must not be escaped in: $1])$1])


# _AS_ECHO_UNQUOTED(STRING, [FD = AS_MESSAGE_FD])
# -----------------------------------------------
# Perform shell expansions on STRING and echo the string to FD.
m4_define([_AS_ECHO_UNQUOTED],
[AS_ECHO(["$1"]) >&m4_default([$2], [AS_MESSAGE_FD])])


# _AS_ECHO(STRING, [FD = AS_MESSAGE_FD])
# --------------------------------------
# Protect STRING from backquote expansion, echo the result to FD.
m4_define([_AS_ECHO],
[_AS_ECHO_UNQUOTED([_AS_QUOTE([$1])], [$2])])


# _AS_ECHO_LOG(STRING)
# --------------------
# Log the string to AS_MESSAGE_LOG_FD.
m4_defun_init([_AS_ECHO_LOG],
[AS_REQUIRE([_AS_LINENO_PREPARE])],
[_AS_ECHO([$as_me:${as_lineno-$LINENO}: $1], AS_MESSAGE_LOG_FD)])


# _AS_ECHO_N_PREPARE
# ------------------
# Check whether to use -n, \c, or newline-tab to separate
# checking messages from result messages.
# Don't try to cache, since the results of this macro are needed to
# display the checking message.  In addition, caching something used once
# has little interest.
# Idea borrowed from dist 3.0.  Use `*c*,', not `*c,' because if `\c'
# failed there is also a newline to match.  Use `xy' because `\c' echoed
# in a command substitution prints only the first character of the output
# with ksh version M-11/16/88f on AIX 6.1; it needs to be reset by another
# backquoted echo.
m4_defun([_AS_ECHO_N_PREPARE],
[ECHO_C= ECHO_N= ECHO_T=
case `echo -n x` in @%:@(((((
-n*)
  case `echo 'xy\c'` in
  *c*) ECHO_T='	';;	# ECHO_T is single tab character.
  xy)  ECHO_C='\c';;
  *)   echo `echo ksh88 bug on AIX 6.1` > /dev/null
       ECHO_T='	';;
  esac;;
*)
  ECHO_N='-n';;
esac
])# _AS_ECHO_N_PREPARE


# _AS_ECHO_N(STRING, [FD = AS_MESSAGE_FD])
# ----------------------------------------
# Same as _AS_ECHO, but echo doesn't return to a new line.
m4_define([_AS_ECHO_N],
[AS_ECHO_N(["_AS_QUOTE([$1])"]) >&m4_default([$2], [AS_MESSAGE_FD])])


# AS_MESSAGE(STRING, [FD = AS_MESSAGE_FD])
# ----------------------------------------
# Output "`basename $0`: STRING" to the open file FD, and if logging
# is enabled, copy it to the log with a reference to LINENO.
m4_defun_init([AS_MESSAGE],
[AS_REQUIRE([_AS_ME_PREPARE])],
[m4_ifval(AS_MESSAGE_LOG_FD,
	  [{ _AS_ECHO_LOG([$1])
_AS_ECHO([$as_me: $1], [$2]);}],
	  [_AS_ECHO([$as_me: $1], [$2])])[]])


# AS_WARN(PROBLEM)
# ----------------
# Output "`basename $0`: WARNING: PROBLEM" to stderr.
m4_define([AS_WARN],
[AS_MESSAGE([WARNING: $1], [2])])# AS_WARN


# _AS_ERROR_PREPARE
# -----------------
# Output the shell function used by AS_ERROR.  This is designed to be
# expanded during the m4_wrap cleanup.
#
# If AS_MESSAGE_LOG_FD is non-empty at the end of the script, then
# make this function take optional parameters that use LINENO at the
# points where AS_ERROR was expanded with non-empty AS_MESSAGE_LOG_FD;
# otherwise, assume the entire script does not do logging.
m4_define([_AS_ERROR_PREPARE],
[AS_REQUIRE_SHELL_FN([as_fn_error],
  [AS_FUNCTION_DESCRIBE([as_fn_error], [STATUS ERROR]m4_ifval(AS_MESSAGE_LOG_FD,
      [[ [[LINENO LOG_FD]]]]),
    [Output "`basename @S|@0`: error: ERROR" to stderr.]
m4_ifval(AS_MESSAGE_LOG_FD,
    [[If LINENO and LOG_FD are provided, also output the error to LOG_FD,
      referencing LINENO.]])
    [Then exit the script with STATUS, using 1 if that was 0.])],
[  as_status=$[1]; test $as_status -eq 0 && as_status=1
m4_ifval(AS_MESSAGE_LOG_FD,
[m4_pushdef([AS_MESSAGE_LOG_FD], [$[4]])dnl
  if test "$[4]"; then
    AS_LINENO_PUSH([$[3]])
    _AS_ECHO_LOG([error: $[2]])
  fi
m4_define([AS_MESSAGE_LOG_FD])], [m4_pushdef([AS_MESSAGE_LOG_FD])])dnl
  AS_MESSAGE([error: $[2]], [2])
_m4_popdef([AS_MESSAGE_LOG_FD])dnl
  AS_EXIT([$as_status])])])

# AS_ERROR(ERROR, [EXIT-STATUS = max($?/1)])
# ------------------------------------------
# Output "`basename $0`: error: ERROR" to stderr, then exit the
# script with EXIT-STATUS.
m4_defun_init([AS_ERROR],
[m4_append_uniq([_AS_CLEANUP],
  [m4_divert_text([M4SH-INIT-FN], [_AS_ERROR_PREPARE[]])])],
[as_fn_error m4_default([$2], [$?]) "_AS_QUOTE([$1])"m4_ifval(AS_MESSAGE_LOG_FD,
  [ "$LINENO" AS_MESSAGE_LOG_FD])])


# AS_LINENO_PUSH([LINENO])
# ------------------------
# If this is the outermost call to AS_LINENO_PUSH, make sure that
# AS_MESSAGE will print LINENO as the line number.
m4_defun([AS_LINENO_PUSH],
[as_lineno=${as_lineno-"$1"} as_lineno_stack=as_lineno_stack=$as_lineno_stack])


# AS_LINENO_POP([LINENO])
# -----------------------
# If this is call balances the outermost call to AS_LINENO_PUSH,
# AS_MESSAGE will restart printing $LINENO as the line number.
#
# No need to use AS_UNSET, since as_lineno is necessarily set.
m4_defun([AS_LINENO_POP],
[eval $as_lineno_stack; ${as_lineno_stack:+:} unset as_lineno])



## -------------------------------------- ##
## 4. Portable versions of common tools.  ##
## -------------------------------------- ##

# This section is lexicographically sorted.


# AS_BASENAME(FILE-NAME)
# ----------------------
# Simulate the command 'basename FILE-NAME'.  Not all systems have basename.
# Also see the comments for AS_DIRNAME.

m4_defun([_AS_BASENAME_EXPR],
[$as_expr X/[]$1 : '.*/\([[^/][^/]*]\)/*$' \| \
	 X[]$1 : 'X\(//\)$' \| \
	 X[]$1 : 'X\(/\)' \| .])

m4_defun([_AS_BASENAME_SED],
[AS_ECHO([X/[]$1]) |
    sed ['/^.*\/\([^/][^/]*\)\/*$/{
	    s//\1/
	    q
	  }
	  /^X\/\(\/\/\)$/{
	    s//\1/
	    q
	  }
	  /^X\/\(\/\).*/{
	    s//\1/
	    q
	  }
	  s/.*/./; q']])

m4_defun_init([AS_BASENAME],
[AS_REQUIRE([_$0_PREPARE])],
[$as_basename -- $1 ||
_AS_BASENAME_EXPR([$1]) 2>/dev/null ||
_AS_BASENAME_SED([$1])])


# _AS_BASENAME_PREPARE
# --------------------
# Avoid Solaris 9 /usr/ucb/basename, as `basename /' outputs an empty line.
# Also, traditional basename mishandles --.  Require here _AS_EXPR_PREPARE,
# to avoid problems when _AS_BASENAME is called from the M4SH-INIT diversion.
m4_defun([_AS_BASENAME_PREPARE],
[AS_REQUIRE([_AS_EXPR_PREPARE])]dnl
[if (basename -- /) >/dev/null 2>&1 && test "X`basename -- / 2>&1`" = "X/"; then
  as_basename=basename
else
  as_basename=false
fi
])# _AS_BASENAME_PREPARE


# AS_DIRNAME(FILE-NAME)
# ---------------------
# Simulate the command 'dirname FILE-NAME'.  Not all systems have dirname.
# This macro must be usable from inside ` `.
#
# Prefer expr to echo|sed, since expr is usually faster and it handles
# backslashes and newlines correctly.  However, older expr
# implementations (e.g. SunOS 4 expr and Solaris 8 /usr/ucb/expr) have
# a silly length limit that causes expr to fail if the matched
# substring is longer than 120 bytes.  So fall back on echo|sed if
# expr fails.
m4_defun_init([_AS_DIRNAME_EXPR],
[AS_REQUIRE([_AS_EXPR_PREPARE])],
[$as_expr X[]$1 : 'X\(.*[[^/]]\)//*[[^/][^/]]*/*$' \| \
	 X[]$1 : 'X\(//\)[[^/]]' \| \
	 X[]$1 : 'X\(//\)$' \| \
	 X[]$1 : 'X\(/\)' \| .])

m4_defun([_AS_DIRNAME_SED],
[AS_ECHO([X[]$1]) |
    sed ['/^X\(.*[^/]\)\/\/*[^/][^/]*\/*$/{
	    s//\1/
	    q
	  }
	  /^X\(\/\/\)[^/].*/{
	    s//\1/
	    q
	  }
	  /^X\(\/\/\)$/{
	    s//\1/
	    q
	  }
	  /^X\(\/\).*/{
	    s//\1/
	    q
	  }
	  s/.*/./; q']])

m4_defun_init([AS_DIRNAME],
[AS_REQUIRE([_$0_PREPARE])],
[$as_dirname -- $1 ||
_AS_DIRNAME_EXPR([$1]) 2>/dev/null ||
_AS_DIRNAME_SED([$1])])


# _AS_DIRNAME_PREPARE
# -------------------
m4_defun([_AS_DIRNAME_PREPARE],
[AS_REQUIRE([_AS_EXPR_PREPARE])]dnl
[if (as_dir=`dirname -- /` && test "X$as_dir" = X/) >/dev/null 2>&1; then
  as_dirname=dirname
else
  as_dirname=false
fi
])# _AS_DIRNAME_PREPARE


# AS_ECHO(WORD)
# -------------
# Output WORD followed by a newline.  WORD must be a single shell word
# (typically a quoted string).  The bytes of WORD are output as-is, even
# if it starts with "-" or contains "\".
m4_defun_init([AS_ECHO],
[AS_REQUIRE([_$0_PREPARE])],
[$as_echo $1])


# AS_ECHO_N(WORD)
# ---------------
# Like AS_ECHO(WORD), except do not output the trailing newline.
m4_defun_init([AS_ECHO_N],
[AS_REQUIRE([_AS_ECHO_PREPARE])],
[$as_echo_n $1])


# _AS_ECHO_PREPARE
# ----------------
# Arrange for $as_echo 'FOO' to echo FOO without escape-interpretation;
# and similarly for $as_echo_n, which omits the trailing newline.
# 'FOO' is an optional single argument; a missing FOO is treated as empty.
m4_defun([_AS_ECHO_PREPARE],
[[as_nl='
'
export as_nl
# Printing a long string crashes Solaris 7 /usr/bin/printf.
as_echo='\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\'
as_echo=$as_echo$as_echo$as_echo$as_echo$as_echo
as_echo=$as_echo$as_echo$as_echo$as_echo$as_echo$as_echo
# Prefer a ksh shell builtin over an external printf program on Solaris,
# but without wasting forks for bash or zsh.
if test -z "$BASH_VERSION$ZSH_VERSION" \
    && (test "X`print -r -- $as_echo`" = "X$as_echo") 2>/dev/null; then
  as_echo='print -r --'
  as_echo_n='print -rn --'
elif (test "X`printf %s $as_echo`" = "X$as_echo") 2>/dev/null; then
  as_echo='printf %s\n'
  as_echo_n='printf %s'
else
  if test "X`(/usr/ucb/echo -n -n $as_echo) 2>/dev/null`" = "X-n $as_echo"; then
    as_echo_body='eval /usr/ucb/echo -n "$][1$as_nl"'
    as_echo_n='/usr/ucb/echo -n'
  else
    as_echo_body='eval expr "X$][1" : "X\\(.*\\)"'
    as_echo_n_body='eval
      arg=$][1;
      case $arg in @%:@(
      *"$as_nl"*)
	expr "X$arg" : "X\\(.*\\)$as_nl";
	arg=`expr "X$arg" : ".*$as_nl\\(.*\\)"`;;
      esac;
      expr "X$arg" : "X\\(.*\\)" | tr -d "$as_nl"
    '
    export as_echo_n_body
    as_echo_n='sh -c $as_echo_n_body as_echo'
  fi
  export as_echo_body
  as_echo='sh -c $as_echo_body as_echo'
fi
]])# _AS_ECHO_PREPARE


# AS_TEST_X
# ---------
# Check whether a file has executable or search permissions.
m4_defun_init([AS_TEST_X],
[AS_REQUIRE([_AS_TEST_PREPARE])],
[$as_test_x $1[]])# AS_TEST_X


# AS_EXECUTABLE_P
# ---------------
# Check whether a file is a regular file that has executable permissions.
m4_defun_init([AS_EXECUTABLE_P],
[AS_REQUIRE([_AS_TEST_PREPARE])],
[{ test -f $1 && AS_TEST_X([$1]); }])# AS_EXECUTABLE_P


# _AS_EXPR_PREPARE
# ----------------
# QNX 4.25 expr computes and issue the right result but exits with failure.
# Tru64 expr mishandles leading zeros in numeric strings.
# Detect these flaws.
m4_defun([_AS_EXPR_PREPARE],
[if expr a : '\(a\)' >/dev/null 2>&1 &&
   test "X`expr 00001 : '.*\(...\)'`" = X001; then
  as_expr=expr
else
  as_expr=false
fi
])# _AS_EXPR_PREPARE


# _AS_ME_PREPARE
# --------------
# Define $as_me to the basename of the executable file's name.
m4_defun([AS_ME_PREPARE], [AS_REQUIRE([_$0])])
m4_defun([_AS_ME_PREPARE],
[AS_REQUIRE([_AS_BASENAME_PREPARE])]dnl
[as_me=`AS_BASENAME("$[0]")`
])

# _AS_LINENO_WORKS
# ----------------
# Succeed if the currently executing shell supports LINENO.
# This macro does not expand to a single shell command, so be careful
# when using it.  Surrounding the body of this macro with {} would
# cause "bash -c '_ASLINENO_WORKS'" to fail (with Bash 2.05, anyway),
# but that bug is irrelevant to our use of LINENO.  We can't use
# AS_VAR_ARITH, as this is expanded prior to shell functions.
#
# Testing for LINENO support is hard; we use _AS_LINENO_WORKS inside
# _AS_RUN, which sometimes eval's its argument (pdksh gives false
# negatives if $LINENO is expanded by eval), and sometimes passes the
# argument to another shell (if the current shell supports LINENO,
# then expanding $LINENO prior to the string leads to false
# positives).  Hence, we perform two tests, and coordinate with
# _AS_DETECT_EXPAND (which ensures that only the first of two LINENO
# is expanded in advance) and _AS_RUN (which sets $as_run to 'a' when
# handing the test to another shell), so that we know which test to
# trust.
m4_define([_AS_LINENO_WORKS],
[  as_lineno_1=$LINENO as_lineno_1a=$LINENO
  as_lineno_2=$LINENO as_lineno_2a=$LINENO
  eval 'test "x$as_lineno_1'$as_run'" != "x$as_lineno_2'$as_run'" &&
  test "x`expr $as_lineno_1'$as_run' + 1`" = "x$as_lineno_2'$as_run'"'])


# _AS_LINENO_PREPARE
# ------------------
# If LINENO is not supported by the shell, produce a version of this
# script where LINENO is hard coded.
# Comparing LINENO against _oline_ is not a good solution, since in
# the case of embedded executables (such as config.status within
# configure) you'd compare LINENO wrt config.status vs. _oline_ wrt
# configure.
#
# AS_ERROR normally uses LINENO if logging, but AS_LINENO_PREPARE uses
# AS_ERROR.  Besides, if the logging fd is open, we don't want to use
# $LINENO in the log complaining about broken LINENO.  We break the
# circular require by changing AS_ERROR and AS_MESSAGE_LOG_FD.
m4_defun([AS_LINENO_PREPARE], [AS_REQUIRE([_$0])])
m4_defun([_AS_LINENO_PREPARE],
[AS_REQUIRE([_AS_CR_PREPARE])]dnl
[AS_REQUIRE([_AS_ME_PREPARE])]dnl
[_AS_DETECT_SUGGESTED([_AS_LINENO_WORKS])]dnl
[m4_pushdef([AS_MESSAGE_LOG_FD])]dnl
[m4_pushdef([AS_ERROR],
  [{ AS_MESSAGE(]m4_dquote([error: $][1])[, [2]); AS_EXIT([1]); }])]dnl
dnl Create $as_me.lineno as a copy of $as_myself, but with $LINENO
dnl uniformly replaced by the line number.  The first 'sed' inserts a
dnl line-number line after each line using $LINENO; the second 'sed'
dnl does the real work.  The second script uses 'N' to pair each
dnl line-number line with the line containing $LINENO, and appends
dnl trailing '-' during substitution so that $LINENO is not a special
dnl case at line end.  (Raja R Harinath suggested sed '=', and Paul
dnl Eggert wrote the scripts with optimization help from Paolo Bonzini).
[_AS_LINENO_WORKS || {
[  # Blame Lee E. McMahon (1931-1989) for sed's syntax.  :-)
  sed -n '
    p
    /[$]LINENO/=
  ' <$as_myself |
    sed '
      s/[$]LINENO.*/&-/
      t lineno
      b
      :lineno
      N
      :loop
      s/[$]LINENO\([^'$as_cr_alnum'_].*\n\)\(.*\)/\2\1\2/
      t loop
      s/-\n.*//
    ' >$as_me.lineno &&
  chmod +x "$as_me.lineno"] ||
    AS_ERROR([cannot create $as_me.lineno; rerun with a POSIX shell])

  # Don't try to exec as it changes $[0], causing all sort of problems
  # (the dirname of $[0] is not the place where we might find the
  # original and so on.  Autoconf is especially sensitive to this).
  . "./$as_me.lineno"
  # Exit status is that of the last command.
  exit
}
_m4_popdef([AS_MESSAGE_LOG_FD], [AS_ERROR])])# _AS_LINENO_PREPARE


# _AS_LN_S_PREPARE
# ----------------
# Don't use conftest.sym to avoid file name issues on DJGPP, where this
# would yield conftest.sym.exe for DJGPP < 2.04.  And don't use `conftest'
# as base name to avoid prohibiting concurrency (e.g., concurrent
# config.statuses).  On read-only media, assume 'cp -p' and hope we
# are just running --help anyway.
m4_defun([_AS_LN_S_PREPARE],
[rm -f conf$$ conf$$.exe conf$$.file
if test -d conf$$.dir; then
  rm -f conf$$.dir/conf$$.file
else
  rm -f conf$$.dir
  mkdir conf$$.dir 2>/dev/null
fi
if (echo >conf$$.file) 2>/dev/null; then
  if ln -s conf$$.file conf$$ 2>/dev/null; then
    as_ln_s='ln -s'
    # ... but there are two gotchas:
    # 1) On MSYS, both `ln -s file dir' and `ln file dir' fail.
    # 2) DJGPP < 2.04 has no symlinks; `ln -s' creates a wrapper executable.
    # In both cases, we have to default to `cp -p'.
    ln -s conf$$.file conf$$.dir 2>/dev/null && test ! -f conf$$.exe ||
      as_ln_s='cp -p'
  elif ln conf$$.file conf$$ 2>/dev/null; then
    as_ln_s=ln
  else
    as_ln_s='cp -p'
  fi
else
  as_ln_s='cp -p'
fi
rm -f conf$$ conf$$.exe conf$$.dir/conf$$.file conf$$.file
rmdir conf$$.dir 2>/dev/null
])# _AS_LN_S_PREPARE


# AS_LN_S(FILE, LINK)
# -------------------
# FIXME: Should we add the glue code to handle properly relative symlinks
# simulated with `ln' or `cp'?
m4_defun_init([AS_LN_S],
[AS_REQUIRE([_AS_LN_S_PREPARE])],
[$as_ln_s $1 $2])


# _AS_MKDIR_P
# -----------
# Emit code that can be used to emulate `mkdir -p` with plain `mkdir';
# the code assumes that "$as_dir" contains the directory to create.
# $as_dir is normalized, so there is no need to worry about using --.
m4_define([_AS_MKDIR_P],
[case $as_dir in #(
  -*) as_dir=./$as_dir;;
  esac
  test -d "$as_dir" || eval $as_mkdir_p || {
    as_dirs=
    while :; do
      case $as_dir in #(
      *\'*) as_qdir=`AS_ECHO(["$as_dir"]) | sed "s/'/'\\\\\\\\''/g"`;; #'(
      *) as_qdir=$as_dir;;
      esac
      as_dirs="'$as_qdir' $as_dirs"
      as_dir=`AS_DIRNAME("$as_dir")`
      test -d "$as_dir" && break
    done
    test -z "$as_dirs" || eval "mkdir $as_dirs"
  } || test -d "$as_dir" || AS_ERROR([cannot create directory $as_dir])
])

# AS_MKDIR_P(DIR)
# ---------------
# Emulate `mkdir -p' with plain `mkdir' if needed.
m4_defun_init([AS_MKDIR_P],
[AS_REQUIRE([_$0_PREPARE])],
[as_dir=$1; as_fn_mkdir_p])# AS_MKDIR_P


# _AS_MKDIR_P_PREPARE
# -------------------
m4_defun([_AS_MKDIR_P_PREPARE],
[AS_REQUIRE_SHELL_FN([as_fn_mkdir_p],
  [AS_FUNCTION_DESCRIBE([as_fn_mkdir_p], [],
    [Create "$as_dir" as a directory, including parents if necessary.])],
[
  _AS_MKDIR_P
])]dnl
[if mkdir -p . 2>/dev/null; then
  as_mkdir_p='mkdir -p "$as_dir"'
else
  test -d ./-p && rmdir ./-p
  as_mkdir_p=false
fi
])# _AS_MKDIR_P_PREPARE


# _AS_PATH_SEPARATOR_PREPARE
# --------------------------
# Compute the path separator.
m4_defun([_AS_PATH_SEPARATOR_PREPARE],
[# The user is always right.
if test "${PATH_SEPARATOR+set}" != set; then
  PATH_SEPARATOR=:
  (PATH='/bin;/bin'; FPATH=$PATH; sh -c :) >/dev/null 2>&1 && {
    (PATH='/bin:/bin'; FPATH=$PATH; sh -c :) >/dev/null 2>&1 ||
      PATH_SEPARATOR=';'
  }
fi
])# _AS_PATH_SEPARATOR_PREPARE


# _AS_PATH_WALK([PATH = $PATH], BODY, [IF-NOT-FOUND])
# ---------------------------------------------------
# Walk through PATH running BODY for each `as_dir'.  If BODY never does a
# `break', evaluate IF-NOT-FOUND.
#
# Still very private as its interface looks quite bad.
#
# `$as_dummy' forces splitting on constant user-supplied paths.
# POSIX.2 field splitting is done only on the result of word
# expansions, not on literal text.  This closes a longstanding sh security
# hole.  Optimize it away when not needed, i.e., if there are no literal
# path separators.
m4_defun_init([_AS_PATH_WALK],
[AS_REQUIRE([_AS_PATH_SEPARATOR_PREPARE])],
[as_save_IFS=$IFS; IFS=$PATH_SEPARATOR
m4_ifvaln([$3], [as_found=false])dnl
m4_bmatch([$1], [[:;]],
[as_dummy="$1"
for as_dir in $as_dummy],
[for as_dir in m4_default([$1], [$PATH])])
do
  IFS=$as_save_IFS
  test -z "$as_dir" && as_dir=.
  m4_ifvaln([$3], [as_found=:])dnl
  $2
  m4_ifvaln([$3], [as_found=false])dnl
done
m4_ifvaln([$3], [$as_found || { $3; }])dnl
IFS=$as_save_IFS
])


# AS_SET_CATFILE(VAR, DIR-NAME, FILE-NAME)
# ----------------------------------------
# Set VAR to DIR-NAME/FILE-NAME.
# Optimize the common case where $2 or $3 is '.'.
m4_define([AS_SET_CATFILE],
[case $2 in @%:@((
.) AS_VAR_SET([$1], [$3]);;
*)
  case $3 in @%:@(((
  .) AS_VAR_SET([$1], [$2]);;
  [[\\/]]* | ?:[[\\/]]* ) AS_VAR_SET([$1], [$3]);;
  *) AS_VAR_SET([$1], [$2/$3]);;
  esac;;
esac[]])# AS_SET_CATFILE


# _AS_TEST_PREPARE
# ----------------
# Find out whether `test -x' works.  If not, prepare a substitute
# that should work well enough for most scripts.
#
# Here are some of the problems with the substitute.
# The 'ls' tests whether the owner, not the current user, can execute/search.
# The eval means '*', '?', and '[' cause inadvertent file name globbing
# after the 'eval', so jam together as many tokens as we can to minimize
# the likelihood that the inadvertent globbing will actually do anything.
# Luckily, this gorp is needed only on really ancient hosts.
#
m4_defun([_AS_TEST_PREPARE],
[if test -x / >/dev/null 2>&1; then
  as_test_x='test -x'
else
  if ls -dL / >/dev/null 2>&1; then
    as_ls_L_option=L
  else
    as_ls_L_option=
  fi
  as_test_x='
    eval sh -c '\''
      if test -d "$[]1"; then
	test -d "$[]1/.";
      else
	case $[]1 in @%:@(
	-*)set "./$[]1";;
	esac;
	case `ls -ld'$as_ls_L_option' "$[]1" 2>/dev/null` in @%:@((
	???[[sx]]*):;;*)false;;esac;fi
    '\'' sh
  '
fi
dnl as_executable_p is present for backward compatibility with Libtool
dnl 1.5.22, but it should go away at some point.
as_executable_p=$as_test_x
])# _AS_TEST_PREPARE




## ------------------ ##
## 5. Common idioms.  ##
## ------------------ ##

# This section is lexicographically sorted.


# AS_BOX(MESSAGE, [FRAME-CHARACTER = `-'])
# ----------------------------------------
# Output MESSAGE, a single line text, framed with FRAME-CHARACTER (which
# must not be `/').
m4_define([AS_BOX],
[_$0(m4_expand([$1]), [$2])])

m4_define([_AS_BOX],
[m4_if(m4_index(m4_translit([[$1]], [`\"], [$$$]), [$]),
  [-1], [$0_LITERAL], [$0_INDIR])($@)])


# _AS_BOX_LITERAL(MESSAGE, [FRAME-CHARACTER = `-'])
# -------------------------------------------------
m4_define([_AS_BOX_LITERAL],
[AS_ECHO(["_AS_ESCAPE(m4_dquote(m4_expand([m4_text_box($@)])), [`], [\"$])"])])


# _AS_BOX_INDIR(MESSAGE, [FRAME-CHARACTER = `-'])
# -----------------------------------------------
m4_define([_AS_BOX_INDIR],
[sed 'h;s/./m4_default([$2], [-])/g;s/^.../@%:@@%:@ /;s/...$/ @%:@@%:@/;p;x;p;x' <<_ASBOX
@%:@@%:@ $1 @%:@@%:@
_ASBOX])


# _AS_CLEAN_DIR(DIR)
# ------------------
# Remove all contents from within DIR, including any unwritable
# subdirectories, but leave DIR itself untouched.
m4_define([_AS_CLEAN_DIR],
[if test -d $1; then
  find $1 -type d ! -perm -700 -exec chmod u+rwx {} \;
  rm -fr $1/* $1/.[[!.]] $1/.??*
fi])


# AS_FUNCTION_DESCRIBE(NAME, [ARGS], DESCRIPTION, [WRAP-COLUMN = 79])
# -------------------------------------------------------------------
# Output a shell comment describing NAME and its arguments ARGS, then
# a separator line, then the DESCRIPTION wrapped at a decimal
# WRAP-COLUMN.  The output resembles:
#  # NAME ARGS
#  # ---------
#  # Wrapped DESCRIPTION text
# NAME and ARGS are expanded, while DESCRIPTION is treated as a
# whitespace-separated list of strings that are not expanded.
m4_define([AS_FUNCTION_DESCRIBE],
[@%:@ $1[]m4_ifval([$2], [ $2])
@%:@ m4_translit(m4_format([%*s],
	   m4_decr(m4_qlen(_m4_expand([$1[]m4_ifval([$2], [ $2])
]))), []), [ ], [-])
m4_text_wrap([$3], [@%:@ ], [], [$4])])


# AS_HELP_STRING(LHS, RHS, [INDENT-COLUMN = 26], [WRAP-COLUMN = 79])
# ------------------------------------------------------------------
#
# Format a help string so that it looks pretty when the user executes
# "script --help".  This macro takes up to four arguments, a
# "left hand side" (LHS), a "right hand side" (RHS), a decimal
# INDENT-COLUMN which is the column where wrapped lines should begin
# (the default of 26 is recommended), and a decimal WRAP-COLUMN which is
# the column where lines should wrap (the default of 79 is recommended).
# LHS is expanded, RHS is not.
#
# For backwards compatibility not documented in the manual, INDENT-COLUMN
# can also be specified as a string of white spaces, whose width
# determines the indentation column.  Using TABs in INDENT-COLUMN is not
# recommended, since screen width of TAB is not computed.
#
# The resulting string is suitable for use in other macros that require
# a help string (e.g. AC_ARG_WITH).
#
# Here is the sample string from the Autoconf manual (Node: External
# Software) which shows the proper spacing for help strings.
#
#    --with-readline         support fancy command line editing
#  ^ ^                       ^
#  | |                       |
#  | column 2                column 26
#  |
#  column 0
#
# A help string is made up of a "left hand side" (LHS) and a "right
# hand side" (RHS).  In the example above, the LHS is
# "--with-readline", while the RHS is "support fancy command line
# editing".
#
# If the LHS contains more than (INDENT-COLUMN - 3) characters, then the
# LHS is terminated with a newline so that the RHS starts on a line of its
# own beginning at INDENT-COLUMN.  In the default case, this corresponds to an
# LHS with more than 23 characters.
#
# Therefore, in the example, if the LHS were instead
# "--with-readline-blah-blah-blah", then the AS_HELP_STRING macro would
# expand into:
#
#
#    --with-readline-blah-blah-blah
#  ^ ^                       support fancy command line editing
#  | |                       ^
#  | column 2                |
#  column 0                  column 26
#
#
# m4_text_wrap hacks^Wworks around the fact that m4_format does not
# know quadrigraphs.
#
m4_define([AS_HELP_STRING],
[m4_text_wrap([$2], m4_cond([[$3]], [], [                          ],
			    [m4_eval([$3]+0)], [0], [[$3]],
			    [m4_format([[%*s]], [$3], [])]),
	      m4_expand([  $1 ]), [$4])])# AS_HELP_STRING


# AS_IDENTIFIER_IF(EXPRESSION, IF-IDENT, IF-NOT-IDENT)
# ----------------------------------------------------
# If EXPRESSION serves as an identifier (ie, after removal of @&t@, it
# matches the regex `^[a-zA-Z_][a-zA-Z_0-9]*$'), execute IF-IDENT,
# otherwise IF-NOT-IDENT.
#
# This is generally faster than the alternative:
#   m4_bmatch(m4_bpatsubst([[$1]], [@&t@]), ^m4_defn([m4_re_word])$,
#             [$2], [$3])
#
# Rather than expand m4_defn every time AS_IDENTIFIER_IF is expanded, we
# inline its expansion up front.  Only use a regular expression if we
# detect a potential quadrigraph.
#
# First, check if the entire string matches m4_cr_symbol2.  Only then do
# we worry if the first character also matches m4_cr_symbol1 (ie. does not
# match m4_cr_digit).
m4_define([AS_IDENTIFIER_IF],
[m4_if(_$0(m4_if(m4_index([$1], [@]), [-1],
  [[$1]], [m4_bpatsubst([[$1]], [@&t@])])), [-], [$2], [$3])])

m4_define([_AS_IDENTIFIER_IF],
[m4_cond([[$1]], [], [],
	 [m4_eval(m4_len(m4_translit([[$1]], ]]dnl
m4_dquote(m4_dquote(m4_defn([m4_cr_symbols2])))[[)) > 0)], [1], [],
	 [m4_len(m4_translit(m4_format([[%.1s]], [$1]), ]]dnl
m4_dquote(m4_dquote(m4_defn([m4_cr_symbols1])))[[))], [0], [-])])


# AS_LITERAL_IF(EXPRESSION, IF-LITERAL, IF-NOT-LITERAL,
#               [IF-SIMPLE-REF = IF-NOT-LITERAL])
# -----------------------------------------------------
# If EXPRESSION has no shell indirections ($var or `expr`), expand
# IF-LITERAL, else IF-NOT-LITERAL.  In some cases, IF-NOT-LITERAL
# must be complex to safely deal with ``, while a simpler
# expression IF-SIMPLE-REF can be used if the indirection
# involves only shell variable expansion (as in ${varname}).
#
# EXPRESSION is treated as a literal if it results in the same
# interpretation whether it is unquoted or contained within double
# quotes, with the exception that whitespace is ignored (on the
# assumption that it will be flattened to _).  Therefore, neither `\$'
# nor `a''b' is a literal, since both backslash and single quotes have
# different quoting behavior in the two contexts; and `a*' is not a
# literal, because it has different globbing.  Note, however, that
# while `${a+b}' is neither a literal nor a simple ref, `a+b' is a
# literal.  This macro is an *approximation*: it is possible that
# there are some EXPRESSIONs which the shell would treat as literals,
# but which this macro does not recognize.
#
# Why do we reject EXPRESSION expanding with `[' or `]' as a literal?
# Because AS_TR_SH is MUCH faster if it can use m4_translit on literals
# instead of m4_bpatsubst; but m4_translit is much tougher to do safely
# if `[' is translated.  That, and file globbing matters.
#
# Note that the quadrigraph @S|@ can result in non-literals, but outright
# rejecting all @ would make AC_INIT complain on its bug report address.
#
# We used to use m4_bmatch(m4_quote($1), [[`$]], [$3], [$2]), but
# profiling shows that it is faster to use m4_translit.
#
# Because the translit is stripping quotes, it must also neutralize
# anything that might be in a macro name, as well as comments, commas,
# or unbalanced parentheses.  Valid shell variable characters and
# unambiguous literal characters are deleted (`a.b'), and remaining
# characters are normalized into `$' if they can form simple refs
# (${a}), `+' if they can potentially form literals (a+b), ``' if they
# can interfere with m4 parsing, or left alone otherwise.  If both `$'
# and `+' are left, it is treated as a complex reference (${a+b}),
# even though it could technically be a simple reference (${a}+b).
# _AS_LITERAL_IF_ only has to check for an empty string after removing
# one of the two normalized characters.
#
# Rather than expand m4_defn every time AS_LITERAL_IF is expanded, we
# inline its expansion up front.  _AS_LITERAL_IF expands to the name
# of a macro that takes three arguments: IF-SIMPLE-REF,
# IF-NOT-LITERAL, IF-LITERAL.  It also takes an optional argument of
# any additional characters to allow as literals (useful for AS_TR_SH
# and AS_TR_CPP to perform inline conversion of whitespace to _).  The
# order of the arguments allows reuse of m4_default.
m4_define([AS_LITERAL_IF],
[_$0(m4_expand([$1]), [	 ][
])([$4], [$3], [$2])])

m4_define([_AS_LITERAL_IF],
[m4_if(m4_index([$1], [@S|@]), [-1], [$0_(m4_translit([$1],
  [-:%/@{}[]#(),.$2]]]m4_dquote(m4_dquote(m4_defn([m4_cr_symbols2])))[[,
  [+++++$$`````]))], [$0_NO])])

m4_define([_AS_LITERAL_IF_],
[m4_if(m4_translit([$1], [+]), [], [$0YES],
       m4_translit([$1], [$]), [], [m4_default], [$0NO])])

m4_define([_AS_LITERAL_IF_YES], [$3])
m4_define([_AS_LITERAL_IF_NO], [$2])

# AS_LITERAL_WORD_IF(EXPRESSION, IF-LITERAL, IF-NOT-LITERAL,
#                    [IF-SIMPLE-REF = IF-NOT-LITERAL])
# ----------------------------------------------------------
# Like AS_LITERAL_IF, except that spaces and tabs in EXPRESSION
# are treated as non-literal.
m4_define([AS_LITERAL_WORD_IF],
[_AS_LITERAL_IF(m4_expand([$1]))([$4], [$3], [$2])])

# AS_LITERAL_HEREDOC_IF(EXPRESSION, IF-LITERAL, IF-NOT-LITERAL)
# -------------------------------------------------------------
# Like AS_LITERAL_IF, except that a string is considered literal
# if it results in the same output in both quoted and unquoted
# here-documents.
m4_define([AS_LITERAL_HEREDOC_IF],
[_$0(m4_expand([$1]))([$2], [$3])])

m4_define([_AS_LITERAL_HEREDOC_IF],
[m4_if(m4_index([$1], [@S|@]), [-1],
  [m4_if(m4_index(m4_translit([[$1]], [\`], [$]), [$]), [-1],
    [$0_YES], [$0_NO])],
  [$0_NO])])

m4_define([_AS_LITERAL_HEREDOC_IF_YES], [$1])
m4_define([_AS_LITERAL_HEREDOC_IF_NO], [$2])


# AS_TMPDIR(PREFIX, [DIRECTORY = $TMPDIR [= /tmp]])
# -------------------------------------------------
# Create as safely as possible a temporary directory in DIRECTORY
# which name is inspired by PREFIX (should be 2-4 chars max).
#
# Even though $tmp does not fit our normal naming scheme of $as_*,
# it is a documented part of the public API and must not be changed.
m4_define([AS_TMPDIR],
[# Create a (secure) tmp directory for tmp files.
m4_if([$2], [], [: "${TMPDIR:=/tmp}"])
{
  tmp=`(umask 077 && mktemp -d "m4_default([$2],
    [$TMPDIR])/$1XXXXXX") 2>/dev/null` &&
  test -d "$tmp"
}  ||
{
  tmp=m4_default([$2], [$TMPDIR])/$1$$-$RANDOM
  (umask 077 && mkdir "$tmp")
} || AS_ERROR([cannot create a temporary directory in m4_default([$2],
	      [$TMPDIR])])])# AS_TMPDIR


# AS_UNAME
# --------
# Try to describe this machine.  Meant for logs.
m4_define([AS_UNAME],
[{
cat <<_ASUNAME
m4_text_box([Platform.])

hostname = `(hostname || uname -n) 2>/dev/null | sed 1q`
uname -m = `(uname -m) 2>/dev/null || echo unknown`
uname -r = `(uname -r) 2>/dev/null || echo unknown`
uname -s = `(uname -s) 2>/dev/null || echo unknown`
uname -v = `(uname -v) 2>/dev/null || echo unknown`

/usr/bin/uname -p = `(/usr/bin/uname -p) 2>/dev/null || echo unknown`
/bin/uname -X     = `(/bin/uname -X) 2>/dev/null     || echo unknown`

/bin/arch              = `(/bin/arch) 2>/dev/null              || echo unknown`
/usr/bin/arch -k       = `(/usr/bin/arch -k) 2>/dev/null       || echo unknown`
/usr/convex/getsysinfo = `(/usr/convex/getsysinfo) 2>/dev/null || echo unknown`
/usr/bin/hostinfo      = `(/usr/bin/hostinfo) 2>/dev/null      || echo unknown`
/bin/machine           = `(/bin/machine) 2>/dev/null           || echo unknown`
/usr/bin/oslevel       = `(/usr/bin/oslevel) 2>/dev/null       || echo unknown`
/bin/universe          = `(/bin/universe) 2>/dev/null          || echo unknown`

_ASUNAME

_AS_PATH_WALK([$PATH], [AS_ECHO(["PATH: $as_dir"])])
}])


# _AS_VERSION_COMPARE_PREPARE
# ---------------------------
# Output variables for comparing version numbers.
m4_defun([_AS_VERSION_COMPARE_PREPARE],
[[as_awk_strverscmp='
  # Use only awk features that work with 7th edition Unix awk (1978).
  # My, what an old awk you have, Mr. Solaris!
  END {
    while (length(v1) && length(v2)) {
      # Set d1 to be the next thing to compare from v1, and likewise for d2.
      # Normally this is a single character, but if v1 and v2 contain digits,
      # compare them as integers and fractions as strverscmp does.
      if (v1 ~ /^[0-9]/ && v2 ~ /^[0-9]/) {
	# Split v1 and v2 into their leading digit string components d1 and d2,
	# and advance v1 and v2 past the leading digit strings.
	for (len1 = 1; substr(v1, len1 + 1) ~ /^[0-9]/; len1++) continue
	for (len2 = 1; substr(v2, len2 + 1) ~ /^[0-9]/; len2++) continue
	d1 = substr(v1, 1, len1); v1 = substr(v1, len1 + 1)
	d2 = substr(v2, 1, len2); v2 = substr(v2, len2 + 1)
	if (d1 ~ /^0/) {
	  if (d2 ~ /^0/) {
	    # Compare two fractions.
	    while (d1 ~ /^0/ && d2 ~ /^0/) {
	      d1 = substr(d1, 2); len1--
	      d2 = substr(d2, 2); len2--
	    }
	    if (len1 != len2 && ! (len1 && len2 && substr(d1, 1, 1) == substr(d2, 1, 1))) {
	      # The two components differ in length, and the common prefix
	      # contains only leading zeros.  Consider the longer to be less.
	      d1 = -len1
	      d2 = -len2
	    } else {
	      # Otherwise, compare as strings.
	      d1 = "x" d1
	      d2 = "x" d2
	    }
	  } else {
	    # A fraction is less than an integer.
	    exit 1
	  }
	} else {
	  if (d2 ~ /^0/) {
	    # An integer is greater than a fraction.
	    exit 2
	  } else {
	    # Compare two integers.
	    d1 += 0
	    d2 += 0
	  }
	}
      } else {
	# The normal case, without worrying about digits.
	d1 = substr(v1, 1, 1); v1 = substr(v1, 2)
	d2 = substr(v2, 1, 1); v2 = substr(v2, 2)
      }
      if (d1 < d2) exit 1
      if (d1 > d2) exit 2
    }
    # Beware Solaris /usr/xgp4/bin/awk (at least through Solaris 10),
    # which mishandles some comparisons of empty strings to integers.
    if (length(v2)) exit 1
    if (length(v1)) exit 2
  }
']])# _AS_VERSION_COMPARE_PREPARE


# AS_VERSION_COMPARE(VERSION-1, VERSION-2,
#                    [ACTION-IF-LESS], [ACTION-IF-EQUAL], [ACTION-IF-GREATER])
# ----------------------------------------------------------------------------
# Compare two strings possibly containing shell variables as version strings.
#
# This usage is portable even to ancient awk,
# so don't worry about finding a "nice" awk version.
m4_defun_init([AS_VERSION_COMPARE],
[AS_REQUIRE([_$0_PREPARE])],
[as_arg_v1=$1
as_arg_v2=$2
awk "$as_awk_strverscmp" v1="$as_arg_v1" v2="$as_arg_v2" /dev/null
AS_CASE([$?],
	[1], [$3],
	[0], [$4],
	[2], [$5])])# AS_VERSION_COMPARE



## --------------------------------------- ##
## 6. Common m4/sh character translation.  ##
## --------------------------------------- ##

# The point of this section is to provide high level macros comparable
# to m4's `translit' primitive, but m4/sh polymorphic.
# Transliteration of literal strings should be handled by m4, while
# shell variables' content will be translated at runtime (tr or sed).


# _AS_CR_PREPARE
# --------------
# Output variables defining common character ranges.
# See m4_cr_letters etc.
m4_defun([_AS_CR_PREPARE],
[# Avoid depending upon Character Ranges.
as_cr_letters='abcdefghijklmnopqrstuvwxyz'
as_cr_LETTERS='ABCDEFGHIJKLMNOPQRSTUVWXYZ'
as_cr_Letters=$as_cr_letters$as_cr_LETTERS
as_cr_digits='0123456789'
as_cr_alnum=$as_cr_Letters$as_cr_digits
])


# _AS_TR_SH_PREPARE
# -----------------
m4_defun([_AS_TR_SH_PREPARE],
[AS_REQUIRE([_AS_CR_PREPARE])]dnl
[# Sed expression to map a string onto a valid variable name.
as_tr_sh="eval sed 'y%*+%pp%;s%[[^_$as_cr_alnum]]%_%g'"
])


# AS_TR_SH(EXPRESSION)
# --------------------
# Transform EXPRESSION into a valid shell variable name.
# sh/m4 polymorphic.
# Be sure to update the definition of `$as_tr_sh' if you change this.
#
# AS_LITERAL_IF guarantees that a literal does not have any nested quotes,
# once $1 is expanded.  m4_translit silently uses only the first occurrence
# of a character that appears multiple times in argument 2, since we know
# that m4_cr_not_symbols2 also contains [ and ].  m4_translit also silently
# ignores characters in argument 3 that do not match argument 2; we use this
# fact to skip worrying about the length of m4_cr_not_symbols2.
#
# For speed, we inline the literal definitions that can be computed up front.
m4_defun_init([AS_TR_SH],
[AS_REQUIRE([_$0_PREPARE])],
[_$0(m4_expand([$1]))])

m4_define([_AS_TR_SH],
[_AS_LITERAL_IF([$1], [*][	 ][
])([], [$0_INDIR], [$0_LITERAL])([$1])])

m4_define([_AS_TR_SH_LITERAL],
[m4_translit([[$1]],
  [*+[]]]m4_dquote(m4_defn([m4_cr_not_symbols2]))[,
  [pp[]]]m4_dquote(m4_for(,1,255,,[[_]]))[)])

m4_define([_AS_TR_SH_INDIR],
[`AS_ECHO(["_AS_ESCAPE([[$1]], [`], [\])"]) | $as_tr_sh`])


# _AS_TR_CPP_PREPARE
# ------------------
m4_defun([_AS_TR_CPP_PREPARE],
[AS_REQUIRE([_AS_CR_PREPARE])]dnl
[# Sed expression to map a string onto a valid CPP name.
as_tr_cpp="eval sed 'y%*$as_cr_letters%P$as_cr_LETTERS%;s%[[^_$as_cr_alnum]]%_%g'"
])


# AS_TR_CPP(EXPRESSION)
# ---------------------
# Map EXPRESSION to an upper case string which is valid as rhs for a
# `#define'.  sh/m4 polymorphic.  Be sure to update the definition
# of `$as_tr_cpp' if you change this.
#
# See implementation comments in AS_TR_SH.
m4_defun_init([AS_TR_CPP],
[AS_REQUIRE([_$0_PREPARE])],
[_$0(m4_expand([$1]))])

m4_define([_AS_TR_CPP],
[_AS_LITERAL_IF([$1], [*][	 ][
])([], [$0_INDIR], [$0_LITERAL])([$1])])

m4_define([_AS_TR_CPP_LITERAL],
[m4_translit([[$1]],
  [*[]]]m4_dquote(m4_defn([m4_cr_letters])m4_defn([m4_cr_not_symbols2]))[,
  [P[]]]m4_dquote(m4_defn([m4_cr_LETTERS])m4_for(,1,255,,[[_]]))[)])

m4_define([_AS_TR_CPP_INDIR],
[`AS_ECHO(["_AS_ESCAPE([[$1]], [`], [\])"]) | $as_tr_cpp`])


# _AS_TR_PREPARE
# --------------
m4_defun([_AS_TR_PREPARE],
[AS_REQUIRE([_AS_TR_SH_PREPARE])AS_REQUIRE([_AS_TR_CPP_PREPARE])])




## ------------------------------------------------------ ##
## 7. Common m4/sh handling of variables (indirections).  ##
## ------------------------------------------------------ ##


# The purpose of this section is to provide a uniform API for
# reading/setting sh variables with or without indirection.
# Typically, one can write
#   AS_VAR_SET(var, val)
# or
#   AS_VAR_SET(as_$var, val)
# and expect the right thing to happen.  In the descriptions below,
# a literal name matches the regex [a-zA-Z_][a-zA-Z0-9_]*, an
# indirect name is a shell expression that produces a literal name
# when passed through eval, and a polymorphic name is either type.


# _AS_VAR_APPEND_PREPARE
# ----------------------
# Define as_fn_append to the optimum definition for the current
# shell (bash and zsh provide the += assignment operator to avoid
# quadratic append growth over repeated appends).
m4_defun([_AS_VAR_APPEND_PREPARE],
[AS_FUNCTION_DESCRIBE([as_fn_append], [VAR VALUE],
[Append the text in VALUE to the end of the definition contained in
VAR.  Take advantage of any shell optimizations that allow amortized
linear growth over repeated appends, instead of the typical quadratic
growth present in naive implementations.])
AS_IF([_AS_RUN(["AS_ESCAPE(m4_quote(_AS_VAR_APPEND_WORKS))"])],
[eval 'as_fn_append ()
  {
    eval $[]1+=\$[]2
  }'],
[as_fn_append ()
  {
    eval $[]1=\$$[]1\$[]2
  }]) # as_fn_append
])

# _AS_VAR_APPEND_WORKS
# --------------------
# Output a shell test to discover whether += works.
m4_define([_AS_VAR_APPEND_WORKS],
[as_var=1; as_var+=2; test x$as_var = x12])

# AS_VAR_APPEND(VAR, VALUE)
# -------------------------
# Append the shell expansion of VALUE to the end of the existing
# contents of the polymorphic shell variable VAR, taking advantage of
# any shell optimizations that allow repeated appends to result in
# amortized linear scaling rather than quadratic behavior.  This macro
# is not worth the overhead unless the expected final size of the
# contents of VAR outweigh the typical VALUE size of repeated appends.
# Note that unlike AS_VAR_SET, VALUE must be properly quoted to avoid
# field splitting and file name expansion.
m4_defun_init([AS_VAR_APPEND],
[AS_REQUIRE([_AS_VAR_APPEND_PREPARE], [], [M4SH-INIT-FN])],
[as_fn_append $1 $2])


# _AS_VAR_ARITH_PREPARE
# ---------------------
# Define as_fn_arith to the optimum definition for the current
# shell (using POSIX $(()) where supported).
m4_defun([_AS_VAR_ARITH_PREPARE],
[AS_FUNCTION_DESCRIBE([as_fn_arith], [ARG...],
[Perform arithmetic evaluation on the ARGs, and store the result in
the global $as_val.  Take advantage of shells that can avoid forks.
The arguments must be portable across $(()) and expr.])
AS_IF([_AS_RUN(["AS_ESCAPE(m4_quote(_AS_VAR_ARITH_WORKS))"])],
[eval 'as_fn_arith ()
  {
    as_val=$(( $[]* ))
  }'],
[as_fn_arith ()
  {
    as_val=`expr "$[]@" || test $? -eq 1`
  }]) # as_fn_arith
])

# _AS_VAR_ARITH_WORKS
# -------------------
# Output a shell test to discover whether $(()) works.
m4_define([_AS_VAR_ARITH_WORKS],
[test $(( 1 + 1 )) = 2])

# AS_VAR_ARITH(VAR, EXPR)
# -----------------------
# Perform the arithmetic evaluation of the arguments in EXPR, and set
# contents of the polymorphic shell variable VAR to the result, taking
# advantage of any shell optimizations that perform arithmetic without
# forks.  Note that numbers occuring within EXPR must be written in
# decimal, and without leading zeroes; variables containing numbers
# must be expanded prior to arithmetic evaluation; the first argument
# must not be a negative number; there is no portable equality
# operator; and operators must be given as separate arguments and
# properly quoted.
m4_defun_init([AS_VAR_ARITH],
[_AS_DETECT_SUGGESTED([_AS_VAR_ARITH_WORKS])]dnl
[AS_REQUIRE([_AS_VAR_ARITH_PREPARE], [], [M4SH-INIT-FN])],
[as_fn_arith $2 && AS_VAR_SET([$1], [$as_val])])


# AS_VAR_COPY(DEST, SOURCE)
# -------------------------
# Set the polymorphic shell variable DEST to the contents of the polymorphic
# shell variable SOURCE.
m4_define([AS_VAR_COPY],
[AS_LITERAL_WORD_IF([$1[]$2], [$1=$$2], [eval $1=\$$2])])


# AS_VAR_GET(VARIABLE)
# --------------------
# Get the value of the shell VARIABLE.
# Evaluates to $VARIABLE if there is no indirection in VARIABLE,
# else to the appropriate `eval' sequence.
# This macro is deprecated because it sometimes mishandles trailing newlines;
# use AS_VAR_COPY instead.
m4_define([AS_VAR_GET],
[AS_LITERAL_WORD_IF([$1],
	       [$$1],
  [`eval 'as_val=${'_AS_ESCAPE([[$1]], [`], [\])'};AS_ECHO(["$as_val"])'`])])


# AS_VAR_IF(VARIABLE, VALUE, IF-TRUE, IF-FALSE)
# ---------------------------------------------
# Implement a shell `if test $VARIABLE = VALUE; then-else'.
# Polymorphic, and avoids sh expansion error upon interrupt or term signal.
m4_define([AS_VAR_IF],
[AS_LITERAL_WORD_IF([$1],
  [AS_IF(m4_ifval([$2], [[test "x$$1" = x[]$2]], [[${$1:+false} :]])],
  [AS_VAR_COPY([as_val], [$1])
   AS_IF(m4_ifval([$2], [[test "x$as_val" = x[]$2]], [[${as_val:+false} :]])],
  [AS_IF(m4_ifval([$2],
    [[eval test \"x\$"$1"\" = x"_AS_ESCAPE([$2], [`], [\"$])"]],
    [[eval \${$1:+false} :]])]),
[$3], [$4])])


# AS_VAR_PUSHDEF and AS_VAR_POPDEF
# --------------------------------
#

# Sometimes we may have to handle literals (e.g. `stdlib.h'), while at
# other moments, the same code may have to get the value from a
# variable (e.g., `ac_header').  To have a uniform handling of both
# cases, when a new value is about to be processed, declare a local
# variable, e.g.:
#
#   AS_VAR_PUSHDEF([header], [ac_cv_header_$1])
#
# and then in the body of the macro, use `header' as is.  It is of
# first importance to use `AS_VAR_*' to access this variable.
#
# If the value `$1' was a literal (e.g. `stdlib.h'), then `header' is
# in fact the value `ac_cv_header_stdlib_h'.  If `$1' was indirect,
# then `header's value in m4 is in fact `$as_header', the shell
# variable that holds all of the magic to get the expansion right.
#
# At the end of the block, free the variable with
#
#   AS_VAR_POPDEF([header])


# AS_VAR_POPDEF(VARNAME)
# ----------------------
# Free the shell variable accessor VARNAME.  To be dnl'ed.
m4_define([AS_VAR_POPDEF],
[m4_popdef([$1])])


# AS_VAR_PUSHDEF(VARNAME, VALUE)
# ------------------------------
# Define the m4 macro VARNAME to an accessor to the shell variable
# named VALUE.  VALUE does not need to be a valid shell variable name:
# the transliteration is handled here.  To be dnl'ed.
#
# AS_TR_SH attempts to play with diversions if _AS_TR_SH_PREPARE has
# not been expanded.  However, users are expected to do subsequent
# calls that trigger AS_LITERAL_IF([VARNAME]), and that macro performs
# expansion inside an argument collection context, where diversions
# don't work.  Therefore, we must require the preparation ourselves.
m4_defun_init([AS_VAR_PUSHDEF],
[AS_REQUIRE([_AS_TR_SH_PREPARE])],
[_$0([$1], m4_expand([$2]))])

m4_define([_AS_VAR_PUSHDEF],
[_AS_LITERAL_IF([$2], [	 ][
])([], [as_$1=_AS_TR_SH_INDIR([$2])
m4_pushdef([$1], [$as_[$1]])],
[m4_pushdef([$1], [_AS_TR_SH_LITERAL([$2])])])])


# AS_VAR_SET(VARIABLE, VALUE)
# ---------------------------
# Set the contents of the polymorphic shell VARIABLE to the shell
# expansion of VALUE.  VALUE is immune to field splitting and file
# name expansion.
m4_define([AS_VAR_SET],
[AS_LITERAL_WORD_IF([$1],
	       [$1=$2],
	       [eval "$1=_AS_ESCAPE([$2], [`], [\"$])"])])


# AS_VAR_SET_IF(VARIABLE, IF-TRUE, IF-FALSE)
# ------------------------------------------
# Implement a shell `if-then-else' depending whether VARIABLE is set
# or not.  Polymorphic.
m4_define([AS_VAR_SET_IF],
[AS_IF([AS_VAR_TEST_SET([$1])], [$2], [$3])])


# AS_VAR_TEST_SET(VARIABLE)
# -------------------------
# Expands into an expression which is true if VARIABLE
# is set.  Polymorphic.
m4_define([AS_VAR_TEST_SET],
[AS_LITERAL_WORD_IF([$1],
  [${$1+:} false],
  [{ as_var=$1; eval \${$as_var+:} false; }],
  [eval \${$1+:} false])])


## -------------------- ##
## 8. Setting M4sh up.  ##
## -------------------- ##


# AS_INIT_GENERATED(FILE, [COMMENT])
# ----------------------------------
# Generate a child script FILE with all initialization necessary to
# reuse the environment learned by the parent script, and make the
# file executable.  If COMMENT is supplied, it is inserted after the
# `#!' sequence but before initialization text begins.  After this
# macro, additional text can be appended to FILE to form the body of
# the child script.  The macro ends with non-zero status if the
# file could not be fully written (such as if the disk is full).
m4_defun([AS_INIT_GENERATED],
[m4_require([AS_PREPARE])]dnl
[m4_pushdef([AS_MESSAGE_LOG_FD])]dnl
[as_write_fail=0
cat >$1 <<_ASEOF || as_write_fail=1
#! $SHELL
# Generated by $as_me.
$2
SHELL=\${CONFIG_SHELL-$SHELL}
export SHELL
_ASEOF
cat >>$1 <<\_ASEOF || as_write_fail=1
_AS_SHELL_SANITIZE
_AS_PREPARE
m4_if(AS_MESSAGE_FD, [1], [], [exec AS_MESSAGE_FD>&1
])]dnl
[m4_text_box([Main body of $1 script.])
_ASEOF
test $as_write_fail = 0 && chmod +x $1[]dnl
_m4_popdef([AS_MESSAGE_LOG_FD])])# AS_INIT_GENERATED


# AS_INIT
# -------
# Initialize m4sh.
m4_define([AS_INIT],
[# Wrap our cleanup prior to m4sugar's cleanup.
m4_wrap([_AS_CLEANUP])
m4_init
m4_provide([AS_INIT])

# Forbidden tokens and exceptions.
m4_pattern_forbid([^_?AS_])

# Bangshe and minimal initialization.
m4_divert_text([BINSH], [@%:@! /bin/sh])
m4_divert_text([HEADER-COMMENT],
	       [@%:@ Generated from __file__ by m4_PACKAGE_STRING.])
m4_divert_text([M4SH-SANITIZE], [_AS_SHELL_SANITIZE])
m4_divert_text([M4SH-INIT-FN], [m4_text_box([M4sh Shell Functions.])])

# Let's go!
m4_divert([BODY])dnl
m4_text_box([Main body of script.])
_AS_DETECT_REQUIRED([_AS_SHELL_FN_WORK])dnl
AS_REQUIRE([_AS_UNSET_PREPARE], [], [M4SH-INIT-FN])dnl
])
