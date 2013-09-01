# This file is part of Autoconf.			-*- Autoconf -*-
# Checking for functions.
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

# Written by David MacKenzie, with help from
# Franc,ois Pinard, Karl Berry, Richard Pixley, Ian Lance Taylor,
# Roland McGrath, Noah Friedman, david d zuhn, and many others.


# Table of contents
#
# 1. Generic tests for functions.
# 2. Functions to check with AC_CHECK_FUNCS
# 3. Tests for specific functions.


## -------------------------------- ##
## 1. Generic tests for functions.  ##
## -------------------------------- ##

# _AC_CHECK_FUNC_BODY
# -------------------
# Shell function body for AC_CHECK_FUNC.
m4_define([_AC_CHECK_FUNC_BODY],
[  AS_LINENO_PUSH([$[]1])
  AC_CACHE_CHECK([for $[]2], [$[]3],
  [AC_LINK_IFELSE([AC_LANG_FUNC_LINK_TRY($[]2)],
		  [AS_VAR_SET([$[]3], [yes])],
		  [AS_VAR_SET([$[]3], [no])])])
  AS_LINENO_POP
])# _AC_CHECK_FUNC_BODY


# AC_CHECK_FUNC(FUNCTION, [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# -----------------------------------------------------------------
# Check whether FUNCTION links in the current language.  Set the cache
# variable ac_cv_func_FUNCTION accordingly, then execute
# ACTION-IF-FOUND or ACTION-IF-NOT-FOUND.
AC_DEFUN([AC_CHECK_FUNC],
[AC_REQUIRE_SHELL_FN([ac_fn_]_AC_LANG_ABBREV[_check_func],
  [AS_FUNCTION_DESCRIBE([ac_fn_]_AC_LANG_ABBREV[_check_func],
    [LINENO FUNC VAR],
    [Tests whether FUNC exists, setting the cache variable VAR accordingly])],
  [_$0_BODY])]dnl
[AS_VAR_PUSHDEF([ac_var], [ac_cv_func_$1])]dnl
[ac_fn_[]_AC_LANG_ABBREV[]_check_func "$LINENO" "$1" "ac_var"
AS_VAR_IF([ac_var], [yes], [$2], [$3])
AS_VAR_POPDEF([ac_var])])# AC_CHECK_FUNC


