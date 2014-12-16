AC_INIT(README)

AM_DISABLE_STATIC
AM_INIT_AUTOMAKE(SDL_Goom, 2k4)

ACLOCAL="$ACLOCAL -I m4"

AM_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_LN_S
AM_PROG_LEX
AC_PROG_YACC

AC_C_BIGENDIAN

dnl Get the CFlags
CFLAGS="${CFLAGS}"
LDFLAGS="${LDFLAGS}"

dnl *** check for xmms ***

HAVE_XMMS="no"
AM_CONDITIONAL(HAVE_XMMS,test "x$HAVE_XMMS" = "xyes")


dnl *** SDL ***

HAVE_SDL="no"
AM_CONDITIONAL(HAVE_SDL,test "x$HAVE_SDL" = "xyes")


dnl *** MMX ***

dnl rm -f mmx_zoom.s
HAVE_MMX="no"
HAVE_PPC="no"
MACTARGET="no"

dnl HOST
case "$host" in
*-apple-darwin*)
	MACTARGET="no"
	MACFOLDER=""
	AC_SUBST(MACFOLDER)
  	CCAS='$(CC)'
  	AC_SUBST(CCAS)
	;;
*-*-cygwin*)
  	CFLAGS="$CFLAGS -mno-cygwin -mwindows"
	LDFLAGS="$LDFLAGS -lmingw32"
	;;
esac

dnl ARCH
case "$host" in
i*86-*-*)
	AC_DEFINE(HAVE_MMX)
	AC_DEFINE(CPU_X86)
	HAVE_MMX="yes"
	;;

powerpc-*-*)
  	CCASFLAGS=-force_cpusubtype_ALL
  	AC_SUBST(CCASFLAGS)
	AC_DEFINE(CPU_POWERPC)
	HAVE_PPC="yes"
	;;

esac
AM_CONDITIONAL(HAVE_MMX,test "x$HAVE_MMX" = "xyes")
AM_CONDITIONAL(HAVE_PPC,test "x$HAVE_PPC" = "xyes")
AM_CONDITIONAL(MACTARGET,test "x$MACTARGET" = "xyes")


AC_CHECK_HEADER(pthread.h,,AC_MSG_ERROR([*** POSIX thread support not installed - please install first ***]))

PTHREAD_LIBS=error
AC_CHECK_LIB(pthread, pthread_attr_init, PTHREAD_LIBS="-lpthread")

if test "x$PTHREAD_LIBS" = xerror; then
    AC_CHECK_LIB(pthreads, pthread_attr_init, PTHREAD_LIBS="-lpthreads")
fi

if test "x$PTHREAD_LIBS" = xerror; then
    AC_CHECK_LIB(c_r, pthread_attr_init, PTHREAD_LIBS="-lc_r")
fi

if test "x$PTHREAD_LIBS" = xerror; then
    AC_CHECK_FUNC(pthread_attr_init, PTHREAD_LIBS="")
fi

AC_SUBST(PTHREAD_LIBS)

dnl rm -f mmx_zoom.s
dnl 	echo -n checking for nasm...
dnl 	if nasm -r 1> /dev/null 2> /dev/null
dnl 	then
dnl 		echo " `nasm -r` founded..";
dnl 	else
dnl 		echo " not found."
dnl 		echo '*** NASM needed to build x86 assembly..***'
dnl 		AC_MSG_ERROR
dnl 	fi
dnl 	esac

dnl AC_DEFINE(USE_ASM_MMX)
dnl ln -s mmx_zoom_x86.s mmx_zoom.s ;;
dnl *)
dnl ln -s mmx_zoom_dummy.s mmx_zoom.s ;;
dnl esac

AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)

AC_OUTPUT(Makefile src/Makefile xmms-goom/Makefile sdl-goom/Makefile libgoom2.pc)

dnl *** nice user info ***

AC_MSG_NOTICE([goom2k4 was configured with the following options:])
if test "x$HAVE_PPC" = "xyes"; then
  AC_MSG_NOTICE([ ** PPC support enabled])
else
  AC_MSG_NOTICE([    PPC support disabled])
fi
if test "x$HAVE_MMX" = "xyes"; then
  AC_MSG_NOTICE([ ** MMX support enabled])
else
  AC_MSG_NOTICE([    MMX support disabled])
fi
AC_MSG_NOTICE([ ** goom lib will be built])
if test "x$HAVE_XMMS" = "xyes"; then
  AC_MSG_NOTICE([ ** XMMS plugin will be built])
else
  AC_MSG_NOTICE([    XMMS plugin will not be built])
fi
if test "x$MACTARGET" = "xyes"; then
  AC_MSG_NOTICE([ ** goom mac application will be built])
  AC_MSG_NOTICE([ ** goom mac iTunes plugin will be built])
else
  AC_MSG_NOTICE([    goom mac application will not be built])
  AC_MSG_NOTICE([    goom mac iTunes plugin will not be built])
fi
if test "x$HAVE_SDL" = "xyes"; then
  AC_MSG_NOTICE([ ** goom sdl application will be built])
else
  AC_MSG_NOTICE([    goom sdl application will not be built])
fi
