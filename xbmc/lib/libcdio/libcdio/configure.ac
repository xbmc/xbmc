dnl Copyright (C) 2003, 2004, 2005, 2006, 2007 Rocky Bernstein <rocky@gnu.org>
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
dnl 02110-1301 USA.

define(RELEASE_NUM, 80)
define(CDIO_VERSION_STR, 0.$1)

AC_PREREQ(2.52)
AC_REVISION([$Id: configure.ac,v 1.221 2008/03/15 17:45:17 rocky Exp $])dnl
AC_INIT(libcdio, CDIO_VERSION_STR(RELEASE_NUM))
AC_CONFIG_SRCDIR(src/cd-info.c)

dnl For multilanguage manual pages (e.g. japanese cdparaoia) we use
dnl $make_p and according to Steve Schultz this is a make_p is a 1.8
dnl feature.

AM_INIT_AUTOMAKE([1.7])

AC_CANONICAL_HOST
AM_CONFIG_HEADER(config.h)

LIBCDIO_VERSION_NUM=`echo RELEASE_NUM | cut -d . -f 1 | tr -d a-z`
AC_SUBST(LIBCDIO_VERSION_NUM)

AM_MISSING_PROG(HELP2MAN, help2man, $missing_dir)
AM_MISSING_PROG(CVS2CL, cvs2cl, $missing_dir)
AM_MAINTAINER_MODE

AM_SANITY_CHECK

