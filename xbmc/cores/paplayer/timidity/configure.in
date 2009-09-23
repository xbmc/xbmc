dnl TiMidity++ -- MIDI to WAVE converter and player
dnl Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
dnl Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2 of the License, or
dnl (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

dnl configure.in created from configure.scan
dnl by URABE, Shyouhei <root@mput.dip.jp> on 2002.07.06

dnl variable note:
dnl CFLAGS	- Flags for compile.
dnl CPPFLAGS	- Flags for cpp
dnl LDFLAGS	- Flags for linker
dnl		ac_cpp:	    $CPP $CPPFLAGS
dnl		ac_compile: ${CC-cc} -c $CFLAGS $CPPFLAGS
dnl		ac_link:    ${CC-cc} -o * $CFLAGS $CPPFLAGS $LDFLAGS *.c $LIBS
dnl EXTRACFLAGS - Extra flags to compile.  Adds to CFLAGS.
dnl EXTRADEFS	- Extra define macros.	Adds to CPPFLAGS.
dnl EXTRALIBS	- Extra libraries  Adds to LIBS.
dnl SYSEXTRAS	- Additional source codes to compile.
dnl lib_*_opt	- Optional libraries. Adds to LIBS.
dnl LIBRARY_PATH
dnl		- add to -L LDFLAGS
dnl C_INCLUDE_PATH
dnl		- add to -I CPPFLAGS
dnl CYGNUS	- `yes' if cygnus windows environment.
dnl MSYS	- `yes' if Mingw windows environment.
dnl WISH	- wish
dnl LN_S	- ln -s
dnl tcltk_dep	- make dependencies for tcl/tk interface
dnl
dnl so		- File extension of shared object library.
dnl SHLD	- Linker to make shared object library.
dnl SHLDFLAGS	- Link flags to link shared object library.
dnl SHCFLAGS	- Additional flags to compile shared object library.
dnl		  (such as -fPIC)

# This is a autoscan-generated configure.scan.
# AC_PREREQ(2.56)

# "AC_INIT(timidity/timidity.c)" is obsolate yet.
AC_INIT([TiMidity++],[2.13.2],[root@mput.dip.jp],[TiMidity++])
AC_CONFIG_AUX_DIR([autoconf])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([gnu dist-bzip2])
AC_CONFIG_SRCDIR([timidity/timidity.c])
AC_CONFIG_HEADERS([config.h interface.h])
AM_MAINTAINER_MODE

SHELL=${CONFIG_SHELL-"/bin/sh"}
WISH=${WISH-"wish"}

dnl to use contains() macro (see autoconf/acinclude.m4)
CONTAINS_INIT

# Check for compiler options -- such as debug flags.

dnl "--enable-debug" turnes -g on.
AC_MSG_CHECKING(if --enable-debug option specified)
AC_ARG_ENABLE(debug,
  [  --enable-debug          Build a debugging version.],
  [timidity_cv_debug="yes"], [timidity_cv_debug="no"])
if test "x$timidity_cv_debug" = "xyes"; then
  AC_DEFINE(DEBUG,1,Define to 1 if you are in debug mode)
  CFLAGS=${CFLAGS-"-g"}
fi
if test "x$timidity_cv_debug" = "xno"; then
  AC_DEFINE(NDEBUG,1,Define to 1 if you are NOT in debug mode)
fi
AC_MSG_RESULT($timidity_cv_debug)

CFLAGS=${CFLAGS-"-O2"}

dnl --with-x turns on if `--with-x' is NOT specified.
if test "x$with_x" = x; then
  with_x=yes
else
  x_config_flag=yes
fi

# Checking gcc environment
for i in `echo $LIBRARY_PATH|sed -e 's,:, ,g'`; do
  LDFLAGS="$LDFLAGS -L${i}"
done
for i in `echo $C_INCLUDE_PATH|sed -e 's,:, ,g'`; do
  CPPFLAGS="$CPPFLAGS -I${i}"
done

# add $prefix if specified.
if test "x$prefix" != xNONE -a "x$prefix" != "x$ac_default_prefix" -a "x$prefix" != "x/usr"; then
  LDFLAGS="-L$prefix/lib $LDFLAGS"
  SHLDFLAGS="-L$prefix/lib $SHLDFLAGS"
  CPPFLAGS="-I$prefix/include $CPPFLAGS"
fi

dnl add --with-includes, --with-libraries
AC_ARG_WITH(includes,
  [  --with-includes=DIR     Specify include directories (colon separated)],
  [ j=' '
    for i in `echo "$withval"|sed -e 's,:, ,g'`; do
      j="${j}-I${i} "
    done
    CPPFLAGS="$j$CPPFLAGS"
  ])

AC_ARG_WITH(libraries,
  [  --with-libraries=DIR    Specify library directories (colon separated)],
  [ j=' '
    for i in `echo "$withval"|sed -e 's,:, ,g'`; do
      j="${j}-L${i} "
    done
    LDFLAGS="$j$LDFLAGS"
  ])


dnl a bit hairly yet.(20020718:mput)
dnl CPPFLAGS="$CPPFLAGS \$(DEF_PKGDATADIR) \$(DEF_PKGLIBDIR) \$(DEF_DEFAULT_PATH)"
AC_ARG_WITH(default-path,
  [  --with-default-path=DIR Where timidity.cfg is  (PREFIX/share/timidity)],
  [pkgdatadir=$withval],
  [
   if test "x$prefix" != "xNONE"; then
      pkgdatadir='${prefix}/share/timidity'
   else
      pkgdatadir='/usr/local/share/timidity'
   fi
  ])
tmpdefpath="`eval \"echo ${pkgdatadir}\"`"
AC_DEFINE_UNQUOTED(DEFAULT_PATH,"$tmpdefpath",place to install patches)
AC_DEFINE(PKGDATADIR,DEFAULT_PATH,a compatibility matter. ignore it.)

AC_ARG_WITH(module-dir,
  [  --with-module-dir=DIR   Where to install interfaces (PREFIX/lib/timidity)],
  [pkglibdir=$withval],
  [
   if test "x$prefix" != "xNONE"; then
      pkglibdir='${prefix}/lib/timidity'
   else
      pkglibdir='/usr/local/lib/timidity'
   fi
  ])
tmplibdir="`eval \"echo ${pkglibdir}\"`"
AC_DEFINE_UNQUOTED(PKGLIBDIR,"$tmplibdir",place to install modules)


# Checks for programs.
AM_PATH_LISPDIR
AC_PROG_CC
AC_PROG_GCC_TRADITIONAL
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_RANLIB
AC_PROG_LN_S
CHECK_COMPILER_OPTION(rdynamic)

# Checks for target dependencies

AC_ARG_WITH(elf,
  [  --with-elf              create ELF shared object instead of AOUT ],
  [ ELFFORMAT=$withval ], [ ELFFORMAT="yes" ])

# AC_ARG_WITH(simd,
#  [  --with-simd             create SIMD friendly binary (default is no) ],
#  [ SIMD=$withval ], [ SIMD="no" ])

case "$target" in
  *-*-hpux*)
    EXTRADEFS="-DHPUX"
    test -n "$SHLD" || SHLD="ld -b"
    so="sl"
    ;;
  *-sgi-irix*)
    dnl IRIX cc needs -signed option
    case ".$CC" in .cc*) EXTRACFLAGS="-signed";; esac
    test -n "$SHLD" || SHLD="cc -shared -all"
    so="so"
    ;;
  *-*-netbsd*)
    test -n "$SHLD" || SHLD="$CC -shared -nostartfiles"
    so="so"
    ;;

  *-*-freebsd2*)
    dnl EXTRALIBS="$EXTRALIBS -lxpg4"
    test -n "$SHLD" || SHLD="ld -Bshareable"
    so="so"
    ;;
  *-*-freebsd*)
    if test "$ELFFORMAT" = yes; then
	test -n "$SHLD" || SHLD="ld -Bshareable"
    else
	SHCFLAGS=-aou
	SHLDFLAGS="-L/usr/X11R6/lib/aout $SHLDFLAGS"
	test -n "$SHLD" || SHLD="env OBJFORMAT=aout ld -shared"
    fi
    dnl EXTRALIBS="$EXTRALIBS -lxpg4"
    so="so"
    ;;

#  *-*-freebsd*)
#    EXTRALIBS="$EXTRALIBS -lxpg4"
#    test -n "$SHLD" || SHLD="ld -Bshareable"
#    so="so"
#  ;;

  *-*-cygwin*)
    EXTRADEFS="-D__W32__"
    # ??
    case "x${CC} " in
      "xbcc32 "*) 
#        test -n "$SHLD" || SHLD="tlink32 -Tpd"
        test -n "$SHLD" || SHLD="BCC32 -WD"
        BORLANDC=yes
        BORLANDC_LDFLAGS="kernel32.lib winspool.lib comdlg32.lib advapi32.lib oleaut32.lib uuid.lib odbc32.lib wsock32.lib user32.lib shell32.lib cw32mt.lib"
		BORLANDC_START='C0X32.OBJ'
        CFLAGS="$CFLAGS  -tWM -VM"
        EXTRADEFS="$EXTRADEFS -D_WINDOWS -DWIN32"
        ;;
      "xwcc386_w.sh "*)
         WATCOM_C=yes
         CFLAGS="$CFLAGS -br -5s -zk0 -bm -zp=16 -w=4 -D_WINDOWS -DWIN32 -DINCLUDE_WINDOWS_H"
         WATCOM_LDFLAGS=
         EXTRALIBS="$EXTRALIBS kernel32.lib winspool.lib comdlg32.lib advapi32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib user32.lib shell32.lib libcmt.lib"
         ;;
      "xcl "*)
         test -n "$SHLD" || SHLD="link -dll" 
         VCPP_LDFLAGS="-nologo -incremental:no  -nodefaultlib:libc.lib -nodefaultlib:msvcrt.lib kernel32.lib winspool.lib comdlg32.lib advapi32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib user32.lib shell32.lib libcmt.lib"
         CFLAGS="$CFLAGS -nologo -Zp16 -MT -W3 -GX  -Ob2"
         EXTRADEFS="$EXTRADEFS -DNDEBUG -D_WINDOWS -DWIN32 -DMBCS -D_MT -YX  -FD -c"
         VCPP=yes
         ;;
      "xgcc"*) test -n "$SHLD" || SHLD="gcc -mdll" ;;
      *) test -n "$SHLD" || SHLD="unknown" ;;
    esac
    so="dll"
    CYGNUS=yes
    lib_user32_test=-luser32
    ;;
  *-*-mingw*)
    EXTRADEFS="-D__W32__"
    # ??
    case "x${CC} " in
      "xbcc32 "*) 
