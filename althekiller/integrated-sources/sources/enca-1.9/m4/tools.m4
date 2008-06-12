## Some non-test macros. This file is in public domain.

## Set directories containing automake m4 macros (bootstraping).
## Defines: (nothing)
dnl AM_ACLOCAL_INCLUDE(macrodir)
AC_DEFUN([AM_ACLOCAL_INCLUDE],
[dnl Append aclocal flags and then add all specified dirs.
  test -n "$ACLOCAL_FLAGS" && ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"
  for k in $1 ; do ACLOCAL="$ACLOCAL -I $k" ; done
])

## Print warning.
## Defines: (nothing)
AC_DEFUN([ye_WARN_FAIL],
[dnl Test if given variable is yes and possibly print warning.
if test "$1" != yes; then
  AC_MSG_WARN(expect build to fail since we depend on $2)
fi])
