# make-case.m4 serial 1

# Copyright (C) 2008, 2009, 2010 Free Software Foundation, Inc.

# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the notice and
# this notice are preserved.  This file is offered as-is, without
# warranty of any kind.

# AC_PROG_MAKE_CASE_SENSITIVE
# ---------------------------
# Checks whether make is configured to be case insensitive; if yes,
# sets AM_CONDITIONAL MAKE_CASE_SENSITIVE.
#
AC_DEFUN([AC_PROG_MAKE_CASE_SENSITIVE],
[AC_REQUIRE([AC_PROG_MAKE_SET])dnl
AC_CACHE_CHECK([whether ${MAKE-make} is case sensitive],
[ac_cv_prog_make_${ac_make}_case],
[echo all: >conftest.make
if ${MAKE-make} -f conftest.make ALL >/dev/null 2>&1; then
  ac_res=no
else
  ac_res=yes
fi
eval ac_cv_prog_make_${ac_make}_case=$ac_res
rm -f conftest.make])
AM_CONDITIONAL([MAKE_CASE_SENSITIVE],
  [eval test \$ac_cv_prog_make_${ac_make}_case = yes])
])