#        test -n "$SHLD" || SHLD="tlink32 -Tpd"
        test -n "$SHLD" || SHLD="BCC32 -WD"
        BORLANDC=yes
        BORLANDC_LDFLAGS="kernel32.lib winspool.lib comdlg32.lib advapi32.lib oleaut32.lib uuid.lib odbc32.lib wsock32.lib user32.lib shell32.lib cw32mt.lib"
        BORLANDC_START='C0X32.OBJ'
        CFLAGS="$CFLAGS  -tWM -VM"
        EXTRADEFS="$EXTRADEFS -D_WINDOWS -DWIN32"
        ;;
      "xwcc386_w.sh "*)
         WATCOM_C=yes
         CFLAGS="$CFLAGS -br -5s -zk0 -bm -zp=16 -w=4 -D_WINDOWS -DWIN32 -DINCLUDE_WINDOWS_H"
         WATCOM_LDFLAGS=
         EXTRALIBS="$EXTRALIBS kernel32.lib winspool.lib comdlg32.lib advapi32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib user32.lib shell32.lib libcmt.lib"
         ;;
      "xcl "*) 
         test -n "$SHLD" || SHLD="link -dll"
         VCPP=yes
         VCPP_LDFLAGS="-nologo -incremental:no  -nodefaultlib:libc.lib -nodefaultlib:msvcrt.lib kernel32.lib winspool.lib comdlg32.lib advapi32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wsock32.lib user32.lib shell32.lib libcmt.lib"
         CFLAGS="$CFLAGS -nologo -Zp16 -MT -W3 -GX  -Ob2"
         EXTRADEFS="$EXTRADEFS -DNDEBUG -D_WINDOWS -DWIN32 -DMBCS -D_MT -YX  -FD -c"
         ;;
      "xgcc"*) test -n "$SHLD" || SHLD="gcc -mdll" ;;
      *) test -n "$SHLD" || SHLD="unknown" ;;
    esac
    so="dll"
    MSYS=yes
    lib_user32_test=-luser32
    ;;
  *-dec-*)
    EXTRADEFS="-DDEC"
    test -n "$SHLD" || SHLD="ld -Bdynamic -G"
    so="so"
    ;;
  *-*-solaris*)
    EXTRADEFS="-DSOLARIS"
    test -n "$SHLD" || SHLD="/usr/ccs/bin/ld -G"
    so="so"
    ;;
  *-*-nextstep*)
    test -n "$SHLD" || SHLD="/usr/ccs/bin/ld -G"
    so="so"
    ;;
  *-*-darwin*)
    if test "x$SIMD" = "xyes"; then
       AC_DEFINE(USE_ALTIVEC,1,Define to 1 if you use altivec)
       # EXTRACFLAGS="$EXTRACFLAGS -faltivec -mabi=altivec -maltivec" # for GCC3.
       EXTRACFLAGS="$EXTRACFLAGS -faltivec"
    else
       AC_DEFINE(USE_ALTIVEC,0,Define to 1 if you use altivec)
    fi
    CPPFLAGS="$CPPFLAGS -no-cpp-precomp"
    LDOPTS="-dynamic -undefined suppress -flat_namespace $LDOPTS"
    test -n "$SHLD" || SHLD="$CC -dynamic -bundle -undefined suppress -flat_namespace $LDFLAGS"
    so="bundle"
    ;;
  *)
    test -n "$SHLD" || \
      if test "x$GCC" = xyes; then
	SHLD="$CC -shared"
      else
	SHLD="ld -Bdynamic -G"
      fi
    so="so"
    ;;
esac

dnl For .exe
dnl AC_EXEEX # obsolate

# Checks for libraries.
AC_CHECK_LIB(m,sqrt)
AC_CHECK_LIB(socket,socket)
AC_CHECK_LIB(dl,dlopen,lib_dl_opt=-ldl)
AC_CHECK_LIB(objc,NSLinkModule,lib_dl_opt=-lobjc) # Mac OS X need this.
if test "x$VCPP" != "xyes" && test "x$BORLANDC" != "xyes" && test "x$WATCOM_C" != "xyes"; then
AC_CHECK_LIB(xpg4,setlocale) # Mac OS X (and maybe NetBSD) need this.
fi
AC_CHECK_FUNCS(gethostbyname,,[ AC_CHECK_LIB(nsl,gethostbyname) ])
AC_PATH_XTRA

# Checks for X
if test "x$with_x" = xyes; then
  if test "x$have_x" = xno; then
    if test "x$x_config_flag" = xyes; then
      AC_MSG_ERROR(Could not configure X)
    else
      AC_MSG_WARN(Could not configure X)
    fi
  fi
fi
if test "x$with_x" = xyes -a "x$have_x" = xyes; then
  if test "x$x_libraries" != x; then
    ldflags_x_opt="-L$x_libraries"
    LDFLAGS="$LDFLAGS $ldflags_x_opt"
    SHLDFLAGS="$SHLDFLAGS $ldflags_x_opt"
  else
    ldflags_x_opt=
  fi
  if test "x$x_includes" != x; then
    CPPFLAGS="$CPPFLAGS -I$x_includes"
  fi

  AC_CHECK_LIB(X11,XOpenDisplay)
  AC_MSG_CHECKING(X11 version 6)
  AC_CACHE_VAL(timidity_cv_x11_version_6,
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <X11/Xlib.h>]], [[
#if XlibSpecificationRelease < 6
fail;
#endif
]])],[timidity_cv_x11_version_6=yes],[timidity_cv_x11_version_6=no]))
  if test "$timidity_cv_x11_version_6" = "yes"; then
    AC_MSG_RESULT(6 or newer)
    dnl AC_DEFINE(HAVE_X11R6)
  else
    AC_MSG_RESULT(before 6)
  fi

  dnl checking order required
  KEEPLIBS=$LIBS
  AC_CHECK_LIB(Xext,XShapeCombineMask,have_xext=yes; LIBS="-lXext $LIBS",have_xext=no)
  AC_CHECK_LIB(ICE,IceConnectionNumber)
  AC_CHECK_LIB(SM,SmcOpenConnection,have_xprelibs=yes; LIBS="-lSM $LIBS",have_xprelibs=no)
  AC_CHECK_LIB(Xt,XtVaAppInitialize,have_xt=yes; LIBS="-lXt $LIBS",have_xt=no)
  AC_CHECK_LIB(Xmu,XmuInternAtom,LIBS="-lXmu $LIBS")
  AC_CHECK_FUNCS(XmuRegisterExternalAgent)
  AC_CHECK_LIB(Xaw3d,XawInitializeWidgetSet,have_xaw=3d,
   [ AC_CHECK_LIB(Xaw,XawInitializeWidgetSet,have_xaw=yes,have_xaw=no) ])
  AC_CHECK_LIB(Xm,XmCreateForm,have_xm=yes,have_xm=no)
  AC_CHECK_FUNCS(XShmCreatePixmap)
  LIBS=$KEEPLIBS
fi
dnl End of X configure

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_STDBOOL
AC_HEADER_SYS_WAIT
if test "x$WATCOM_C" != xyes ; then
AC_HEADER_TIME
fi
AC_HEADER_DIRENT
AC_CHECK_HEADERS( \
  [arpa/inet.h \
  errno.h \
  getopt.h \
  glob.h \
  dlfcn.h \
  fcntl.h \
  inttypes.h \
  limits.h \
  machine/endian.h \
  malloc.h \
  memory.h \
  netdb.h \
  netinet/in.h \
  nlist.h \
  stddef.h \
  stdlib.h \
  stdint.h \
  string.h \
  strings.h \
  stropts.h \
  soundcard.h \
  alsa/asoundlib.h \
  sys/asoundlib.h \
  sys/audioio.h \
  sys/awe_voice.h\
  sys/ioctl.h \
  sys/ipc.h \
  sys/param.h \
  sys/shm.h \
  sys/socket.h \
  sys/soundcard.h \
  sys/param.h \
  sys/time.h \
  sys/types.h \
  sys/sysctl.h \
  termios.h \
  unistd.h \
  X11/Xlib.h])
AC_CHECK_HEADERS(\
  [X11/extensions/XShm.h X11/Xmu/ExtAgent.h],[],[],[
#ifdef HAVE_X11_XLIB_H
#include <X11/Xlib.h>
#endif
  ])
AC_CHECK_HEADER(string.h,, [ AC_DEFINE(NO_STRING_H,1,Define to 1 if you do not have <string.h>.)])

# Checks for typedefs, structures, and compiler characteristics.
if test "x$WATCOM_C" != xyes ; then
AC_C_BIGENDIAN
fi
AC_C_CHAR_UNSIGNED
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE
AC_C_STRINGIZE
AC_C_PROTOTYPES
AC_STRUCT_TM
AC_TYPE_SIZE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T

dnl keyword "volatile" check
AC_CACHE_CHECK(volatile declaration,timidity_cv_type_volatile,
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM(, [[volatile int x]])],[timidity_cv_type_volatile=yes],[timidity_cv_type_volatile=no]))
if test $timidity_cv_type_volatile = no; then
  AC_DEFINE(NO_VOLATILE,1,Define to 1 if you cannot use volatile keyword)
fi

dnl union usenum check
AC_CACHE_CHECK(union semun declaration,timidity_cv_type_union_semun,
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
]], [[union semun x]])],[timidity_cv_type_union_semun=yes],[timidity_cv_type_union_semun=no]))
if test $timidity_cv_type_union_semun = yes; then
  AC_DEFINE(HAVE_UNION_SEMUN,1,Define to 1 if you use semun keyword)
fi

dnl Cygnus and Mingw memory struct optiontest
if test "x$CYGNUS" = xyes || test "x$MSYS" = xyes; then
	case "x${CC}" in
	"xgcc"*) 
		msnative_struct=''
	    AC_MSG_CHECKING([how to get MSVC-compatible struct packing])
	    if test -z "$ac_cv_prog_CC"; then
			our_gcc="$CC"
		else
			our_gcc="$ac_cv_prog_CC"
		fi
		case `$our_gcc --version | sed -e 's,\..*,.,' -e q` in
			2.)
		if $our_gcc -v --help 2>/dev/null | grep fnative-struct >/dev/null; then
	  		msnative_struct='-fnative-struct'
		fi
		;;
			*)
		if $our_gcc -v --help 2>/dev/null | grep ms-bitfields >/dev/null; then
			msnative_struct='-mms-bitfields'
		fi
		;;
		esac
		if test x"$msnative_struct" = x ; then
			AC_MSG_RESULT([no way])
			AC_MSG_WARN([produced binary will be incompatible with windows' GUI ])
		else
			CFLAGS="$CFLAGS $msnative_struct"
			CPPFLAGS="$CPPFLAGS $msnative_struct"
			AC_MSG_RESULT([${msnative_struct}])
		fi
		;;
	*) 
		;;
	esac
fi
dnl Cygnus Mingw32 mmsystem check
if test "x$CYGNUS" = xyes || test "x$MSYS" = xyes; then
  AC_CACHE_CHECK(Cygwin new mmsystem,timidity_cv_header_new_mmsystem,
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <windows.h>
#include <mmsystem.h>
]], [[return WOM_OPEN != 0x3BB]])],[timidity_cv_header_new_mmsystem=yes],[timidity_cv_header_new_mmsystem=no]))
  if test $timidity_cv_header_new_mmsystem = yes; then
    AC_DEFINE(HAVE_NEW_MMSYSTEM,1,Define to 1 if you have <mmsystem.h> file)
  fi
