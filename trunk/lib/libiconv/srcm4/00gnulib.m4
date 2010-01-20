# 00gnulib.m4 serial 2
dnl Copyright (C) 2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This file must be named something that sorts before all other
dnl gnulib-provided .m4 files.  It is needed until such time as we can
dnl assume Autoconf 2.64, with its improved AC_DEFUN_ONCE semantics.

# AC_DEFUN_ONCE([NAME], VALUE)
# ----------------------------
# Define NAME to expand to VALUE on the first use (whether by direct
# expansion, or by AC_REQUIRE), and to nothing on all subsequent uses.
# Avoid bugs in AC_REQUIRE in Autoconf 2.63 and earlier.  This
# definition is slower than the version in Autoconf 2.64, because it
# can only use interfaces that existed since 2.59; but it achieves the
# same effect.  Quoting is necessary to avoid confusing Automake.
m4_version_prereq([2.63.263], [],
[m4_define([AC][_DEFUN_ONCE],
  [AC][_DEFUN([$1],
    [AC_REQUIRE([_gl_DEFUN_ONCE([$1])],
      [m4_indir([_gl_DEFUN_ONCE([$1])])])])]dnl
[AC][_DEFUN([_gl_DEFUN_ONCE([$1])], [$2])])])

# gl_00GNULIB
# -----------
# Witness macro that this file has been included.  Needed to force
# Automake to include this file prior to all other gnulib .m4 files.
AC_DEFUN([gl_00GNULIB])