# _AH_CHECK_FUNC(FUNCTION)
# ------------------------
# Prepare the autoheader snippet for FUNCTION.
m4_define([_AH_CHECK_FUNC],
[AH_TEMPLATE(AS_TR_CPP([HAVE_$1]),
  [Define to 1 if you have the `$1' function.])])


# AC_CHECK_FUNCS(FUNCTION..., [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# ---------------------------------------------------------------------
# Check for each whitespace-separated FUNCTION, and perform
# ACTION-IF-FOUND or ACTION-IF-NOT-FOUND for each function.
# Additionally, make the preprocessor definition HAVE_FUNCTION
# available for each found function.  Either ACTION may include
# `break' to stop the search.
AC_DEFUN([AC_CHECK_FUNCS],
[m4_map_args_w([$1], [_AH_CHECK_FUNC(], [)])]dnl
[AS_FOR([AC_func], [ac_func], [$1],
[AC_CHECK_FUNC(AC_func,
	       [AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_]AC_func)) $2],
	       [$3])dnl])
])# AC_CHECK_FUNCS


# _AC_CHECK_FUNC_ONCE(FUNCTION)
# -----------------------------
# Check for a single FUNCTION once.
m4_define([_AC_CHECK_FUNC_ONCE],
[_AH_CHECK_FUNC([$1])AC_DEFUN([_AC_Func_$1],
  [m4_divert_text([INIT_PREPARE], [AS_VAR_APPEND([ac_func_list], [" $1"])])
_AC_FUNCS_EXPANSION])AC_REQUIRE([_AC_Func_$1])])

# AC_CHECK_FUNCS_ONCE(FUNCTION...)
# --------------------------------
# Add each whitespace-separated name in FUNCTION to the list of functions
# to check once.
AC_DEFUN([AC_CHECK_FUNCS_ONCE],
[m4_map_args_w([$1], [_AC_CHECK_FUNC_ONCE(], [)])])

m4_define([_AC_FUNCS_EXPANSION],
[
  m4_divert_text([DEFAULTS], [ac_func_list=])
  AC_CHECK_FUNCS([$ac_func_list])
  m4_define([_AC_FUNCS_EXPANSION], [])
])


# _AC_REPLACE_FUNC(FUNCTION)
# --------------------------
# If FUNCTION exists, define HAVE_FUNCTION; else add FUNCTION.c
# to the list of library objects.  FUNCTION must be literal.
m4_define([_AC_REPLACE_FUNC],
[AC_CHECK_FUNC([$1],
  [_AH_CHECK_FUNC([$1])AC_DEFINE(AS_TR_CPP([HAVE_$1]))],
  [_AC_LIBOBJ([$1])AC_LIBSOURCE([$1.c])])])

# AC_REPLACE_FUNCS(FUNCTION...)
# -----------------------------
# For each FUNCTION in the whitespace separated list, perform the
# equivalent of AC_CHECK_FUNC, then call AC_LIBOBJ if the function
# was not found.
AC_DEFUN([AC_REPLACE_FUNCS],
[_$0(m4_flatten([$1]))])

m4_define([_AC_REPLACE_FUNCS],
[AS_LITERAL_IF([$1],
[m4_map_args_w([$1], [_AC_REPLACE_FUNC(], [)
])],
[AC_CHECK_FUNCS([$1],
  [_AH_CHECK_FUNC([$ac_func])AC_DEFINE(AS_TR_CPP([HAVE_$ac_func]))],
  [_AC_LIBOBJ([$ac_func])])])])


# AC_TRY_LINK_FUNC(FUNC, ACTION-IF-FOUND, ACTION-IF-NOT-FOUND)
# ------------------------------------------------------------
# Try to link a program that calls FUNC, handling GCC builtins.  If
# the link succeeds, execute ACTION-IF-FOUND; otherwise, execute
# ACTION-IF-NOT-FOUND.
AC_DEFUN([AC_TRY_LINK_FUNC],
[AC_LINK_IFELSE([AC_LANG_CALL([], [$1])], [$2], [$3])])


# AU::AC_FUNC_CHECK
# -----------------
AU_ALIAS([AC_FUNC_CHECK], [AC_CHECK_FUNC])


# AU::AC_HAVE_FUNCS
# -----------------
AU_ALIAS([AC_HAVE_FUNCS], [AC_CHECK_FUNCS])




## ------------------------------------------- ##
## 2. Functions to check with AC_CHECK_FUNCS.  ##
## ------------------------------------------- ##

AN_FUNCTION([__argz_count],            [AC_CHECK_FUNCS])
AN_FUNCTION([__argz_next],             [AC_CHECK_FUNCS])
AN_FUNCTION([__argz_stringify],        [AC_CHECK_FUNCS])
AN_FUNCTION([__fpending],              [AC_CHECK_FUNCS])
AN_FUNCTION([acl],                     [AC_CHECK_FUNCS])
AN_FUNCTION([alarm],                   [AC_CHECK_FUNCS])
AN_FUNCTION([atexit],                  [AC_CHECK_FUNCS])
AN_FUNCTION([btowc],                   [AC_CHECK_FUNCS])
AN_FUNCTION([bzero],                   [AC_CHECK_FUNCS])
AN_FUNCTION([clock_gettime],           [AC_CHECK_FUNCS])
AN_FUNCTION([doprnt],                  [AC_CHECK_FUNCS])
AN_FUNCTION([dup2],                    [AC_CHECK_FUNCS])
AN_FUNCTION([endgrent],                [AC_CHECK_FUNCS])
AN_FUNCTION([endpwent],                [AC_CHECK_FUNCS])
AN_FUNCTION([euidaccess],              [AC_CHECK_FUNCS])
AN_FUNCTION([fchdir],                  [AC_CHECK_FUNCS])
AN_FUNCTION([fdatasync],               [AC_CHECK_FUNCS])
AN_FUNCTION([fesetround],              [AC_CHECK_FUNCS])
AN_FUNCTION([floor],                   [AC_CHECK_FUNCS])
AN_FUNCTION([fs_stat_dev],             [AC_CHECK_FUNCS])
AN_FUNCTION([ftime],                   [AC_CHECK_FUNCS])
AN_FUNCTION([ftruncate],               [AC_CHECK_FUNCS])
AN_FUNCTION([getcwd],                  [AC_CHECK_FUNCS])
AN_FUNCTION([getdelim],                [AC_CHECK_FUNCS])
AN_FUNCTION([gethostbyaddr],           [AC_CHECK_FUNCS])
AN_FUNCTION([gethostbyname],           [AC_CHECK_FUNCS])
AN_FUNCTION([gethostname],             [AC_CHECK_FUNCS])
AN_FUNCTION([gethrtime],               [AC_CHECK_FUNCS])
AN_FUNCTION([getmntent],               [AC_CHECK_FUNCS])
AN_FUNCTION([getmntinfo],              [AC_CHECK_FUNCS])
AN_FUNCTION([getpagesize],             [AC_CHECK_FUNCS])
AN_FUNCTION([getpass],                 [AC_CHECK_FUNCS])
AN_FUNCTION([getspnam],                [AC_CHECK_FUNCS])
AN_FUNCTION([gettimeofday],            [AC_CHECK_FUNCS])
AN_FUNCTION([getusershell],            [AC_CHECK_FUNCS])
AN_FUNCTION([hasmntopt],               [AC_CHECK_FUNCS])
AN_FUNCTION([inet_ntoa],               [AC_CHECK_FUNCS])
AN_FUNCTION([isascii],                 [AC_CHECK_FUNCS])
AN_FUNCTION([iswprint],                [AC_CHECK_FUNCS])
AN_FUNCTION([lchown],                  [AC_CHECK_FUNCS])
AN_FUNCTION([listmntent],              [AC_CHECK_FUNCS])
AN_FUNCTION([localeconv],              [AC_CHECK_FUNCS])
AN_FUNCTION([localtime_r],             [AC_CHECK_FUNCS])
AN_FUNCTION([mblen],                   [AC_CHECK_FUNCS])
AN_FUNCTION([mbrlen],                  [AC_CHECK_FUNCS])
AN_FUNCTION([memchr],                  [AC_CHECK_FUNCS])
AN_FUNCTION([memmove],                 [AC_CHECK_FUNCS])
AN_FUNCTION([mempcpy],                 [AC_CHECK_FUNCS])
AN_FUNCTION([memset],                  [AC_CHECK_FUNCS])
AN_FUNCTION([mkdir],                   [AC_CHECK_FUNCS])
AN_FUNCTION([mkfifo],                  [AC_CHECK_FUNCS])
AN_FUNCTION([modf],                    [AC_CHECK_FUNCS])
AN_FUNCTION([munmap],                  [AC_CHECK_FUNCS])
AN_FUNCTION([next_dev],                [AC_CHECK_FUNCS])
AN_FUNCTION([nl_langinfo],             [AC_CHECK_FUNCS])
AN_FUNCTION([pathconf],                [AC_CHECK_FUNCS])
AN_FUNCTION([pow],                     [AC_CHECK_FUNCS])
AN_FUNCTION([pstat_getdynamic],        [AC_CHECK_FUNCS])
AN_FUNCTION([putenv],                  [AC_CHECK_FUNCS])
AN_FUNCTION([re_comp],                 [AC_CHECK_FUNCS])
AN_FUNCTION([realpath],                [AC_CHECK_FUNCS])
AN_FUNCTION([regcmp],                  [AC_CHECK_FUNCS])
AN_FUNCTION([regcomp],                 [AC_CHECK_FUNCS])
AN_FUNCTION([resolvepath],             [AC_CHECK_FUNCS])
AN_FUNCTION([rint],                    [AC_CHECK_FUNCS])
AN_FUNCTION([rmdir],                   [AC_CHECK_FUNCS])
AN_FUNCTION([rpmatch],                 [AC_CHECK_FUNCS])
AN_FUNCTION([select],                  [AC_CHECK_FUNCS])
AN_FUNCTION([setenv],                  [AC_CHECK_FUNCS])
AN_FUNCTION([sethostname],             [AC_CHECK_FUNCS])
AN_FUNCTION([setlocale],               [AC_CHECK_FUNCS])
AN_FUNCTION([socket],                  [AC_CHECK_FUNCS])
AN_FUNCTION([sqrt],                    [AC_CHECK_FUNCS])
AN_FUNCTION([stime],                   [AC_CHECK_FUNCS])
AN_FUNCTION([stpcpy],                  [AC_CHECK_FUNCS])
AN_FUNCTION([strcasecmp],              [AC_CHECK_FUNCS])
AN_FUNCTION([strchr],                  [AC_CHECK_FUNCS])
AN_FUNCTION([strcspn],                 [AC_CHECK_FUNCS])
AN_FUNCTION([strdup],                  [AC_CHECK_FUNCS])
AN_FUNCTION([strerror],                [AC_CHECK_FUNCS])
AN_FUNCTION([strncasecmp],             [AC_CHECK_FUNCS])
AN_FUNCTION([strndup],                 [AC_CHECK_FUNCS])
AN_FUNCTION([strpbrk],                 [AC_CHECK_FUNCS])
AN_FUNCTION([strrchr],                 [AC_CHECK_FUNCS])
AN_FUNCTION([strspn],                  [AC_CHECK_FUNCS])
AN_FUNCTION([strstr],                  [AC_CHECK_FUNCS])
AN_FUNCTION([strtol],                  [AC_CHECK_FUNCS])
AN_FUNCTION([strtoul],                 [AC_CHECK_FUNCS])
AN_FUNCTION([strtoull],                [AC_CHECK_FUNCS])
AN_FUNCTION([strtoumax],               [AC_CHECK_FUNCS])
AN_FUNCTION([strverscmp],              [AC_CHECK_FUNCS])
AN_FUNCTION([sysinfo],                 [AC_CHECK_FUNCS])
AN_FUNCTION([tzset],                   [AC_CHECK_FUNCS])
AN_FUNCTION([uname],                   [AC_CHECK_FUNCS])
AN_FUNCTION([utime],                   [AC_CHECK_FUNCS])
AN_FUNCTION([utmpname],                [AC_CHECK_FUNCS])
AN_FUNCTION([utmpxname],               [AC_CHECK_FUNCS])
AN_FUNCTION([wcwidth],                 [AC_CHECK_FUNCS])


AN_FUNCTION([dcgettext],    [AM_GNU_GETTEXT])
AN_FUNCTION([getwd],        [warn: getwd is deprecated, use getcwd instead])


## --------------------------------- ##
## 3. Tests for specific functions.  ##
## --------------------------------- ##


# The macros are sorted:
#
# 1. AC_FUNC_* macros are sorted by alphabetical order.
#
# 2. Helping macros such as _AC_LIBOBJ_* are before the macro that
#    uses it.
#
# 3. Obsolete macros are right after the modern macro.



# _AC_LIBOBJ_ALLOCA
# -----------------
# Set up the LIBOBJ replacement of `alloca'.  Well, not exactly
# AC_LIBOBJ since we actually set the output variable `ALLOCA'.
# Nevertheless, for Automake, AC_LIBSOURCES it.
m4_define([_AC_LIBOBJ_ALLOCA],
[# The SVR3 libPW and SVR4 libucb both contain incompatible functions
# that cause trouble.  Some versions do not even contain alloca or
# contain a buggy version.  If you still want to use their alloca,
# use ar to extract alloca.o from them instead of compiling alloca.c.
AC_LIBSOURCES(alloca.c)
AC_SUBST([ALLOCA], [\${LIBOBJDIR}alloca.$ac_objext])dnl
AC_DEFINE(C_ALLOCA, 1, [Define to 1 if using `alloca.c'.])

AC_CACHE_CHECK(whether `alloca.c' needs Cray hooks, ac_cv_os_cray,
[AC_EGREP_CPP(webecray,
[#if defined CRAY && ! defined CRAY2
webecray
#else
wenotbecray
#endif
], ac_cv_os_cray=yes, ac_cv_os_cray=no)])
if test $ac_cv_os_cray = yes; then
  for ac_func in _getb67 GETB67 getb67; do
    AC_CHECK_FUNC($ac_func,
		  [AC_DEFINE_UNQUOTED(CRAY_STACKSEG_END, $ac_func,
				      [Define to one of `_getb67', `GETB67',
				       `getb67' for Cray-2 and Cray-YMP
				       systems. This function is required for
				       `alloca.c' support on those systems.])
    break])
  done
fi

AC_CACHE_CHECK([stack direction for C alloca],
	       [ac_cv_c_stack_direction],
[AC_RUN_IFELSE([AC_LANG_SOURCE(
[AC_INCLUDES_DEFAULT
int
find_stack_direction ()
{
  static char *addr = 0;
  auto char dummy;
  if (addr == 0)
    {
      addr = &dummy;
      return find_stack_direction ();
    }
  else
    return (&dummy > addr) ? 1 : -1;
}

int
main ()
{
  return find_stack_direction () < 0;
}])],
	       [ac_cv_c_stack_direction=1],
	       [ac_cv_c_stack_direction=-1],
	       [ac_cv_c_stack_direction=0])])
AH_VERBATIM([STACK_DIRECTION],
[/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
@%:@undef STACK_DIRECTION])dnl
AC_DEFINE_UNQUOTED(STACK_DIRECTION, $ac_cv_c_stack_direction)
])# _AC_LIBOBJ_ALLOCA


# AC_FUNC_ALLOCA
# --------------
AN_FUNCTION([alloca], [AC_FUNC_ALLOCA])
AN_HEADER([alloca.h], [AC_FUNC_ALLOCA])
AC_DEFUN([AC_FUNC_ALLOCA],
[AC_REQUIRE([AC_TYPE_SIZE_T])]dnl
[# The Ultrix 4.2 mips builtin alloca declared by alloca.h only works
# for constant arguments.  Useless!
AC_CACHE_CHECK([for working alloca.h], ac_cv_working_alloca_h,
[AC_LINK_IFELSE(
       [AC_LANG_PROGRAM([[@%:@include <alloca.h>]],
			[[char *p = (char *) alloca (2 * sizeof (int));
			  if (p) return 0;]])],
		[ac_cv_working_alloca_h=yes],
		[ac_cv_working_alloca_h=no])])
if test $ac_cv_working_alloca_h = yes; then
  AC_DEFINE(HAVE_ALLOCA_H, 1,
	    [Define to 1 if you have <alloca.h> and it should be used
	     (not on Ultrix).])
fi

AC_CACHE_CHECK([for alloca], ac_cv_func_alloca_works,
[AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[#ifdef __GNUC__
# define alloca __builtin_alloca
#else
# ifdef _MSC_VER
#  include <malloc.h>
#  define alloca _alloca
# else
#  ifdef HAVE_ALLOCA_H
#   include <alloca.h>
#  else
#   ifdef _AIX
 #pragma alloca
#   else
#    ifndef alloca /* predefined by HP cc +Olibcalls */
void *alloca (size_t);
#    endif
#   endif
#  endif
# endif
#endif
]],                               [[char *p = (char *) alloca (1);
				    if (p) return 0;]])],
		[ac_cv_func_alloca_works=yes],
		[ac_cv_func_alloca_works=no])])

if test $ac_cv_func_alloca_works = yes; then
  AC_DEFINE(HAVE_ALLOCA, 1,
	    [Define to 1 if you have `alloca', as a function or macro.])
else
  _AC_LIBOBJ_ALLOCA
fi
])# AC_FUNC_ALLOCA


# AU::AC_ALLOCA
# -------------
AU_ALIAS([AC_ALLOCA], [AC_FUNC_ALLOCA])


# AC_FUNC_CHOWN
# -------------
# Determine whether chown accepts arguments of -1 for uid and gid.
AN_FUNCTION([chown], [AC_FUNC_CHOWN])
AC_DEFUN([AC_FUNC_CHOWN],
[AC_REQUIRE([AC_TYPE_UID_T])dnl
AC_CHECK_HEADERS(unistd.h)
AC_CACHE_CHECK([for working chown], ac_cv_func_chown_works,
[AC_RUN_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT
#include <fcntl.h>
],
[[  char *f = "conftest.chown";
  struct stat before, after;

  if (creat (f, 0600) < 0)
    return 1;
  if (stat (f, &before) < 0)
    return 1;
  if (chown (f, (uid_t) -1, (gid_t) -1) == -1)
    return 1;
  if (stat (f, &after) < 0)
    return 1;
  return ! (before.st_uid == after.st_uid && before.st_gid == after.st_gid);
]])],
	       [ac_cv_func_chown_works=yes],
	       [ac_cv_func_chown_works=no],
	       [ac_cv_func_chown_works=no])
rm -f conftest.chown
])
if test $ac_cv_func_chown_works = yes; then
  AC_DEFINE(HAVE_CHOWN, 1,
	    [Define to 1 if your system has a working `chown' function.])
fi
])# AC_FUNC_CHOWN


# AC_FUNC_CLOSEDIR_VOID
# ---------------------
# Check whether closedir returns void, and #define CLOSEDIR_VOID in
# that case.
AC_DEFUN([AC_FUNC_CLOSEDIR_VOID],
[AC_REQUIRE([AC_HEADER_DIRENT])dnl
AC_CACHE_CHECK([whether closedir returns void],
	       [ac_cv_func_closedir_void],
[AC_RUN_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT
#include <$ac_header_dirent>
#ifndef __cplusplus
int closedir ();
#endif
],
				[[return closedir (opendir (".")) != 0;]])],
	       [ac_cv_func_closedir_void=no],
	       [ac_cv_func_closedir_void=yes],
	       [ac_cv_func_closedir_void=yes])])