fi

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MEMCMP
AC_FUNC_MMAP
AC_FUNC_SETVBUF_REVERSED
AC_TYPE_SIGNAL
if test "x$VCPP" != xyes && test "x$BORLANDC" != xyes && test "x$WATCOM_C" != "xyes"; then
AC_FUNC_VPRINTF
fi
AC_CHECK_FUNCS( \
  [alarm \
  dup2 \
  floor \
  getcwd \
  getopt \
  gethostbyname \
  gettimeofday \
  getwd \
  isatty \
  memchr \
  memmove \
  memset \
  modf \
  munmap \
  popen \
  pow \
  select \
  signal \
  socket \
  sleep \
  vsnprintf \
  snprintf \
  sqrt \
  strchr \
  strdup \
  strstr \
  strerror \
  strcasecmp \
  strncasecmp \
  strrchr \
  strstr \
  strtol \
  strlcat \
  strlcpy \
  usleep])
if test "x$VCPP" = xyes; then
  AC_DEFINE(RETSIGTYPE,int)
  AC_DEFINE(HAVE_GETCWD,int)
fi
if test "x$VCPP" = xyes || test "x$BORLANDC" = xyes || test "x$WATCOM_C" = xyes; then
  AC_DEFINE(HAVE_GETHOSTBYNAME,1)
  AC_DEFINE(HAVE_POPEN,1)
  AC_DEFINE(popen,_popen,[for VC])
  AC_DEFINE(pclose,_pclose,[for VC])
  AC_DEFINE(HAVE_SELECT,1)
  AC_DEFINE(HAVE_SNPRINTF,1)
  AC_DEFINE(HAVE_STRDUP,1)
  AC_DEFINE(HAVE_STRERROR,1)
  AC_DEFINE(HAVE_STRNCASECMP,1)
  AC_DEFINE(HAVE_STRSTR,1)
  AC_DEFINE(snprintf,_snprintf,[for VC])
  AC_DEFINE(HAVE_SOCKET,1)
  AC_DEFINE(HAVE_VSNPRINTF,1)
  AC_DEFINE(vsnprintf,_vsnprintf,[for VC])
  AC_DEFINE(TIMID_VERSION,[$(PACKAGE_VERSION)],[for windows gui])
fi

