dnl @synopsis LIB_SOCKET_NSL
dnl
dnl This macro figures out what libraries are required on this platform
dnl to link sockets programs.
dnl
dnl The common cases are not to need any extra libraries, or to need
dnl -lsocket and -lnsl. We need to avoid linking with libnsl unless we
dnl need it, though, since on some OSes where it isn't necessary it
dnl will totally break networking. Unisys also includes gethostbyname()
dnl in libsocket but needs libnsl for socket().
dnl
dnl @category Misc
dnl @author Russ Allbery <rra@stanford.edu>
dnl @author Stepan Kasal <kasal@ucw.cz>
dnl @author Warren Young <warren@etr-usa.com>
dnl @version 2005-09-06
dnl @license AllPermissive

AC_DEFUN([LIB_SOCKET_NSL],
[
	AC_SEARCH_LIBS([gethostbyname], [nsl])
	AC_SEARCH_LIBS([socket], [socket], [], [
		AC_CHECK_LIB([socket], [socket], [LIBS="-lsocket -lnsl $LIBS"],
		[], [-lnsl])])
])
