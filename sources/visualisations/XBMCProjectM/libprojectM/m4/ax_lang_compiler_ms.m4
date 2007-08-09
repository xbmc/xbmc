dnl @synopsis AX_LANG_COMPILER_MS
dnl
dnl Check whether the compiler for the current language is Microsoft.
dnl
dnl This macro is modeled after _AC_LANG_COMPILER_GNU in the GNU
dnl Autoconf implementation.
dnl
dnl @category InstalledPackages
dnl @author Braden McDaniel <braden@endoframe.com>
dnl @version 2004-11-15
dnl @license AllPermissive

AC_DEFUN([AX_LANG_COMPILER_MS],
[AC_CACHE_CHECK([whether we are using the Microsoft _AC_LANG compiler],
                [ax_cv_[]_AC_LANG_ABBREV[]_compiler_ms],
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [[#ifndef _MSC_VER
       choke me
#endif
]])],
                   [ax_compiler_ms=yes],
                   [ax_compiler_ms=no])
ax_cv_[]_AC_LANG_ABBREV[]_compiler_ms=$ax_compiler_ms
])])
