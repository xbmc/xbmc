# This file is part of Autoconf.                       -*- Autoconf -*-
# Checking for libraries.
# Copyright (C) 1992, 1993, 1994, 1995, 1996, 1998, 1999, 2000, 2001,
# 2002, 2003, 2004, 2005, 2006, 2008, 2009, 2010 Free Software
# Foundation, Inc.

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
# 1. Generic tests for libraries
# 2. Tests for specific libraries


## --------------------------------- ##
## 1. Generic tests for libraries.## ##
## --------------------------------- ##



# AC_SEARCH_LIBS(FUNCTION, SEARCH-LIBS,
#                [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#                [OTHER-LIBRARIES])
# --------------------------------------------------------
# Search for a library defining FUNC, if it's not already available.
AC_DEFUN([AC_SEARCH_LIBS],
[AS_VAR_PUSHDEF([ac_Search], [ac_cv_search_$1])dnl
AC_CACHE_CHECK([for library containing $1], [ac_Search],
[ac_func_search_save_LIBS=$LIBS
AC_LANG_CONFTEST([AC_LANG_CALL([], [$1])])
for ac_lib in '' $2; do
  if test -z "$ac_lib"; then
    ac_res="none required"
  else
    ac_res=-l$ac_lib
    LIBS="-l$ac_lib $5 $ac_func_search_save_LIBS"
  fi
  AC_LINK_IFELSE([], [AS_VAR_SET([ac_Search], [$ac_res])])
  AS_VAR_SET_IF([ac_Search], [break])
done
AS_VAR_SET_IF([ac_Search], , [AS_VAR_SET([ac_Search], [no])])
rm conftest.$ac_ext
LIBS=$ac_func_search_save_LIBS])
AS_VAR_COPY([ac_res], [ac_Search])
AS_IF([test "$ac_res" != no],
  [test "$ac_res" = "none required" || LIBS="$ac_res $LIBS"
  $3],
      [$4])
AS_VAR_POPDEF([ac_Search])dnl
])



# AC_CHECK_LIB(LIBRARY, FUNCTION,
#              [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#              [OTHER-LIBRARIES])
# ------------------------------------------------------
#
# Use a cache variable name containing both the library and function name,
# because the test really is for library $1 defining function $2, not
# just for library $1.  Separate tests with the same $1 and different $2s
# may have different results.
#
# Note that using directly AS_VAR_PUSHDEF([ac_Lib], [ac_cv_lib_$1_$2])
# is asking for troubles, since AC_CHECK_LIB($lib, fun) would give
# ac_cv_lib_$lib_fun, which is definitely not what was meant.  Hence
# the AS_LITERAL_IF indirection.
#
# FIXME: This macro is extremely suspicious.  It DEFINEs unconditionally,
# whatever the FUNCTION, in addition to not being a *S macro.  Note
# that the cache does depend upon the function we are looking for.
#
# It is on purpose we used `ac_check_lib_save_LIBS' and not just
# `ac_save_LIBS': there are many macros which don't want to see `LIBS'
# changed but still want to use AC_CHECK_LIB, so they save `LIBS'.
# And ``ac_save_LIBS' is too tempting a name, so let's leave them some
# freedom.
AC_DEFUN([AC_CHECK_LIB],
[m4_ifval([$3], , [AH_CHECK_LIB([$1])])dnl
AS_LITERAL_WORD_IF([$1],
	      [AS_VAR_PUSHDEF([ac_Lib], [ac_cv_lib_$1_$2])],
	      [AS_VAR_PUSHDEF([ac_Lib], [ac_cv_lib_$1''_$2])])dnl
AC_CACHE_CHECK([for $2 in -l$1], [ac_Lib],
[ac_check_lib_save_LIBS=$LIBS
LIBS="-l$1 $5 $LIBS"
AC_LINK_IFELSE([AC_LANG_CALL([], [$2])],
	       [AS_VAR_SET([ac_Lib], [yes])],
	       [AS_VAR_SET([ac_Lib], [no])])
LIBS=$ac_check_lib_save_LIBS])
AS_VAR_IF([ac_Lib], [yes],
      [m4_default([$3], [AC_DEFINE_UNQUOTED(AS_TR_CPP(HAVE_LIB$1))
  LIBS="-l$1 $LIBS"
])],
      [$4])
AS_VAR_POPDEF([ac_Lib])dnl
])# AC_CHECK_LIB


