AC_INIT(libdvdcss, 1.2.13)
AC_CONFIG_SRCDIR([src/libdvdcss.c])

AC_PREREQ(2.50)
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([foreign dist-bzip2 no-dist-gzip subdir-objects])
AM_MAINTAINER_MODE([enable])
dnl Enable silent rules only when available (automake 1.11 or later).
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

LT_INIT([win32-dll])

AC_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

dnl The libtool version numbers (DVDCSS_LT_*); Don't even think about faking this!
dnl
dnl immediately before every release do:
dnl ===================================
dnl if (the interface is totally unchanged from previous release)
dnl    DVDCSS_LT_REVISION ++;
dnl else { /* interfaces have been added, removed or changed */
dnl    DVDCSS_LT_REVISION = 0;
dnl    DVDCSS_LT_CURRENT ++;
dnl    if (any interfaces have been _added_ since last release)
dnl       AGE ++;
dnl    if (any interfaces have been _removed_ or _incompatibly changed_)
dnl       AGE = 0;
dnl }
dnl
dnl If you want to know more about what you are doing, here are some details:
dnl  * DVDCSS_LT_CURRENT is the current API version
dnl  * DVDCSS_LT_REVISION is an internal revision number which is increased when the API
dnl    itself did not change
dnl  * DVDCSS_LT_AGE is the number of previous API versions still supported by this library
dnl  * libtool has its own numbering scheme, because local library numbering schemes
dnl    are platform dependent
dnl  * in Linux, the library will be named
dnl    libname.so.(DVDCSS_LT_CURRENT - DVDCSS_LT_AGE).DVDCSS_LT_AGE.DVDCSS_LT_REVISION

DVDCSS_LT_CURRENT=3
DVDCSS_LT_AGE=0
DVDCSS_LT_REVISION=1

AC_SUBST([DVDCSS_LTVERSION], [$DVDCSS_LT_CURRENT:$DVDCSS_LT_AGE:$DVDCSS_LT_REVISION])

AC_PROG_CC
AC_HEADER_STDC

AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

AC_CHECK_HEADERS([unistd.h sys/param.h sys/uio.h limits.h pwd.h]dnl
  [errno.h sys/types.h sys/stat.h fcntl.h io.h])

AC_CHECK_DECL([O_BINARY], [],
  [AC_DEFINE([O_BINARY], [0], [Define O_BINARY if missing])],
  [
    #if HAVE_SYS_TYPES_H
    # include <sys/types.h>
    #endif
    #if HAVE_SYS_STAT_H
    # include <sys/stat.h>
    #endif
    #if HAVE_FCNTL_H
    # include <fcntl.h>
    #endif
  ])

AC_SYS_LARGEFILE

AC_CACHE_CHECK(
  [for posix mkdir()],
  [css_cv_mkdir_posix], [
    AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#ifdef HAVE_IO_H
# include <io.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

int mkdir(const char *path, mode_t mode);
      ])
    ],
    [css_cv_mkdir_posix=yes],
    [css_cv_mkdir_posix=no])
  ])

AS_IF([test "$css_cv_mkdir_posix" = "no"], [
  AC_DEFINE([HAVE_BROKEN_MKDIR], 1, [Define if you have a broken mkdir])
])

dnl
dnl  Check the operating system
dnl

case x"${host_os}" in
  xdarwin*)
    CFLAGS="${CFLAGS} -no-cpp-precomp"
    ;;
  x*cygwin*)
    CFLAGS="${CFLAGS} -mwin32"
    ;;
  xos2*)
    LDFLAGS="-Zbin-files"
    ;;
  x*msvc*)
    SYS_MSVC=1
    AC_DEFINE([_WIN32_IE], 0x0401, [Define to '0x0401' for IE 4.01 (and shell) APIs.])
    ;;
  x*mingw* | *wince* | *mingwce*)
    AC_DEFINE([_WIN32_IE], 0x0401, [Define to '0x0401' for IE 4.01 (and shell) APIs.])
    ;;
  x*)
    ;;
esac

dnl
dnl  libdvdcss: check for DVD ioctls
dnl

dnl  default is no
CAN_BUILD_LIBDVDCSS=0

dnl  for windoze
AC_CHECK_HEADERS(windows.h,[
  AC_CHECK_HEADERS(winioctl.h,[
    CAN_BUILD_LIBDVDCSS=1
  ],,[
  #include <windows.h>
  ])
])

