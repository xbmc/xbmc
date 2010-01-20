# include_next.m4 serial 14
dnl Copyright (C) 2006-2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Paul Eggert and Derek Price.

dnl Sets INCLUDE_NEXT and PRAGMA_SYSTEM_HEADER.
dnl
dnl INCLUDE_NEXT expands to 'include_next' if the compiler supports it, or to
dnl 'include' otherwise.
dnl
dnl INCLUDE_NEXT_AS_FIRST_DIRECTIVE expands to 'include_next' if the compiler
dnl supports it in the special case that it is the first include directive in
dnl the given file, or to 'include' otherwise.
dnl
dnl PRAGMA_SYSTEM_HEADER can be used in files that contain #include_next,
dnl so as to avoid GCC warnings when the gcc option -pedantic is used.
dnl '#pragma GCC system_header' has the same effect as if the file was found
dnl through the include search path specified with '-isystem' options (as
dnl opposed to the search path specified with '-I' options). Namely, gcc
dnl does not warn about some things, and on some systems (Solaris and Interix)
dnl __STDC__ evaluates to 0 instead of to 1. The latter is an undesired side
dnl effect; we are therefore careful to use 'defined __STDC__' or '1' instead
dnl of plain '__STDC__'.

AC_DEFUN([gl_INCLUDE_NEXT],
[
  AC_LANG_PREPROC_REQUIRE()
  AC_CACHE_CHECK([whether the preprocessor supports include_next],
    [gl_cv_have_include_next],
    [rm -rf conftestd1a conftestd1b conftestd2
     mkdir conftestd1a conftestd1b conftestd2
     dnl IBM C 9.0, 10.1 (original versions, prior to the 2009-01 updates) on
     dnl AIX 6.1 support include_next when used as first preprocessor directive
     dnl in a file, but not when preceded by another include directive. Check
     dnl for this bug by including <stdio.h>.
     dnl Additionally, with this same compiler, include_next is a no-op when
     dnl used in a header file that was included by specifying its absolute
     dnl file name. Despite these two bugs, include_next is used in the
     dnl compiler's <math.h>. By virtue of the second bug, we need to use
     dnl include_next as well in this case.
     cat <<EOF > conftestd1a/conftest.h
#define DEFINED_IN_CONFTESTD1
#include_next <conftest.h>
#ifdef DEFINED_IN_CONFTESTD2
int foo;
#else
#error "include_next doesn't work"
#endif
EOF
     cat <<EOF > conftestd1b/conftest.h
#define DEFINED_IN_CONFTESTD1
#include <stdio.h>
#include_next <conftest.h>
#ifdef DEFINED_IN_CONFTESTD2
int foo;
#else
#error "include_next doesn't work"
#endif
EOF
     cat <<EOF > conftestd2/conftest.h
#ifndef DEFINED_IN_CONFTESTD1
#error "include_next test doesn't work"
#endif
#define DEFINED_IN_CONFTESTD2
EOF
     gl_save_CPPFLAGS="$CPPFLAGS"
     CPPFLAGS="$gl_save_CPPFLAGS -Iconftestd1b -Iconftestd2"
     AC_COMPILE_IFELSE([#include <conftest.h>],
       [gl_cv_have_include_next=yes],
       [CPPFLAGS="$gl_save_CPPFLAGS -Iconftestd1a -Iconftestd2"
        AC_COMPILE_IFELSE([#include <conftest.h>],
          [gl_cv_have_include_next=buggy],
          [gl_cv_have_include_next=no])
       ])
     CPPFLAGS="$gl_save_CPPFLAGS"
     rm -rf conftestd1a conftestd1b conftestd2
    ])
  PRAGMA_SYSTEM_HEADER=
  if test $gl_cv_have_include_next = yes; then
    INCLUDE_NEXT=include_next
    INCLUDE_NEXT_AS_FIRST_DIRECTIVE=include_next
    if test -n "$GCC"; then
      PRAGMA_SYSTEM_HEADER='#pragma GCC system_header'
    fi
  else
    if test $gl_cv_have_include_next = buggy; then
      INCLUDE_NEXT=include
      INCLUDE_NEXT_AS_FIRST_DIRECTIVE=include_next
    else
      INCLUDE_NEXT=include
      INCLUDE_NEXT_AS_FIRST_DIRECTIVE=include
    fi
  fi
  AC_SUBST([INCLUDE_NEXT])
  AC_SUBST([INCLUDE_NEXT_AS_FIRST_DIRECTIVE])
  AC_SUBST([PRAGMA_SYSTEM_HEADER])
])

# gl_CHECK_NEXT_HEADERS(HEADER1 HEADER2 ...)
# ------------------------------------------
# For each arg foo.h, if #include_next works, define NEXT_FOO_H to be
# '<foo.h>'; otherwise define it to be
# '"///usr/include/foo.h"', or whatever other absolute file name is suitable.
# Also, if #include_next works as first preprocessing directive in a file,
# define NEXT_AS_FIRST_DIRECTIVE_FOO_H to be '<foo.h>'; otherwise define it to
# be
# '"///usr/include/foo.h"', or whatever other absolute file name is suitable.
# That way, a header file with the following line:
#	#@INCLUDE_NEXT@ @NEXT_FOO_H@
# or
#	#@INCLUDE_NEXT_AS_FIRST_DIRECTIVE@ @NEXT_AS_FIRST_DIRECTIVE_FOO_H@
# behaves (after sed substitution) as if it contained
#	#include_next <foo.h>
# even if the compiler does not support include_next.
# The three "///" are to pacify Sun C 5.8, which otherwise would say
# "warning: #include of /usr/include/... may be non-portable".
# Use `""', not `<>', so that the /// cannot be confused with a C99 comment.
# Note: This macro assumes that the header file is not empty after
# preprocessing, i.e. it does not only define preprocessor macros but also
# provides some type/enum definitions or function/variable declarations.
AC_DEFUN([gl_CHECK_NEXT_HEADERS],
[
  AC_REQUIRE([gl_INCLUDE_NEXT])
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_CHECK_HEADERS_ONCE([$1])

  m4_foreach_w([gl_HEADER_NAME], [$1],
    [AS_VAR_PUSHDEF([gl_next_header],
		    [gl_cv_next_]m4_defn([gl_HEADER_NAME]))
     if test $gl_cv_have_include_next = yes; then
       AS_VAR_SET([gl_next_header], ['<'gl_HEADER_NAME'>'])
     else
       AC_CACHE_CHECK(
	 [absolute name of <]m4_defn([gl_HEADER_NAME])[>],
	 m4_defn([gl_next_header]),
	 [AS_VAR_PUSHDEF([gl_header_exists],
			 [ac_cv_header_]m4_defn([gl_HEADER_NAME]))
	  if test AS_VAR_GET(gl_header_exists) = yes; then
	    AC_LANG_CONFTEST(
	      [AC_LANG_SOURCE(
		 [[#include <]]m4_dquote(m4_defn([gl_HEADER_NAME]))[[>]]
	       )])
	    dnl AIX "xlc -E" and "cc -E" omit #line directives for header files
	    dnl that contain only a #include of other header files and no
	    dnl non-comment tokens of their own. This leads to a failure to
	    dnl detect the absolute name of <dirent.h>, <signal.h>, <poll.h>
	    dnl and others. The workaround is to force preservation of comments
	    dnl through option -C. This ensures all necessary #line directives
	    dnl are present. GCC supports option -C as well.
	    case "$host_os" in
	      aix*) gl_absname_cpp="$ac_cpp -C" ;;
	      *)    gl_absname_cpp="$ac_cpp" ;;
	    esac
	    dnl eval is necessary to expand gl_absname_cpp.
	    dnl Ultrix and Pyramid sh refuse to redirect output of eval,
	    dnl so use subshell.
	    AS_VAR_SET([gl_next_header],
	      ['"'`(eval "$gl_absname_cpp conftest.$ac_ext") 2>&AS_MESSAGE_LOG_FD |
	       sed -n '\#/]m4_defn([gl_HEADER_NAME])[#{
		 s#.*"\(.*/]m4_defn([gl_HEADER_NAME])[\)".*#\1#
		 s#^/[^/]#//&#
		 p
		 q
	       }'`'"'])
	  else
	    AS_VAR_SET([gl_next_header], ['<'gl_HEADER_NAME'>'])
	  fi
	  AS_VAR_POPDEF([gl_header_exists])])
     fi
     AC_SUBST(
       AS_TR_CPP([NEXT_]m4_defn([gl_HEADER_NAME])),
       [AS_VAR_GET([gl_next_header])])
     if test $gl_cv_have_include_next = yes || test $gl_cv_have_include_next = buggy; then
       # INCLUDE_NEXT_AS_FIRST_DIRECTIVE='include_next'
       gl_next_as_first_directive='<'gl_HEADER_NAME'>'
     else
       # INCLUDE_NEXT_AS_FIRST_DIRECTIVE='include'
       gl_next_as_first_directive=AS_VAR_GET([gl_next_header])
     fi
     AC_SUBST(
       AS_TR_CPP([NEXT_AS_FIRST_DIRECTIVE_]m4_defn([gl_HEADER_NAME])),
       [$gl_next_as_first_directive])
     AS_VAR_POPDEF([gl_next_header])])
])