# AH_CHECK_LIB(LIBNAME)
# ---------------------
m4_define([AH_CHECK_LIB],
[AH_TEMPLATE(AS_TR_CPP([HAVE_LIB$1]),
	     [Define to 1 if you have the `$1' library (-l$1).])])


# AC_HAVE_LIBRARY(LIBRARY,
#                 [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#                 [OTHER-LIBRARIES])
# ---------------------------------------------------------
#
# This macro is equivalent to calling `AC_CHECK_LIB' with a FUNCTION
# argument of `main'.  In addition, LIBRARY can be written as any of
# `foo', `-lfoo', or `libfoo.a'.  In all of those cases, the compiler
# is passed `-lfoo'.  However, LIBRARY cannot be a shell variable;
# it must be a literal name.
AU_DEFUN([AC_HAVE_LIBRARY],
[m4_pushdef([AC_Lib_Name],
	    m4_bpatsubst(m4_bpatsubst([[$1]],
				    [lib\([^\.]*\)\.a], [\1]),
			[-l], []))dnl
AC_CHECK_LIB(AC_Lib_Name, main, [$2], [$3], [$4])dnl
ac_cv_lib_[]AC_Lib_Name()=ac_cv_lib_[]AC_Lib_Name()_main
m4_popdef([AC_Lib_Name])dnl
])




## --------------------------------- ##
## 2. Tests for specific libraries.  ##
## --------------------------------- ##



# --------------------- #
# Checks for X window.  #
# --------------------- #


# _AC_PATH_X_XMKMF
# ----------------
# Internal subroutine of _AC_PATH_X.
# Set ac_x_includes and/or ac_x_libraries.
m4_define([_AC_PATH_X_XMKMF],
[AC_ARG_VAR(XMKMF, [Path to xmkmf, Makefile generator for X Window System])dnl
rm -f -r conftest.dir
if mkdir conftest.dir; then
  cd conftest.dir
  cat >Imakefile <<'_ACEOF'
incroot:
	@echo incroot='${INCROOT}'
usrlibdir:
	@echo usrlibdir='${USRLIBDIR}'
libdir:
	@echo libdir='${LIBDIR}'
_ACEOF
  if (export CC; ${XMKMF-xmkmf}) >/dev/null 2>/dev/null && test -f Makefile; then
    # GNU make sometimes prints "make[1]: Entering ...", which would confuse us.
    for ac_var in incroot usrlibdir libdir; do
      eval "ac_im_$ac_var=\`\${MAKE-make} $ac_var 2>/dev/null | sed -n 's/^$ac_var=//p'\`"
    done
    # Open Windows xmkmf reportedly sets LIBDIR instead of USRLIBDIR.
    for ac_extension in a so sl dylib la dll; do
      if test ! -f "$ac_im_usrlibdir/libX11.$ac_extension" &&
	 test -f "$ac_im_libdir/libX11.$ac_extension"; then
	ac_im_usrlibdir=$ac_im_libdir; break
      fi
    done
    # Screen out bogus values from the imake configuration.  They are
    # bogus both because they are the default anyway, and because
    # using them would break gcc on systems where it needs fixed includes.
    case $ac_im_incroot in
	/usr/include) ac_x_includes= ;;
	*) test -f "$ac_im_incroot/X11/Xos.h" && ac_x_includes=$ac_im_incroot;;
    esac
    case $ac_im_usrlibdir in
	/usr/lib | /usr/lib64 | /lib | /lib64) ;;
	*) test -d "$ac_im_usrlibdir" && ac_x_libraries=$ac_im_usrlibdir ;;
    esac
  fi
  cd ..
  rm -f -r conftest.dir
fi
])# _AC_PATH_X_XMKMF