dnl  for Un*x and BeOS
AC_CHECK_HEADERS(sys/ioctl.h,[
  CAN_BUILD_LIBDVDCSS=1
  AC_CHECK_HEADERS(sys/cdio.h sys/dvdio.h linux/cdrom.h dvd.h)
  BSD_DVD_STRUCT=0
  LINUX_DVD_STRUCT=0
  OPENBSD_DVD_STRUCT=0
  dnl
  dnl Old FreeBSD: sys/cdio.h
  dnl
  AC_EGREP_HEADER(dvd_struct,sys/cdio.h,[
    AC_DEFINE(DVD_STRUCT_IN_SYS_CDIO_H, 1,
              Define if <sys/cdio.h> defines dvd_struct.)
    AC_EGREP_HEADER(struct dvd_struct,sys/cdio.h,[
                    BSD_DVD_STRUCT=1],[LINUX_DVD_STRUCT=1; OPENBSD_DVD_STRUCT=1])
  ])
  dnl
  dnl Newer FreeBSD: sys/dvdio.h
  dnl
  AC_EGREP_HEADER(dvd_struct,sys/dvdio.h,[
    AC_DEFINE(DVD_STRUCT_IN_SYS_DVDIO_H, 1,
              Define if <sys/dvdio.h> defines dvd_struct.)
    AC_EGREP_HEADER(struct dvd_struct,sys/dvdio.h,[
                    BSD_DVD_STRUCT=1],[LINUX_DVD_STRUCT=1])
  ])
  dnl
  dnl Linux: linux/cdrom.h
  dnl
  AC_EGREP_HEADER(dvd_struct,linux/cdrom.h,[
    AC_DEFINE(DVD_STRUCT_IN_LINUX_CDROM_H, 1,
              Define if <linux/cdrom.h> defines DVD_STRUCT.)
    LINUX_DVD_STRUCT=1
  ])
  dnl
  dnl Solaris: sys/scsi/scsi_types.h, sys/scsi/impl/uscsi.h
  dnl
  solaris_uscsi=yes
  AC_CHECK_HEADERS([sys/scsi/scsi_types.h, sys/scsi/impl/uscsi.h], [],
    [solaris_uscsi=no; break;])
  AS_IF([test "$solaris_uscsi" = "yes"], [
      AC_DEFINE(SOLARIS_USCSI, 1, Have userspace SCSI headers.)
    ])
  dnl
  dnl HP-UX: sys/scsi.h
  dnl
  AC_CHECK_HEADER(sys/scsi.h,[
    AC_EGREP_HEADER(sctl_io,sys/scsi.h,[
      AC_DEFINE(HPUX_SCTL_IO, 1, Define if <sys/scsi.h> defines sctl_io.)
    ])
  ])
  dnl
  dnl Darwin 
  dnl
  AC_CHECK_HEADER(IOKit/storage/IODVDMediaBSDClient.h,[
    AC_DEFINE(DARWIN_DVD_IOCTL, 1, Have IOKit DVD IOCTL headers)
    DVDCSS_LDFLAGS="-Wl,-framework -Wl,CoreFoundation -Wl,-framework -Wl,IOKit"
  ])
  dnl
  dnl Final tests to check what was detected
  dnl
  if test x$LINUX_DVD_STRUCT = x1; then
    AC_DEFINE(HAVE_LINUX_DVD_STRUCT, 1, 
              Define if Linux-like dvd_struct is defined.)
    if test x$OPENBSD_DVD_STRUCT = x1; then
      AC_DEFINE(HAVE_OPENBSD_DVD_STRUCT, 1,
                Define if OpenBSD-like dvd_struct is defined.)
    fi
  else
    if test x$BSD_DVD_STRUCT = x1; then
      AC_DEFINE(HAVE_BSD_DVD_STRUCT, 1,
                Define if FreeBSD-like dvd_struct is defined.)
    fi
  fi
])

CC_CHECK_CFLAGS_APPEND([-Wall -Wsign-compare])
CC_FLAG_VISIBILITY([
  CC_ATTRIBUTE_VISIBILITY([default], [
    CFLAGS="${CFLAGS} -fvisibility=hidden"
  ])
])

AM_CONDITIONAL(SYS_MSVC, test "${SYS_MSVC}" = "1")

AC_ARG_ENABLE([doc],
  AS_HELP_STRING([--disable-apidoc], [Disable building (with Doxygen) and intalling API documentation @<:@default=auto@:@>]))

AC_PATH_PROG([DOXYGEN], [doxygen])

AS_IF([test "x$DOXYGEN" = "x"], [
  AS_IF([test "x$enable_apidoc" = "xyes"], [
    AC_MSG_ERROR([You need Doxygen to build API documentation])
  ])
])
AM_CONDITIONAL([APIDOC], [test "x$DOXYGEN" != "x"])

AC_SUBST(DVDCSS_LDFLAGS)
AC_OUTPUT([
  Makefile
  doc/doxygen.cfg
  src/libdvdcss.pc
])
