#serial 1   -*- autoconf -*-
dnl Copyright (C) 2006 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_DOUBLE_SLASH_ROOT],
[
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_CACHE_CHECK([whether // is distinct from /], [ac_cv_double_slash_root],
    [ if test x"$cross_compiling" = xyes ; then
	# When cross-compiling, there is no way to tell whether // is special
	# short of a list of hosts.  However, the only known hosts to date
	# that have a distinct // are Apollo DomainOS (too old to port to)
	# and Cygwin.  If anyone knows of another system for which // has
	# special semantics and is distinct from /, please report it to
	# <bug-coreutils@gnu.org>.
	case $host in
	  *-cygwin)
	    ac_cv_double_slash_root=yes ;;
	  *)
	    # Be optimistic and assume that / and // are the same when we
	    # don't know.
	    ac_cv_double_slash_root='unknown, assuming no' ;;
	esac
      else
	set x `ls -di / //`
	if test $[2] = $[4]; then
	  ac_cv_double_slash_root=no
	else
	  ac_cv_double_slash_root=yes
	fi
      fi])
  if test x"$ac_cv_double_slash_root" = xyes; then
    ac_double_slash_root=1
  else
    ac_double_slash_root=0
  fi

  AC_DEFINE_UNQUOTED([DOUBLE_SLASH_IS_DISTINCT_ROOT],
   $ac_double_slash_root,
   [Define to 1 if // is a file system root distinct from /.])
])