# _AC_PATH_X_DIRECT
# -----------------
# Internal subroutine of _AC_PATH_X.
# Set ac_x_includes and/or ac_x_libraries.
m4_define([_AC_PATH_X_DIRECT],
[# Standard set of common directories for X headers.
# Check X11 before X11Rn because it is often a symlink to the current release.
ac_x_header_dirs='
/usr/X11/include
/usr/X11R7/include
/usr/X11R6/include
/usr/X11R5/include
/usr/X11R4/include

/usr/include/X11
/usr/include/X11R7
/usr/include/X11R6
/usr/include/X11R5
/usr/include/X11R4

/usr/local/X11/include
/usr/local/X11R7/include
/usr/local/X11R6/include
/usr/local/X11R5/include
/usr/local/X11R4/include

/usr/local/include/X11
/usr/local/include/X11R7
/usr/local/include/X11R6
/usr/local/include/X11R5
/usr/local/include/X11R4

/usr/X386/include
/usr/x386/include
/usr/XFree86/include/X11

/usr/include
/usr/local/include
/usr/unsupported/include
/usr/athena/include
/usr/local/x11r5/include
/usr/lpp/Xamples/include

/usr/openwin/include
/usr/openwin/share/include'

if test "$ac_x_includes" = no; then
  # Guess where to find include files, by looking for Xlib.h.
  # First, try using that file with no special directory specified.
  AC_PREPROC_IFELSE([AC_LANG_SOURCE([@%:@include <X11/Xlib.h>])],
[# We can compile using X headers with no special include directory.
ac_x_includes=],
[for ac_dir in $ac_x_header_dirs; do
  if test -r "$ac_dir/X11/Xlib.h"; then
    ac_x_includes=$ac_dir
    break
  fi
done])
fi # $ac_x_includes = no

if test "$ac_x_libraries" = no; then
  # Check for the libraries.
  # See if we find them without any special options.
  # Don't add to $LIBS permanently.
  ac_save_LIBS=$LIBS
  LIBS="-lX11 $LIBS"
  AC_LINK_IFELSE([AC_LANG_PROGRAM([@%:@include <X11/Xlib.h>],
				  [XrmInitialize ()])],
		 [LIBS=$ac_save_LIBS
# We can link X programs with no special library path.
ac_x_libraries=],
		 [LIBS=$ac_save_LIBS
for ac_dir in `AS_ECHO(["$ac_x_includes $ac_x_header_dirs"]) | sed s/include/lib/g`
do
  # Don't even attempt the hair of trying to link an X program!
  for ac_extension in a so sl dylib la dll; do
    if test -r "$ac_dir/libX11.$ac_extension"; then
      ac_x_libraries=$ac_dir
      break 2
    fi
  done
done])
fi # $ac_x_libraries = no
])# _AC_PATH_X_DIRECT


# _AC_PATH_X
# ----------
# Compute ac_cv_have_x.
AC_DEFUN([_AC_PATH_X],
[AC_CACHE_VAL(ac_cv_have_x,
[# One or both of the vars are not set, and there is no cached value.
ac_x_includes=no ac_x_libraries=no
_AC_PATH_X_XMKMF
_AC_PATH_X_DIRECT
case $ac_x_includes,$ac_x_libraries in #(
  no,* | *,no | *\'*)
    # Didn't find X, or a directory has "'" in its name.
    ac_cv_have_x="have_x=no";; #(
  *)
    # Record where we found X for the cache.
    ac_cv_have_x="have_x=yes\
	ac_x_includes='$ac_x_includes'\
	ac_x_libraries='$ac_x_libraries'"
esac])dnl
])