if test $ac_cv_func_closedir_void = yes; then
  AC_DEFINE(CLOSEDIR_VOID, 1,
	    [Define to 1 if the `closedir' function returns void instead
	     of `int'.])
fi
])


# AC_FUNC_ERROR_AT_LINE
# ---------------------
AN_FUNCTION([error],         [AC_FUNC_ERROR_AT_LINE])
AN_FUNCTION([error_at_line], [AC_FUNC_ERROR_AT_LINE])
AC_DEFUN([AC_FUNC_ERROR_AT_LINE],
[AC_LIBSOURCES([error.h, error.c])dnl
AC_CACHE_CHECK([for error_at_line], ac_cv_lib_error_at_line,
[AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <error.h>],
				 [error_at_line (0, 0, "", 0, "an error occurred");])],
		[ac_cv_lib_error_at_line=yes],
		[ac_cv_lib_error_at_line=no])])
if test $ac_cv_lib_error_at_line = no; then
  AC_LIBOBJ(error)
fi
])


# AU::AM_FUNC_ERROR_AT_LINE
# -------------------------
AU_ALIAS([AM_FUNC_ERROR_AT_LINE], [AC_FUNC_ERROR_AT_LINE])


# _AC_FUNC_FNMATCH_IF(STANDARD = GNU | POSIX, CACHE_VAR, IF-TRUE, IF-FALSE)
# -------------------------------------------------------------------------
# If a STANDARD compliant fnmatch is found, run IF-TRUE, otherwise
# IF-FALSE.  Use CACHE_VAR.
AC_DEFUN([_AC_FUNC_FNMATCH_IF],
[AC_CACHE_CHECK(
   [for working $1 fnmatch],
   [$2],
  [# Some versions of Solaris, SCO, and the GNU C Library
   # have a broken or incompatible fnmatch.
   # So we run a test program.  If we are cross-compiling, take no chance.
   # Thanks to John Oleynick, Franc,ois Pinard, and Paul Eggert for this test.
   AC_RUN_IFELSE(
      [AC_LANG_PROGRAM(
	 [#include <fnmatch.h>
#	   define y(a, b, c) (fnmatch (a, b, c) == 0)
#	   define n(a, b, c) (fnmatch (a, b, c) == FNM_NOMATCH)
	 ],
	 [return
	   (!(y ("a*", "abc", 0)
	      && n ("d*/*1", "d/s/1", FNM_PATHNAME)
	      && y ("a\\\\bc", "abc", 0)
	      && n ("a\\\\bc", "abc", FNM_NOESCAPE)
	      && y ("*x", ".x", 0)
	      && n ("*x", ".x", FNM_PERIOD)
	      && m4_if([$1], [GNU],
		   [y ("xxXX", "xXxX", FNM_CASEFOLD)
		    && y ("a++(x|yy)b", "a+xyyyyxb", FNM_EXTMATCH)
		    && n ("d*/*1", "d/s/1", FNM_FILE_NAME)
		    && y ("*", "x", FNM_FILE_NAME | FNM_LEADING_DIR)
		    && y ("x*", "x/y/z", FNM_FILE_NAME | FNM_LEADING_DIR)
		    && y ("*c*", "c/x", FNM_FILE_NAME | FNM_LEADING_DIR)],
		   1)));])],
      [$2=yes],
      [$2=no],
      [$2=cross])])
AS_IF([test $$2 = yes], [$3], [$4])
])# _AC_FUNC_FNMATCH_IF


