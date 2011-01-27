# relocatable.m4 serial 14
dnl Copyright (C) 2003, 2005-2007, 2009 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Bruno Haible.

dnl gl_RELOCATABLE([RELOCWRAPPER-DIR])
dnl ----------------------------------------------------------
dnl Support for relocatable programs.
dnl Supply RELOCWRAPPER-DIR as the directory where relocwrapper.c may be found.
AC_DEFUN([gl_RELOCATABLE],
[
  AC_REQUIRE([gl_RELOCATABLE_BODY])
  gl_RELOCATABLE_LIBRARY
  if test $RELOCATABLE = yes; then
    AC_LIBOBJ([progreloc])
  fi
  : ${RELOCATABLE_CONFIG_H_DIR='$(top_builddir)'}
  RELOCATABLE_SRC_DIR="\$(top_srcdir)/$gl_source_base"
  RELOCATABLE_BUILD_DIR="\$(top_builddir)/$gl_source_base"
])
dnl The guts of gl_RELOCATABLE. Needs to be expanded only once.
AC_DEFUN([gl_RELOCATABLE_BODY],
[
  AC_REQUIRE([AC_PROG_INSTALL])
  dnl This AC_BEFORE invocation leads to unjustified autoconf warnings
  dnl when gl_RELOCATABLE_BODY is invoked more than once.
  dnl We need this AC_BEFORE because AC_PROG_INSTALL is documented to
  dnl overwrite earlier settings of INSTALL and INSTALL_PROGRAM (even
  dnl though in autoconf-2.52..2.60 it doesn't do so), but we want this
  dnl macro's setting of INSTALL_PROGRAM to persist.
  AC_BEFORE([AC_PROG_INSTALL],[gl_RELOCATABLE_BODY])
  AC_REQUIRE([AC_LIB_LIBPATH])
  AC_REQUIRE([gl_RELOCATABLE_LIBRARY_BODY])
  is_noop=no
  use_elf_origin_trick=no
  if test $RELOCATABLE = yes; then
    # --enable-relocatable implies --disable-rpath
    enable_rpath=no
    AC_CHECK_HEADERS([mach-o/dyld.h])
    AC_CHECK_FUNCS([_NSGetExecutablePath])
    case "$host_os" in
      mingw*) is_noop=yes ;;
      linux*) use_elf_origin_trick=yes ;;
    esac
    if test $is_noop = yes; then
      RELOCATABLE_LDFLAGS=:
      AC_SUBST([RELOCATABLE_LDFLAGS])
    else
      if test $use_elf_origin_trick = yes; then
        dnl Use the dynamic linker's support for relocatable programs.
        case "$ac_aux_dir" in
          /*) reloc_ldflags="$ac_aux_dir/reloc-ldflags" ;;
          *) reloc_ldflags="\$(top_builddir)/$ac_aux_dir/reloc-ldflags" ;;
        esac
        RELOCATABLE_LDFLAGS="\"$reloc_ldflags\" \"\$(host)\" \"\$(RELOCATABLE_LIBRARY_PATH)\""
        AC_SUBST([RELOCATABLE_LDFLAGS])
      else
        dnl Unfortunately we cannot define INSTALL_PROGRAM to a command
        dnl consisting of more than one word - libtool doesn't support this.
        dnl So we abuse the INSTALL_PROGRAM_ENV hook, originally meant for the
        dnl 'install-strip' target.
        INSTALL_PROGRAM_ENV="RELOC_LIBRARY_PATH_VAR=\"$shlibpath_var\" RELOC_LIBRARY_PATH_VALUE=\"\$(RELOCATABLE_LIBRARY_PATH)\" RELOC_PREFIX=\"\$(prefix)\" RELOC_DESTDIR=\"\$(DESTDIR)\" RELOC_COMPILE_COMMAND=\"\$(CC) \$(CPPFLAGS) \$(CFLAGS) \$(LDFLAGS)\" RELOC_SRCDIR=\"\$(RELOCATABLE_SRC_DIR)\" RELOC_BUILDDIR=\"\$(RELOCATABLE_BUILD_DIR)\" RELOC_CONFIG_H_DIR=\"\$(RELOCATABLE_CONFIG_H_DIR)\" RELOC_EXEEXT=\"\$(EXEEXT)\" RELOC_STRIP_PROG=\"\$(RELOCATABLE_STRIP)\" RELOC_INSTALL_PROG=\"$INSTALL_PROGRAM\""
        AC_SUBST([INSTALL_PROGRAM_ENV])
        case "$ac_aux_dir" in
          /*) INSTALL_PROGRAM="$ac_aux_dir/install-reloc" ;;
          *) INSTALL_PROGRAM="\$(top_builddir)/$ac_aux_dir/install-reloc" ;;
        esac
      fi
    fi
  fi
  AM_CONDITIONAL([RELOCATABLE_VIA_LD],
    [test $is_noop = yes || test $use_elf_origin_trick = yes])

  dnl RELOCATABLE_LIBRARY_PATH can be set in configure.ac. Default is empty.
  AC_SUBST([RELOCATABLE_LIBRARY_PATH])
  AC_SUBST([RELOCATABLE_CONFIG_H_DIR])
  AC_SUBST([RELOCATABLE_SRC_DIR])
  AC_SUBST([RELOCATABLE_BUILD_DIR])
])

dnl Determine the platform dependent parameters needed to use relocatability:
dnl shlibpath_var.
AC_DEFUN([AC_LIB_LIBPATH],
[
  AC_REQUIRE([AC_LIB_PROG_LD])            dnl we use $LD
  AC_REQUIRE([AC_CANONICAL_HOST])         dnl we use $host
  AC_REQUIRE([AC_CONFIG_AUX_DIR_DEFAULT]) dnl we use $ac_aux_dir
  AC_CACHE_CHECK([for shared library path variable], [acl_cv_libpath], [
    LD="$LD" \
    ${CONFIG_SHELL-/bin/sh} "$ac_aux_dir/config.libpath" "$host" > conftest.sh
    . ./conftest.sh
    rm -f ./conftest.sh
    acl_cv_libpath=${acl_cv_shlibpath_var:-none}
  ])
  shlibpath_var="$acl_cv_shlibpath_var"
])