# AC_PATH_X
# ---------
# If we find X, set shell vars x_includes and x_libraries to the
# paths, otherwise set no_x=yes.
# Uses ac_ vars as temps to allow command line to override cache and checks.
# --without-x overrides everything else, but does not touch the cache.
AN_HEADER([X11/Xlib.h],  [AC_PATH_X])
AC_DEFUN([AC_PATH_X],
[dnl Document the X abnormal options inherited from history.
m4_divert_once([HELP_BEGIN], [
X features:
  --x-includes=DIR    X include files are in DIR
  --x-libraries=DIR   X library files are in DIR])dnl
AC_MSG_CHECKING([for X])

AC_ARG_WITH(x, [  --with-x                use the X Window System])
# $have_x is `yes', `no', `disabled', or empty when we do not yet know.
if test "x$with_x" = xno; then
  # The user explicitly disabled X.
  have_x=disabled
else
  case $x_includes,$x_libraries in #(
    *\'*) AC_MSG_ERROR([cannot use X directory names containing ']);; #(
    *,NONE | NONE,*) _AC_PATH_X;; #(
    *) have_x=yes;;
  esac
  eval "$ac_cv_have_x"
fi # $with_x != no

if test "$have_x" != yes; then
  AC_MSG_RESULT([$have_x])
  no_x=yes
else
  # If each of the values was on the command line, it overrides each guess.
  test "x$x_includes" = xNONE && x_includes=$ac_x_includes
  test "x$x_libraries" = xNONE && x_libraries=$ac_x_libraries
  # Update the cache value to reflect the command line values.
  ac_cv_have_x="have_x=yes\
	ac_x_includes='$x_includes'\
	ac_x_libraries='$x_libraries'"
  AC_MSG_RESULT([libraries $x_libraries, headers $x_includes])
fi
])# AC_PATH_X



