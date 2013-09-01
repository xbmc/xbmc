# This file is part of Autoconf.			-*- Autoconf -*-
# Type related macros: existence, sizeof, and structure members.
#
# Copyright (C) 2000, 2001, 2002, 2004, 2005, 2006, 2007, 2008, 2009,
# 2010 Free Software Foundation, Inc.

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
## Type existence.  ##
## ---------------- ##

# ---------------- #
# General checks.  #
# ---------------- #

# Up to 2.13 included, Autoconf used to provide the macro
#
#    AC_CHECK_TYPE(TYPE, DEFAULT)
#
# Since, it provides another version which fits better with the other
# AC_CHECK_ families:
#
#    AC_CHECK_TYPE(TYPE,
#		   [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#		   [INCLUDES = DEFAULT-INCLUDES])
#
# In order to provide backward compatibility, the new scheme is
# implemented as _AC_CHECK_TYPE_NEW, the old scheme as _AC_CHECK_TYPE_OLD,
# and AC_CHECK_TYPE branches to one or the other, depending upon its
# arguments.


# _AC_CHECK_TYPE_NEW_BODY
# -----------------------
# Shell function body for _AC_CHECK_TYPE_NEW.  This macro implements the
# former task of AC_CHECK_TYPE, with one big difference though: AC_CHECK_TYPE
# used to grep in the headers, which, BTW, led to many problems until the
# extended regular expression was correct and did not give false positives.
# It turned out there are even portability issues with egrep...
#
# The most obvious way to check for a TYPE is just to compile a variable
# definition:
#
#	  TYPE my_var;
#
# (TYPE being the second parameter to the shell function, hence $[]2 in m4).
# Unfortunately this does not work for const qualified types in C++, where
# you need an initializer.  So you think of
#
#	  TYPE my_var = (TYPE) 0;
#
# Unfortunately, again, this is not valid for some C++ classes.
#
# Then you look for another scheme.  For instance you think of declaring
# a function which uses a parameter of type TYPE:
#
#	  int foo (TYPE param);
#
# but of course you soon realize this does not make it with K&R
# compilers.  And by no ways you want to
#
#	  int foo (param)
#	    TYPE param
#	  { ; }
#
# since this time it's C++ who is not happy.
#
# Don't even think of the return type of a function, since K&R cries
# there too.  So you start thinking of declaring a *pointer* to this TYPE:
#
#	  TYPE *p;
#
# but you know fairly well that this is legal in C for aggregates which
# are unknown (TYPE = struct does-not-exist).
#
# Then you think of using sizeof to make sure the TYPE is really
# defined:
#
#	  sizeof (TYPE);
#
# But this succeeds if TYPE is a variable: you get the size of the
# variable's type!!!
#
# So, to filter out the last possibility, you try this too:
#
#	  sizeof ((TYPE));
#
# This fails if TYPE is a type, but succeeds if TYPE is actually a variable.
#
# Also note that we use
#
#	  if (sizeof (TYPE))
#
# to `read' sizeof (to avoid warnings), while not depending on its type
# (not necessarily size_t etc.).
#
# C++ disallows defining types inside `sizeof ()', but that's OK,
# since we don't want to consider unnamed structs to be types for C++,
# precisely because they don't work in cases like that.
m4_define([_AC_CHECK_TYPE_NEW_BODY],
[  AS_LINENO_PUSH([$[]1])
  AC_CACHE_CHECK([for $[]2], [$[]3],
  [AS_VAR_SET([$[]3], [no])
  AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM([$[]4],
       [if (sizeof ($[]2))
	 return 0;])],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM([$[]4],
	  [if (sizeof (($[]2)))
	    return 0;])],
       [],
       [AS_VAR_SET([$[]3], [yes])])])])
  AS_LINENO_POP
])dnl

# _AC_CHECK_TYPE_NEW(TYPE,
#		     [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#		     [INCLUDES = DEFAULT-INCLUDES])
# ------------------------------------------------------------
# Check whether the type TYPE is supported by the system, maybe via the
# the provided includes.
AC_DEFUN([_AC_CHECK_TYPE_NEW],
[AC_REQUIRE_SHELL_FN([ac_fn_]_AC_LANG_ABBREV[_check_type],
  [AS_FUNCTION_DESCRIBE([ac_fn_]_AC_LANG_ABBREV[_check_type],
    [LINENO TYPE VAR INCLUDES],
    [Tests whether TYPE exists after having included INCLUDES, setting
     cache variable VAR accordingly.])],
    [$0_BODY])]dnl
[AS_VAR_PUSHDEF([ac_Type], [ac_cv_type_$1])]dnl
[ac_fn_[]_AC_LANG_ABBREV[]_check_type "$LINENO" "$1" "ac_Type" ]dnl
["AS_ESCAPE([AC_INCLUDES_DEFAULT([$4])], [""])"
AS_VAR_IF([ac_Type], [yes], [$2], [$3])
AS_VAR_POPDEF([ac_Type])dnl
])# _AC_CHECK_TYPE_NEW


# _AC_CHECK_TYPES(TYPE)
# ---------------------
# Helper to AC_CHECK_TYPES, which generates two of the four arguments
# to _AC_CHECK_TYPE_NEW that are based on TYPE.
m4_define([_AC_CHECK_TYPES],
[[$1], [AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_$1]), [1],
  [Define to 1 if the system has the type `$1'.])]])


# AC_CHECK_TYPES(TYPES,
#		 [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#		 [INCLUDES = DEFAULT-INCLUDES])
# --------------------------------------------------------
# TYPES is an m4 list.  There are no ambiguities here, we mean the newer
# AC_CHECK_TYPE.
AC_DEFUN([AC_CHECK_TYPES],
[m4_map_args_sep([_AC_CHECK_TYPE_NEW(_$0(], [)[
$2], [$3], [$4])], [], $1)])


# _AC_CHECK_TYPE_OLD(TYPE, DEFAULT)
# ---------------------------------
# FIXME: This is an extremely badly chosen name, since this
# macro actually performs an AC_REPLACE_TYPE.  Some day we
# have to clean this up.
m4_define([_AC_CHECK_TYPE_OLD],
[_AC_CHECK_TYPE_NEW([$1],,
   [AC_DEFINE_UNQUOTED([$1], [$2],
		       [Define to `$2' if <sys/types.h> does not define.])])dnl
])# _AC_CHECK_TYPE_OLD