# AC_FUNC_FNMATCH
# ---------------
AC_DEFUN([AC_FUNC_FNMATCH],
[_AC_FUNC_FNMATCH_IF([POSIX], [ac_cv_func_fnmatch_works],
		     [AC_DEFINE([HAVE_FNMATCH], 1,
		     [Define to 1 if your system has a working POSIX `fnmatch'
		      function.])])
])# AC_FUNC_FNMATCH


# _AC_LIBOBJ_FNMATCH
# ------------------
# Prepare the replacement of fnmatch.
AC_DEFUN([_AC_LIBOBJ_FNMATCH],
[AC_REQUIRE([AC_C_CONST])dnl
AC_REQUIRE([AC_FUNC_ALLOCA])dnl
AC_REQUIRE([AC_TYPE_MBSTATE_T])dnl
AC_CHECK_DECLS([getenv])
AC_CHECK_FUNCS([btowc mbsrtowcs mempcpy wmempcpy])
AC_CHECK_HEADERS([wchar.h wctype.h])
AC_LIBOBJ([fnmatch])
AC_CONFIG_LINKS([$ac_config_libobj_dir/fnmatch.h:$ac_config_libobj_dir/fnmatch_.h])
AC_DEFINE(fnmatch, rpl_fnmatch,
	  [Define to rpl_fnmatch if the replacement function should be used.])
])# _AC_LIBOBJ_FNMATCH


# AC_REPLACE_FNMATCH
# ------------------
AC_DEFUN([AC_REPLACE_FNMATCH],
[_AC_FUNC_FNMATCH_IF([POSIX], [ac_cv_func_fnmatch_works],
		     [rm -f "$ac_config_libobj_dir/fnmatch.h"],
		     [_AC_LIBOBJ_FNMATCH])
])# AC_REPLACE_FNMATCH


# AC_FUNC_FNMATCH_GNU
# -------------------
AC_DEFUN([AC_FUNC_FNMATCH_GNU],
[AC_REQUIRE([AC_GNU_SOURCE])
_AC_FUNC_FNMATCH_IF([GNU], [ac_cv_func_fnmatch_gnu],
		    [rm -f "$ac_config_libobj_dir/fnmatch.h"],
		    [_AC_LIBOBJ_FNMATCH])
])# AC_FUNC_FNMATCH_GNU


# AU::AM_FUNC_FNMATCH
# AU::fp_FUNC_FNMATCH
# -------------------
AU_ALIAS([AM_FUNC_FNMATCH], [AC_FUNC_FNMATCH])
AU_ALIAS([fp_FUNC_FNMATCH], [AC_FUNC_FNMATCH])


# AC_FUNC_FSEEKO
# --------------
AN_FUNCTION([ftello], [AC_FUNC_FSEEKO])
AN_FUNCTION([fseeko], [AC_FUNC_FSEEKO])
AC_DEFUN([AC_FUNC_FSEEKO],
[_AC_SYS_LARGEFILE_MACRO_VALUE(_LARGEFILE_SOURCE, 1,
   [ac_cv_sys_largefile_source],
   [Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2).],
   [[#include <sys/types.h> /* for off_t */
     #include <stdio.h>]],
   [[int (*fp) (FILE *, off_t, int) = fseeko;
     return fseeko (stdin, 0, 0) && fp (stdin, 0, 0);]])

# We used to try defining _XOPEN_SOURCE=500 too, to work around a bug
# in glibc 2.1.3, but that breaks too many other things.
# If you want fseeko and ftello with glibc, upgrade to a fixed glibc.
if test $ac_cv_sys_largefile_source != unknown; then
  AC_DEFINE(HAVE_FSEEKO, 1,
    [Define to 1 if fseeko (and presumably ftello) exists and is declared.])
fi
])# AC_FUNC_FSEEKO


# AC_FUNC_GETGROUPS
# -----------------
# Try to find `getgroups', and check that it works.
# When cross-compiling, assume getgroups is broken.
AN_FUNCTION([getgroups], [AC_FUNC_GETGROUPS])
AC_DEFUN([AC_FUNC_GETGROUPS],
[AC_REQUIRE([AC_TYPE_GETGROUPS])dnl
AC_REQUIRE([AC_TYPE_SIZE_T])dnl
AC_CHECK_FUNC(getgroups)

# If we don't yet have getgroups, see if it's in -lbsd.
# This is reported to be necessary on an ITOS 3000WS running SEIUX 3.1.
ac_save_LIBS=$LIBS
if test $ac_cv_func_getgroups = no; then
  AC_CHECK_LIB(bsd, getgroups, [GETGROUPS_LIB=-lbsd])
fi

# Run the program to test the functionality of the system-supplied
# getgroups function only if there is such a function.
if test $ac_cv_func_getgroups = yes; then
  AC_CACHE_CHECK([for working getgroups], ac_cv_func_getgroups_works,
   [AC_RUN_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT],
      [[/* On Ultrix 4.3, getgroups (0, 0) always fails.  */
       return getgroups (0, 0) == -1;]])],
		  [ac_cv_func_getgroups_works=yes],
		  [ac_cv_func_getgroups_works=no],
		  [ac_cv_func_getgroups_works=no])
   ])
else
  ac_cv_func_getgroups_works=no
fi
if test $ac_cv_func_getgroups_works = yes; then
  AC_DEFINE(HAVE_GETGROUPS, 1,
	    [Define to 1 if your system has a working `getgroups' function.])
fi
LIBS=$ac_save_LIBS
])# AC_FUNC_GETGROUPS


# _AC_LIBOBJ_GETLOADAVG
# ---------------------
# Set up the AC_LIBOBJ replacement of `getloadavg'.
m4_define([_AC_LIBOBJ_GETLOADAVG],
[AC_LIBOBJ(getloadavg)
AC_DEFINE(C_GETLOADAVG, 1, [Define to 1 if using `getloadavg.c'.])
# Figure out what our getloadavg.c needs.
ac_have_func=no
AC_CHECK_HEADER(sys/dg_sys_info.h,
[ac_have_func=yes
 AC_DEFINE(DGUX, 1, [Define to 1 for DGUX with <sys/dg_sys_info.h>.])
 AC_CHECK_LIB(dgc, dg_sys_info)])

AC_CHECK_HEADER(locale.h)
AC_CHECK_FUNCS(setlocale)

# We cannot check for <dwarf.h>, because Solaris 2 does not use dwarf (it
# uses stabs), but it is still SVR4.  We cannot check for <elf.h> because
# Irix 4.0.5F has the header but not the library.
if test $ac_have_func = no && test "$ac_cv_lib_elf_elf_begin" = yes \
    && test "$ac_cv_lib_kvm_kvm_open" = yes; then
  ac_have_func=yes
  AC_DEFINE(SVR4, 1, [Define to 1 on System V Release 4.])
fi

if test $ac_have_func = no; then
  AC_CHECK_HEADER(inq_stats/cpustats.h,
  [ac_have_func=yes
   AC_DEFINE(UMAX, 1, [Define to 1 for Encore UMAX.])
   AC_DEFINE(UMAX4_3, 1,
	     [Define to 1 for Encore UMAX 4.3 that has <inq_status/cpustats.h>
	      instead of <sys/cpustats.h>.])])
fi

if test $ac_have_func = no; then
  AC_CHECK_HEADER(sys/cpustats.h,
  [ac_have_func=yes; AC_DEFINE(UMAX)])
fi

if test $ac_have_func = no; then
  AC_CHECK_HEADERS(mach/mach.h)
fi

AC_CHECK_HEADERS(nlist.h,
[AC_CHECK_MEMBERS([struct nlist.n_un.n_name],
		  [AC_DEFINE(NLIST_NAME_UNION, 1,
			     [Define to 1 if your `struct nlist' has an
			      `n_un' member.  Obsolete, depend on
			      `HAVE_STRUCT_NLIST_N_UN_N_NAME])], [],
		  [@%:@include <nlist.h>])
])dnl
])# _AC_LIBOBJ_GETLOADAVG


# AC_FUNC_GETLOADAVG
# ------------------
AC_DEFUN([AC_FUNC_GETLOADAVG],
[ac_have_func=no # yes means we've found a way to get the load average.

# Make sure getloadavg.c is where it belongs, at configure-time.
test -f "$srcdir/$ac_config_libobj_dir/getloadavg.c" ||
  AC_MSG_ERROR([$srcdir/$ac_config_libobj_dir/getloadavg.c is missing])

ac_save_LIBS=$LIBS

# Check for getloadavg, but be sure not to touch the cache variable.
(AC_CHECK_FUNC(getloadavg, exit 0, exit 1)) && ac_have_func=yes

# On HPUX9, an unprivileged user can get load averages through this function.
AC_CHECK_FUNCS(pstat_getdynamic)

# Solaris has libkstat which does not require root.
AC_CHECK_LIB(kstat, kstat_open)
test $ac_cv_lib_kstat_kstat_open = yes && ac_have_func=yes

# Some systems with -lutil have (and need) -lkvm as well, some do not.
# On Solaris, -lkvm requires nlist from -lelf, so check that first
# to get the right answer into the cache.
# For kstat on solaris, we need libelf to force the definition of SVR4 below.
if test $ac_have_func = no; then
  AC_CHECK_LIB(elf, elf_begin, LIBS="-lelf $LIBS")
fi
if test $ac_have_func = no; then
  AC_CHECK_LIB(kvm, kvm_open, LIBS="-lkvm $LIBS")
  # Check for the 4.4BSD definition of getloadavg.
  AC_CHECK_LIB(util, getloadavg,
    [LIBS="-lutil $LIBS" ac_have_func=yes ac_cv_func_getloadavg_setgid=yes])
fi

if test $ac_have_func = no; then
  # There is a commonly available library for RS/6000 AIX.
  # Since it is not a standard part of AIX, it might be installed locally.
  ac_getloadavg_LIBS=$LIBS
  LIBS="-L/usr/local/lib $LIBS"
  AC_CHECK_LIB(getloadavg, getloadavg,
	       [LIBS="-lgetloadavg $LIBS"], [LIBS=$ac_getloadavg_LIBS])
fi

# Make sure it is really in the library, if we think we found it,
# otherwise set up the replacement function.
AC_CHECK_FUNCS(getloadavg, [],
	       [_AC_LIBOBJ_GETLOADAVG])

# Some definitions of getloadavg require that the program be installed setgid.
AC_CACHE_CHECK(whether getloadavg requires setgid,
	       ac_cv_func_getloadavg_setgid,
[AC_EGREP_CPP([Yowza Am I SETGID yet],
[#include "$srcdir/$ac_config_libobj_dir/getloadavg.c"
#ifdef LDAV_PRIVILEGED
Yowza Am I SETGID yet
@%:@endif],
	      ac_cv_func_getloadavg_setgid=yes,
	      ac_cv_func_getloadavg_setgid=no)])
if test $ac_cv_func_getloadavg_setgid = yes; then
  NEED_SETGID=true
  AC_DEFINE(GETLOADAVG_PRIVILEGED, 1,
	    [Define to 1 if the `getloadavg' function needs to be run setuid
	     or setgid.])
else
  NEED_SETGID=false
fi
AC_SUBST(NEED_SETGID)dnl

if test $ac_cv_func_getloadavg_setgid = yes; then
  AC_CACHE_CHECK(group of /dev/kmem, ac_cv_group_kmem,
[ # On Solaris, /dev/kmem is a symlink.  Get info on the real file.
  ac_ls_output=`ls -lgL /dev/kmem 2>/dev/null`
  # If we got an error (system does not support symlinks), try without -L.
  test -z "$ac_ls_output" && ac_ls_output=`ls -lg /dev/kmem`
  ac_cv_group_kmem=`AS_ECHO(["$ac_ls_output"]) \
    | sed -ne ['s/[	 ][	 ]*/ /g;
	       s/^.[sSrwx-]* *[0-9]* *\([^0-9]*\)  *.*/\1/;
	       / /s/.* //;p;']`
])
  AC_SUBST(KMEM_GROUP, $ac_cv_group_kmem)dnl
fi
if test "x$ac_save_LIBS" = x; then
  GETLOADAVG_LIBS=$LIBS
else
  GETLOADAVG_LIBS=`AS_ECHO(["$LIBS"]) | sed "s|$ac_save_LIBS||"`
fi
LIBS=$ac_save_LIBS

AC_SUBST(GETLOADAVG_LIBS)dnl
])# AC_FUNC_GETLOADAVG


# AU::AC_GETLOADAVG
# -----------------
AU_ALIAS([AC_GETLOADAVG], [AC_FUNC_GETLOADAVG])


# AC_FUNC_GETMNTENT
# -----------------
AN_FUNCTION([getmntent], [AC_FUNC_GETMNTENT])
AC_DEFUN([AC_FUNC_GETMNTENT],
[# getmntent is in the standard C library on UNICOS, in -lsun on Irix 4,
# -lseq on Dynix/PTX, -lgen on Unixware.
AC_SEARCH_LIBS(getmntent, [sun seq gen],
	       [ac_cv_func_getmntent=yes
		AC_DEFINE([HAVE_GETMNTENT], 1,
			  [Define to 1 if you have the `getmntent' function.])],
	       [ac_cv_func_getmntent=no])
])


# AC_FUNC_GETPGRP
# ---------------
# Figure out whether getpgrp requires zero arguments.
AC_DEFUN([AC_FUNC_GETPGRP],
[AC_CACHE_CHECK(whether getpgrp requires zero arguments,
 ac_cv_func_getpgrp_void,
[# Use it with a single arg.
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT], [getpgrp (0);])],
		  [ac_cv_func_getpgrp_void=no],
		  [ac_cv_func_getpgrp_void=yes])
])
if test $ac_cv_func_getpgrp_void = yes; then
  AC_DEFINE(GETPGRP_VOID, 1,
	    [Define to 1 if the `getpgrp' function requires zero arguments.])
fi
])# AC_FUNC_GETPGRP


# AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
# -------------------------------------
# When cross-compiling, be pessimistic so we will end up using the
# replacement version of lstat that checks for trailing slashes and
# calls lstat a second time when necessary.
AN_FUNCTION([lstat], [AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK])
AC_DEFUN([AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK],
[AC_CACHE_CHECK(
       [whether lstat correctly handles trailing slash],
       [ac_cv_func_lstat_dereferences_slashed_symlink],
[rm -f conftest.sym conftest.file
echo >conftest.file
if test "$as_ln_s" = "ln -s" && ln -s conftest.file conftest.sym; then
  AC_RUN_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT],
    [struct stat sbuf;
     /* Linux will dereference the symlink and fail, as required by POSIX.
	That is better in the sense that it means we will not
	have to compile and use the lstat wrapper.  */
     return lstat ("conftest.sym/", &sbuf) == 0;])],
		[ac_cv_func_lstat_dereferences_slashed_symlink=yes],
		[ac_cv_func_lstat_dereferences_slashed_symlink=no],
		[ac_cv_func_lstat_dereferences_slashed_symlink=no])
else
  # If the `ln -s' command failed, then we probably don't even
  # have an lstat function.
  ac_cv_func_lstat_dereferences_slashed_symlink=no
fi
rm -f conftest.sym conftest.file
])

test $ac_cv_func_lstat_dereferences_slashed_symlink = yes &&
  AC_DEFINE_UNQUOTED([LSTAT_FOLLOWS_SLASHED_SYMLINK], [1],
		     [Define to 1 if `lstat' dereferences a symlink specified
		      with a trailing slash.])

if test "x$ac_cv_func_lstat_dereferences_slashed_symlink" = xno; then
  AC_LIBOBJ([lstat])
fi
])


# _AC_FUNC_MALLOC_IF(IF-WORKS, IF-NOT)
# ------------------------------------
# If `malloc (0)' properly handled, run IF-WORKS, otherwise, IF-NOT.
AC_DEFUN([_AC_FUNC_MALLOC_IF],
[AC_REQUIRE([AC_HEADER_STDC])dnl
AC_CHECK_HEADERS(stdlib.h)
AC_CACHE_CHECK([for GNU libc compatible malloc], ac_cv_func_malloc_0_nonnull,
[AC_RUN_IFELSE(
[AC_LANG_PROGRAM(
[[#if defined STDC_HEADERS || defined HAVE_STDLIB_H
# include <stdlib.h>
#else
char *malloc ();
#endif
]],
		 [return ! malloc (0);])],
	       [ac_cv_func_malloc_0_nonnull=yes],
	       [ac_cv_func_malloc_0_nonnull=no],
	       [ac_cv_func_malloc_0_nonnull=no])])
AS_IF([test $ac_cv_func_malloc_0_nonnull = yes], [$1], [$2])
])# _AC_FUNC_MALLOC_IF


# AC_FUNC_MALLOC
# --------------
# Report whether `malloc (0)' properly handled, and replace malloc if
# needed.
AN_FUNCTION([malloc], [AC_FUNC_MALLOC])
AC_DEFUN([AC_FUNC_MALLOC],
[_AC_FUNC_MALLOC_IF(
  [AC_DEFINE([HAVE_MALLOC], 1,
	     [Define to 1 if your system has a GNU libc compatible `malloc'
	      function, and to 0 otherwise.])],
  [AC_DEFINE([HAVE_MALLOC], 0)
   AC_LIBOBJ(malloc)
   AC_DEFINE([malloc], [rpl_malloc],
      [Define to rpl_malloc if the replacement function should be used.])])
])# AC_FUNC_MALLOC


# AC_FUNC_MBRTOWC
# ---------------
AN_FUNCTION([mbrtowc], [AC_FUNC_MBRTOWC])
AC_DEFUN([AC_FUNC_MBRTOWC],
[
  AC_CACHE_CHECK([whether mbrtowc and mbstate_t are properly declared],
    ac_cv_func_mbrtowc,
    [AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
	    [[@%:@include <wchar.h>]],
	    [[wchar_t wc;
	      char const s[] = "";
	      size_t n = 1;
	      mbstate_t state;
	      return ! (sizeof state && (mbrtowc) (&wc, s, n, &state));]])],
       ac_cv_func_mbrtowc=yes,
       ac_cv_func_mbrtowc=no)])
  if test $ac_cv_func_mbrtowc = yes; then
    AC_DEFINE([HAVE_MBRTOWC], 1,
      [Define to 1 if mbrtowc and mbstate_t are properly declared.])
  fi
])


# AC_FUNC_MEMCMP
# --------------
AC_DEFUN([AC_FUNC_MEMCMP],
[AC_CACHE_CHECK([for working memcmp], ac_cv_func_memcmp_working,
[AC_RUN_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT], [[
  /* Some versions of memcmp are not 8-bit clean.  */
  char c0 = '\100', c1 = '\200', c2 = '\201';
  if (memcmp(&c0, &c2, 1) >= 0 || memcmp(&c1, &c2, 1) >= 0)
    return 1;

  /* The Next x86 OpenStep bug shows up only when comparing 16 bytes
     or more and with at least one buffer not starting on a 4-byte boundary.
     William Lewis provided this test program.   */
  {
    char foo[21];
    char bar[21];
    int i;
    for (i = 0; i < 4; i++)
      {
	char *a = foo + i;
	char *b = bar + i;
	strcpy (a, "--------01111111");
	strcpy (b, "--------10000000");
	if (memcmp (a, b, 16) >= 0)
	  return 1;
      }
    return 0;
  }
]])],
	       [ac_cv_func_memcmp_working=yes],
	       [ac_cv_func_memcmp_working=no],
	       [ac_cv_func_memcmp_working=no])])
test $ac_cv_func_memcmp_working = no && AC_LIBOBJ([memcmp])
])# AC_FUNC_MEMCMP


# AC_FUNC_MKTIME
# --------------
AN_FUNCTION([mktime], [AC_FUNC_MKTIME])
AC_DEFUN([AC_FUNC_MKTIME],
[AC_REQUIRE([AC_HEADER_TIME])dnl
AC_CHECK_HEADERS_ONCE(sys/time.h unistd.h)
AC_CHECK_FUNCS_ONCE(alarm)
AC_CACHE_CHECK([for working mktime], ac_cv_func_working_mktime,
[AC_RUN_IFELSE([AC_LANG_SOURCE(
[[/* Test program from Paul Eggert and Tony Leneis.  */
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <limits.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifndef HAVE_ALARM
# define alarm(X) /* empty */
#endif

/* Work around redefinition to rpl_putenv by other config tests.  */
#undef putenv

static time_t time_t_max;
static time_t time_t_min;

/* Values we'll use to set the TZ environment variable.  */
static const char *tz_strings[] = {
  (const char *) 0, "TZ=GMT0", "TZ=JST-9",
  "TZ=EST+3EDT+2,M10.1.0/00:00:00,M2.3.0/00:00:00"
};
#define N_STRINGS (sizeof (tz_strings) / sizeof (tz_strings[0]))

/* Return 0 if mktime fails to convert a date in the spring-forward gap.
   Based on a problem report from Andreas Jaeger.  */
static int
spring_forward_gap ()
{
  /* glibc (up to about 1998-10-07) failed this test. */
  struct tm tm;

  /* Use the portable POSIX.1 specification "TZ=PST8PDT,M4.1.0,M10.5.0"
     instead of "TZ=America/Vancouver" in order to detect the bug even
     on systems that don't support the Olson extension, or don't have the
     full zoneinfo tables installed.  */
  putenv ((char*) "TZ=PST8PDT,M4.1.0,M10.5.0");

  tm.tm_year = 98;
  tm.tm_mon = 3;
  tm.tm_mday = 5;
  tm.tm_hour = 2;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  tm.tm_isdst = -1;
  return mktime (&tm) != (time_t) -1;
}

static int
mktime_test1 (time_t now)
{
  struct tm *lt;
  return ! (lt = localtime (&now)) || mktime (lt) == now;
}

static int
mktime_test (time_t now)
{
  return (mktime_test1 (now)
	  && mktime_test1 ((time_t) (time_t_max - now))
	  && mktime_test1 ((time_t) (time_t_min + now)));
}

static int
irix_6_4_bug ()
{
  /* Based on code from Ariel Faigon.  */
  struct tm tm;
  tm.tm_year = 96;
  tm.tm_mon = 3;
  tm.tm_mday = 0;
  tm.tm_hour = 0;
  tm.tm_min = 0;
  tm.tm_sec = 0;
  tm.tm_isdst = -1;
  mktime (&tm);
  return tm.tm_mon == 2 && tm.tm_mday == 31;
}

static int
bigtime_test (int j)
{
  struct tm tm;
  time_t now;
  tm.tm_year = tm.tm_mon = tm.tm_mday = tm.tm_hour = tm.tm_min = tm.tm_sec = j;
  now = mktime (&tm);
  if (now != (time_t) -1)
    {
      struct tm *lt = localtime (&now);
      if (! (lt
	     && lt->tm_year == tm.tm_year
	     && lt->tm_mon == tm.tm_mon
	     && lt->tm_mday == tm.tm_mday
	     && lt->tm_hour == tm.tm_hour
	     && lt->tm_min == tm.tm_min
	     && lt->tm_sec == tm.tm_sec
	     && lt->tm_yday == tm.tm_yday
	     && lt->tm_wday == tm.tm_wday
	     && ((lt->tm_isdst < 0 ? -1 : 0 < lt->tm_isdst)
		  == (tm.tm_isdst < 0 ? -1 : 0 < tm.tm_isdst))))
	return 0;
    }
  return 1;
}

static int
year_2050_test ()
{
  /* The correct answer for 2050-02-01 00:00:00 in Pacific time,
     ignoring leap seconds.  */
  unsigned long int answer = 2527315200UL;

  struct tm tm;
  time_t t;
  tm.tm_year = 2050 - 1900;
  tm.tm_mon = 2 - 1;
  tm.tm_mday = 1;
  tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
  tm.tm_isdst = -1;

  /* Use the portable POSIX.1 specification "TZ=PST8PDT,M4.1.0,M10.5.0"
     instead of "TZ=America/Vancouver" in order to detect the bug even
     on systems that don't support the Olson extension, or don't have the
     full zoneinfo tables installed.  */
  putenv ((char*) "TZ=PST8PDT,M4.1.0,M10.5.0");

  t = mktime (&tm);

  /* Check that the result is either a failure, or close enough
     to the correct answer that we can assume the discrepancy is
     due to leap seconds.  */
  return (t == (time_t) -1
	  || (0 < t && answer - 120 <= t && t <= answer + 120));
}

int
main ()
{
  time_t t, delta;
  int i, j;

  /* This test makes some buggy mktime implementations loop.
     Give up after 60 seconds; a mktime slower than that
     isn't worth using anyway.  */
  alarm (60);

  for (;;)
    {
      t = (time_t_max << 1) + 1;
      if (t <= time_t_max)
	break;
      time_t_max = t;
    }
  time_t_min = - ((time_t) ~ (time_t) 0 == (time_t) -1) - time_t_max;

  delta = time_t_max / 997; /* a suitable prime number */
  for (i = 0; i < N_STRINGS; i++)
    {
      if (tz_strings[i])
	putenv ((char*) tz_strings[i]);

      for (t = 0; t <= time_t_max - delta; t += delta)
	if (! mktime_test (t))
	  return 1;
      if (! (mktime_test ((time_t) 1)
	     && mktime_test ((time_t) (60 * 60))
	     && mktime_test ((time_t) (60 * 60 * 24))))
	return 1;

      for (j = 1; ; j <<= 1)
	if (! bigtime_test (j))
	  return 1;
	else if (INT_MAX / 2 < j)
	  break;
      if (! bigtime_test (INT_MAX))
	return 1;
    }
  return ! (irix_6_4_bug () && spring_forward_gap () && year_2050_test ());
}]])],
	       [ac_cv_func_working_mktime=yes],
	       [ac_cv_func_working_mktime=no],
	       [ac_cv_func_working_mktime=no])])
if test $ac_cv_func_working_mktime = no; then
  AC_LIBOBJ([mktime])
fi
])# AC_FUNC_MKTIME


# AU::AM_FUNC_MKTIME
# ------------------
AU_ALIAS([AM_FUNC_MKTIME], [AC_FUNC_MKTIME])


# AC_FUNC_MMAP
# ------------
AN_FUNCTION([mmap], [AC_FUNC_MMAP])
AC_DEFUN([AC_FUNC_MMAP],
[AC_CHECK_HEADERS_ONCE([stdlib.h unistd.h sys/param.h])
AC_CHECK_FUNCS([getpagesize])
AC_CACHE_CHECK([for working mmap], [ac_cv_func_mmap_fixed_mapped],
[AC_RUN_IFELSE([AC_LANG_SOURCE([AC_INCLUDES_DEFAULT]
[[/* malloc might have been renamed as rpl_malloc. */
#undef malloc

/* Thanks to Mike Haertel and Jim Avera for this test.
   Here is a matrix of mmap possibilities:
	mmap private not fixed
	mmap private fixed at somewhere currently unmapped
	mmap private fixed at somewhere already mapped
	mmap shared not fixed
	mmap shared fixed at somewhere currently unmapped
	mmap shared fixed at somewhere already mapped
   For private mappings, we should verify that changes cannot be read()
   back from the file, nor mmap's back from the file at a different
   address.  (There have been systems where private was not correctly
   implemented like the infamous i386 svr4.0, and systems where the
   VM page cache was not coherent with the file system buffer cache
   like early versions of FreeBSD and possibly contemporary NetBSD.)
   For shared mappings, we should conversely verify that changes get
   propagated back to all the places they're supposed to be.

   Grep wants private fixed already mapped.
   The main things grep needs to know about mmap are:
   * does it exist and is it safe to write into the mmap'd area
   * how to use it (BSD variants)  */

#include <fcntl.h>
#include <sys/mman.h>

#if !defined STDC_HEADERS && !defined HAVE_STDLIB_H
char *malloc ();
#endif

/* This mess was copied from the GNU getpagesize.h.  */
#ifndef HAVE_GETPAGESIZE
# ifdef _SC_PAGESIZE
#  define getpagesize() sysconf(_SC_PAGESIZE)
# else /* no _SC_PAGESIZE */
#  ifdef HAVE_SYS_PARAM_H
#   include <sys/param.h>
#   ifdef EXEC_PAGESIZE
#    define getpagesize() EXEC_PAGESIZE
#   else /* no EXEC_PAGESIZE */
#    ifdef NBPG
#     define getpagesize() NBPG * CLSIZE
#     ifndef CLSIZE
#      define CLSIZE 1
#     endif /* no CLSIZE */
#    else /* no NBPG */
#     ifdef NBPC
#      define getpagesize() NBPC
#     else /* no NBPC */
#      ifdef PAGESIZE
#       define getpagesize() PAGESIZE
#      endif /* PAGESIZE */
#     endif /* no NBPC */
#    endif /* no NBPG */
#   endif /* no EXEC_PAGESIZE */
#  else /* no HAVE_SYS_PARAM_H */
#   define getpagesize() 8192	/* punt totally */
#  endif /* no HAVE_SYS_PARAM_H */
# endif /* no _SC_PAGESIZE */

#endif /* no HAVE_GETPAGESIZE */

int
main ()
{
  char *data, *data2, *data3;
  const char *cdata2;
  int i, pagesize;
  int fd, fd2;

  pagesize = getpagesize ();

  /* First, make a file with some known garbage in it. */
  data = (char *) malloc (pagesize);
  if (!data)
    return 1;
  for (i = 0; i < pagesize; ++i)
    *(data + i) = rand ();
  umask (0);
  fd = creat ("conftest.mmap", 0600);
  if (fd < 0)
    return 2;
  if (write (fd, data, pagesize) != pagesize)
    return 3;
  close (fd);

  /* Next, check that the tail of a page is zero-filled.  File must have
     non-zero length, otherwise we risk SIGBUS for entire page.  */
  fd2 = open ("conftest.txt", O_RDWR | O_CREAT | O_TRUNC, 0600);
  if (fd2 < 0)
    return 4;
  cdata2 = "";
  if (write (fd2, cdata2, 1) != 1)
    return 5;
  data2 = (char *) mmap (0, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0L);
  if (data2 == MAP_FAILED)
    return 6;
  for (i = 0; i < pagesize; ++i)
    if (*(data2 + i))
      return 7;
  close (fd2);
  if (munmap (data2, pagesize))
    return 8;

  /* Next, try to mmap the file at a fixed address which already has
     something else allocated at it.  If we can, also make sure that
     we see the same garbage.  */
  fd = open ("conftest.mmap", O_RDWR);
  if (fd < 0)
    return 9;
  if (data2 != mmap (data2, pagesize, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_FIXED, fd, 0L))
    return 10;
  for (i = 0; i < pagesize; ++i)
    if (*(data + i) != *(data2 + i))
      return 11;

  /* Finally, make sure that changes to the mapped area do not
     percolate back to the file as seen by read().  (This is a bug on
     some variants of i386 svr4.0.)  */
  for (i = 0; i < pagesize; ++i)
    *(data2 + i) = *(data2 + i) + 1;
  data3 = (char *) malloc (pagesize);
  if (!data3)
    return 12;
  if (read (fd, data3, pagesize) != pagesize)
    return 13;
  for (i = 0; i < pagesize; ++i)
    if (*(data + i) != *(data3 + i))
      return 14;
  close (fd);
  return 0;
}]])],
	       [ac_cv_func_mmap_fixed_mapped=yes],
	       [ac_cv_func_mmap_fixed_mapped=no],
	       [ac_cv_func_mmap_fixed_mapped=no])])
if test $ac_cv_func_mmap_fixed_mapped = yes; then
  AC_DEFINE([HAVE_MMAP], [1],
	    [Define to 1 if you have a working `mmap' system call.])
fi
rm -f conftest.mmap conftest.txt
])# AC_FUNC_MMAP


# AU::AC_MMAP
# -----------
AU_ALIAS([AC_MMAP], [AC_FUNC_MMAP])


# AC_FUNC_OBSTACK
# ---------------
# Ensure obstack support.  Yeah, this is not exactly a `FUNC' check.
AN_FUNCTION([obstack_init], [AC_FUNC_OBSTACK])
AN_IDENTIFIER([obstack],    [AC_FUNC_OBSTACK])
AC_DEFUN([AC_FUNC_OBSTACK],
[AC_LIBSOURCES([obstack.h, obstack.c])dnl
AC_CACHE_CHECK([for obstacks], ac_cv_func_obstack,
[AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT
		      [@%:@include "obstack.h"]],
		     [[struct obstack mem;
		       @%:@define obstack_chunk_alloc malloc
		       @%:@define obstack_chunk_free free
		       obstack_init (&mem);
		       obstack_free (&mem, 0);]])],
		[ac_cv_func_obstack=yes],
		[ac_cv_func_obstack=no])])
if test $ac_cv_func_obstack = yes; then
  AC_DEFINE(HAVE_OBSTACK, 1, [Define to 1 if libc includes obstacks.])
else
  AC_LIBOBJ(obstack)
fi
])# AC_FUNC_OBSTACK


# AU::AM_FUNC_OBSTACK
# -------------------
AU_ALIAS([AM_FUNC_OBSTACK], [AC_FUNC_OBSTACK])



# _AC_FUNC_REALLOC_IF(IF-WORKS, IF-NOT)
# -------------------------------------
# If `realloc (0, 0)' is properly handled, run IF-WORKS, otherwise, IF-NOT.
AC_DEFUN([_AC_FUNC_REALLOC_IF],
[AC_REQUIRE([AC_HEADER_STDC])dnl
AC_CHECK_HEADERS(stdlib.h)
AC_CACHE_CHECK([for GNU libc compatible realloc], ac_cv_func_realloc_0_nonnull,
[AC_RUN_IFELSE(
[AC_LANG_PROGRAM(
[[#if defined STDC_HEADERS || defined HAVE_STDLIB_H
# include <stdlib.h>
#else
char *realloc ();
#endif
]],
		 [return ! realloc (0, 0);])],
	       [ac_cv_func_realloc_0_nonnull=yes],
	       [ac_cv_func_realloc_0_nonnull=no],
	       [ac_cv_func_realloc_0_nonnull=no])])
AS_IF([test $ac_cv_func_realloc_0_nonnull = yes], [$1], [$2])
])# AC_FUNC_REALLOC


# AC_FUNC_REALLOC
# ---------------
# Report whether `realloc (0, 0)' is properly handled, and replace realloc if
# needed.
AN_FUNCTION([realloc], [AC_FUNC_REALLOC])
AC_DEFUN([AC_FUNC_REALLOC],
[_AC_FUNC_REALLOC_IF(
  [AC_DEFINE([HAVE_REALLOC], 1,
	     [Define to 1 if your system has a GNU libc compatible `realloc'
	      function, and to 0 otherwise.])],
  [AC_DEFINE([HAVE_REALLOC], 0)
   AC_LIBOBJ([realloc])
   AC_DEFINE([realloc], [rpl_realloc],
      [Define to rpl_realloc if the replacement function should be used.])])
])# AC_FUNC_REALLOC


# AC_FUNC_SELECT_ARGTYPES
# -----------------------
# Determine the correct type to be passed to each of the `select'
# function's arguments, and define those types in `SELECT_TYPE_ARG1',
# `SELECT_TYPE_ARG234', and `SELECT_TYPE_ARG5'.
AC_DEFUN([AC_FUNC_SELECT_ARGTYPES],
[AC_CHECK_HEADERS(sys/select.h sys/socket.h)
AC_CACHE_CHECK([types of arguments for select],
[ac_cv_func_select_args],
[for ac_arg234 in 'fd_set *' 'int *' 'void *'; do
 for ac_arg1 in 'int' 'size_t' 'unsigned long int' 'unsigned int'; do
  for ac_arg5 in 'struct timeval *' 'const struct timeval *'; do
   AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
[AC_INCLUDES_DEFAULT
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
],
			[extern int select ($ac_arg1,
					    $ac_arg234, $ac_arg234, $ac_arg234,
					    $ac_arg5);])],
	      [ac_cv_func_select_args="$ac_arg1,$ac_arg234,$ac_arg5"; break 3])
  done
 done
done
# Provide a safe default value.
: "${ac_cv_func_select_args=int,int *,struct timeval *}"
])
ac_save_IFS=$IFS; IFS=','
set dummy `echo "$ac_cv_func_select_args" | sed 's/\*/\*/g'`
IFS=$ac_save_IFS
shift
AC_DEFINE_UNQUOTED(SELECT_TYPE_ARG1, $[1],
		   [Define to the type of arg 1 for `select'.])
AC_DEFINE_UNQUOTED(SELECT_TYPE_ARG234, ($[2]),
		   [Define to the type of args 2, 3 and 4 for `select'.])
AC_DEFINE_UNQUOTED(SELECT_TYPE_ARG5, ($[3]),
		   [Define to the type of arg 5 for `select'.])
rm -f conftest*
])# AC_FUNC_SELECT_ARGTYPES


# AC_FUNC_SETPGRP
# ---------------
AC_DEFUN([AC_FUNC_SETPGRP],
[AC_CACHE_CHECK(whether setpgrp takes no argument, ac_cv_func_setpgrp_void,
[AC_RUN_IFELSE(
[AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT],
[/* If this system has a BSD-style setpgrp which takes arguments,
  setpgrp(1, 1) will fail with ESRCH and return -1, in that case
  exit successfully. */
  return setpgrp (1,1) != -1;])],
	       [ac_cv_func_setpgrp_void=no],
	       [ac_cv_func_setpgrp_void=yes],
	       [AC_MSG_ERROR([cannot check setpgrp when cross compiling])])])
if test $ac_cv_func_setpgrp_void = yes; then
  AC_DEFINE(SETPGRP_VOID, 1,
	    [Define to 1 if the `setpgrp' function takes no argument.])
fi
])# AC_FUNC_SETPGRP


# _AC_FUNC_STAT(STAT | LSTAT)
# ---------------------------
# Determine whether stat or lstat have the bug that it succeeds when
# given the zero-length file name argument.  The stat and lstat from
# SunOS4.1.4 and the Hurd (as of 1998-11-01) do this.
#
# If it does, then define HAVE_STAT_EMPTY_STRING_BUG (or
# HAVE_LSTAT_EMPTY_STRING_BUG) and arrange to compile the wrapper
# function.
m4_define([_AC_FUNC_STAT],
[AC_REQUIRE([AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK])dnl
AC_CACHE_CHECK([whether $1 accepts an empty string],
	       [ac_cv_func_$1_empty_string_bug],
[AC_RUN_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT],
[[struct stat sbuf;
  return $1 ("", &sbuf) == 0;]])],
	    [ac_cv_func_$1_empty_string_bug=no],
	    [ac_cv_func_$1_empty_string_bug=yes],
	    [ac_cv_func_$1_empty_string_bug=yes])])
if test $ac_cv_func_$1_empty_string_bug = yes; then
  AC_LIBOBJ([$1])
  AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_$1_EMPTY_STRING_BUG]), 1,
		     [Define to 1 if `$1' has the bug that it succeeds when
		      given the zero-length file name argument.])
fi
])# _AC_FUNC_STAT


# AC_FUNC_STAT & AC_FUNC_LSTAT
# ----------------------------
AC_DEFUN([AC_FUNC_STAT],  [_AC_FUNC_STAT(stat)])
AC_DEFUN([AC_FUNC_LSTAT], [_AC_FUNC_STAT(lstat)])


# _AC_LIBOBJ_STRTOD
# -----------------
m4_define([_AC_LIBOBJ_STRTOD],
[AC_LIBOBJ(strtod)
AC_CHECK_FUNC(pow)
if test $ac_cv_func_pow = no; then
  AC_CHECK_LIB(m, pow,
	       [POW_LIB=-lm],
	       [AC_MSG_WARN([cannot find library containing definition of pow])])
fi
])# _AC_LIBOBJ_STRTOD


# AC_FUNC_STRTOD
# --------------
AN_FUNCTION([strtod], [AC_FUNC_STRTOD])
AC_DEFUN([AC_FUNC_STRTOD],
[AC_SUBST(POW_LIB)dnl
AC_CACHE_CHECK(for working strtod, ac_cv_func_strtod,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
]AC_INCLUDES_DEFAULT[
#ifndef strtod
double strtod ();
#endif
int
main()
{
  {
    /* Some versions of Linux strtod mis-parse strings with leading '+'.  */
    char *string = " +69";
    char *term;
    double value;
    value = strtod (string, &term);
    if (value != 69 || term != (string + 4))
      return 1;
  }

  {
    /* Under Solaris 2.4, strtod returns the wrong value for the
       terminating character under some conditions.  */
    char *string = "NaN";
    char *term;
    strtod (string, &term);
    if (term != string && *(term - 1) == 0)
      return 1;
  }
  return 0;
}
]])],
	       ac_cv_func_strtod=yes,
	       ac_cv_func_strtod=no,
	       ac_cv_func_strtod=no)])
if test $ac_cv_func_strtod = no; then
  _AC_LIBOBJ_STRTOD
fi
])


# AC_FUNC_STRTOLD
# ---------------
AC_DEFUN([AC_FUNC_STRTOLD],
[
  AC_CACHE_CHECK([whether strtold conforms to C99],
    [ac_cv_func_strtold],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
	  [[/* On HP-UX before 11.23, strtold returns a struct instead of
		long double.  Reject implementations like that, by requiring
		compatibility with the C99 prototype.  */
#	     include <stdlib.h>
	     static long double (*p) (char const *, char **) = strtold;
	     static long double
	     test (char const *nptr, char **endptr)
	     {
	       long double r;
	       r = strtold (nptr, endptr);
	       return r;
	     }]],
	   [[return test ("1.0", NULL) != 1 || p ("1.0", NULL) != 1;]])],
       [ac_cv_func_strtold=yes],
       [ac_cv_func_strtold=no])])
  if test $ac_cv_func_strtold = yes; then
    AC_DEFINE([HAVE_STRTOLD], 1,
      [Define to 1 if strtold exists and conforms to C99.])
  fi
])# AC_FUNC_STRTOLD


# AU::AM_FUNC_STRTOD
# ------------------
AU_ALIAS([AM_FUNC_STRTOD], [AC_FUNC_STRTOD])


# AC_FUNC_STRERROR_R
# ------------------
AN_FUNCTION([strerror_r], [AC_FUNC_STRERROR_R])
AC_DEFUN([AC_FUNC_STRERROR_R],
[AC_CHECK_DECLS([strerror_r])
AC_CHECK_FUNCS([strerror_r])
AC_CACHE_CHECK([whether strerror_r returns char *],
	       ac_cv_func_strerror_r_char_p,
   [
    ac_cv_func_strerror_r_char_p=no
    if test $ac_cv_have_decl_strerror_r = yes; then
      AC_COMPILE_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT],
	[[
	  char buf[100];
	  char x = *strerror_r (0, buf, sizeof buf);
	  char *p = strerror_r (0, buf, sizeof buf);
	  return !p || x;
	]])],
			ac_cv_func_strerror_r_char_p=yes)
    else
      # strerror_r is not declared.  Choose between
      # systems that have relatively inaccessible declarations for the
      # function.  BeOS and DEC UNIX 4.0 fall in this category, but the
      # former has a strerror_r that returns char*, while the latter
      # has a strerror_r that returns `int'.
      # This test should segfault on the DEC system.
      AC_RUN_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT
	extern char *strerror_r ();],
	[[char buf[100];
	  char x = *strerror_r (0, buf, sizeof buf);
	  return ! isalpha (x);]])],
		    ac_cv_func_strerror_r_char_p=yes, , :)
    fi
  ])
if test $ac_cv_func_strerror_r_char_p = yes; then
  AC_DEFINE([STRERROR_R_CHAR_P], 1,
	    [Define to 1 if strerror_r returns char *.])
fi
])# AC_FUNC_STRERROR_R


# AC_FUNC_STRFTIME
# ----------------
AC_DEFUN([AC_FUNC_STRFTIME],
[AC_CHECK_FUNCS(strftime, [],
[# strftime is in -lintl on SCO UNIX.
AC_CHECK_LIB(intl, strftime,
	     [AC_DEFINE(HAVE_STRFTIME)
LIBS="-lintl $LIBS"])])dnl
])# AC_FUNC_STRFTIME


# AC_FUNC_STRNLEN
# ---------------
AN_FUNCTION([strnlen], [AC_FUNC_STRNLEN])
AC_DEFUN([AC_FUNC_STRNLEN],
[AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])dnl
AC_REQUIRE([AC_CANONICAL_HOST]) dnl for cross-compiles
AC_CACHE_CHECK([for working strnlen], ac_cv_func_strnlen_working,
[AC_RUN_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT], [[
#define S "foobar"
#define S_LEN (sizeof S - 1)

  /* At least one implementation is buggy: that of AIX 4.3 would
     give strnlen (S, 1) == 3.  */

  int i;
  for (i = 0; i < S_LEN + 1; ++i)
    {
      int expected = i <= S_LEN ? i : S_LEN;
      if (strnlen (S, i) != expected)
	return 1;
    }
  return 0;
]])],
	       [ac_cv_func_strnlen_working=yes],
	       [ac_cv_func_strnlen_working=no],
	       [# Guess no on AIX systems, yes otherwise.
		case "$host_os" in
		  aix*) ac_cv_func_strnlen_working=no;;
		  *)    ac_cv_func_strnlen_working=yes;;
		esac])])