# AC_PATH_XTRA
# ------------
# Find additional X libraries, magic flags, etc.
AC_DEFUN([AC_PATH_XTRA],
[AC_REQUIRE([AC_PATH_X])dnl
if test "$no_x" = yes; then
  # Not all programs may use this symbol, but it does not hurt to define it.
  AC_DEFINE([X_DISPLAY_MISSING], 1,
	    [Define to 1 if the X Window System is missing or not being used.])
  X_CFLAGS= X_PRE_LIBS= X_LIBS= X_EXTRA_LIBS=
else
  if test -n "$x_includes"; then
    X_CFLAGS="$X_CFLAGS -I$x_includes"
  fi

  # It would also be nice to do this for all -L options, not just this one.
  if test -n "$x_libraries"; then
    X_LIBS="$X_LIBS -L$x_libraries"
    # For Solaris; some versions of Sun CC require a space after -R and
    # others require no space.  Words are not sufficient . . . .
    AC_MSG_CHECKING([whether -R must be followed by a space])
    ac_xsave_LIBS=$LIBS; LIBS="$LIBS -R$x_libraries"
    ac_xsave_[]_AC_LANG_ABBREV[]_werror_flag=$ac_[]_AC_LANG_ABBREV[]_werror_flag
    ac_[]_AC_LANG_ABBREV[]_werror_flag=yes
    AC_LINK_IFELSE([AC_LANG_PROGRAM()],
      [AC_MSG_RESULT([no])
       X_LIBS="$X_LIBS -R$x_libraries"],
      [LIBS="$ac_xsave_LIBS -R $x_libraries"
       AC_LINK_IFELSE([AC_LANG_PROGRAM()],
	 [AC_MSG_RESULT([yes])
	  X_LIBS="$X_LIBS -R $x_libraries"],
	 [AC_MSG_RESULT([neither works])])])
    ac_[]_AC_LANG_ABBREV[]_werror_flag=$ac_xsave_[]_AC_LANG_ABBREV[]_werror_flag
    LIBS=$ac_xsave_LIBS
  fi

  # Check for system-dependent libraries X programs must link with.
  # Do this before checking for the system-independent R6 libraries
  # (-lICE), since we may need -lsocket or whatever for X linking.

  if test "$ISC" = yes; then
    X_EXTRA_LIBS="$X_EXTRA_LIBS -lnsl_s -linet"
  else
    # Martyn Johnson says this is needed for Ultrix, if the X
    # libraries were built with DECnet support.  And Karl Berry says
    # the Alpha needs dnet_stub (dnet does not exist).
    ac_xsave_LIBS="$LIBS"; LIBS="$LIBS $X_LIBS -lX11"
    AC_LINK_IFELSE([AC_LANG_CALL([], [XOpenDisplay])],
		   [],
    [AC_CHECK_LIB(dnet, dnet_ntoa, [X_EXTRA_LIBS="$X_EXTRA_LIBS -ldnet"])
    if test $ac_cv_lib_dnet_dnet_ntoa = no; then
      AC_CHECK_LIB(dnet_stub, dnet_ntoa,
	[X_EXTRA_LIBS="$X_EXTRA_LIBS -ldnet_stub"])
    fi])
    LIBS="$ac_xsave_LIBS"

    # msh@cis.ufl.edu says -lnsl (and -lsocket) are needed for his 386/AT,
    # to get the SysV transport functions.
    # Chad R. Larson says the Pyramis MIS-ES running DC/OSx (SVR4)
    # needs -lnsl.
    # The nsl library prevents programs from opening the X display
    # on Irix 5.2, according to T.E. Dickey.
    # The functions gethostbyname, getservbyname, and inet_addr are
    # in -lbsd on LynxOS 3.0.1/i386, according to Lars Hecking.
    AC_CHECK_FUNC(gethostbyname)
    if test $ac_cv_func_gethostbyname = no; then
      AC_CHECK_LIB(nsl, gethostbyname, X_EXTRA_LIBS="$X_EXTRA_LIBS -lnsl")
      if test $ac_cv_lib_nsl_gethostbyname = no; then
	AC_CHECK_LIB(bsd, gethostbyname, X_EXTRA_LIBS="$X_EXTRA_LIBS -lbsd")
      fi
    fi

    # lieder@skyler.mavd.honeywell.com says without -lsocket,
    # socket/setsockopt and other routines are undefined under SCO ODT
    # 2.0.  But -lsocket is broken on IRIX 5.2 (and is not necessary
    # on later versions), says Simon Leinen: it contains gethostby*
    # variants that don't use the name server (or something).  -lsocket
    # must be given before -lnsl if both are needed.  We assume that
    # if connect needs -lnsl, so does gethostbyname.
    AC_CHECK_FUNC(connect)
    if test $ac_cv_func_connect = no; then
      AC_CHECK_LIB(socket, connect, X_EXTRA_LIBS="-lsocket $X_EXTRA_LIBS", ,
	$X_EXTRA_LIBS)
    fi

    # Guillermo Gomez says -lposix is necessary on A/UX.
    AC_CHECK_FUNC(remove)
    if test $ac_cv_func_remove = no; then
      AC_CHECK_LIB(posix, remove, X_EXTRA_LIBS="$X_EXTRA_LIBS -lposix")
    fi

    # BSDI BSD/OS 2.1 needs -lipc for XOpenDisplay.
    AC_CHECK_FUNC(shmat)
    if test $ac_cv_func_shmat = no; then
      AC_CHECK_LIB(ipc, shmat, X_EXTRA_LIBS="$X_EXTRA_LIBS -lipc")
    fi
  fi

  # Check for libraries that X11R6 Xt/Xaw programs need.
  ac_save_LDFLAGS=$LDFLAGS
  test -n "$x_libraries" && LDFLAGS="$LDFLAGS -L$x_libraries"
  # SM needs ICE to (dynamically) link under SunOS 4.x (so we have to
  # check for ICE first), but we must link in the order -lSM -lICE or
  # we get undefined symbols.  So assume we have SM if we have ICE.
  # These have to be linked with before -lX11, unlike the other
  # libraries we check for below, so use a different variable.
  # John Interrante, Karl Berry
  AC_CHECK_LIB(ICE, IceConnectionNumber,
    [X_PRE_LIBS="$X_PRE_LIBS -lSM -lICE"], , $X_EXTRA_LIBS)
  LDFLAGS=$ac_save_LDFLAGS

fi
AC_SUBST(X_CFLAGS)dnl
AC_SUBST(X_PRE_LIBS)dnl
AC_SUBST(X_LIBS)dnl
AC_SUBST(X_EXTRA_LIBS)dnl
])# AC_PATH_XTRA