# _AC_CHECK_TYPE_REPLACEMENT_TYPE_P(STRING)
# -----------------------------------------
# Return `1' if STRING seems to be a builtin C/C++ type, i.e., if it
# starts with `_Bool', `bool', `char', `double', `float', `int',
# `long', `short', `signed', or `unsigned' followed by characters
# that are defining types.
# Because many people have used `off_t' and `size_t' too, they are added
# for better common-useward backward compatibility.
m4_define([_AC_CHECK_TYPE_REPLACEMENT_TYPE_P],
[m4_bmatch([$1],
	  [^\(_Bool\|bool\|char\|double\|float\|int\|long\|short\|\(un\)?signed\|[_a-zA-Z][_a-zA-Z0-9]*_t\)[][_a-zA-Z0-9() *]*$],
	  1, 0)dnl
])# _AC_CHECK_TYPE_REPLACEMENT_TYPE_P


# _AC_CHECK_TYPE_MAYBE_TYPE_P(STRING)
# -----------------------------------
# Return `1' if STRING looks like a C/C++ type.
m4_define([_AC_CHECK_TYPE_MAYBE_TYPE_P],
[m4_bmatch([$1], [^[_a-zA-Z0-9 ]+\([_a-zA-Z0-9() *]\|\[\|\]\)*$],
	  1, 0)dnl
])# _AC_CHECK_TYPE_MAYBE_TYPE_P