AC_ARG_WITH(cd_drive,
[  --without-cd-drive       don't build program cd-drive (default with)],
enable_cd_drive="${withval}", enable_cd_drive=yes)

AC_ARG_WITH(cd_info,
[  --without-cd-info        don't build program cd-info (default with)],
enable_cd_info="${withval}", enable_cd_info=yes)

AC_ARG_WITH(cd_paranoia,
[  --without-cd-paranoia    don't build program cd-paranoia and paranoia libraries (default with)],
enable_cd_paranoia="${withval}", enable_cd_paranoia=yes)

AC_ARG_WITH(cdda_player,
[  --without-cdda-player    don't build program cdda-player (default with)],
enable_cdda_player="${withval}", enable_cdda_player=yes)

AC_ARG_WITH(cd_paranoia_name,
[  --with-cd-paranoia-name  name to use as the cd-paranoia program name (default cd-paranoia)],
cd_paranoia_name="${withval}", cd_paranoia_name="cd-paranoia")
CDPARANOIA_NAME="$cd_paranoia_name"
AC_SUBST(CDPARANOIA_NAME)

AC_ARG_WITH(cd_read,
[  --without-cd-read        don't build program cd-read (default with)],
enable_cd_read="${withval}", enable_cd_read=yes)

AC_ARG_WITH(iso_info,
[  --without-iso-info       don't build program iso-info (default with)],
enable_iso_info="${withval}", enable_iso_info=yes)

AC_ARG_WITH(iso_read,
[  --without-iso-read       don't build program iso-read (default with)],
enable_iso_read="${withval}", enable_iso_read=yes)

AC_ARG_WITH(versioned_libs,
[  --without-versioned-libs build versioned library symbols (default enabled if you have GNU ld)],
enable_versioned_libs="${withval}", enable_versioned_libs=yes)

AC_ARG_ENABLE([cxx],
	AC_HELP_STRING([--disable-cxx], [Disable C++ bindings (default enabled)]))
AM_CONDITIONAL([ENABLE_CXX_BINDINGS], [test "x$enable_cxx" != "xno"])

AC_ARG_ENABLE(cpp_progs,
	[  --enable-cpp-progs      make C++ example programs (default enabled)])
AM_CONDITIONAL(ENABLE_CPP, test x"$enable_cpp_progs" = "xyes")

AC_ARG_ENABLE(example_progs,
	AC_HELP_STRING([--disable-example-progs], [Disable libcdio example programs building]))

AM_CONDITIONAL(BUILD_EXAMPLES, test "x$enable_example_progs" != "xno")

dnl We use C 
AC_PROG_CC
AM_PROG_CC_C_O
dnl We also use C++ in example programs and for CXX bindings
AC_PROG_CXX

dnl Checks for programs.
AC_AIX

cd_drivers='cdrdao, BIN/CUE, NRG'

if test "x$GCC" != "xyes" 
then
  echo "*** non GNU CC compiler detected."
  echo "*** This package has not been tested very well with non GNU compilers"
  echo "*** you should try to use 'gcc' for compiling this package."
else
  WARN_CFLAGS="-Wall -Wchar-subscripts -Wmissing-prototypes -Wmissing-declarations -Wunused -Wpointer-arith -Wwrite-strings -Wnested-externs -Wno-sign-compare"

  for WOPT in $WARN_CFLAGS; do
    SAVE_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS $WOPT"
    AC_MSG_CHECKING([whether $CC understands $WOPT])
    AC_TRY_COMPILE([], [], has_option=yes, has_option=no,)
    CFLAGS="$SAVE_CFLAGS"
    AC_MSG_RESULT($has_option)
    if test $has_option = yes; then
      warning_flags="$warning_flags $option"
    fi
    unset has_option
    unset SAVE_CFLAGS
  done
  WARNING_FLAGS="$warning_flags"
  unset warning_flags
fi

# We use Perl for documentation and regression tests
AC_PATH_PROG(PERL, perl, false)
AC_SUBST(PERL)
AM_CONDITIONAL(HAVE_PERL, test "$PERL" != "false")

# We use a diff in regression testing
AC_PATH_PROG(DIFF, diff, no)
DIFF_OPTS=

if test "$DIFF" = no ; then
   AC_PATH_PROG(DIFF, cmp, no)
else 
  # Try for GNU diff options.
  # MSDOG output uses \r\n rather than \n in tests
  for diff_opt in -w --unified ; do 
    if $DIFF $diff_opt ./configure ./configure > /dev/null 2>&1; then
      AC_MSG_RESULT([adding $diff_opt to diff in regression tests])
      DIFF_OPTS="$DIFF_OPTS $diff_opt"
    fi
  done
fi
AC_SUBST(DIFF)
AC_SUBST(DIFF_OPTS)

dnl check for large file support
AC_SYS_LARGEFILE

dnl we need to define _FILE_OFFSET_BITS or _LARGE_FILES on the compiler command
dnl line because otherwise the system headers risk being included before
dnl problems
if test "x$ac_cv_sys_largefiles" = "xyes"; then
    if test "x$ac_cv_sys_file_offset_bits" = "x64"; then
        LIBCDIO_LARGEFILE_FLAGS="-D_FILE_OFFSET_BITS=64 -D_LARGE_FILES"
    else
        LIBCDIO_LARGEFILE_FLAGS="-D_LARGE_FILES"
    fi
    dnl AC_FUNC_FSEEKO sets HAVE_FSEEKO and $ac_cv_sys_largefile_source
    AC_FUNC_FSEEKO
    if test "$ac_cv_sys_largefile_source" != no; then
        LIBCDIO_LARGEFILE_FLAGS="$LIBDDIO_LARGEFILE_FLAGS -D_LARGEFILE_SOURCE=$ac_cv_sys_largefile_source"
    fi
    CPPFLAGS="$CPPFLAGS $LIBCDIO_LARGEFILE_FLAGS"
fi

# We use  cmp and cdparanoia in cd-paranoia regression testing
AC_PATH_PROG(CMP, cmp, no)
AC_SUBST(CMP)

AC_PATH_PROG(OLD_CDPARANOIA, cdparanoia, no)
AC_SUBST(OLD_CDPARANOIA)

AC_DEFINE(LIBCDIO_CONFIG_H, 1, 
  [Is set when libcdio's config.h has been included. Applications wishing to sue their own config.h values (such as set by the application's configure script can define this before including any of libcdio's headers.])

dnl headers

AC_HEADER_STDC
AC_CHECK_HEADERS(errno.h fcntl.h glob.h limits.h pwd.h)
AC_CHECK_HEADERS(stdarg.h stdbool.h stdio.h sys/cdio.h sys/param.h)
AC_CHECK_HEADERS(sys/time.h sys/timeb.h)
AC_CHECK_HEADERS(ncurses.h curses.h, break, [enable_cdda_player='no'])

dnl FreeBSD 4 has getopt in unistd.h. So we include that before
dnl getopt.h 
AC_CHECK_HEADERS(unistd.h getopt.h)

AC_SUBST(SBPCD_H)
AC_SUBST(UCDROM_H)
AC_SUBST(TYPESIZES)

dnl compiler
AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE

dnl ISOC99_PRAGMA
AC_MSG_CHECKING([whether $CC supports ISOC99 _Pragma()])
AC_TRY_COMPILE([],[_Pragma("pack(1)")], [
  ISOC99_PRAGMA=yes 
  AC_DEFINE(HAVE_ISOC99_PRAGMA, [], [Supports ISO _Pragma() macro])
],ISOC99_PRAGMA=no)
AC_MSG_RESULT($ISOC99_PRAGMA)

##
## Check for S_ISSOCK() and S_ISLNK() macros
##
AC_MSG_CHECKING(for S_ISLNK() macro)
AC_TRY_RUN([
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

int main(int argc, char **argv) {
  int i = S_ISLNK(0);
  return 0;
}
], [AC_MSG_RESULT(yes); ac_have_islnk=yes; 
     AC_DEFINE(HAVE_S_ISLNK, [], 
     [Define this <sys/stat.h> defines S_ISLNK()])],
   [AC_MSG_RESULT(no); ac_have_islnk=no],
   [AC_MSG_RESULT(no); ac_have_islnk=no])

AC_MSG_CHECKING(for S_ISSOCK() macro)
AC_TRY_RUN([
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

int main(int argc, char **argv) {
  int i = S_ISSOCK(0);
  return 0;
}
], [AC_MSG_RESULT(yes); ac_have_islnk=yes; 
     AC_DEFINE(HAVE_S_ISSOCK, [], 
     [Define this <sys/stat.h> defines S_ISSOCK()])],
   [AC_MSG_RESULT(no); ac_have_issock=no],
   [AC_MSG_RESULT(no); ac_have_issock=no])

dnl empty_array_size
AC_MSG_CHECKING([how to create empty arrays])

empty_array_size="xxx"
AC_TRY_COMPILE([],[struct { int foo; int bar[]; } doo;], empty_array_size="")

if test "x$empty_array_size" = "xxxx";then
 AC_TRY_COMPILE([],[struct { int foo; int bar[0]; } doo;], empty_array_size="0")
fi

if test "x$empty_array_size" = "xxxx"
then
  AC_MSG_ERROR([compiler is unable to creaty empty arrays])
else
  AC_DEFINE_UNQUOTED(EMPTY_ARRAY_SIZE, $empty_array_size, 
	[what to put between the brackets for empty arrays])
  changequote(`,')
  msg="[${empty_array_size}]"
  changequote([,])
  AC_MSG_RESULT($msg)
fi
dnl empty_array_size

dnl bitfield order
AC_MSG_CHECKING(bitfield ordering in structs)
AC_TRY_RUN([
int
 main() {
  struct { char bit_0:1, bit_12:2, bit_345:3, bit_67:2; }
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
  __attribute__((packed))
#endif
  bf = { 1,1,1,1 };
  if (sizeof (bf) != 1) return 1;
  return *((unsigned char*) &bf) != 0x4b; }
], bf_lsbf=1, AC_TRY_RUN([ 
int
main() {
  struct { char bit_0:1, bit_12:2, bit_345:3, bit_67:2; }
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
  __attribute__((packed))
#endif
  bf = { 1,1,1,1 };
  if (sizeof (bf) != 1) return 1;
  return *((unsigned char*) &bf) != 0xa5; }
], bf_lsbf=0, AC_MSG_ERROR([unsupported bitfield ordering])))
if test "x$bf_lsbf" = "x1"; then
 AC_MSG_RESULT(LSBF)
 AC_DEFINE(BITFIELD_LSBF, [], [compiler does least-significant bit first in struct bitfields])
else
 AC_MSG_RESULT(MSBF)
fi
dnl

dnl AM_PROG_LIBTOOL tests whether we have GNU ld
dnl this must come before checking --with-versioned-libs
dnl which requires GNU ld.
AM_PROG_LIBTOOL

dnl system

# FIXME:
# I believe some OS's require -lm, but I don't recall for what function
# When we find it, put it in below instead of "cos".
AC_CHECK_LIB(m, cos, [LIBS="$LIBS -lm"; COS_LIB="-lm"])
CFLAGS="$CFLAGS $WARN_CFLAGS"
AC_SUBST(COS_LIB)

# Do we have GNU ld? If we don't, we can't build versioned symbols.
if test "$with_gnu_ld" != yes; then
   AC_MSG_WARN([I don't see GNU ld. I'm going to assume --without-versioned-libs])
   enable_versioned_libs='no'
fi
# We also need GNU make to build versioned symbols.
if test "x$enable_versioned_libs" = "xyes" ; then 
  if test -n "$MAKE" ; then
    $MAKE --version 2>/dev/null >/dev/null
    if test "$?" -ne 0 ; then
       AC_MSG_ERROR(Either set MAKE variable to GNU make or use 
   --without-versioned-libs option)
    fi
  else 
    make --version 2>/dev/null >/dev/null
    if test "$?" -ne 0 ; then
       AC_MSG_ERROR(Either set MAKE variable to GNU make or use 
   --without-versioned-libs option)
    fi
  fi
fi

AM_CONDITIONAL(CYGWIN, test "x$CYGWIN" = "xyes")
AM_CONDITIONAL(BUILD_CD_DRIVE, test "x$enable_cd_drive" = "xyes")
AM_CONDITIONAL(BUILD_CDINFO, test "x$enable_cd_info" = "xyes")
AM_CONDITIONAL(BUILD_CD_READ, test "x$enable_cd_read" = "xyes")
AM_CONDITIONAL(BUILD_CD_PARANOIA, test "x$enable_cd_paranoia" = "xyes")
AM_CONDITIONAL(BUILD_ISO_INFO, test "x$enable_iso_info" = "xyes")
AM_CONDITIONAL(BUILD_ISO_READ, test "x$enable_iso_read" = "xyes")
AM_CONDITIONAL(BUILD_CDINFO_LINUX, test "x$enable_cd_info_linux" = "xyes")
AM_CONDITIONAL(BUILD_CDIOTEST, test "x$enable_cdiotest" = "xyes")
AM_CONDITIONAL(BUILD_VERSIONED_LIBS, test "x$enable_versioned_libs" = "xyes")
AM_CONDITIONAL(DISABLE_CPP, test "x$disable_cpp" = "xyes")

dnl Checks for header files.
     
LIBCDIO_CDDA_LIBS='$(top_builddir)/lib/cdda_interface/libcdio_cdda.la'
LIBCDIO_CFLAGS='-I$(top_srcdir)/lib/driver -I$(top_srcdir)/include/'
LIBCDIO_LIBS='$(top_builddir)/lib/driver/libcdio.la'
LIBCDIO_DEPS="$LIBCDIO_LIBS"
LIBCDIOPP_LIBS='$(top_builddir)/lib/cdio++/libcdio++.la'
LIBISO9660PP_LIBS='$(top_builddir)/lib/cdio++/libiso9660++.la'
LIBCDIO_PARANOIA_LIBS='$(top_builddir)/lib/paranoia/libcdio_paranoia.la'
LIBISO9660_CFLAGS='-I$(top_srcdir)/lib/iso9660/'
LIBISO9660_LIBS='$(top_builddir)/lib/iso9660/libiso9660.la'
LIBUDF_CFLAGS='-I$(top_srcdir)/lib/udf/'
LIBUDF_LIBS='$(top_builddir)/lib/udf/libudf.la'
AC_SUBST(LIBCDIO_CDDA_LIBS)
AC_SUBST(LIBCDIO_CFLAGS)
AC_SUBST(LIBISO9660_CFLAGS)
AC_SUBST(LIBISO9660PP_LIBS)
AC_SUBST(LIBCDIO_LIBS)
AC_SUBST(LIBCDIOPP_LIBS)
AC_SUBST(LIBCDIO_DEPS)
AC_SUBST(LIBCDIO_PARANOIA_LIBS)
AC_SUBST(LIBISO9660_LIBS)
AC_SUBST(LIBUDF_LIBS)

case $host_os in
     aix*)
       ## Don't use AIX driver until starts to really work
       ## cd_drivers="${cd_drivers}, AIX"
       ## AC_DEFINE([HAVE_AIX_CDROM], [1],
       ##     [Define 1 if you have AIX CD-ROM support])
       ;;
     darwin6*|darwin7*|darwin8*|darwin9*)
       AC_CHECK_HEADERS(IOKit/IOKitLib.h CoreFoundation/CFBase.h, 
                        [have_iokit_h="yes"])
       if test "x$have_iokit_h" = "xyes" ; then 
         AC_DEFINE([HAVE_DARWIN_CDROM], [1],
	      [Define 1 if you have Darwin OS X-type CD-ROM support])
	 DARWIN_PKG_LIB_HACK="-Wl,-framework,CoreFoundation -Wl,-framework,IOKit"

	 dnl Prior to Mac OS X 10.4 (Tiger), DiskArbitration was a private framework.
	 dnl It's now public, and it's needed to do cd/dvd unmount/eject.
	 AC_MSG_CHECKING([for DiskArbitration framework])
	 ac_save_LIBS="$LIBS" LIBS="$LIBS -framework CoreFoundation -framework DiskArbitration"
	 AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <DiskArbitration/DiskArbitration.h>]], [[]])],
	      [have_diskarbitration_framework=yes], [have_diskarbitration_framework=no])
	 LIBS="$ac_save_LIBS"
	 AC_MSG_RESULT([$have_diskarbitration_framework])
	 if test x"$have_diskarbitration_framework" = x"yes"; then
	     AC_DEFINE([HAVE_DISKARBITRATION], 1, [Define to 1 if you have the Apple DiskArbitration framework])
	     DARWIN_PKG_LIB_HACK="$DARWIN_PKG_LIB_HACK -Wl,-framework,DiskArbitration"
	 fi

	 AC_SUBST(DARWIN_PKG_LIB_HACK)
	 LIBCDIO_LIBS="$LIBCDIO_LIBS $DARWIN_PKG_LIB_HACK"
         cd_drivers="${cd_drivers}, Darwin"
       fi
     ;;
     linux*)
        AC_CHECK_HEADERS(linux/version.h linux/major.h)
        AC_CHECK_HEADERS(linux/cdrom.h, [have_linux_cdrom_h="yes"])
	if test "x$have_linux_cdrom_h" = "xyes" ; then
	   AC_TRY_COMPILE(,[
#include <linux/cdrom.h>
struct cdrom_generic_command test;
int has_timeout=sizeof(test.timeout);],
            [AC_DEFINE([HAVE_LINUX_CDROM_TIMEOUT], [1], 
                   [Define 1 if timeout is in cdrom_generic_command struct])])
	   AC_DEFINE([HAVE_LINUX_CDROM], [1], 
                     [Define 1 if you have Linux-type CD-ROM support])
	   cd_drivers="${cd_drivers}, GNU/Linux"
	fi
	;;
     bsdi*)
        AC_CHECK_HEADERS(dvd.h, [have_bsdi_dvd_h="yes"])
	if test "x$have_bsdi_dvd_h" = "xyes" ; then
     	  AC_DEFINE([HAVE_BSDI_CDROM], [1], 
                    [Define 1 if you have BSDI-type CD-ROM support])
	 LIBS="$LIBS -ldvd -lcdrom"
         LIBCDIO_LIBS="$LIBCDIO_LIBS  -lcdrom"
	  cd_drivers="${cd_drivers}, BSDI"
	fi
	;;
     sunos*|sun*|solaris*)
     	AC_DEFINE([HAVE_SOLARIS_CDROM], [1], 
                  [Define 1 if you have Solaris CD-ROM support])
	  cd_drivers="${cd_drivers}, Solaris"
	;;
     cygwin*)
	AC_DEFINE([CYGWIN], [1], 
                  [Define 1 if you are compiling using cygwin])
     	AC_DEFINE([HAVE_WIN32_CDROM], [1], 
                  [Define 1 if you have MinGW CD-ROM support])
 	LIBS="$LIBS -lwinmm"
  	cd_drivers="${cd_drivers}, MinGW"
     	AC_DEFINE([NEED_TIMEZONEVAR], [1], 
                  [Define 1 if you need timezone defined to get timzone
		   defined as a variable. In cygwin it is a function too])
	;;
     mingw*)
	AC_DEFINE([MINGW32], [1], 
                  [Define 1 if you are compiling using MinGW])
     	AC_DEFINE([HAVE_WIN32_CDROM], [1], 
                  [Define 1 if you have MinGW CD-ROM support])
 	LIBS="$LIBS -lwinmm -mwindows"
  	cd_drivers="${cd_drivers}, MinGW "
	;;
     freebsd4.*|freebsd5.*|freebsd6*|freebsd7*|dragonfly*|kfreebsd*)
     	AC_DEFINE([HAVE_FREEBSD_CDROM], [1], 
                  [Define 1 if you have FreeBSD CD-ROM support])
 	LIBS="$LIBS -lcam"
  	cd_drivers="${cd_drivers}, FreeBSD "
	;;
     *)
	AC_MSG_WARN([Don't have OS CD-reading support for ${host_os}...])
	AC_MSG_WARN([Will use generic support.])
	;;
esac

AC_MSG_CHECKING(extern long timezone variable)
AC_TRY_RUN([
#ifdef NEED_TIMEZONEVAR
#define timezonevar 1
#endif

#include <time.h>
extern long timezone;
int main(int argc, char **argv) {
  long test_timezone = timezone;
  return 0; 
}
], [AC_MSG_RESULT(yes); 
   AC_DEFINE([HAVE_TIMEZONE_VAR], 1, 
	[Define if you have an extern long timenzone variable.])],
   [AC_MSG_RESULT(no)])
dnl

AC_SUBST(LINUX_CDROM_TIMEOUT)
AC_SUBST(DARWIN_PKG_LIB_HACK)
AC_SUBST(HAVE_BSDI_CDROM)
AC_SUBST(HAVE_DARWIN_CDROM)
AC_SUBST(HAVE_FREEBSD_CDROM)
AC_SUBST(HAVE_LINUX_CDROM)
AC_SUBST(HAVE_SOLARIS_CDROM)
AC_SUBST(HAVE_WIN32_CDROM)

LIBCDIO_SOURCE_PATH="`pwd`"
AC_DEFINE_UNQUOTED(LIBCDIO_SOURCE_PATH, "$LIBCDIO_SOURCE_PATH",
	[Full path to libcdio top_sourcedir.])
AC_SUBST(LIBCDIO_SOURCE_PATH)

AC_CHECK_FUNCS( [bzero drand48 ftruncate geteuid getgid \
		 getuid getpwuid gettimeofday lstat memcpy memset \
		 rand seteuid setegid snprintf  tzset vsnprintf readlink] )

AC_CHECK_MEMBER([struct tm.tm_gmtoff],
                [AC_DEFINE(HAVE_TM_GMTOFF, 1,
                        [Define if struct tm has the tm_gmtoff member.])],
                 ,
                 [#include <time.h>])

if test $ac_cv_member_struct_tm_tm_gmtoff = yes ; then
  AC_MSG_CHECKING([whether time.h defines daylight and timezone variables])
  AC_TRY_RUN([
#include <time.h>


       extern char *tzname[2];
       extern long timezone;
       extern int daylight;

int
 main() {
  return (timezone != 0) + daylight;
}
  ],[AC_DEFINE(HAVE_DAYLIGHT, 1,
   [Define if time.h defines extern long timezone and int daylight vars.])
   has_daylight=yes
   ],[has_daylight=no])
  AC_MSG_RESULT($has_daylight)
  AC_MSG_CHECKING([whether time.h defines tzname variable])
  AC_TRY_RUN([
#include <time.h>


       extern char *tzname[2];

int
 main() {
  return (tzname != NULL);
}
  ],[AC_DEFINE(HAVE_TZNAME, 1,
   [Define if time.h defines extern extern char *tzname[2] variable])
   has_tzname=yes
   ],[has_tzname=no])
   AC_MSG_RESULT($has_tzname)
fi

AC_ARG_ENABLE(joliet,
	AS_HELP_STRING([--disable-joliet], [don't include Joliet extension support (default enabled)]),
	[enable_joliet=$enableval],
	[enable_joliet=yes])
if test "${enable_joliet}" != "no" ; then
  AM_ICONV
  AM_LANGINFO_CODESET
  if test "$am_cv_func_iconv" = yes ; then 
     AC_DEFINE(HAVE_JOLIET, [1], 
     [Define 1 if you want ISO-9660 Joliet extension support. 
     You must have also libiconv installed to get Joliet extension support.])
     HAVE_JOLIET=1
  else
     AC_MSG_ERROR([You must have iconv installed.])
  fi
fi
AC_SUBST(HAVE_JOLIET)

if test x$enable_rock = x; then
  enable_rock=yes
  AC_ARG_ENABLE(rock,
	[  --enable-rock           include Rock-Ridge extension support (default enabled)],
	enable_rock=yes, enable_rock=no)
fi

if test "${enable_rock}" != "no" ; then
   AC_DEFINE(HAVE_ROCK, [1], 
   [Define 1 if you want ISO-9660 Rock-Ridge extension support.])
   HAVE_ROCK=1
fi
AC_SUBST(HAVE_ROCK)
AM_CONDITIONAL(ENABLE_ROCK, test x"$enable_rock" = "xyes")

if test x$enable_cddb = x; then
  enable_cddb=yes
  AC_ARG_ENABLE(cddb,
	[  --enable-cddb           include CDDB lookups in cd_info (default enabled)],	
	enable_cddb=yes, enable_cddb=no)
fi
if test x$enable_cddb != "no" ; then
   PKG_CHECK_MODULES(CDDB, libcddb >= 1.0.1, [
    HAVE_CDDB=yes 
    AC_DEFINE(HAVE_CDDB, [], [Define this if you have libcddb installed])
    ],
    [AC_MSG_WARN([new enough libcddb not found. CDDB access disabled. Get libcddb from http://libcddb.sourceforge.net])
    HAVE_CDDB=no])
    AC_CHECK_LIB(socket, connect)
    AC_CHECK_FUNC(gethostbyname, , AC_CHECK_LIB(nsl, gethostbyname))
fi


AC_SUBST(CDDB_LIBS)

AC_DEFINE(HAVE_KEYPAD, [], [Define this if your libcurses has keypad])
if test x$enable_cdda_player = xyes; then
    AC_CHECK_LIB(ncurses, mvprintw, 
      [LIBCURSES=ncurses; CDDA_PLAYER_LIBS="$CDDA_PLAYER_LIBS -lncurses"],
      AC_CHECK_LIB(curses, mvprintw, 
      [LIBCURSES=curses; CDDA_PLAYER_LIBS="$CDDA_PLAYER_LIBS -lcurses"],
      [AC_MSG_WARN([Will not build cdda-player - did not find libcurses or libncurses]) 
      enable_cdda_player=no]))
    if test x$enable_cdda_player = xyes; then
      AC_CHECK_LIB($LIBCURSES, keypad, [HAVE_KEYPAD=yes])
    fi   
fi

AM_CONDITIONAL(BUILD_CDDA_PLAYER, test "x$enable_cdda_player" = "xyes")
AC_SUBST(CDDA_PLAYER_LIBS)

if test x$enable_vcd_info = x; then
AC_ARG_ENABLE(vcd_info,
	[  --enable-vcd-info      include Video CD Info from libvcd],
		enable_vcd_info=yes,
		enable_vcd_info=no)
fi
if test x$enable_vcd_info = xyes; then
  PKG_CHECK_MODULES(VCDINFO, libvcdinfo >= 0.7.21, 
  [AC_DEFINE([HAVE_VCDINFO],1, 
  [Define this if you have libvcdinfo installed])], 
  [AC_MSG_WARN(a new enough libvcdinfo not found. 
VCD info display in cd-info disabled.
libvcdinfo is part of vcdimager. Get it from http://vcdimager.org)
   enable_vcd_info=no])
fi

AC_SUBST(VCDINFO_LIBS)
AC_SUBST(VCDINFO_CFLAGS)

dnl
dnl Newest automake workaround - needed for multi-language manual pages
dnl
AC_SUBST(mkdir_p)

AC_CONFIG_COMMANDS([checks], 
		   [chmod +x test/check_cue.sh; chmod +x test/check_nrg.sh
		   ])

dnl
dnl Output configuration files
dnl
## AC_CONFIG_FILES([  po/Makefile.in\
AC_CONFIG_FILES([
       Makefile \
       example/C++/Makefile \
       example/C++/OO/Makefile \
       example/Makefile \
       include/Makefile \
       include/cdio/Makefile \
       include/cdio++/Makefile \
       include/cdio/version.h \
       doc/doxygen/Doxyfile \
       doc/Makefile \
       lib/Makefile \
       lib/cdda_interface/Makefile \
       lib/cdio++/Makefile \
       lib/driver/Makefile \
       lib/iso9660/Makefile \
       lib/paranoia/Makefile \
       lib/udf/Makefile \
       libcdio.pc \
       libcdio++.pc \
       libcdio_cdda.pc \
       libcdio_paranoia.pc \
       libiso9660.pc \
       libiso9660++.pc \
       libudf.pc \
       package/libcdio.spec \
       src/cd-paranoia/Makefile \
       src/cd-paranoia/usage.txt \
       src/cd-paranoia/doc/Makefile \
       src/cd-paranoia/doc/en/cd-paranoia.1 \
       src/cd-paranoia/doc/en/Makefile \
       src/cd-paranoia/doc/ja/cd-paranoia.1 \
       src/cd-paranoia/doc/ja/Makefile \
       src/Makefile \
       test/testbincue.c \
       test/testisocd2.c \
       test/check_common_fn \
       test/Makefile \
       ])

# AC_CONFIG_FILES([po/Makefile])
AC_CONFIG_FILES([test/check_cue.sh], [chmod +x test/check_cue.sh])
AC_CONFIG_FILES([test/check_iso.sh], [chmod +x test/check_iso.sh])
AC_CONFIG_FILES([test/check_nrg.sh], [chmod +x test/check_nrg.sh])
AC_CONFIG_FILES([test/check_paranoia.sh], [chmod +x test/check_paranoia.sh])

AC_CONFIG_COMMANDS([default],[[
# Make sure scripts are executable
# This is a big hack, but I don't know any other way around it.
cat > include/cdio/cdio_config.h << EOF
/** \file cdio_config.h 
 *  \brief  configuration-time settings useful in compilation; a run-time
    version of config.h
*/
EOF
head -n 233 config.h >> include/cdio/cdio_config.h
]],[[]])

AC_OUTPUT

echo "Using CD-ROM drivers: $cd_drivers"
echo -n "Building cd-paranoia: "
if test "x$enable_cd_paranoia" = "xyes"
then
echo "yes"
else
echo "no"
fi
echo -n "Building cd-info    : "
if test "x$enable_cd_info" = "xyes" 
then
echo "yes"
else
echo "no"
fi
echo -n "Building cd-read    : "
if test "x$enable_cd_read" = "xyes" 
then
echo "yes"
else
echo "no"
fi
echo -n "Building cdda-player: "
if test "x$enable_cdda_player" = "xyes" 
then
echo "yes"
else
echo "no"
fi
echo -n "Building iso-info   : "
if test "x$enable_iso_info" = "xyes" 
then
echo "yes"
else
echo "no"
fi
echo -n "Building iso-read   : "
if test "x$enable_iso_read" = "xyes" 
then
echo "yes"
else
echo "no"
fi