AC_CHECK_FUNC(getopt_long,
	 AC_DEFINE([HAVE_GETOPT_LONG],1,[Define to 1 if you have `getopt_long function'])
	 tm_cv_needgetopt="no",
	 tm_cv_needgetopt="yes")
AM_CONDITIONAL([NEEDGETOPT], test "x$tm_cv_needgetopt" = "xyes")

# Checks on cygnus and MYSYS
if test "x$CYGNUS" = xyes || test "x$MSYS" = xyes; then
  case "$ac_cv_header_dirent_dirent_h$ac_cv_header_dirent_sys_ndir_h$ac_cv_header_dirent_sys_dir_h$ac_cv_header_dirent_ndir_h" in
  *yes*)
    AC_CHECK_FUNC(opendir,,
  [ EXTRADEFS="$EXTRADEFS -D__W32READDIR__"
    W32READDIR=yes ])
    ;;
  *)EXTRADEFS="$EXTRADEFS -D__W32READDIR__"
    W32READDIR=yes
    ;;
  esac
  if test "x$enable_network" = "xyes"; then
  	if test "x$VCPP" = xyes || test "x$BORLANDC" = xyes || test "x$WATCOM_C" = xyes; then
      EXTRALIBS="$EXTRALIBS  wsock32.lib"
      EXTRADEFS="$EXTRADEFS -DWINSOCK" 
    else
      AC_CHECK_FUNC(socket,,
       WAPI_CHECK_LIB(wsock32,WSAStartup,
  	     [#include <winsock.h>],
  	     [WSAStartup(0,0);],
         [ EXTRALIBS="$EXTRALIBS -lwsock32"
          EXTRADEFS="$EXTRADEFS -DWINSOCK" ]
       )
      )
    fi
  fi
fi

# Checks on extra paths

dnl tcltk paths
AC_ARG_WITH(tcl-includes,
  [  --with-tcl-includes=DIR Tcl include file path ],
  [ if test ! -d $withval; then
      AC_MSG_WARN($withval is not found.)
    fi
    tcl_include_dir="-I$withval"
  ])
AC_ARG_WITH(tcl-libs,
  [  --with-tcl-libs=DIR     Tcl library file path ],
  [ if test ! -d $withval; then
      AC_MSG_WARN($withval is not found.)
    fi
    tcl_libdir="-L$withval" ])
AC_ARG_WITH(tk-includes,
  [  --with-tk-includes=DIR  Tk include file path ],
  [ if test ! -d $withval; then
      AC_MSG_WARN($withval is not found.)
    fi
    tk_includes="-I$withval" ])
AC_ARG_WITH(tk-libs,
  [  --with-tk-libs=DIR      Tk library file path ],
  [ if test ! -d $withval; then
      AC_MSG_WARN($withval is not found.)
    fi
    tk_libdir="-L$withval" ])

dnl offix paths
AC_ARG_WITH(offix-includes,
  [  --with-offix-includes=DIR Offix include file path ],
  [ if test ! -d $withval; then
      AC_MSG_WARN($withval is not found.)
    fi
    offix_include_dir="-I$withval" ])
AC_ARG_WITH(offix-libs,
  [  --with-offix-libs=DIR   Offix include file path ],
  [ if test ! -d $withval; then
      AC_MSG_WARN($withval is not found.)
    fi
    offix_lib_dir="-L$withval" ])


#
# audio section
#
dnl default(d): auto selection
dnl oss(d):     OSS /dev/dsp
dnl alsa(d):    ALSA pcm device
dnl sun(d):     SunOS /dev/audio
dnl hpux(d):    HPUX /dev/audio
dnl irix(d):    IRIX audio
dnl mme(d):     OSF/1 MME
dnl sb_dsp(d):  BSD/OS 2.0 /dev/sb_dsp
dnl w32(d):     Windows MMS
dnl darwin(d):  Mac OS X pcm device
dnl alsa(s):    ALSA pcm device
dnl alib(A):    HPUX network audio (Alib)
dnl nas(n):     Network Audio System
dnl arts(R):	aRts
dnl esd(e):     EsounD
dnl portaudio(p) PortAudio
dnl vorbis(v):  Ogg Vorbis
dnl flac(F):    FLAC / OggFLAC
dnl speex(S):   Ogg Speex
dnl gogo(g):    MP3 GOGO
dnl jack(j):    JACK
dnl ao(O):      Libao

audio_targets='default oss alsa sun hpux irix mme sb_dsp w32 alib nas arts esd vorbis flac gogo portaudio jack ao'

AC_ARG_WITH(nas-library,
  [  --with-nas-library=library NAS absolute library path(Don't use -laudio)])
AC_ARG_WITH(nas-includes,
  [  --with-nas-includes=DIR NAS include files are in dir])


AC_ARG_ENABLE(audio,
  [  --enable-audio[=mode_list] Enable audio (Specify comma separated mode list):
                              default:   Automatically select audio device.
                              oss:       OSS /dev/dsp
                              sun:       SunOS /dev/audio
                              hpux:      HPUX /dev/audio
                              irix:      IRIX audio library
                              mme:       OSF/1 MME
                              sb_dsp:    BSD/OS 2.0 /dev/sb_dsp
                              w32:       Windows MMS
                              darwin:    Darwin CoreAudio
                              alsa:      ALSA pcm device
                              alib:      HPUX network audio (Alib)
                              nas:       NAS - Network Audio System
                              arts:      aRts
                              esd:       EsounD - Enlightened Sound Daemon
                              portaudio: PortAudio
                              jack:      JACK
                              ao:        Libao
                              vorbis:    Ogg Vorbis
                              flac:      FLAC / OggFLAC
                              speex:     Ogg Speex
                              gogo:      MP3 GOGO (Only Windows is supported)],
  [ enable_audio=$enableval
    have_audio_opt=yes ],
  [ enable_audio=yes
    have_audio_opt=no ])

dnl yes -> default
test "x$enable_audio" = xyes && enable_audio=default

if test "x$enable_audio" != xno; then
  for i in `echo $enable_audio | sed 's/,/ /g'`; do
    eval "au_enable_$i=yes"
  done
fi

dnl Turn on default output mode
DEFAULT_PLAYMODE=
AC_ARG_WITH(default-output,
  [  --with-default-output=<mode>  Specify default output mode (optional):
                                (default|alsa|alib|arts|nas|
                                esd|wav|au|aiff|list|vorbis|flac|speex|
                                gogo|portaudio|jack|ao)],
  [ if test "$enable_audio" != no; then
    DEFAULT_PLAYMODE=$withval
    eval "au_enable_$DEFAULT_PLAYMODE=yes"
  else
    AC_MSG_WARN(--with-default-output=$withval: audio is not enabled)
  fi])

dnl compatibility matters.
dnl AC_ARG_ENABLE(esd,
dnl   [  --enable-esd         EsounD (Obsoleted.  Use --enable-audio=esd)],
dnl   [ au_enable_esd=$enableval ])
dnl AC_ARG_ENABLE(nas,
dnl   [  --enable-nas         NAS  (Obsoleted.  Use --enable-audio=nas)],
dnl   [ au_enable_nas=$enableval ])
dnl AC_ARG_ENABLE(alsa,
dnl   [  --enable-alsa        ALSA   (Obsoleted.  Use --enable-audio=alsa)],
dnl   [ au_enable_alsa=$enableval ])

dnl target-specific defaults
if test "x$au_enable_default" = xyes; then
  case "$target" in
  *-*-linux*|*-*-freebsd*)
    au_enable_oss=yes
    ;;
  *-*-bsdi2.0)
    au_enable_sb_dsp=yes
    ;;
  *-*-bsdi2.1|*-*-bsdi3.?|*-*-bsdi4.?)
    au_enable_oss=yes
    ;;
  *-*-hpux*)
    au_enable_hpux=yes
    ;;
  *-dec-*)
    au_enable_mme=yes
    ;;
  *irix*)
    au_enable_irix=yes
    ;;
  *-*-sunos4*)
    au_enable_sun=yes
    ;;
  *-*-solaris*)
    au_enable_sun=yes
    ;;
  *-*-netbsd*)
    au_enable_sun=yes
    ;;
  *-*-cygwin*)
    au_enable_w32=yes
    ;;
  *-*-mingw*)
    au_enable_w32=yes
    ;;
  *-*-darwin*)
    au_enable_darwin=yes
    ;;
  *)
    AC_MSG_WARN(No --enable-audio=default audio for $target)
    ;;
  esac
fi


# Each audio mode's configurations
dnl oss
AC_MSG_CHECKING(enable_audio=oss)
if test "x$au_enable_oss" = xyes; then
  EXTRADEFS="$EXTRADEFS -DAU_OSS"
  SYSEXTRAS="$SYSEXTRAS oss_a.c"
  if test "x$ac_cv_header_sys_soundcard_h" = xyes; then
    AC_MSG_RESULT(yes - <sys/soundcard.h>)
  else
    case "$target" in
      *linux*|*freebsd*)
        dnl <linux/soundcard.h> or <machine/soundcard.h>
        AC_MSG_RESULT(yes)
        ;;
      *)
        if test "x$ac_cv_header_soundcard_h" = xyes; then
          AC_MSG_RESULT(yes - <soundcard.h>)
        else
          AC_MSG_WARN(<sys/soundcard.h> is not found)
        fi
	;;
    esac
  fi
  AC_CHECK_LIB(ossaudio,open)

  dnl Why OpenBSD use /dev/audio instead of /dev/dsp for OSS
  case "$target" in
    *openbsd*)
      if test ! -e "/dev/dsp"; then
        oss_device=/dev/audio
      fi
      ;;
  esac
else
  AC_MSG_RESULT(no)
fi

dnl SunOS's
AC_MSG_CHECKING(enable_audio=sun)
if test "x$au_enable_sun" = xyes; then
  case "$target" in
    *-*-sunos4*)
      if test -f /usr/demo/SOUND/libaudio.a; then
        EXTRALIBS="$EXTRALIBS /usr/demo/SOUND/libaudio.a"
	AC_MSG_RESULT(yes)
      else
	AC_MSG_RESULT(Notice: libaudio.a is not found [(ignore)])
      fi
      ;;
    *-*-solaris*)
      if test -f /usr/demo/SOUND/lib/libaudio.a; then
        EXTRALIBS="$EXTRALIBS /usr/demo/SOUND/lib/libaudio.a"
	AC_MSG_RESULT(yes)
      else
        AC_MSG_RESULT(Notice: libaudio.a is not found [(ignore)])
      fi
      ;;
    *)   AC_MSG_RESULT(yes)
      ;;
  esac
  EXTRADEFS="$EXTRADEFS -DAU_SUN"
  SYSEXTRAS="$SYSEXTRAS sun_a.c"
else
  AC_MSG_RESULT(no)
fi

dnl HP-UX's
AC_MSG_CHECKING(enable_audio=hpux)
if test "x$au_enable_hpux" = xyes; then
  case "$target" in
    *-*-hpux*) AC_MSG_RESULT(yes) ;;
    *) AC_MSG_WARN(may not work on $target) ;;
  esac
  EXTRADEFS="$EXTRADEFS -DAU_HPUX_AUDIO"
  SYSEXTRAS="$SYSEXTRAS hpux_d_a.c"
else
  AC_MSG_RESULT(no)
fi

dnl Irix's
AC_MSG_CHECKING(enable_audio=irix)
if test "x$au_enable_irix" = xyes; then
  case "$target" in
    *-sgi-irix5*|*-sgi-irix6.2)
      EXTRADEFS="$EXTRADEFS -DAU_AUDRIV -DSGI_OLDAL"
      AC_MSG_RESULT(yes)
      ;;
    *-sgi-irix6*)
      EXTRADEFS="$EXTRADEFS -DAU_AUDRIV -DSGI_NEWAL"
      AC_MSG_RESULT(yes)
      ;;
    *) AC_MSG_WARN(may not work on $target) ;;
  esac
  SYSEXTRAS="$SYSEXTRAS audriv_a.c audriv_al.c"
  EXTRALIBS="$EXTRASLIBS -laudio"
else
  AC_MSG_RESULT(no)
fi

dnl OSF/1 MME
AC_MSG_CHECKING(enable_audio=mme)
if test "x$au_enable_mme" = xyes; then
  case "$target" in
    *-dec-*) AC_MSG_RESULT(yes) ;;
    *) AC_MSG_WARN(may not work on $target) ;;
  esac
  EXTRADEFS="$EXTRADEFS -DAU_DEC -DAU_AUDRIV"
  SYSEXTRAS="$SYSEXTRAS audriv_a.c audriv_mme.c"
  CPPFLAGS="$CPPFLAGS -I/usr/opt/MME210/include"
  EXTRALIBS="$EXTRALIBS /usr/opt/MME220/lib/libmme.a"
else
  AC_MSG_RESULT(no)
fi

dnl BSD/OS's sb_dsp
AC_MSG_CHECKING(enable_audio=sb_dsp)
if test "x$au_enable_sb_dsp" = xyes; then
  case "$target" in
    *-*-bsdi2.0) AC_MSG_RESULT(yes) ;;
    *) AC_MSG_WARN(may not work on $target) ;;
  esac
  EXTRADEFS="$EXTRADEFS -DAU_BSDI -DDEFAULT_RATE=22500"
  SYSEXTRAS="$SYSEXTRAS bsd20_a.c"
else
  AC_MSG_RESULT(no)
fi

dnl Windows' MME
AC_MSG_CHECKING(enable_audio=w32)
if test "x$au_enable_w32" = xyes; then
  case "$target" in
    *-*-cygwin*)
     AC_MSG_RESULT(yes)
     ;;
    *-*-mingw32)
     AC_MSG_RESULT(yes)
     ;;
    *)
     AC_MSG_WARN(may not work on $target)
     ;;
  esac
  SYSEXTRAS="$SYSEXTRAS w32_a.c"
  if test "x$VCPP" = xyes || test "x$BORLANDC" = xyes || test "x$WATCOM_C" = xyes; then
  	EXTRALIBS="$EXTRALIBS winmm.lib"
  else
    EXTRALIBS="$EXTRALIBS -lwinmm"
  fi
  EXTRADEFS="$EXTRADEFS -DAU_W32"
else
  AC_MSG_RESULT(no)
fi

dnl Mac OS X / darwin-x86
AC_MSG_CHECKING(enable_audio=darwin)
if test "x$au_enable_darwin" = xyes; then
  EXTRADEFS="$EXTRADEFS -DAU_DARWIN"
  SYSEXTRAS="$SYSEXTRAS darwin_a.c"
  EXTRALIBS="$EXTRALIBS -framework CoreAudio"
  dnl just say yes or no :-P
  case "$target" in
    *-*-darwin*)
     AC_MSG_RESULT(yes);;
   *)
     AC_MSG_WARN(may not work on $target)
     ;;
  esac
else
  AC_MSG_RESULT(no)
fi


dnl alsa - Advanced Linux Sound Architecture
AC_MSG_CHECKING(enable_audio=alsa)
if test "x$au_enable_alsa" = xyes; then
  AC_MSG_RESULT([yes, configuring alsa])
  KEEPCFLAGS=$CFLAGS
  KEEPLIBS=$LIBS
  KEEPLDFLAGS=$LDFLAGS
  AM_PATH_ALSA()
  if test "x$alsa_found" = "xyes" ; then
    dnl AC_MSG_RESULT(yes)
    EXTRADEFS="$EXTRADEFS -DAU_ALSA"
    SYSEXTRAS="$SYSEXTRAS alsa_a.c"
    EXTRACT_CPPFLAGS(CPPFLAGS,CFLAGS,$ALSA_CFLAGS)
    LIBS="$LIBS $ALSA_LIBS"
    AC_CHECK_FUNCS(snd_seq_port_info_set_timestamping)
  else
    AC_MSG_WARN(Couldn't configure alsa.)
    CFLAGS=$KEEPCFLAGS
    LIBS=$KEEPLIBS
    LDFLAGS=$KEEPLDFLAGS
  fi
else
  AC_MSG_RESULT(no)
fi

dnl HP-UX Alib
AC_MSG_CHECKING(enable_audio=alib)
if test "x$au_enable_alib" = xyes; then
  case "$target" in
    *-*-hpux*) AC_MSG_RESULT(yes) ;;
    *) AC_MSG_WARN(may not work on $target) ;;
  esac
  SYSEXTRAS="$SYSEXTRAS hpux_a.c"
  EXTRADEFS="$EXTRADEFS -DAU_HPUX_ALIB"
  if test -d /opt/audio/include; then
    CPPFLAGS="$CPPFLAGS -I/opt/audio/include"
  fi
  if test -d /opt/audio/lib; then
    EXTRALIBS="$EXTRALIBS -L/opt/audio/lib"
  fi
  EXTRALIBS="$EXTRALIBS -lAlib"
else
  AC_MSG_RESULT(no)
fi

dnl nas
AC_MSG_CHECKING(enable_audio=nas)
if test "x$au_enable_nas" = xyes; then
  if test "x$with_x" != xyes; then
    AC_MSG_ERROR(nas: --with-x option must be specified)
  fi
  EXTRADEFS="$EXTRADEFS -DAU_NAS"
  SYSEXTRAS="$SYSEXTRAS nas_a.c"

  dnl -laudio conflicts another audio library,
  dnl so I specify NAS library with absolete path.

  if test "x$with_nas_library" != x; then
    AC_MSG_RESULT($with_nas_library);
    EXTRALIBS="$EXTRALIBS $with_nas_library"
  else
    AC_MSG_RESULT()
    MY_SEARCH_LIBS(AuOpenServer,
	[$x_libraries/libaudio.a \
	 /usr/lib/libaudio.so \
	 /usr/lib/libaudio.a ],
	[ AC_MSG_RESULT(NAS: $timidity_cv_search_AuOpenServer)
	  EXTRALIBS="$EXTRALIBS $timidity_cv_search_AuOpenServer"],
	[AC_MSG_WARN(NAS library is not found.)])
  fi
  test "x$with_nas_includes" != x && CPPFLAGS="$CPPFLAGS -I$with_nas_includes"
  lib_xt_opt=-lXt
  if test "x$have_xext" = xyes; then
    lib_xext_opt=-lXext
  fi
  if test "x$have_xprelibs" = xyes; then
    lib_xprelibs_opt='-lSM -lICE'
  fi
else
  AC_MSG_RESULT(no)
fi

dnl aRts of KDE
AC_MSG_CHECKING(enable_audio=arts)
if test "x$au_enable_arts" = xyes; then
  AC_MSG_RESULT([yes, configuring aRts])
  KEEPCFLAGS=$CFLAGS
  KEEPLIBS=$LIBS
  KEEPLDFLAGS=$LDFLAGS
  AM_PATH_ARTS()
  CFLAGS=$KEEPCFLAGS
  LIBS=$KEEPLIBS
  LDFLAGS=$KEEPLDFLAGS
  if test "x$no_arts" = x; then
    dnl AC_MSG_RESULT(aRts: Enabled)
    EXTRADEFS="$EXTRADEFS -DAU_ARTS"
    SYSEXTRAS="$SYSEXTRAS aRts_a.c"
    EXTRACT_CPPFLAGS(CPPFLAGS,CFLAGS,$ARTS_CFLAGS)
    LIBS="$LIBS $ARTS_LIBS"
  else
    AC_MSG_WARN(aRts: Couldn't configure)
  fi
else
  AC_MSG_RESULT(no)
fi

dnl EsounD
AC_MSG_CHECKING(enable_audio=esd)
if test "x$au_enable_esd" = xyes; then
  AC_MSG_RESULT([yes, configuring esd])
  KEEPCFLAGS=$CFLAGS
  KEEPLIBS=$LIBS
  KEEPLDFLAGS=$LDFLAGS
  AM_PATH_ESD()
  CFLAGS=$KEEPCFLAGS
  LIBS=$KEEPLIBS
  LDFLAGS=$KEEPLDFLAGS
  if test "x$no_esd" = x; then
    dnl AC_MSG_RESULT(EsounD: Enabled)
    EXTRADEFS="$EXTRADEFS -DAU_ESD"
    SYSEXTRAS="$SYSEXTRAS esd_a.c"
    EXTRACT_CPPFLAGS(CPPFLAGS,CFLAGS,$ESD_CFLAGS)
    LIBS="$LIBS $ESD_LIBS"
  else
    AC_MSG_WARN(EsounD: Couldn't configure)
  fi
else
  AC_MSG_RESULT(no)
fi

dnl portaudio E
AC_MSG_CHECKING(enable_audio=portaudio)
if test "x$au_enable_portaudio" = xyes; then
  AC_MSG_RESULT(yes)
  if test "x$CYGNUS" = xyes ||test "x$MSYS" = xyes; then
	SYSEXTRAS="$SYSEXTRAS portaudio_a.c w32_portaudio_dll.c"
  	EXTRALIBS="$EXTRALIBS -lportaudio"
  	EXTRADEFS="$EXTRADEFS -DAU_PORTAUDIO -DAU_PORTAUDIO_DLL"
  else
  	SYSEXTRAS="$SYSEXTRAS portaudio_a.c"
  	EXTRALIBS="$EXTRALIBS -lportaudio"
  	EXTRADEFS="$EXTRADEFS -DAU_PORTAUDIO"
  fi
else
  AC_MSG_RESULT(no)
fi

dnl JACK
AC_MSG_CHECKING(enable_audio=jack)
if test "x$au_enable_jack" = xyes; then
  AC_MSG_RESULT(yes)
  SYSEXTRAS="$SYSEXTRAS jack_a.c"
  EXTRALIBS="$EXTRALIBS $(pkg-config --libs jack)"
  EXTRADEFS="$EXTRADEFS -DAU_JACK $(pkg-config --cflags jack)"
else
  AC_MSG_RESULT(no)
fi

dnl AO
AC_MSG_CHECKING(enable_audio=ao)
if test "x$au_enable_ao" = xyes; then
  AC_MSG_RESULT([yes, configuring ao])
  XIPH_PATH_AO()
  if test "x$no_ao" = "x" ; then
    # AC_MSG_RESULT(yes)
    EXTRADEFS="$EXTRADEFS -DAU_AO"
    SYSEXTRAS="$SYSEXTRAS ao_a.c"
    EXTRACT_CPPFLAGS(CPPFLAGS,CFLAGS,$AO_CFLAGS)
    LIBS="$LIBS $AO_LIBS"
  else
    AC_MSG_WARN(Couldn't configure libao.)
  fi
else
  AC_MSG_RESULT(no)
fi

dnl ogg's vorbis
AC_MSG_CHECKING(enable_audio=vorbis)
if test "x$au_enable_vorbis" = xyes; then
  AC_MSG_RESULT([yes, configuring vorbis])
  AM_PATH_OGG([
    AM_PATH_VORBIS([
      have_vorbis=yes
      SYSEXTRAS="$SYSEXTRAS vorbis_a.c"
      EXTRADEFS="$EXTRADEFS -DAU_VORBIS $OGG_CFLAGS $VORBIS_CFLAGS"
      EXTRALIBS="$EXTRALIBS $VORBIS_LIBS $VORBISENC_LIBS $OGG_LIBS"
      if test "x$CYGNUS" = "xyes"; then
      EXTRADEFS="-DAU_VORBIS_DLL $EXTRADEFS"
	  SYSEXTRAS="$SYSEXTRAS w32g_vorbisenc_dll.c w32g_vorbis_dll.c w32g_ogg_dll.c"
      fi
      if test "x$MSYS" = "xyes"; then
      EXTRADEFS="-DAU_VORBIS_DLL $EXTRADEFS"
	  SYSEXTRAS="$SYSEXTRAS w32g_vorbisenc_dll.c w32g_vorbis_dll.c w32g_ogg_dll.c"
      fi
    ],[
      AC_MSG_WARN([Unable to configure vorbis, but ogg is there (???)])
    ])
  ],[
    AC_MSG_WARN(Unable to configure vorbis!)
    have_vorbis=no
  ])
else
  AC_MSG_RESULT(no)
fi

dnl libFLAC - Free Lossless Audio Codec
if test "x$CYGNUS" = xyes || test "x$MSYS" = xyes ; then
  AC_MSG_CHECKING(enable_audio=flac)
  if test "x$au_enable_flac" = xyes; then
    AC_MSG_RESULT([yes, configuring flac])
    SYSEXTRAS="$SYSEXTRAS flac_a.c w32_libFLAC_dll.c w32_libOggFLAC_dll.c"
    AM_PATH_LIBFLAC([
      EXTRADEFS="$EXTRADEFS -DAU_FLAC -DAU_FLAC_DLL"
      EXTRACT_CPPFLAGS(CPPFLAGS,CFLAGS,$LIBFLAC_CFLAGS)
      LIBS="$LIBS $LIBFLAC_LIBS"
    ])
    AM_PATH_LIBOGGFLAC([
      EXTRADEFS="$EXTRADEFS -DAU_OGGFLAC -DAU_OGGFLAC_DLL"
      EXTRACT_CPPFLAGS(CPPFLAGS,CFLAGS,$LIBOGGFLAC_CFLAGS)
      LIBS="$LIBS $LIBOGGFLAC_LIBS"
    ])
  else
    AC_MSG_RESULT(no)
  fi
else
  AC_MSG_CHECKING(enable_audio=flac)
  if test "x$au_enable_flac" = xyes; then
    AC_MSG_RESULT([yes, configuring flac])
    SYSEXTRAS="$SYSEXTRAS flac_a.c"
    AM_PATH_LIBFLAC([
      EXTRADEFS="$EXTRADEFS -DAU_FLAC"
      EXTRACT_CPPFLAGS(CPPFLAGS,CFLAGS,$LIBFLAC_CFLAGS)
      LIBS="$LIBS $LIBFLAC_LIBS"
    ])
    AM_PATH_LIBOGGFLAC([
      EXTRADEFS="$EXTRADEFS -DAU_OGGFLAC"
      EXTRACT_CPPFLAGS(CPPFLAGS,CFLAGS,$LIBOGGFLAC_CFLAGS)
      LIBS="$LIBS $LIBOGGFLAC_LIBS"
    ])
  else
    AC_MSG_RESULT(no)
  fi
fi
dnl ogg's speex
AC_MSG_CHECKING(enable_audio=speex)
if test "x$au_enable_speex" = xyes; then
  AC_CACHE_VAL(have_speex,
  [AC_TRY_LINK([
  #include <speex.h>
  #include <ogg/ogg.h>
  ],
  [
    SpeexBits *dummy;
  ],
  [have_speex=yes], [have_speex=no])])
  AC_MSG_RESULT($have_speex)
  if test "x$have_speex" = xyes; then
    SYSEXTRAS="$SYSEXTRAS speex_a.c"
    EXTRALIBS="$EXTRALIBS -lspeex -logg"
    EXTRADEFS="$EXTRADEFS -DAU_SPEEX"
  fi
else
  AC_MSG_RESULT(no)
fi

dnl Win32 Gogo-no-coder
AC_MSG_CHECKING(enable_audio=gogo)
if test "x$au_enable_gogo" = xyes; then
  if test "x$CYGNUS" = "xyes"; then
    AC_MSG_RESULT(yes, configuring gogo)
  else
    if test "x$MSYS" = "xyes"; then
    AC_MSG_RESULT(yes, configuring gogo)
    else
      AC_MSG_WARN(May be gogo is not supported.  You need hack the gogo_a.c)
    fi
  fi
  have_gogo=yes
  SYSEXTRAS="$SYSEXTRAS gogo_a.c w32_gogo.c"
  EXTRADEFS="$EXTRADEFS -DAU_GOGO -DAU_GOGO_DLL"
  EXTRALIBS="$EXTRALIBS -lgogo"
else
  AC_MSG_RESULT(no)
fi

dnl Pseudo audio device (Undocumented).
if test "x$au_enable_none" = xyes; then
    EXTRADEFS="$EXTRADEFS -DAU_NONE -DAU_AUDRIV"
    SYSEXTRAS="$SYSEXTRAS audriv_a.c audriv_none.c"
fi
dnl
dnl TIMIDITY_OUTPUT_ID
dnl
AC_MSG_CHECKING(default output mode)
if test "x$DEFAULT_PLAYMODE" = x; then
  DEFAULT_PLAYMODE=`echo $enable_audio | sed 's/,.*//'`
fi
case ".$DEFAULT_PLAYMODE" in
  .default)  TIMIDITY_OUTPUT_ID=d ;;
  .oss)      TIMIDITY_OUTPUT_ID=d ;;
  .alsa)     TIMIDITY_OUTPUT_ID=s ;;
  .sun)      TIMIDITY_OUTPUT_ID=d ;;
  .hpux)     TIMIDITY_OUTPUT_ID=d ;;
  .irix)     TIMIDITY_OUTPUT_ID=d ;;
  .mme)      TIMIDITY_OUTPUT_ID=d ;;
  .sb_dsp)   TIMIDITY_OUTPUT_ID=d ;;
  .darwin)   TIMIDITY_OUTPUT_ID=d ;;
  .w32)      TIMIDITY_OUTPUT_ID=d ;;
  .alib)     TIMIDITY_OUTPUT_ID=A ;;
  .nas)      TIMIDITY_OUTPUT_ID=n ;;
  .arts)     TIMIDITY_OUTPUT_ID=R ;;
  .esd)      TIMIDITY_OUTPUT_ID=e ;;
  .portaudio)      TIMIDITY_OUTPUT_ID=p ;;  
  .wav)      TIMIDITY_OUTPUT_ID=w ;;
  .au)       TIMIDITY_OUTPUT_ID=u ;;
  .aiff)     TIMIDITY_OUTPUT_ID=a ;;
  .list)     TIMIDITY_OUTPUT_ID=l ;;
  .vorbis)   TIMIDITY_OUTPUT_ID=v ;;
  .flac)     TIMIDITY_OUTPUT_ID=F ;;
  .speex)    TIMIDITY_OUTPUT_ID=S ;;
  .gogo)     TIMIDITY_OUTPUT_ID=g ;;
  .jack)     TIMIDITY_OUTPUT_ID=j ;;
  *)         TIMIDITY_OUTPUT_ID= ;;
esac
AC_MSG_RESULT($DEFAULT_PLAYMODE/$TIMIDITY_OUTPUT_ID)

#
# Interface Section
#

interface_targets='dynamic ncurses slang motif tcltk emacs vt100 xaw xskin gtk alsaseq winsyn winsyng portmidisyng'

AC_ARG_ENABLE(interface,
  [  --enable-interface=[interface_list]
                          Comma separated interface list ],
  [ for i in `echo $enableval | sed 's/,/ /g'`; do
      eval "enable_$i=yes"
    done ])

#  Dynamic link configuration
AC_ARG_ENABLE(dynamic,
  [  --enable-dynamic        Enable dynamic link interface   (default is no)
  --enable-dynamic=dynamic_list
                          Comma separated dynamic interface list],
  [ dnl --enable-dynamic=yes to --enable-dynamic=dynamic
    if test "x$enable_dynamic" = xyes; then
      enable_dynamic=dynamic
    elif test "x$enable_dynamic" != xno; then
      for i in `echo $enableval | sed 's/,/ /g'`; do
	eval "enable_$i=dynamic"
      done
    fi])

join_targets=
for i in $interface_targets; do
  eval "join_targets=$join_targets\$enable_$i"
done

case ".$join_targets" in
.*dynamic*) NEEDDLOPEN=yes;;
*)	    NEEDDLOPEN=no;;
esac

dnl check -fpic option for gcc
if test "x$GCC" = xyes; then SHCFLAGS="$SHCFLAGS -fPIC"; fi
case "x$enable_ump" in xyes|xdynamic) EXTRACFLAGS="$EXTRACFLAGS -DXP_UNIX $SHCFLAGS";; esac

dnl Which type of dynamic loader?
if test "$NEEDDLOPEN" = "yes"; then
  have_dl=no

  WAPI_CHECK_FUNC(LoadLibrary,,[LoadLibrary(0);],have_dl=w32)
  test $wapi_cv_func_LoadLibrary = yes && have_dl=w32

  if test "$have_dl" = "no"; then
    AC_CHECK_FUNC(shl_load,have_dl=hpux)
  fi
  if test "$have_dl" = "no"; then
    dnl darwin has both dyld and libdl: in case user installed dlcompat.
    dnl dyld is recommended because libdl is a wrapper for dyld.
    AC_CHECK_FUNC(NSLinkModule,have_dl=dyld)
  fi
  if test "$have_dl" = "no" -a "x$ac_cv_lib_dl_dlopen" = xyes; then
    have_dl=dlopen
  fi
  if test "$have_dl" = "no"; then
    AC_CHECK_FUNC(dlopen,have_dl=dlopen)
  fi
  if test "$have_dl" = "no"; then
    AC_CHECK_FUNC(dld_init,have_dl=dld)
  fi
  if test "$have_dl" = "no"; then
    AC_CHECK_LIB(dld,dld_init,have_dl=dld;lib_dl_opt=-ldld)
  fi

  AC_MSG_CHECKING(dynamic link method)
  case "$have_dl" in
    dlopen)
      AC_MSG_RESULT(use dl_dlopen.c)
      SYSEXTRAS="$SYSEXTRAS dl_dlopen.c"
      CHECK_DLSYM_UNDERSCORE(AC_DEFINE(DLSYM_NEEDS_UNDERSCORE,1,Define to 1 if you have to add "_" to every identifiers.))
      ;;
    dld)
      AC_MSG_RESULT(use dl_dld.c)
      SYSEXTRAS="$SYSEXTRAS dl_dld.c"
      ;;
    hpux)
      AC_MSG_RESULT(use dl_hpux.c)
      SYSEXTRAS="$SYSEXTRAS dl_hpux.c"
      LDFLAGS="$LDFLAGS -Wl,-E $LDFLAGS"
      ;;
    w32)
      AC_MSG_RESULT(use dl_w32.c)
      SYSEXTRAS="$SYSEXTRAS dl_w32.c"
      ;;
    dyld)
      AC_MSG_RESULT(use dl_dyld.c)
      SYSEXTRAS="$SYSEXTRAS dl_dyld.c"
      ;;
    *)
      NEEDDLOPEN="no"
      AC_MSG_WARN(no method to open a shared object)
      ;;
  esac
fi

if test "$NEEDDLOPEN" = "yes"; then
  MY_DEFINE(IA_DYNAMIC)
  AC_DEFINE_UNQUOTED(SHARED_LIB_EXT,".$so", shared library extention)
  dnl needless?
  dnl EXTRADEFS="$EXTRADEFS \$(DEF_SHLIB_DIR)"
  test "x$GCC" = xyes -a "x$timidity_cv_ccoption_rdynamic" = xyes  && timidity_LDFLAGS="-rdynamic"
fi

AM_CONDITIONAL(NEEDDLOPEN, test "$NEEDDLOPEN" = "yes")

dnl
dnl Each interface section
dnl

dnl ncurses
AM_CONDITIONAL(ENABLE_NCURSES, false)
CONFIG_INTERFACE(ncurses,NCURSES,n,
  [  --enable-ncurses        Enable ncurses interface        (default is no)],
  [ AC_CHECK_HEADERS(ncurses.h ncurses/curses.h curses.h)

dnl #include <ncurses/curses.h> is failure on Plamo Linux 1.3/ncurses 1.9.9e
dnl because <ncurses/curses.h> includes <unctrl.h>:
dnl /usr/include/ncurses/curses.h:34: unctrl.h: No such file or directory
dnl But surely there is unctl.h at /usr/include/ncurses/unctrl.h.
dnl configure must check ncurses header with -I/usr/include/ncurses option.

  case "$ac_cv_header_curses_h$ac_cv_header_ncurses_curses_h$ac_cv_header_ncurses_h" in
    *yes*);;
    *)	for i in /usr/include /usr/local/include; do
	  if test -f "$i/ncurses/curses.h" -a -f "$i/ncurses/unctrl.h"; then
	    AC_MSG_WARN(ncurses test is failure.  Please check config.h and common.makefile later)
	    CPPFLAGS="$CPPFLAGS -I$i/ncurses"
	    break
	  fi
	done
	;;
    esac

    AC_CHECK_LIB(ncurses,initscr,lib_curses_opt=-lncurses,
      [ dnl checking pdcurses
	AC_CHECK_LIB(curses,PDC_set_ctrl_break,
          [ lib_curses_opt=-lcurses
	    AC_DEFINE(USE_PDCURSES,1,Define to 1 if you use PDcurses)
	    lib_user32_opt="$lib_user32_test"
	  ],
	  [ dnl checking libpdcurses
	    AC_CHECK_LIB(pdcurses,PDC_set_ctrl_break,
	    [ lib_curses_opt=-lpdcurses
	      AC_DEFINE(USE_PDCURSES,1,Define to 1 if you use PDcurses)
	      lib_user32_opt="$lib_user32_test"
	    ],
	    [ dnl OpenBSD use -lcurses instead of -lncurses.
	      case "$target" in
		*openbsd*)
		  AC_CHECK_LIB(curses,initscr,lib_curses_opt=-lcurses,
		    [ AC_MSG_WARN(ncurses interface is not enabled)
		      enable_ncurses=no ])
		  ;;
		*)
		  if test "x$VCPP" = xyes || test "x$BORLANDC" = xyes || test "x$WATCOM_C" = xyes; then
		    lib_curses_opt=libpdcurses.lib
		    AC_DEFINE(USE_PDCURSES,1,Define to 1 if you use PDcurses)
		  else
		    AC_MSG_WARN(ncurses interface is not enabled)
		    enable_ncurses=no
		  fi
		  ;;
	      esac
	    ])
	],
	$lib_user32_test)
      ])
  ],
  [ LIBS="$LIBS $lib_curses_opt"
    case "$target" in
       *-*-freebsd[123]*)
         EXTRALIBS="$EXTRALIBS -lmytinfo"
         ;;
     esac
    INTERFACE_SRCS="$INTERFACE_SRCS ncurs_c.c"
  ],
  [ n_so_libs="$lib_curses_opt"
    case "$target" in
       *-*-freebsd[123]*)
         EXTRALIBS="$EXTRALIBS -lmytinfo"
         ;;
     esac])

dnl slang
AM_CONDITIONAL(ENABLE_SLANG, false)
CONFIG_INTERFACE(slang,SLANG,s,
  [  --enable-slang          Enable slang interface          (default is no)],
  [ AC_CHECK_LIB(slang,SLang_init_tty,:,
    [ enable_slang=no
      AC_MSG_WARN(slang interface is not enabled)
    ])
    AC_CHECK_HEADERS(slang/slang.h slang.h)
  ],
  [ LIBS="$LIBS -lslang"
    INTERFACE_SRCS="$INTERFACE_SRCS slang_c.c"
  ],
  [ s_so_libs="-lslang"
    AC_CHECK_LIB(termcap,initscr,s_so_libs="$s_so_libs -ltermcap")
  ])

dnl Motif
AM_CONDITIONAL(ENABLE_MOTIF, false)
CONFIG_INTERFACE(motif,MOTIF,m,
  [  --enable-motif          Enable motif interface          (default is no)],
  [ if test "x$with_x" != xyes; then
	AC_MSG_ERROR(motif: --with-x option must be specified)
    fi
    if test "x$have_xm" != xyes; then
      AC_MSG_WARN(Motif library is not found.)
      enable_motif=no
    fi
  ],
  [ lib_xm_opt=-lXm
    lib_xt_opt=-lXt
    lib_xext_opt=-lXext
    if test "x$have_xprelibs" = xyes; then
	lib_xprelibs_opt='-lSM -lICE'
    fi
    INTERFACE_SRCS="$INTERFACE_SRCS motif_c.c motif_i.c motif_p.c"
  ],
  [ if test x$have_xprelibs = xyes; then
      m_so_libs="-lXext -lSM -lICE"
    fi
    m_so_libs="$ldflags_x_opt -lXm -lXt $m_so_libs -lX11"
  ])

dnl Tcl/Tk
AM_CONDITIONAL(ENABLE_TCLTK, false)
CONFIG_INTERFACE(tcltk,TCLTK,k,
  [  --enable-tcltk          Enable tcltk interface          (default is no)],
  [ if test "x$with_x" != xyes; then
	AC_MSG_ERROR(tcltk: --with-x option must be specified)
    fi

    dnl save flags
    KEEPCPPFLAGS=$CPPFLAGS
    KEEPLDFLAGS=$LDFLAGS
    KEEPLIBS=$LIBS

    CPPFLAGS="$tcl_include_dir $tk_includes $CPPFLAGS"
    LDFLAGS="$tcl_libdir $tk_libdir $LDFLAGS"
    LIBS="$LIBS $lib_dl_opt"

    tcl_lib=
    tk_lib=
    for l in tcl tcl8.4 tcl8.3 tcl8.0jp tcl7.6jp tcl80jp tcl76jp tcl8.0 tcl7.6 tcl80 tcl76; do
      case "x$tcl_lib" in x) AC_CHECK_LIB($l,Tcl_Init,tcl_lib=-l$l);; esac
    done
    LIBS="$LIBS $tcl_lib"
    for l in tk tk8.4 tk8.3 tk8.0jp tk4.2jp tk80jp tk42jp tk8.0 tk4.2 tk80 tk42; do
      case "x$tk_lib" in x) AC_CHECK_LIB($l,Tk_Init,tk_lib=-l$l);; esac
    done

    LIBS=$KEEPLIBS
    if test "x$tcl_lib" = x -o "x$tk_lib" = x; then
      CPPFLAGS=$KEEPCPPFLAGS
      LDFLAGS=$KEEPLDFLAGS
      enable_tcltk=no
      AC_MSG_WARN(tcl/tk is not enable)
    else
      tcltk_dep='tclIndex'
    fi
  ],
  [ LIBS=`echo $LIBS | sed "s/-lX11/$tk_lib $tcl_lib -lX11/"`
    case "$target" in
      *-*-bsdi*)
	EXTRALIBS="$EXTRALIBS -lipc"
	;;
    esac
    INTERFACE_SRCS="$INTERFACE_SRCS tk_c.c"
  ],
  [ k_so_libs="$tcl_libdir $tk_libdir $tcl_lib $tk_lib $lib_dl_opt"
    case "$target" in
      *-*-bsdi*)
	k_so_libs="$k_so_libs -lipc"
	;;
    esac
    k_so_libs="$k_so_libs -lX11 -lm"
  ])
AM_CONDITIONAL(ENABLE_DYNAMIC_TCLTK, test "x$enable_tcltk" = "xdynamic")

dnl Emacs
AM_CONDITIONAL(ENABLE_EMACS, false)
CONFIG_INTERFACE(emacs,EMACS,e,
  [  --enable-emacs          Enable emacs interface          (default is no)],
  ,
  [ ELFILES="$ELFILES timidity.el"
    INTERFACE_SRCS="$INTERFACE_SRCS emacs_c.c"
  ],
  [ ELFILES="$ELFILES timidity.el" ])

dnl VT100
AM_CONDITIONAL(ENABLE_VT100, false)
CONFIG_INTERFACE(vt100,VT100,T,
  [  --enable-vt100          Enable VT100 interface          (default is no)],
  ,
  [ INTERFACE_SRCS="$INTERFACE_SRCS vt100_c.c vt100.c" ])

dnl X Athena Widget
AM_CONDITIONAL(ENABLE_XAW, false)
CONFIG_INTERFACE(xaw,XAW,a,
  [  --enable-xaw            Enable athena interface         (default is no)],
  [ if test "x$with_x" != xyes; then
	AC_MSG_ERROR(xaw: --with-x option must be specified)
    fi
    if test "x$have_xaw" != "xyes" -a "x$have_xaw" != "x3d"; then
      AC_MSG_WARN(X Athena Widget library is not found.)
      enable_xaw=no
    fi ],
  [ dnl Add -lXext -lSM -lICE -lXmu -lXt {-lXaw|-lXaw3d}
    if test "x$have_xext" = xyes; then
      lib_xext_opt=-lXext
    fi
    if test "x$have_xprelibs" = xyes; then
      lib_xprelibs_opt='-lSM -lICE'
    fi
    lib_xmu_opt=-lXmu
    lib_xt_opt=-lXt
    if test "x$have_xaw" = x3d; then
      AC_DEFINE(XAW3D,1,Define to 1 if you have libXaw3d.)
      lib_xaw_opt=-lXaw3d
    else
      lib_xaw_opt=-lXaw
    fi
    INTERFACE_SRCS="$INTERFACE_SRCS xaw_c.c xaw_i.c"
  ],
  [ if test "x$have_xext" = xyes; then
      a_so_libs="-lXext -lX11"
    else
      a_so_libs="-lX11"
    fi
    case "$target" in
      *darwin*)
	a_so_libs="$a_so_libs -lcc_dynamic"
	;;
    esac
    if test "x$have_xprelibs" = xyes; then
      a_so_libs="-lSM -lICE $a_so_libs"
    fi
    if test "x$have_xaw" = xyes; then
      a_so_libs="-lXaw -lXmu -lXt $a_so_libs"
    elif test "x$have_xaw" = x3d; then
      AC_DEFINE(XAW3D,1,Define to 1 if you have libXaw3d.)
      a_so_libs="-lXaw3d -lXmu -lXt $a_so_libs"
    fi
    a_so_libs="$ldflags_x_opt $a_so_libs"
  ])
AM_CONDITIONAL(ENABLE_DYNAMIC_XAW, test "x$enable_xaw" = "xdynamic")

dnl X skin
AM_CONDITIONAL(ENABLE_XSKIN, false)
CONFIG_INTERFACE(xskin,XSKIN,i,
  [  --enable-xskin          Enable X skin interface         (default is no)],
  [ if test "x$with_x" != xyes; then
	AC_MSG_ERROR(xskin: --with-x option must be specified)
    fi
  ],
  [ INTERFACE_SRCS="$INTERFACE_SRCS xskin_c.c xskin_i.c xskin_loadBMP.c xskin_spectrum.c" ],
  [ i_so_libs="$ldflags_x_opt -lX11" ])

dnl GTK+
AM_CONDITIONAL(ENABLE_GTK, false)
CONFIG_INTERFACE(gtk, GTK, g,
  [  --enable-gtk            Enable GTK+ interface           (default is no)],
  [ AM_PATH_GTK_2_0(2.0.0,
    [AC_DEFINE(HAVE_GTK_2,1,Define to 1 if you have GTK+ 2.x)],
    [AM_PATH_GTK(1.1.3,,enable_gtk=no)]) ],
  [ LIBS="$LIBS $GTK_LIBS"
    EXTRACT_CPPFLAGS(CPPFLAGS,CFLAGS,$GTK_CFLAGS)
    INTERFACE_SRCS="$INTERFACE_SRCS gtk_c.c gtk_i.c gtk_p.c"
  ],
  [ if test "x$GCC" = xyes; then
	dnl Do not include -rdynamic option in $g_so_libs.
	for i in $GTK_LIBS; do
	    case "x$i" in
		x-rdynamic) ;;
		*) g_so_libs="$g_so_libs $i" ;;
	    esac
	done
    else
	g_so_libs="$GTK_LIBS"
    fi
    EXTRACT_CPPFLAGS(CPPFLAGS,CFLAGS,$GTK_CFLAGS) ])

dnl TiMidity Network MIDI Server
AM_CONDITIONAL(ENABLE_SERVER, false)
CONFIG_INTERFACE(server,SERVER,r,
  [  --enable-server         Enable server interface         (default is no)],
  ,
  [ INTERFACE_SRCS="$INTERFACE_SRCS server_c.c"
    AC_CHECK_HEADERS(sys/soundcard.h)
  ])

dnl TiMidity ALSA sequencer server
AM_CONDITIONAL(ENABLE_ALSASEQ, false)
CONFIG_INTERFACE(alsaseq,ALSASEQ,A,
  [  --enable-alsaseq        Enable ALSA sequencer server interface
                                                          (default is no)],
  ,
  [ INTERFACE_SRCS="$INTERFACE_SRCS alsaseq_c.c"
    AM_PATH_ALSA
  ])

dnl TiMidity Windows synthesizer server
AM_CONDITIONAL(ENABLE_WINSYN, false)
CONFIG_INTERFACE(winsyn,WINSYN,W,
  [  --enable-winsyn        Enable Windows Synthesizer interface
                                                          (default is no)],
  ,
  [ INTERFACE_SRCS="$INTERFACE_SRCS winsyn_c.c rtsyn_common.c rtsyn_winmm.c" ])

dnl TiMidity PortMIDI synthesizer server
AM_CONDITIONAL(ENABLE_PORTMIDISYN, false)
CONFIG_INTERFACE(portmidisyn,PORTMIDISYN,P,
  [  --enable-portmidisyn        Enable PortMIDI Synthesizer interface
                                                          (default is no)],
  ,
  [ INTERFACE_SRCS="$INTERFACE_SRCS portmidisyn_c.c rtsyn_common.c rtsyn_portmidi.c" ])

dnl TiMidity Windows GUI synthesizer server
AM_CONDITIONAL(ENABLE_W32G_SYN, false)
CONFIG_INTERFACE(winsyng,W32G_SYN,W,
  [  --enable-winsyng        Enable Windows GUI Synthesizer interface
                                                          (default is no)],
  ,
  [ 
  if test "x$VCPP" = xyes || test "x$BORLANDC" = xyes || test "x$WATCOM_C" = xyes; then
    INTERFACE_SRCS="$INTERFACE_SRCS w32g_ini.c w32g_pref.c w32g_syn.c w32g_utl.c winsyn_c.c rtsyn_common.c rtsyn_winmm.c"
    lib_w32gui_opt='gdi32.lib comctl32.lib comdlg32.lib ole32.lib'
    lib_user32_opt='user32.lib'
    if test "x$VCPP" = xyes; then
	  VCPP_LDFLAGS="$VCPP_LDFLAGS -subsystem:windows "
	fi
	if test "x$BORLANDC" = xyes; then
	  BORLANDC_START='C0W32.OBJ'
	  CFLAGS="$CFLAGS -tW"
	  lib_w32gui_opt="-aa $lib_w32gui_opt"
	fi
	if test "x$WATCOM_C" = xyes; then
	  CFLAGS="$CFLAGS -bw"
	  WATCOM_LDFLAGS="$WATCOM_LDFLAGS SYS nt_win"
	fi
  else
  	lib_w32gui_opt='-lgdi32 -lcomctl32 -lcomdlg32 -lole32'
    lib_user32_opt='-luser32'
    LDFLAGS="-mwindows $LDFLAGS"
    INTERFACE_SRCS="$INTERFACE_SRCS w32g_ini.c w32g_pref.c w32g_syn_escaped.c w32g_utl.c winsyn_c.c rtsyn_common.c rtsyn_winmm.c"
  fi
  ])


dnl Windows GUI
AM_CONDITIONAL(ENABLE_W32GUI, false)
CONFIG_INTERFACE(w32gui, W32GUI, w,
  [  --enable-w32gui         Enable Windows GUI interface    (default is no)],
  ,
  [ 
  if test "x$VCPP" = xyes || test "x$BORLANDC" = xyes || test "x$WATCOM_C" = xyes; then
 	INTERFACE_SRCS="$INTERFACE_SRCS w32g_c.c w32g_ini.c w32g_i.c w32g_playlist.c w32g_utl.c w32g_pref.c w32g_subwin.c w32g_subwin2.c w32g_subwin3.c w32g_ut2.c w32g_dib.c  wrdt_w32g.c w32g_mag.c"
    lib_w32gui_opt='gdi32.lib comctl32.lib comdlg32.lib ole32.lib'
    lib_user32_opt='user32.lib'
    if test "x$VCPP" = xyes; then
	  VCPP_LDFLAGS="$VCPP_LDFLAGS -subsystem:windows "
	fi
	if test "x$BORLANDC" = xyes; then
	  BORLANDC_START='C0W32.OBJ'
	  CFLAGS="$CFLAGS -tW"
	  lib_w32gui_opt="-aa $lib_w32gui_opt"
	fi
	if test "x$WATCOM_C" = xyes; then
	  CFLAGS="$CFLAGS -bw"
	  WATCOM_LDFLAGS="$WATCOM_LDFLAGS SYS nt_win"
	fi
  else
    lib_w32gui_opt='-lgdi32 -lcomctl32 -lcomdlg32 -lole32'
    lib_user32_opt='-luser32'
    LDFLAGS="-mwindows $LDFLAGS"
    INTERFACE_SRCS="$INTERFACE_SRCS w32g_c.c w32g_ini.c w32g_i.c w32g_playlist.c w32g_utl.c w32g_pref.c w32g_subwin_escaped.c w32g_subwin2.c w32g_subwin3.c w32g_ut2.c w32g_dib.c  wrdt_w32g.c w32g_mag.c"
  fi
  ])

dnl UMP configuration
AM_CONDITIONAL(ENABLE_PLUGIN, false)
CONFIG_INTERFACE(ump, PLUGIN, p,
  [  --enable-ump            UMP configuration               (default is no)],
  [ if test ! -d ump; then
	AC_MSG_ERROR(UMP Package is not exists.  Can't configure it.)
    fi
    AC_CHECK_HEADER(X11/xpm.h,,AC_MSG_ERROR([UMP Package needs xpm.h[,] but could not find it.]))
    AC_CHECK_LIB(Xpm,XpmCreatePixmapFromData,:,AC_MSG_ERROR([UMP Package needs libXpm.a[,] but could not find it.]))
    if test "x$have_xext" = xyes; then
      lib_xext_opt=-lXext
    fi
    if test "x$have_xprelibs" = xyes; then
      lib_xprelibs_opt='-lSM -lICE'
    fi
    lib_xmu_opt=-lXmu
    lib_xt_opt=-lXt
  ])


dnl Offix support
dnl this rely on  CFLAGS and LIBS
AC_ARG_ENABLE(offix,
  [  --enable-offix          Enable offix support            (default is no)],
  [ if test "$enableval" = "yes"; then
      KEEPCPPFLAGS=$CPPFLAGS
      KEEPLIBS=$LIBS
      KEEPLDFLAGS=$LDFLAGS
      CPPFLAGS="$CPPFLAGS $offix_include_dir"
      LDFLAGS="$LDFLAGS $offix_lib_dir"
      LIBS="$LIBS $lib_xmu_opt $lib_xt_opt $lib_xprelibs_opt $lib_xext_opt -lX11"
      AC_CHECK_LIB(Dnd,DndInitialize,[
	LIBS=$KEEPLIBS
	lib_offix_opt="-lDnd"
	AC_DEFINE(OFFIX,1,Define to 1 if you need offix support)
      ],
      [ LDFLAGS=$KEEPLDFLAGS
	CPPFLAGS=$KEEPCPPFLAGS
	LIBS=$KEEPLIBS
      ])
    fi
  ])

dnl Network section
AC_ARG_ENABLE(network,
  [  --enable-network        Enable network support          (default is no)],
  [ if test "$enableval" = "yes"; then
      AC_DEFINE(SUPPORT_SOCKET,1,Define to 1 if you need network support)
    fi ])
AM_CONDITIONAL(ENABLE_NETWORK, test "x$enable_network" = "xyes")

dnl Sound Spectrogram Viewer
AC_ARG_ENABLE(spectrogram,
  [  --enable-spectrogram    Enable Sound Spectrogram Viewer (default is no)],
  [ if test "$enableval" = "yes"; then
      if test "x$with_x" != xyes; then
	AC_MSG_ERROR(spectrogram: --with-x option must be specified)
      fi
      AC_DEFINE(SUPPORT_SOUNDSPEC,1,Define to 1 if you need spectrogram vewer.)
    fi ])
AM_CONDITIONAL(ENABLE_SOUND_SPEC, test "x$enable_spectrogram" = "xyes")

dnl Wave Spline Interpolations
AC_ARG_ENABLE(spline,
  [  --enable-spline=[method]  Specify spline method.
                                  one of no,linear,cubic,lagrange,newton,gauss
                                                          (default is linear)],
  [ case "x$enableval" in
      xlinear)
        AC_DEFINE(DEFAULT_RESAMPLATION,resample_linear,Define resampler.)
	;;
      xcubic)
        AC_DEFINE(DEFAULT_RESAMPLATION,resample_cspline,Define resampler.)
	;;
      xlagrange)
        AC_DEFINE(DEFAULT_RESAMPLATION,resample_lagrange,Define resampler.)
	;;
      xnewton)
        AC_DEFINE(DEFAULT_RESAMPLATION,resample_newton,Define resampler.)
	;;
      xgauss)
        AC_DEFINE(DEFAULT_RESAMPLATION,resample_gauss,Define resampler.)
	;;
      xno)
        AC_DEFINE(DEFAULT_RESAMPLATION,resample_none,Define resampler.)
	;;
      *)
	AC_MSG_ERROR(Invalid method of --enable-spline)
	;;
    esac ])

dnl WRD interface
AC_ARG_ENABLE(wrd,
  [  --enable-wrd            Enable WRD interface for X      (default is no)],
  [ if test "$enableval" = "yes"; then
      if test "x$with_x" != xyes; then
	AC_MSG_ERROR(wrd: --with-x option must be specified)
      fi
      AC_DEFINE(WRDT_X,1,Define to 1 if you need wrd support for X.)
      enable_sherry_wrd=yes
      AC_CHECK_LIB(z,zlibVersion,:,enable_sherry_wrd=no)
      AC_CHECK_HEADER(png.h,,enable_sherry_wrd=no)
      AC_CHECK_LIB(png,png_init_io,:,enable_sherry_wrd=no,-lz)
      if test "$enable_sherry_wrd" = yes; then
	AC_DEFINE(ENABLE_SHERRY,1,Define to 1 if you enable Sherry WRD)
	lib_zip_opt=-lz
	lib_png_opt=-lpng
      else
	 AC_MSG_WARN([Sherry WRD is disabled: png.h, libpng.a, libz.a are required])
      fi
      if test "x$have_xext" = xyes; then
	lib_xext_opt=-lXext
      fi
    fi ])

dnl end of interfaces


#
# Finish up
#

AM_CONDITIONAL(ENABLE_WRD, test "x$enable_wrd" = "xyes")
AM_CONDITIONAL(CYGNUS, test "x$CYGNUS" = xyes)
AM_CONDITIONAL(MSYS, test "x$MSYS" = xyes)
AM_CONDITIONAL(VCPP, test "x$VCPP" = xyes)
AM_CONDITIONAL(BORLANDC, test "x$BORLANDC" = xyes)
AM_CONDITIONAL(WATCOM_C, test "x$WATCOM_C" = xyes)

AM_CONDITIONAL(W32READDIR, test "x$W32READDIR" = "xyes")

SET_UNIQ_WORDS(LDFLAGS,$LDFLAGS)
SET_UNIQ_WORDS(SHLDFLAGS,$SHLDFLAGS)
SET_UNIQ_WORDS(CFLAGS,$EXTRACFLAGS $CFLAGS)
SET_UNIQ_WORDS(CPPFLAGS,$CPPFLAGS $EXTRADEFS)

if test "x$oss_device" != x; then
  AC_DEFINE_UNQUOTED(OSS_DEVICE,"$oss_device", oss device name)
fi

if test "x$TIMIDITY_OUTPUT_ID" != x; then
  AC_DEFINE_UNQUOTED(TIMIDITY_OUTPUT_ID,"$TIMIDITY_OUTPUT_ID", the output specification)
fi

if test "x$with_x" = xyes; then
  dnl Order of X-library is: -lXm -lXaw -lXmu -lXt -lSM -lICE -lXext -lX11
  LIBS=`echo $LIBS | sed "s/-lX11/$lib_offix_opt $lib_xm_opt $lib_xaw_opt $lib_xmu_opt $lib_xt_opt $lib_xprelibs_opt $lib_xext_opt -lX11/"`
fi
LIBS="$LIBS $lib_dl_opt $lib_png_opt $lib_zip_opt $lib_user32_opt $lib_w32gui_opt $EXTRALIBS"
SHLD="$SHLD $SHLDFLAGS"

AC_SUBST(BORLANDC_START)
AC_SUBST(BORLANDC_LDFLAGS)
AC_SUBST(SHELL)
AC_SUBST(timidity_LDFLAGS)
AC_SUBST(WATCOM_LDFLAGS)
AC_SUBST(VCPP_LDFLAGS)
AC_SUBST(SYSEXTRAS)
AC_SUBST(EXTRALIBS)
AC_SUBST(NETSRCS)
AC_SUBST(ELFILES)
AC_SUBST(SHLD)
AC_SUBST(SHCFLAGS)
AC_SUBST(dynamic_targets)
AC_SUBST(so)
AC_SUBST(LN_S)
AC_SUBST(WISH)
AC_SUBST(tcltk_dep)
AC_SUBST(program_transform_name)
AC_SUBST(INTERFACE_SRCS)
AC_SUBST(pkgdatadir)
AC_SUBST(pkglibdir)

AC_CONFIG_FILES([
  Makefile
  autoconf/Makefile
  common.makefile
  configs/Makefile
  doc/Makefile
  doc/C/Makefile
  doc/ja_JP.eucJP/Makefile
  interface/Makefile
  interface/motif_bitmaps/Makefile
  interface/bitmaps/Makefile
  interface/pixmaps/Makefile
  libarc/Makefile
  libunimod/Makefile
  timidity/Makefile
  utils/Makefile
  script/Makefile
  TiMidity.ad
  TiMidity-uj.ad
])
AC_OUTPUT