test $ac_cv_func_strnlen_working = no && AC_LIBOBJ([strnlen])
])# AC_FUNC_STRNLEN


# AC_FUNC_SETVBUF_REVERSED
# ------------------------
AC_DEFUN([AC_FUNC_SETVBUF_REVERSED],
[AC_DIAGNOSE([obsolete],
[The macro `$0' is obsolete.  Remove it and all references to SETVBUF_REVERSED.])dnl
AC_CACHE_VAL([ac_cv_func_setvbuf_reversed], [ac_cv_func_setvbuf_reversed=no])
])# AC_FUNC_SETVBUF_REVERSED


# AU::AC_SETVBUF_REVERSED
# -----------------------
AU_ALIAS([AC_SETVBUF_REVERSED], [AC_FUNC_SETVBUF_REVERSED])


# AC_FUNC_STRCOLL
# ---------------
AN_FUNCTION([strcoll], [AC_FUNC_STRCOLL])
AC_DEFUN([AC_FUNC_STRCOLL],
[AC_CACHE_CHECK(for working strcoll, ac_cv_func_strcoll_works,
[AC_RUN_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT],
  [[return (strcoll ("abc", "def") >= 0 ||
	 strcoll ("ABC", "DEF") >= 0 ||
	 strcoll ("123", "456") >= 0)]])],
	       ac_cv_func_strcoll_works=yes,
	       ac_cv_func_strcoll_works=no,
	       ac_cv_func_strcoll_works=no)])
if test $ac_cv_func_strcoll_works = yes; then
  AC_DEFINE(HAVE_STRCOLL, 1,
	    [Define to 1 if you have the `strcoll' function and it is properly
	     defined.])
fi
])# AC_FUNC_STRCOLL