# AC_CHECK_TYPE(TYPE, DEFAULT)
#  or
# AC_CHECK_TYPE(TYPE,
#		[ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#		[INCLUDES = DEFAULT-INCLUDES])
# -------------------------------------------------------
#
# Dispatch respectively to _AC_CHECK_TYPE_OLD or _AC_CHECK_TYPE_NEW.
# 1. More than two arguments	     => NEW
# 2. $2 seems to be replacement type => OLD
#    See _AC_CHECK_TYPE_REPLACEMENT_TYPE_P for `replacement type'.
# 3. $2 seems to be a type	     => NEW plus a warning
# 4. default			     => NEW
AC_DEFUN([AC_CHECK_TYPE],
[m4_cond([$#], [3],
  [_AC_CHECK_TYPE_NEW],
	 [$#], [4],
  [_AC_CHECK_TYPE_NEW],
	 [_AC_CHECK_TYPE_REPLACEMENT_TYPE_P([$2])], [1],
  [_AC_CHECK_TYPE_OLD],
	 [_AC_CHECK_TYPE_MAYBE_TYPE_P([$2])], [1],
  [AC_DIAGNOSE([syntax],
	       [$0: assuming `$2' is not a type])_AC_CHECK_TYPE_NEW],
  [_AC_CHECK_TYPE_NEW])($@)])# AC_CHECK_TYPE



# ---------------------------- #
# Types that must be checked.  #
# ---------------------------- #

AN_IDENTIFIER([ptrdiff_t], [AC_CHECK_TYPES])


# ----------------- #
# Specific checks.  #
# ----------------- #

# AC_TYPE_GETGROUPS
# -----------------
AC_DEFUN([AC_TYPE_GETGROUPS],
[AC_REQUIRE([AC_TYPE_UID_T])dnl
AC_CACHE_CHECK(type of array argument to getgroups, ac_cv_type_getgroups,
[AC_RUN_IFELSE([AC_LANG_SOURCE(
[[/* Thanks to Mike Rendell for this test.  */
]AC_INCLUDES_DEFAULT[
#define NGID 256
#undef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))

int
main ()
{
  gid_t gidset[NGID];
  int i, n;
  union { gid_t gval; long int lval; }  val;

  val.lval = -1;
  for (i = 0; i < NGID; i++)
    gidset[i] = val.gval;
  n = getgroups (sizeof (gidset) / MAX (sizeof (int), sizeof (gid_t)) - 1,
		 gidset);
  /* Exit non-zero if getgroups seems to require an array of ints.  This
     happens when gid_t is short int but getgroups modifies an array
     of ints.  */
  return n > 0 && gidset[n] != val.gval;
}]])],
	       [ac_cv_type_getgroups=gid_t],
	       [ac_cv_type_getgroups=int],
	       [ac_cv_type_getgroups=cross])
if test $ac_cv_type_getgroups = cross; then
  dnl When we can't run the test program (we are cross compiling), presume
  dnl that <unistd.h> has either an accurate prototype for getgroups or none.
  dnl Old systems without prototypes probably use int.
  AC_EGREP_HEADER([getgroups.*int.*gid_t], unistd.h,
		  ac_cv_type_getgroups=gid_t, ac_cv_type_getgroups=int)
fi])
AC_DEFINE_UNQUOTED(GETGROUPS_T, $ac_cv_type_getgroups,
		   [Define to the type of elements in the array set by
		    `getgroups'. Usually this is either `int' or `gid_t'.])
])# AC_TYPE_GETGROUPS


# AU::AM_TYPE_PTRDIFF_T
# ---------------------
AU_DEFUN([AM_TYPE_PTRDIFF_T],
[AC_CHECK_TYPES(ptrdiff_t)])


# AC_TYPE_INTMAX_T
# ----------------
AC_DEFUN([AC_TYPE_INTMAX_T],
[
  AC_REQUIRE([AC_TYPE_LONG_LONG_INT])
  AC_CHECK_TYPE([intmax_t],
    [AC_DEFINE([HAVE_INTMAX_T], 1,
       [Define to 1 if the system has the type `intmax_t'.])],
    [test $ac_cv_type_long_long_int = yes \
       && ac_type='long long int' \
       || ac_type='long int'
     AC_DEFINE_UNQUOTED([intmax_t], [$ac_type],
       [Define to the widest signed integer type
	if <stdint.h> and <inttypes.h> do not define.])])
])


# AC_TYPE_UINTMAX_T
# -----------------
AC_DEFUN([AC_TYPE_UINTMAX_T],
[
  AC_REQUIRE([AC_TYPE_UNSIGNED_LONG_LONG_INT])
  AC_CHECK_TYPE([uintmax_t],
    [AC_DEFINE([HAVE_UINTMAX_T], 1,
       [Define to 1 if the system has the type `uintmax_t'.])],
    [test $ac_cv_type_unsigned_long_long_int = yes \
       && ac_type='unsigned long long int' \
       || ac_type='unsigned long int'
     AC_DEFINE_UNQUOTED([uintmax_t], [$ac_type],
       [Define to the widest unsigned integer type
	if <stdint.h> and <inttypes.h> do not define.])])
])


# AC_TYPE_INTPTR_T
# ----------------
AC_DEFUN([AC_TYPE_INTPTR_T],
[
  AC_CHECK_TYPE([intptr_t],
    [AC_DEFINE([HAVE_INTPTR_T], 1,
       [Define to 1 if the system has the type `intptr_t'.])],
    [for ac_type in 'int' 'long int' 'long long int'; do
       AC_COMPILE_IFELSE(
	 [AC_LANG_BOOL_COMPILE_TRY(
	    [AC_INCLUDES_DEFAULT],
	    [[sizeof (void *) <= sizeof ($ac_type)]])],
	 [AC_DEFINE_UNQUOTED([intptr_t], [$ac_type],
	    [Define to the type of a signed integer type wide enough to
	     hold a pointer, if such a type exists, and if the system
	     does not define it.])
	  ac_type=])
       test -z "$ac_type" && break
     done])
])


# AC_TYPE_UINTPTR_T
# -----------------
AC_DEFUN([AC_TYPE_UINTPTR_T],
[
  AC_CHECK_TYPE([uintptr_t],
    [AC_DEFINE([HAVE_UINTPTR_T], 1,
       [Define to 1 if the system has the type `uintptr_t'.])],
    [for ac_type in 'unsigned int' 'unsigned long int' \
	'unsigned long long int'; do
       AC_COMPILE_IFELSE(
	 [AC_LANG_BOOL_COMPILE_TRY(
	    [AC_INCLUDES_DEFAULT],
	    [[sizeof (void *) <= sizeof ($ac_type)]])],
	 [AC_DEFINE_UNQUOTED([uintptr_t], [$ac_type],
	    [Define to the type of an unsigned integer type wide enough to
	     hold a pointer, if such a type exists, and if the system
	     does not define it.])
	  ac_type=])
       test -z "$ac_type" && break
     done])
])


# AC_TYPE_LONG_DOUBLE
# -------------------
AC_DEFUN([AC_TYPE_LONG_DOUBLE],
[
  AC_CACHE_CHECK([for long double], [ac_cv_type_long_double],
    [if test "$GCC" = yes; then
       ac_cv_type_long_double=yes
     else
       AC_COMPILE_IFELSE(
	 [AC_LANG_BOOL_COMPILE_TRY(
	    [[/* The Stardent Vistra knows sizeof (long double), but does
		 not support it.  */
	      long double foo = 0.0L;]],
	    [[/* On Ultrix 4.3 cc, long double is 4 and double is 8.  */
	      sizeof (double) <= sizeof (long double)]])],
	 [ac_cv_type_long_double=yes],
	 [ac_cv_type_long_double=no])
     fi])
  if test $ac_cv_type_long_double = yes; then
    AC_DEFINE([HAVE_LONG_DOUBLE], 1,
      [Define to 1 if the system has the type `long double'.])
  fi
])


# AC_TYPE_LONG_DOUBLE_WIDER
# -------------------------
AC_DEFUN([AC_TYPE_LONG_DOUBLE_WIDER],
[
  AC_CACHE_CHECK(
    [for long double with more range or precision than double],
    [ac_cv_type_long_double_wider],
    [AC_COMPILE_IFELSE(
       [AC_LANG_BOOL_COMPILE_TRY(
	  [[#include <float.h>
	    long double const a[] =
	      {
		 0.0L, DBL_MIN, DBL_MAX, DBL_EPSILON,
		 LDBL_MIN, LDBL_MAX, LDBL_EPSILON
	      };
	    long double
	    f (long double x)
	    {
	       return ((x + (unsigned long int) 10) * (-1 / x) + a[0]
			+ (x ? f (x) : 'c'));
	    }
	  ]],
	  [[(0 < ((DBL_MAX_EXP < LDBL_MAX_EXP)
		   + (DBL_MANT_DIG < LDBL_MANT_DIG)
		   - (LDBL_MAX_EXP < DBL_MAX_EXP)
		   - (LDBL_MANT_DIG < DBL_MANT_DIG)))
	    && (int) LDBL_EPSILON == 0
	  ]])],
       ac_cv_type_long_double_wider=yes,
       ac_cv_type_long_double_wider=no)])
  if test $ac_cv_type_long_double_wider = yes; then
    AC_DEFINE([HAVE_LONG_DOUBLE_WIDER], 1,
      [Define to 1 if the type `long double' works and has more range or
       precision than `double'.])
  fi
])# AC_TYPE_LONG_DOUBLE_WIDER


# AC_C_LONG_DOUBLE
# ----------------
AU_DEFUN([AC_C_LONG_DOUBLE],
  [
    AC_TYPE_LONG_DOUBLE_WIDER
    ac_cv_c_long_double=$ac_cv_type_long_double_wider
    if test $ac_cv_c_long_double = yes; then
      AC_DEFINE([HAVE_LONG_DOUBLE], 1,
	[Define to 1 if the type `long double' works and has more range or
	 precision than `double'.])
    fi
  ],
  [The macro `AC_C_LONG_DOUBLE' is obsolete.
You should use `AC_TYPE_LONG_DOUBLE' or `AC_TYPE_LONG_DOUBLE_WIDER' instead.]
)


# _AC_TYPE_LONG_LONG_SNIPPET
# --------------------------
# Expands to a C program that can be used to test for simultaneous support
# of 'long long' and 'unsigned long long'. We don't want to say that
# 'long long' is available if 'unsigned long long' is not, or vice versa,
# because too many programs rely on the symmetry between signed and unsigned
# integer types (excluding 'bool').
AC_DEFUN([_AC_TYPE_LONG_LONG_SNIPPET],
[
  AC_LANG_PROGRAM(
    [[/* For now, do not test the preprocessor; as of 2007 there are too many
	 implementations with broken preprocessors.  Perhaps this can
	 be revisited in 2012.  In the meantime, code should not expect
	 #if to work with literals wider than 32 bits.  */
      /* Test literals.  */
      long long int ll = 9223372036854775807ll;
      long long int nll = -9223372036854775807LL;
      unsigned long long int ull = 18446744073709551615ULL;
      /* Test constant expressions.   */
      typedef int a[((-9223372036854775807LL < 0 && 0 < 9223372036854775807ll)
		     ? 1 : -1)];
      typedef int b[(18446744073709551615ULL <= (unsigned long long int) -1
		     ? 1 : -1)];
      int i = 63;]],
    [[/* Test availability of runtime routines for shift and division.  */
      long long int llmax = 9223372036854775807ll;
      unsigned long long int ullmax = 18446744073709551615ull;
      return ((ll << 63) | (ll >> 63) | (ll < i) | (ll > i)
	      | (llmax / ll) | (llmax % ll)
	      | (ull << 63) | (ull >> 63) | (ull << i) | (ull >> i)
	      | (ullmax / ull) | (ullmax % ull));]])
])


# AC_TYPE_LONG_LONG_INT
# ---------------------
AC_DEFUN([AC_TYPE_LONG_LONG_INT],
[
  AC_CACHE_CHECK([for long long int], [ac_cv_type_long_long_int],
    [AC_LINK_IFELSE(
       [_AC_TYPE_LONG_LONG_SNIPPET],
       [dnl This catches a bug in Tandem NonStop Kernel (OSS) cc -O circa 2004.
	dnl If cross compiling, assume the bug isn't important, since
	dnl nobody cross compiles for this platform as far as we know.
	AC_RUN_IFELSE(
	  [AC_LANG_PROGRAM(
	     [[@%:@include <limits.h>
	       @%:@ifndef LLONG_MAX
	       @%:@ define HALF \
			(1LL << (sizeof (long long int) * CHAR_BIT - 2))
	       @%:@ define LLONG_MAX (HALF - 1 + HALF)
	       @%:@endif]],
	     [[long long int n = 1;
	       int i;
	       for (i = 0; ; i++)
		 {
		   long long int m = n << i;
		   if (m >> i != n)
		     return 1;
		   if (LLONG_MAX / 2 < m)
		     break;
		 }
	       return 0;]])],
	  [ac_cv_type_long_long_int=yes],
	  [ac_cv_type_long_long_int=no],
	  [ac_cv_type_long_long_int=yes])],
       [ac_cv_type_long_long_int=no])])
  if test $ac_cv_type_long_long_int = yes; then
    AC_DEFINE([HAVE_LONG_LONG_INT], 1,
      [Define to 1 if the system has the type `long long int'.])
  fi
])


# AC_TYPE_UNSIGNED_LONG_LONG_INT
# ------------------------------
AC_DEFUN([AC_TYPE_UNSIGNED_LONG_LONG_INT],
[
  AC_CACHE_CHECK([for unsigned long long int],
    [ac_cv_type_unsigned_long_long_int],
    [AC_LINK_IFELSE(
       [_AC_TYPE_LONG_LONG_SNIPPET],
       [ac_cv_type_unsigned_long_long_int=yes],
       [ac_cv_type_unsigned_long_long_int=no])])
  if test $ac_cv_type_unsigned_long_long_int = yes; then
    AC_DEFINE([HAVE_UNSIGNED_LONG_LONG_INT], 1,
      [Define to 1 if the system has the type `unsigned long long int'.])
  fi
])


# AC_TYPE_MBSTATE_T
# -----------------
AC_DEFUN([AC_TYPE_MBSTATE_T],
  [AC_CACHE_CHECK([for mbstate_t], ac_cv_type_mbstate_t,
     [AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
	   [AC_INCLUDES_DEFAULT
#	    include <wchar.h>],
	   [mbstate_t x; return sizeof x;])],
	[ac_cv_type_mbstate_t=yes],
	[ac_cv_type_mbstate_t=no])])
   if test $ac_cv_type_mbstate_t = yes; then
     AC_DEFINE([HAVE_MBSTATE_T], 1,
	       [Define to 1 if <wchar.h> declares mbstate_t.])
   else
     AC_DEFINE([mbstate_t], int,
	       [Define to a type if <wchar.h> does not define.])
   fi])


# AC_TYPE_UID_T
# -------------
# FIXME: Rewrite using AC_CHECK_TYPE.
AN_IDENTIFIER([gid_t], [AC_TYPE_UID_T])
AN_IDENTIFIER([uid_t], [AC_TYPE_UID_T])
AC_DEFUN([AC_TYPE_UID_T],
[AC_CACHE_CHECK(for uid_t in sys/types.h, ac_cv_type_uid_t,
[AC_EGREP_HEADER(uid_t, sys/types.h,
  ac_cv_type_uid_t=yes, ac_cv_type_uid_t=no)])
if test $ac_cv_type_uid_t = no; then
  AC_DEFINE(uid_t, int, [Define to `int' if <sys/types.h> doesn't define.])
  AC_DEFINE(gid_t, int, [Define to `int' if <sys/types.h> doesn't define.])
fi
])


AN_IDENTIFIER([size_t], [AC_TYPE_SIZE_T])
AC_DEFUN([AC_TYPE_SIZE_T], [AC_CHECK_TYPE(size_t, unsigned int)])

AN_IDENTIFIER([ssize_t], [AC_TYPE_SSIZE_T])
AC_DEFUN([AC_TYPE_SSIZE_T], [AC_CHECK_TYPE(ssize_t, int)])

AN_IDENTIFIER([pid_t], [AC_TYPE_PID_T])
AC_DEFUN([AC_TYPE_PID_T],  [AC_CHECK_TYPE(pid_t,  int)])

AN_IDENTIFIER([off_t], [AC_TYPE_OFF_T])
AC_DEFUN([AC_TYPE_OFF_T],  [AC_CHECK_TYPE(off_t,  long int)])

AN_IDENTIFIER([mode_t], [AC_TYPE_MODE_T])
AC_DEFUN([AC_TYPE_MODE_T], [AC_CHECK_TYPE(mode_t, int)])

AN_IDENTIFIER([int8_t], [AC_TYPE_INT8_T])
AN_IDENTIFIER([int16_t], [AC_TYPE_INT16_T])
AN_IDENTIFIER([int32_t], [AC_TYPE_INT32_T])
AN_IDENTIFIER([int64_t], [AC_TYPE_INT64_T])
AN_IDENTIFIER([uint8_t], [AC_TYPE_UINT8_T])
AN_IDENTIFIER([uint16_t], [AC_TYPE_UINT16_T])
AN_IDENTIFIER([uint32_t], [AC_TYPE_UINT32_T])
AN_IDENTIFIER([uint64_t], [AC_TYPE_UINT64_T])
AC_DEFUN([AC_TYPE_INT8_T], [_AC_TYPE_INT(8)])
AC_DEFUN([AC_TYPE_INT16_T], [_AC_TYPE_INT(16)])
AC_DEFUN([AC_TYPE_INT32_T], [_AC_TYPE_INT(32)])
AC_DEFUN([AC_TYPE_INT64_T], [_AC_TYPE_INT(64)])
AC_DEFUN([AC_TYPE_UINT8_T], [_AC_TYPE_UNSIGNED_INT(8)])
AC_DEFUN([AC_TYPE_UINT16_T], [_AC_TYPE_UNSIGNED_INT(16)])
AC_DEFUN([AC_TYPE_UINT32_T], [_AC_TYPE_UNSIGNED_INT(32)])
AC_DEFUN([AC_TYPE_UINT64_T], [_AC_TYPE_UNSIGNED_INT(64)])

# _AC_TYPE_INT_BODY
# -----------------
# Shell function body for _AC_TYPE_INT.
m4_define([_AC_TYPE_INT_BODY],
[  AS_LINENO_PUSH([$[]1])
  AC_CACHE_CHECK([for int$[]2_t], [$[]3],
    [AS_VAR_SET([$[]3], [no])
     # Order is important - never check a type that is potentially smaller
     # than half of the expected target width.
     for ac_type in int$[]2_t 'int' 'long int' \
	 'long long int' 'short int' 'signed char'; do
       AC_COMPILE_IFELSE(
	 [AC_LANG_BOOL_COMPILE_TRY(
	    [AC_INCLUDES_DEFAULT
	     enum { N = $[]2 / 2 - 1 };],
	    [0 < ($ac_type) ((((($ac_type) 1 << N) << N) - 1) * 2 + 1)])],
	 [AC_COMPILE_IFELSE(
	    [AC_LANG_BOOL_COMPILE_TRY(
	       [AC_INCLUDES_DEFAULT
	        enum { N = $[]2 / 2 - 1 };],
	       [($ac_type) ((((($ac_type) 1 << N) << N) - 1) * 2 + 1)
		 < ($ac_type) ((((($ac_type) 1 << N) << N) - 1) * 2 + 2)])],
	    [],
	    [AS_CASE([$ac_type], [int$[]2_t],
	       [AS_VAR_SET([$[]3], [yes])],
	       [AS_VAR_SET([$[]3], [$ac_type])])])])
       AS_VAR_IF([$[]3], [no], [], [break])
     done])
  AS_LINENO_POP
])# _AC_TYPE_INT_BODY

# _AC_TYPE_INT(NBITS)
# -------------------
# Set a variable ac_cv_c_intNBITS_t to `yes' if intNBITS_t is available,
# `no' if it is not and no replacement types could be found, and a C type
# if it is not available but a replacement signed integer type of width
# exactly NBITS bits was found.  In the third case, intNBITS_t is AC_DEFINEd
# to type, as well.
AC_DEFUN([_AC_TYPE_INT],
[AC_REQUIRE_SHELL_FN([ac_fn_c_find_intX_t],
  [AS_FUNCTION_DESCRIBE([ac_fn_c_find_intX_t], [LINENO BITS VAR],
    [Finds a signed integer type with width BITS, setting cache variable VAR
     accordingly.])],
    [$0_BODY])]dnl
[ac_fn_c_find_intX_t "$LINENO" "$1" "ac_cv_c_int$1_t"
case $ac_cv_c_int$1_t in #(
  no|yes) ;; #(
  *)
    AC_DEFINE_UNQUOTED([int$1_t], [$ac_cv_c_int$1_t],
      [Define to the type of a signed integer type of width exactly $1 bits
       if such a type exists and the standard includes do not define it.]);;
esac
])# _AC_TYPE_INT

# _AC_TYPE_UNSIGNED_INT_BODY
# --------------------------
# Shell function body for _AC_TYPE_UNSIGNED_INT.
m4_define([_AC_TYPE_UNSIGNED_INT_BODY],
[  AS_LINENO_PUSH([$[]1])
  AC_CACHE_CHECK([for uint$[]2_t], $[]3,
    [AS_VAR_SET([$[]3], [no])
     # Order is important - never check a type that is potentially smaller
     # than half of the expected target width.
     for ac_type in uint$[]2_t 'unsigned int' 'unsigned long int' \
	 'unsigned long long int' 'unsigned short int' 'unsigned char'; do
       AC_COMPILE_IFELSE(
	 [AC_LANG_BOOL_COMPILE_TRY(
	    [AC_INCLUDES_DEFAULT],
	    [(($ac_type) -1 >> ($[]2 / 2 - 1)) >> ($[]2 / 2 - 1) == 3])],
	 [AS_CASE([$ac_type], [uint$[]2_t],
	    [AS_VAR_SET([$[]3], [yes])],
	    [AS_VAR_SET([$[]3], [$ac_type])])])
       AS_VAR_IF([$[]3], [no], [], [break])
     done])
  AS_LINENO_POP
])# _AC_TYPE_UNSIGNED_INT_BODY


# _AC_TYPE_UNSIGNED_INT(NBITS)
# ----------------------------
# Set a variable ac_cv_c_uintNBITS_t to `yes' if uintNBITS_t is available,
# `no' if it is not and no replacement types could be found, and a C type
# if it is not available but a replacement unsigned integer type of width
# exactly NBITS bits was found.  In the third case, uintNBITS_t is AC_DEFINEd
# to type, as well.
AC_DEFUN([_AC_TYPE_UNSIGNED_INT],
[AC_REQUIRE_SHELL_FN([ac_fn_c_find_uintX_t],
  [AS_FUNCTION_DESCRIBE([ac_fn_c_find_uintX_t], [LINENO BITS VAR],
    [Finds an unsigned integer type with width BITS, setting cache variable VAR
     accordingly.])],
  [$0_BODY])]dnl
[ac_fn_c_find_uintX_t "$LINENO" "$1" "ac_cv_c_uint$1_t"
case $ac_cv_c_uint$1_t in #(
  no|yes) ;; #(
  *)
    m4_bmatch([$1], [^\(8\|32\|64\)$],
      [AC_DEFINE([_UINT$1_T], 1,
	 [Define for Solaris 2.5.1 so the uint$1_t typedef from
	  <sys/synch.h>, <pthread.h>, or <semaphore.h> is not used.
	  If the typedef were allowed, the #define below would cause a
	  syntax error.])])
    AC_DEFINE_UNQUOTED([uint$1_t], [$ac_cv_c_uint$1_t],
      [Define to the type of an unsigned integer type of width exactly $1 bits
       if such a type exists and the standard includes do not define it.]);;
  esac
])# _AC_TYPE_UNSIGNED_INT

# AC_TYPE_SIGNAL
# --------------
# Note that identifiers starting with SIG are reserved by ANSI C.
# C89 requires signal handlers to return void; only K&R returned int;
# modern code does not need to worry about using this macro (not to
# mention that sigaction is better than signal).
AU_DEFUN([AC_TYPE_SIGNAL],
[AC_CACHE_CHECK([return type of signal handlers], ac_cv_type_signal,
[AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([#include <sys/types.h>
#include <signal.h>
],
		 [return *(signal (0, 0)) (0) == 1;])],
		   [ac_cv_type_signal=int],
		   [ac_cv_type_signal=void])])
AC_DEFINE_UNQUOTED(RETSIGTYPE, $ac_cv_type_signal,
		   [Define as the return type of signal handlers
		    (`int' or `void').])
], [your code may safely assume C89 semantics that RETSIGTYPE is void.
Remove this warning and the `AC_CACHE_CHECK' when you adjust the code.])


## ------------------------ ##
## Checking size of types.  ##
## ------------------------ ##

# ---------------- #
# Generic checks.  #
# ---------------- #


# AC_CHECK_SIZEOF(TYPE, [IGNORED], [INCLUDES = DEFAULT-INCLUDES])
# ---------------------------------------------------------------
AC_DEFUN([AC_CHECK_SIZEOF],
[AS_LITERAL_IF(m4_translit([[$1]], [*], [p]), [],
	       [m4_fatal([$0: requires literal arguments])])]dnl
[# The cast to long int works around a bug in the HP C Compiler
# version HP92453-01 B.11.11.23709.GP, which incorrectly rejects
# declarations like `int a3[[(sizeof (unsigned char)) >= 0]];'.
# This bug is HP SR number 8606223364.
_AC_CACHE_CHECK_INT([size of $1], [AS_TR_SH([ac_cv_sizeof_$1])],
  [(long int) (sizeof ($1))],
  [AC_INCLUDES_DEFAULT([$3])],
  [if test "$AS_TR_SH([ac_cv_type_$1])" = yes; then
     AC_MSG_FAILURE([cannot compute sizeof ($1)], 77)
   else
     AS_TR_SH([ac_cv_sizeof_$1])=0
   fi])

AC_DEFINE_UNQUOTED(AS_TR_CPP(sizeof_$1), $AS_TR_SH([ac_cv_sizeof_$1]),
		   [The size of `$1', as computed by sizeof.])
])# AC_CHECK_SIZEOF


# AC_CHECK_ALIGNOF(TYPE, [INCLUDES = DEFAULT-INCLUDES])
# -----------------------------------------------------
# TYPE can include braces and semicolon, which AS_TR_CPP and AS_TR_SH
# (correctly) recognize as potential shell metacharacters.  So we
# have to flatten problematic characters ourselves to guarantee that
# AC_DEFINE_UNQUOTED will see a literal.
AC_DEFUN([AC_CHECK_ALIGNOF],
[m4_if(m4_index(m4_translit([[$1]], [`\"], [$]), [$]), [-1], [],
       [m4_fatal([$0: requires literal arguments])])]dnl
[_$0([$1], [$2], m4_translit([[$1]], [{;}], [___]))])

m4_define([_AC_CHECK_ALIGNOF],
[# The cast to long int works around a bug in the HP C Compiler,
# see AC_CHECK_SIZEOF for more information.
_AC_CACHE_CHECK_INT([alignment of $1], [AS_TR_SH([ac_cv_alignof_$3])],
  [(long int) offsetof (ac__type_alignof_, y)],
  [AC_INCLUDES_DEFAULT([$2])
#ifndef offsetof
# define offsetof(type, member) ((char *) &((type *) 0)->member - (char *) 0)
#endif
typedef struct { char x; $1 y; } ac__type_alignof_;],
  [if test "$AS_TR_SH([ac_cv_type_$3])" = yes; then
     AC_MSG_FAILURE([cannot compute alignment of $1], 77)
   else
     AS_TR_SH([ac_cv_alignof_$3])=0
   fi])

AC_DEFINE_UNQUOTED(AS_TR_CPP(alignof_$3), $AS_TR_SH([ac_cv_alignof_$3]),
		   [The normal alignment of `$1', in bytes.])
])# AC_CHECK_ALIGNOF


# AU::AC_INT_16_BITS
# ------------------
# What a great name :)
AU_DEFUN([AC_INT_16_BITS],
[AC_CHECK_SIZEOF([int])
test $ac_cv_sizeof_int = 2 &&
  AC_DEFINE(INT_16_BITS, 1,
	    [Define to 1 if `sizeof (int)' = 2.  Obsolete, use `SIZEOF_INT'.])
], [your code should no longer depend upon `INT_16_BITS', but upon
`SIZEOF_INT == 2'.  Remove this warning and the `AC_DEFINE' when you
adjust the code.])


# AU::AC_LONG_64_BITS
# -------------------
AU_DEFUN([AC_LONG_64_BITS],
[AC_CHECK_SIZEOF([long int])
test $ac_cv_sizeof_long_int = 8 &&
  AC_DEFINE(LONG_64_BITS, 1,
	    [Define to 1 if `sizeof (long int)' = 8.  Obsolete, use
	     `SIZEOF_LONG_INT'.])
], [your code should no longer depend upon `LONG_64_BITS', but upon
`SIZEOF_LONG_INT == 8'.  Remove this warning and the `AC_DEFINE' when
you adjust the code.])



## -------------------------- ##
## Generic structure checks.  ##
## -------------------------- ##


# ---------------- #
# Generic checks.  #
# ---------------- #

# _AC_CHECK_MEMBER_BODY
# ---------------------
# Shell function body for AC_CHECK_MEMBER.
m4_define([_AC_CHECK_MEMBER_BODY],
[  AS_LINENO_PUSH([$[]1])
  AC_CACHE_CHECK([for $[]2.$[]3], [$[]4],
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([$[]5],
[static $[]2 ac_aggr;
if (ac_aggr.$[]3)
return 0;])],
		[AS_VAR_SET([$[]4], [yes])],
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([$[]5],
[static $[]2 ac_aggr;
if (sizeof ac_aggr.$[]3)
return 0;])],
		[AS_VAR_SET([$[]4], [yes])],
		[AS_VAR_SET([$[]4], [no])])])])
  AS_LINENO_POP
])dnl

# AC_CHECK_MEMBER(AGGREGATE.MEMBER,
#		  [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#		  [INCLUDES = DEFAULT-INCLUDES])
# ---------------------------------------------------------
# AGGREGATE.MEMBER is for instance `struct passwd.pw_gecos', shell
# variables are not a valid argument.
AC_DEFUN([AC_CHECK_MEMBER],
[AC_REQUIRE_SHELL_FN([ac_fn_]_AC_LANG_ABBREV[_check_member],
  [AS_FUNCTION_DESCRIBE([ac_fn_]_AC_LANG_ABBREV[_check_member],
    [LINENO AGGR MEMBER VAR INCLUDES],
    [Tries to find if the field MEMBER exists in type AGGR, after including
     INCLUDES, setting cache variable VAR accordingly.])],
    [_$0_BODY])]dnl
[AS_LITERAL_IF([$1], [], [m4_fatal([$0: requires literal arguments])])]dnl
[m4_if(m4_index([$1], [.]), [-1],
  [m4_fatal([$0: Did not see any dot in `$1'])])]dnl
[AS_VAR_PUSHDEF([ac_Member], [ac_cv_member_$1])]dnl
[ac_fn_[]_AC_LANG_ABBREV[]_check_member "$LINENO" ]dnl
[m4_bpatsubst([$1], [^\([^.]*\)\.\(.*\)], ["\1" "\2"]) "ac_Member" ]dnl
["AS_ESCAPE([AC_INCLUDES_DEFAULT([$4])], [""])"
AS_VAR_IF([ac_Member], [yes], [$2], [$3])
AS_VAR_POPDEF([ac_Member])dnl
])# AC_CHECK_MEMBER


# _AC_CHECK_MEMBERS(AGGREGATE.MEMBER)
# -----------------------------------
# Helper to AC_CHECK_MEMBERS, which generates two of the four
# arguments to AC_CHECK_MEMBER that are based on AGGREGATE and MEMBER.
m4_define([_AC_CHECK_MEMBERS],
[[$1], [AC_DEFINE_UNQUOTED(AS_TR_CPP([HAVE_$1]), [1],
  [Define to 1 if `]m4_bpatsubst([$1],
    [^\([^.]*\)\.\(.*\)], [[\2' is a member of `\1]])['.])]])

# AC_CHECK_MEMBERS([AGGREGATE.MEMBER, ...],
#		   [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#		   [INCLUDES = DEFAULT-INCLUDES])
# ----------------------------------------------------------
# The first argument is an m4 list.
AC_DEFUN([AC_CHECK_MEMBERS],
[m4_map_args_sep([AC_CHECK_MEMBER(_$0(], [)[
$2], [$3], [$4])], [], $1)])



# ------------------------------------------------------- #
# Members that ought to be tested with AC_CHECK_MEMBERS.  #
# ------------------------------------------------------- #

AN_IDENTIFIER([st_blksize], [AC_CHECK_MEMBERS([struct stat.st_blksize])])
AN_IDENTIFIER([st_rdev],    [AC_CHECK_MEMBERS([struct stat.st_rdev])])


# Alphabetic order, please.

# _AC_STRUCT_DIRENT(MEMBER)
# -------------------------
AC_DEFUN([_AC_STRUCT_DIRENT],
[
  AC_REQUIRE([AC_HEADER_DIRENT])
  AC_CHECK_MEMBERS([struct dirent.$1], [], [],
    [[
#include <sys/types.h>
#ifdef HAVE_DIRENT_H
# include <dirent.h>
#else
# define dirent direct
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif
    ]])
])

# AC_STRUCT_DIRENT_D_INO
# ----------------------
AC_DEFUN([AC_STRUCT_DIRENT_D_INO], [_AC_STRUCT_DIRENT([d_ino])])

# AC_STRUCT_DIRENT_D_TYPE
# -----------------------
AC_DEFUN([AC_STRUCT_DIRENT_D_TYPE], [_AC_STRUCT_DIRENT([d_type])])


# AC_STRUCT_ST_BLKSIZE
# --------------------
AU_DEFUN([AC_STRUCT_ST_BLKSIZE],
[AC_CHECK_MEMBERS([struct stat.st_blksize],
		 [AC_DEFINE(HAVE_ST_BLKSIZE, 1,
			    [Define to 1 if your `struct stat' has
			     `st_blksize'.  Deprecated, use
			     `HAVE_STRUCT_STAT_ST_BLKSIZE' instead.])])
], [your code should no longer depend upon `HAVE_ST_BLKSIZE', but
`HAVE_STRUCT_STAT_ST_BLKSIZE'.  Remove this warning and
the `AC_DEFINE' when you adjust the code.])# AC_STRUCT_ST_BLKSIZE


# AC_STRUCT_ST_BLOCKS
# -------------------
# If `struct stat' contains an `st_blocks' member, define
# HAVE_STRUCT_STAT_ST_BLOCKS.  Otherwise, add `fileblocks.o' to the
# output variable LIBOBJS.  We still define HAVE_ST_BLOCKS for backward
# compatibility.  In the future, we will activate specializations for
# this macro, so don't obsolete it right now.
#
# AC_OBSOLETE([$0], [; replace it with
#   AC_CHECK_MEMBERS([struct stat.st_blocks],
#		      [AC_LIBOBJ([fileblocks])])
# Please note that it will define `HAVE_STRUCT_STAT_ST_BLOCKS',
# and not `HAVE_ST_BLOCKS'.])dnl
#
AN_IDENTIFIER([st_blocks],  [AC_STRUCT_ST_BLOCKS])
AC_DEFUN([AC_STRUCT_ST_BLOCKS],
[AC_CHECK_MEMBERS([struct stat.st_blocks],
		  [AC_DEFINE(HAVE_ST_BLOCKS, 1,
			     [Define to 1 if your `struct stat' has
			      `st_blocks'.  Deprecated, use
			      `HAVE_STRUCT_STAT_ST_BLOCKS' instead.])],
		  [AC_LIBOBJ([fileblocks])])
])# AC_STRUCT_ST_BLOCKS


# AC_STRUCT_ST_RDEV
# -----------------
AU_DEFUN([AC_STRUCT_ST_RDEV],
[AC_CHECK_MEMBERS([struct stat.st_rdev],
		 [AC_DEFINE(HAVE_ST_RDEV, 1,
			    [Define to 1 if your `struct stat' has `st_rdev'.
			     Deprecated, use `HAVE_STRUCT_STAT_ST_RDEV'
			     instead.])])
], [your code should no longer depend upon `HAVE_ST_RDEV', but
`HAVE_STRUCT_STAT_ST_RDEV'.  Remove this warning and
the `AC_DEFINE' when you adjust the code.])# AC_STRUCT_ST_RDEV


# AC_STRUCT_TM
# ------------
# FIXME: This macro is badly named, it should be AC_CHECK_TYPE_STRUCT_TM.
# Or something else, but what? AC_CHECK_TYPE_STRUCT_TM_IN_SYS_TIME?
AC_DEFUN([AC_STRUCT_TM],
[AC_CACHE_CHECK([whether struct tm is in sys/time.h or time.h],
  ac_cv_struct_tm,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <sys/types.h>
#include <time.h>
],
				    [struct tm tm;
				     int *p = &tm.tm_sec;
				     return !p;])],
		   [ac_cv_struct_tm=time.h],
		   [ac_cv_struct_tm=sys/time.h])])
if test $ac_cv_struct_tm = sys/time.h; then
  AC_DEFINE(TM_IN_SYS_TIME, 1,
	    [Define to 1 if your <sys/time.h> declares `struct tm'.])
fi
])# AC_STRUCT_TM


# AC_STRUCT_TIMEZONE
# ------------------
# Figure out how to get the current timezone.  If `struct tm' has a
# `tm_zone' member, define `HAVE_TM_ZONE'.  Otherwise, if the
# external array `tzname' is found, define `HAVE_TZNAME'.
AN_IDENTIFIER([tm_zone], [AC_STRUCT_TIMEZONE])
AC_DEFUN([AC_STRUCT_TIMEZONE],
[AC_REQUIRE([AC_STRUCT_TM])dnl
AC_CHECK_MEMBERS([struct tm.tm_zone],,,[#include <sys/types.h>
#include <$ac_cv_struct_tm>
])
if test "$ac_cv_member_struct_tm_tm_zone" = yes; then
  AC_DEFINE(HAVE_TM_ZONE, 1,
	    [Define to 1 if your `struct tm' has `tm_zone'. Deprecated, use
	     `HAVE_STRUCT_TM_TM_ZONE' instead.])
else
  AC_CHECK_DECLS([tzname], , , [#include <time.h>])
  AC_CACHE_CHECK(for tzname, ac_cv_var_tzname,
[AC_LINK_IFELSE([AC_LANG_PROGRAM(
[[#include <time.h>
#if !HAVE_DECL_TZNAME
extern char *tzname[];
#endif
]],
[[return tzname[0][0];]])],
		[ac_cv_var_tzname=yes],
		[ac_cv_var_tzname=no])])
  if test $ac_cv_var_tzname = yes; then
    AC_DEFINE(HAVE_TZNAME, 1,
	      [Define to 1 if you don't have `tm_zone' but do have the external
	       array `tzname'.])
  fi
fi
])# AC_STRUCT_TIMEZONE
