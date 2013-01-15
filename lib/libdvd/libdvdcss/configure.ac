AC_INIT(src/libdvdcss.c)

AC_PREREQ(2.50)
AC_CONFIG_AUX_DIR(.auto)
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE(libdvdcss, 1.2.10)
AC_CONFIG_HEADERS(config.h)

AC_PROG_CC
AC_STDC_HEADERS

AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

AC_CHECK_HEADERS(unistd.h sys/param.h limits.h pwd.h errno.h)

dnl
dnl  Check the operating system
dnl
case x"${target_os}" in
  xdarwin*)
    CFLAGS="${CFLAGS} -no-cpp-precomp"
    ;;
  x*cygwin*)
    dnl Check if we are using the mno-cygwin mode in which case we are
    dnl actually dealing with a mingw32 compiler.
    AC_EGREP_CPP(yes, [#ifndef WIN32
                       yes
                       #endif],
                 AC_DEFINE(SYS_CYGWIN, 1, Have a Cygwin system.))
    AC_DEFINE(WIN32, 1, Using Win32.)
    ;;
  xbeos*)
    AC_DEFINE(SYS_BEOS, 1, Have a BeOS system.)
    ;;
  x*msvc*)
    SYS_MSVC=1
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
  AC_CHECK_HEADERS(direct.h,,,[
  #include <windows.h>
  ])
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
  dnl BSDI: dvd.h - may require -ldvd IF a local copy of libdvd is installed
  dnl
  AC_EGREP_HEADER(dvd_struct,dvd.h,[
    AC_DEFINE(DVD_STRUCT_IN_DVD_H, 1,
              Define if <dvd.h> defines DVD_STRUCT.)
    SYS_BSDI_LIBDVD=1
    LINUX_DVD_STRUCT=1
  ],[
    dnl
    dnl BSDI: /sys/dev/scsi/scsi_ioctl.h, using our own libdvd
    dnl
    AC_CHECK_HEADERS(/sys/dev/scsi/scsi_ioctl.h,[
      AC_DEFINE(DVD_STRUCT_IN_BSDI_DVDIOCTL_DVD_H, 1,
                Define if <extras/BSDI_dvdioctl/dvd.h> defines DVD_STRUCT.)
      SYS_BSDI=1
      LINUX_DVD_STRUCT=1
    ])
  ])
  dnl
  dnl Solaris: sys/scsi/scsi_types.h, sys/scsi/impl/uscsi.h
  dnl
  AC_CHECK_HEADER(sys/scsi/scsi_types.h,[
    AC_CHECK_HEADER(sys/scsi/impl/uscsi.h,[
      AC_DEFINE(SOLARIS_USCSI, 1, Have userspace SCSI headers.)
    ])
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

dnl
dnl  Check for available warning flags
dnl
CFLAGS_save="${CFLAGS}"

AC_CACHE_CHECK([if \$CC accepts -Wall],
    [ac_cv_c_Wall],
    [CFLAGS="-Wall ${CFLAGS_save}"
     AC_TRY_COMPILE([],,ac_cv_c_Wall=yes, ac_cv_c_Wall=no)])
if test "${ac_cv_c_Wall}" != "no"; then
    CFLAGS_save="-Wall ${CFLAGS_save}"
fi

AC_CACHE_CHECK([if \$CC accepts -Wsign-compare],
    [ac_cv_c_Wsign_compare],
    [CFLAGS="-Wsign-compare ${CFLAGS_save}"
     AC_TRY_COMPILE([],,ac_cv_c_Wsign_compare=yes, ac_cv_c_Wsign_compare=no)])
if test "${ac_cv_c_Wsign_compare}" != "no"; then
    CFLAGS_save="-Wsign-compare ${CFLAGS_save}"
fi

CFLAGS="${CFLAGS_save}"

dnl
dnl  Use 64 bits offsets
dnl
CFLAGS="-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DDVDCSS_DIST ${CFLAGS}"

AM_CONDITIONAL(SYS_BSDI, test "${SYS_BSDI}" = "1")
AM_CONDITIONAL(SYS_BSDI_LIBDVD, test "${SYS_BSDI_LIBDVD}" = "1")
AM_CONDITIONAL(SYS_MSVC, test "${SYS_MSVC}" = "1")

dnl
dnl  Shall we build documentation?
dnl
AC_ARG_ENABLE([doc], AC_HELP_STRING([--disable-doc], [do not build API documentation with Doxygen]))
if test "$enable_doc" != "no"; then
  AC_PATH_PROG(DOXYGEN, doxygen, no)
  AC_PATH_PROG(LATEX, latex, no)
else
  DOXYGEN="no"
  LATEX="no"
fi
AM_CONDITIONAL(DOXYGEN, test "${DOXYGEN}" != "no")
AM_CONDITIONAL(LATEX, test "${LATEX}" != "no")

AC_SUBST(DVDCSS_LDFLAGS)
AC_OUTPUT([
  Makefile
  doc/Makefile
  src/Makefile
  src/dvdcss/Makefile
  test/Makefile
  src/libdvdcss.pc
])