# AU::AC_STRCOLL
# --------------
AU_ALIAS([AC_STRCOLL], [AC_FUNC_STRCOLL])


# AC_FUNC_UTIME_NULL
# ------------------
AC_DEFUN([AC_FUNC_UTIME_NULL],
[AC_CHECK_HEADERS_ONCE(utime.h)
AC_CACHE_CHECK(whether utime accepts a null argument, ac_cv_func_utime_null,
[rm -f conftest.data; >conftest.data
# Sequent interprets utime(file, 0) to mean use start of epoch.  Wrong.
AC_RUN_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT
	       #ifdef HAVE_UTIME_H
	       # include <utime.h>
	       #endif],
[[struct stat s, t;
  return ! (stat ("conftest.data", &s) == 0
	    && utime ("conftest.data", 0) == 0
	    && stat ("conftest.data", &t) == 0
	    && t.st_mtime >= s.st_mtime
	    && t.st_mtime - s.st_mtime < 120);]])],
	      ac_cv_func_utime_null=yes,
	      ac_cv_func_utime_null=no,
	      ac_cv_func_utime_null='guessing yes')])
if test "x$ac_cv_func_utime_null" != xno; then
  ac_cv_func_utime_null=yes
  AC_DEFINE(HAVE_UTIME_NULL, 1,
	    [Define to 1 if `utime(file, NULL)' sets file's timestamp to the
	     present.])
fi
rm -f conftest.data
])# AC_FUNC_UTIME_NULL


# AU::AC_UTIME_NULL
# -----------------
AU_ALIAS([AC_UTIME_NULL], [AC_FUNC_UTIME_NULL])


# AC_FUNC_FORK
# ------------
AN_FUNCTION([fork],  [AC_FUNC_FORK])
AN_FUNCTION([vfork], [AC_FUNC_FORK])
AC_DEFUN([AC_FUNC_FORK],
[AC_REQUIRE([AC_TYPE_PID_T])dnl
AC_CHECK_HEADERS(vfork.h)
AC_CHECK_FUNCS(fork vfork)
if test "x$ac_cv_func_fork" = xyes; then
  _AC_FUNC_FORK
else
  ac_cv_func_fork_works=$ac_cv_func_fork
fi
if test "x$ac_cv_func_fork_works" = xcross; then
  case $host in
    *-*-amigaos* | *-*-msdosdjgpp*)
      # Override, as these systems have only a dummy fork() stub
      ac_cv_func_fork_works=no
      ;;
    *)
      ac_cv_func_fork_works=yes
      ;;
  esac
  AC_MSG_WARN([result $ac_cv_func_fork_works guessed because of cross compilation])
fi
ac_cv_func_vfork_works=$ac_cv_func_vfork
if test "x$ac_cv_func_vfork" = xyes; then
  _AC_FUNC_VFORK
fi;
if test "x$ac_cv_func_fork_works" = xcross; then
  ac_cv_func_vfork_works=$ac_cv_func_vfork
  AC_MSG_WARN([result $ac_cv_func_vfork_works guessed because of cross compilation])
fi

if test "x$ac_cv_func_vfork_works" = xyes; then
  AC_DEFINE(HAVE_WORKING_VFORK, 1, [Define to 1 if `vfork' works.])
else
  AC_DEFINE(vfork, fork, [Define as `fork' if `vfork' does not work.])
fi
if test "x$ac_cv_func_fork_works" = xyes; then
  AC_DEFINE(HAVE_WORKING_FORK, 1, [Define to 1 if `fork' works.])
fi
])# AC_FUNC_FORK


# _AC_FUNC_FORK
# -------------
AC_DEFUN([_AC_FUNC_FORK],
  [AC_CACHE_CHECK(for working fork, ac_cv_func_fork_works,
    [AC_RUN_IFELSE(
      [AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT],
	[
	  /* By Ruediger Kuhlmann. */
	  return fork () < 0;
	])],
      [ac_cv_func_fork_works=yes],
      [ac_cv_func_fork_works=no],
      [ac_cv_func_fork_works=cross])])]
)# _AC_FUNC_FORK


# _AC_FUNC_VFORK
# --------------
AC_DEFUN([_AC_FUNC_VFORK],
[AC_CACHE_CHECK(for working vfork, ac_cv_func_vfork_works,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[/* Thanks to Paul Eggert for this test.  */
]AC_INCLUDES_DEFAULT[
#include <sys/wait.h>
#ifdef HAVE_VFORK_H
# include <vfork.h>
#endif
/* On some sparc systems, changes by the child to local and incoming
   argument registers are propagated back to the parent.  The compiler
   is told about this with #include <vfork.h>, but some compilers
   (e.g. gcc -O) don't grok <vfork.h>.  Test for this by using a
   static variable whose address is put into a register that is
   clobbered by the vfork.  */
static void
#ifdef __cplusplus
sparc_address_test (int arg)
# else
sparc_address_test (arg) int arg;
#endif
{
  static pid_t child;
  if (!child) {
    child = vfork ();
    if (child < 0) {
      perror ("vfork");
      _exit(2);
    }
    if (!child) {
      arg = getpid();
      write(-1, "", 0);
      _exit (arg);
    }
  }
}

int
main ()
{
  pid_t parent = getpid ();
  pid_t child;

  sparc_address_test (0);

  child = vfork ();

  if (child == 0) {
    /* Here is another test for sparc vfork register problems.  This
       test uses lots of local variables, at least as many local
       variables as main has allocated so far including compiler
       temporaries.  4 locals are enough for gcc 1.40.3 on a Solaris
       4.1.3 sparc, but we use 8 to be safe.  A buggy compiler should
       reuse the register of parent for one of the local variables,
       since it will think that parent can't possibly be used any more
       in this routine.  Assigning to the local variable will thus
       munge parent in the parent process.  */
    pid_t
      p = getpid(), p1 = getpid(), p2 = getpid(), p3 = getpid(),
      p4 = getpid(), p5 = getpid(), p6 = getpid(), p7 = getpid();
    /* Convince the compiler that p..p7 are live; otherwise, it might
       use the same hardware register for all 8 local variables.  */
    if (p != p1 || p != p2 || p != p3 || p != p4
	|| p != p5 || p != p6 || p != p7)
      _exit(1);

    /* On some systems (e.g. IRIX 3.3), vfork doesn't separate parent
       from child file descriptors.  If the child closes a descriptor
       before it execs or exits, this munges the parent's descriptor
       as well.  Test for this by closing stdout in the child.  */
    _exit(close(fileno(stdout)) != 0);
  } else {
    int status;
    struct stat st;

    while (wait(&status) != child)
      ;
    return (
	 /* Was there some problem with vforking?  */
	 child < 0

	 /* Did the child fail?  (This shouldn't happen.)  */
	 || status

	 /* Did the vfork/compiler bug occur?  */
	 || parent != getpid()

	 /* Did the file descriptor bug occur?  */
	 || fstat(fileno(stdout), &st) != 0
	 );
  }
}]])],
	    [ac_cv_func_vfork_works=yes],
	    [ac_cv_func_vfork_works=no],
	    [ac_cv_func_vfork_works=cross])])
])# _AC_FUNC_VFORK


# AU::AC_FUNC_VFORK
# -----------------
AU_ALIAS([AC_FUNC_VFORK], [AC_FUNC_FORK])

# AU::AC_VFORK
# ------------
AU_ALIAS([AC_VFORK], [AC_FUNC_FORK])


# AC_FUNC_VPRINTF
# ---------------
# Why the heck is that _doprnt does not define HAVE__DOPRNT???
# That the logical name!
AC_DEFUN([AC_FUNC_VPRINTF],
[AC_CHECK_FUNCS(vprintf, []
[AC_CHECK_FUNC(_doprnt,
	       [AC_DEFINE(HAVE_DOPRNT, 1,
			  [Define to 1 if you don't have `vprintf' but do have
			  `_doprnt.'])])])
])


# AU::AC_VPRINTF
# --------------
AU_ALIAS([AC_VPRINTF], [AC_FUNC_VPRINTF])


# AC_FUNC_WAIT3
# -------------
# Don't bother too hard maintaining this macro, as it's obsoleted.
# We don't AU define it, since we don't have any alternative to propose,
# any invocation should be removed, and the code adjusted.
AN_FUNCTION([wait3], [AC_FUNC_WAIT3])
AC_DEFUN([AC_FUNC_WAIT3],
[AC_DIAGNOSE([obsolete],
[$0: `wait3' has been removed from POSIX.
Remove this `AC_FUNC_WAIT3' and adjust your code to use `waitpid' instead.])dnl
AC_CACHE_CHECK([for wait3 that fills in rusage],
	       [ac_cv_func_wait3_rusage],
[AC_RUN_IFELSE([AC_LANG_SOURCE(
[AC_INCLUDES_DEFAULT[
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
/* HP-UX has wait3 but does not fill in rusage at all.  */
int
main ()
{
  struct rusage r;
  int i;
  /* Use a field that we can force nonzero --
     voluntary context switches.
     For systems like NeXT and OSF/1 that don't set it,
     also use the system CPU time.  And page faults (I/O) for Linux.  */
  r.ru_nvcsw = 0;
  r.ru_stime.tv_sec = 0;
  r.ru_stime.tv_usec = 0;
  r.ru_majflt = r.ru_minflt = 0;
  switch (fork ())
    {
    case 0: /* Child.  */
      sleep(1); /* Give up the CPU.  */
      _exit(0);
      break;
    case -1: /* What can we do?  */
      _exit(0);
      break;
    default: /* Parent.  */
      wait3(&i, 0, &r);
      /* Avoid "text file busy" from rm on fast HP-UX machines.  */
      sleep(2);
      return (r.ru_nvcsw == 0 && r.ru_majflt == 0 && r.ru_minflt == 0
	      && r.ru_stime.tv_sec == 0 && r.ru_stime.tv_usec == 0);
    }
}]])],
	       [ac_cv_func_wait3_rusage=yes],
	       [ac_cv_func_wait3_rusage=no],
	       [ac_cv_func_wait3_rusage=no])])
if test $ac_cv_func_wait3_rusage = yes; then
  AC_DEFINE(HAVE_WAIT3, 1,
	    [Define to 1 if you have the `wait3' system call.
	     Deprecated, you should no longer depend upon `wait3'.])
fi
])# AC_FUNC_WAIT3


# AU::AC_WAIT3
# ------------
AU_ALIAS([AC_WAIT3], [AC_FUNC_WAIT3])
