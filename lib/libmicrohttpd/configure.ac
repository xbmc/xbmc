# This file is part of libmicrohttpd.
# (C) 2006, 2007, 2008, 2009, 2010 Christian Grothoff (and other contributing authors)
#
# libmicrohttpd is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published
# by the Free Software Foundation; either version 2, or (at your
# option) any later version.
#
# libmicrohttpd is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with libmicrohttpd; see the file COPYING.  If not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
#
#
# Process this file with autoconf to produce a configure script.
#
#
AC_PREREQ(2.57)
AC_INIT([libmicrohttpd], [0.4.5],[libmicrohttpd@gnu.org])
AM_INIT_AUTOMAKE([libmicrohttpd], [0.4.5])
AM_CONFIG_HEADER([MHD_config.h])
AC_CONFIG_MACRO_DIR([m4])
AH_TOP([#define _GNU_SOURCE  1])

LIB_VERSION_CURRENT=7
LIB_VERSION_REVISION=0
LIB_VERSION_AGE=2
AC_SUBST(LIB_VERSION_CURRENT)
AC_SUBST(LIB_VERSION_REVISION)
AC_SUBST(LIB_VERSION_AGE)

if test `uname -s` = "OS/390"
then
# configure binaries for z/OS
  if test -z "$CC"
  then
    CC=`pwd`"/contrib/xcc"
    chmod +x $CC || true
  fi
  if test -z "$CPP"
  then
    CPP="c89 -E"
  fi
  if test -z "$CXXCPP"
  then
    CXXCPP="c++ -E -+"
  fi
#  _CCC_CCMODE=1
#  _C89_CCMODE=1
fi

# Checks for programs.
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_CANONICAL_HOST
AM_PROG_CC_C_O
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

# set GCC options
# use '-fno-strict-aliasing', but only if the compiler can take it
if gcc -fno-strict-aliasing -S -o /dev/null -xc /dev/null >/dev/null 2>&1; 
then
 CFLAGS="-fno-strict-aliasing $CFLAGS"
fi


# Check system type
case "$host_os" in
*darwin* | *rhapsody* | *macosx*)
     AC_DEFINE_UNQUOTED(OSX,1,[This is an OS X system])
     CFLAGS="-no-cpp-precomp -fno-common $CFLAGS"
     AM_CONDITIONAL(HAVE_GNU_LD, false)    
     ;;
linux*)
     AC_DEFINE_UNQUOTED(LINUX,1,[This is a Linux system])
     AM_CONDITIONAL(HAVE_GNU_LD, true)    
     ;;
freebsd*)
     AC_DEFINE_UNQUOTED(SOMEBSD,1,[This is a BSD system])
     AC_DEFINE_UNQUOTED(FREEBSD,1,[This is a FreeBSD system])
     AM_CONDITIONAL(HAVE_GNU_LD, true)    
     CFLAGS="-D_THREAD_SAFE $CFLAGS"
     ;;
openbsd*)
     AC_DEFINE_UNQUOTED(SOMEBSD,1,[This is a BSD system])
     AC_DEFINE_UNQUOTED(OPENBSD,1,[This is an OpenBSD system])
     AM_CONDITIONAL(HAVE_GNU_LD, true)    
     ;;
netbsd*)
     AC_DEFINE_UNQUOTED(SOMEBSD,1,[This is a BSD system])
     AC_DEFINE_UNQUOTED(NETBSD,1,[This is a NetBSD system])
     AM_CONDITIONAL(HAVE_GNU_LD, true)    
     ;;
*solaris*)
     AC_DEFINE_UNQUOTED(SOLARIS,1,[This is a Solaris system])
     AC_DEFINE_UNQUOTED(_REENTRANT,1,[Need with solaris or errno doesnt work])
     AM_CONDITIONAL(HAVE_GNU_LD, false)    
     LDFLAGS="$LDFLAGS -lpthread"
     ;;
*arm-linux*)
     AC_DEFINE_UNQUOTED(LINUX,1,[This is a Linux system])
     CFLAGS="-D_REENTRANT -fPIC -pipe $CFLAGS"
     AM_CONDITIONAL(HAVE_GNU_LD, true)    
     ;;
*cygwin*)
     AC_DEFINE_UNQUOTED(CYGWIN,1,[This is a Cygwin system])
     AM_CONDITIONAL(HAVE_GNU_LD, false)    
     LDFLAGS="$LDFLAGS -no-undefined"
     ;;
*mingw*)
     AC_DEFINE_UNQUOTED(MINGW,1,[This is a MinGW system])
     AC_DEFINE_UNQUOTED(WINDOWS,1,[This is a Windows system])
     LDFLAGS="$LDFLAGS -no-undefined -Wl,--export-all-symbols -lws2_32 -lplibc"
     AM_CONDITIONAL(HAVE_GNU_LD, true)    
     ;;
*openedition*)
     AC_DEFINE_UNQUOTED(OS390,1,[This is a OS/390 system])
     AM_CONDITIONAL(HAVE_GNU_LD, false)    
    ;;
*)
     AC_MSG_RESULT(Unrecognised OS $host_os)
     AC_DEFINE_UNQUOTED(OTHEROS,1,[Some strange OS])
     AM_CONDITIONAL(HAVE_GNU_LD, false)    
;;
esac


# for pkg-config
MHD_LIBDEPS=""

CHECK_PTHREAD
LIBS="$PTHREAD_LIBS $LIBS"
AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_LDFLAGS)
AC_SUBST(PTHREAD_CPPFLAGS)

# Check for headers that are ALWAYS required
AC_CHECK_HEADERS([fcntl.h math.h errno.h limits.h stdio.h locale.h sys/stat.h sys/types.h pthread.h],,AC_MSG_ERROR([Compiling libmicrohttpd requires standard UNIX headers files]))

# Check for optional headers
AC_CHECK_HEADERS([sys/types.h sys/time.h sys/msg.h netdb.h netinet/in.h netinet/tcp.h time.h sys/socket.h sys/mman.h arpa/inet.h sys/select.h poll.h])

AC_CHECK_FUNCS(memmem)

# IPv6
AC_MSG_CHECKING(for IPv6)
AC_TRY_COMPILE([
#include <stdio.h>
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
],[
int af=AF_INET6;
int pf=PF_INET6;
struct sockaddr_in6 sa;
printf("%d %d %p\n", af, pf, &sa);
],[
have_inet6=yes;
AC_DEFINE([HAVE_INET6], [1], [Provides IPv6 headers])
],
have_inet6=no
)
AC_MSG_RESULT($have_inet6)

# TCP_CORK
AC_CHECK_DECLS([TCP_CORK], [], [], [[#include <netinet/tcp.h>]])

# libcurl (required for testing)
SAVE_LIBS=$LIBS

AC_MSG_CHECKING(whether to use libcurl for testing)
AC_ARG_ENABLE(curl,
  [AS_HELP_STRING([--enable-curl],[support cURL based testcases])],
  [enable_curl=$enableval],
  [enable_curl="yes"])

curl=0
if test "$enable_curl" = "yes"
then
  LIBCURL_CHECK_CONFIG(,,curl=1,curl=0)
# Lib cURL & cURL - OpenSSL versions
  MHD_REQ_CURL_VERSION=7.16.4
  MHD_REQ_CURL_OPENSSL_VERSION=0.9.8
  MHD_REQ_CURL_GNUTLS_VERSION=2.2.3
  AC_DEFINE_UNQUOTED([MHD_REQ_CURL_VERSION], "$MHD_REQ_CURL_VERSION", [required cURL version to run tests])
  AC_DEFINE_UNQUOTED([MHD_REQ_CURL_OPENSSL_VERSION], "$MHD_REQ_CURL_OPENSSL_VERSION", [required cURL SSL version to run tests])
  AC_DEFINE_UNQUOTED([MHD_REQ_CURL_GNUTLS_VERSION], "$MHD_REQ_CURL_GNUTLS_VERSION", [gnuTLS lib version - used in conjunction with cURL])
fi
LIBS=$SAVE_LIBS
AM_CONDITIONAL(HAVE_CURL, test x$curl = x1)

# large file support (> 4 GB)
AC_SYS_LARGEFILE
AC_FUNC_FSEEKO

# optional: have error messages ?
AC_MSG_CHECKING(whether to enable error messages)
AC_ARG_ENABLE(messages,
   [AS_HELP_STRING([--enable-messages],
               [enable MHD error messages])],
   [enable_messages=$enableval],
   [enable_messages="no"])
AC_MSG_RESULT($enable_messages)
if test "$enable_messages" = "yes"
then
 AC_DEFINE([HAVE_MESSAGES],[1],[Include error messages])
fi


# optional: have zzuf, socat?
AC_CHECK_PROG([HAVE_ZZUF],[zzuf], 1, 0)
AC_CHECK_PROG([HAVE_SOCAT],[socat], 1, 0)
AM_CONDITIONAL(HAVE_ZZUF, test 0 != $HAVE_ZZUF)
AM_CONDITIONAL(HAVE_SOCAT, test 0 != $HAVE_SOCAT)


# libgcrypt linkage: required for HTTPS support 
AM_PATH_LIBGCRYPT(1.2.2, gcrypt=true)

# define the minimal version of libgcrypt required   
MHD_GCRYPT_VERSION=1:1.2.2
AC_DEFINE_UNQUOTED([MHD_GCRYPT_VERSION], "$MHD_GCRYPT_VERSION", [gcrypt lib version])


# optional: HTTPS support.  Enabled by default
AC_MSG_CHECKING(whether to enable HTTPS support)
AC_ARG_ENABLE([https],
   [AS_HELP_STRING([--enable-https],
               [enable HTTPS support (default is yes)])],
   [enable_https=$enableval],
   [enable_https="yes"])
if test "$enable_https" = "yes"
then
 if test "$gcrypt" = "true"
 then
   AC_DEFINE([HTTPS_SUPPORT],[1],[include HTTPS support])
   MHD_LIBDEPS="$LIBGCRYPT_LIBS"
 else
   AC_DEFINE([HTTPS_SUPPORT],[0],[no libgcrypt])
   enable_https="no (lacking libgcrypt)"
 fi
else
 AC_DEFINE([HTTPS_SUPPORT],[0],[disable HTTPS support])
fi
AC_MSG_RESULT($enable_https)

AM_CONDITIONAL(ENABLE_HTTPS, test "$enable_https" = "yes")

MHD_LIB_LDFLAGS="-export-dynamic -no-undefined"

dnl TODO insert a proper check here
AC_CACHE_CHECK([whether -export-symbols-regex works],
 gn_cv_export_symbols_regex_works, 
 [
  case "$host_os" in
  mingw*) gn_cv_export_symbols_regex_works=no;;
  *) gn_cv_export_symbols_regex_works=yes;;
  esac
 ])

# optional: compile TLS client side code [only for test running] ?
AC_MSG_CHECKING(whether to compile TLS client side code)
AC_ARG_ENABLE(client-side,
   [AS_HELP_STRING([--enable-client-side],
               [compile client side code])],
   [enable_client_side=$enableval],
   [enable_client_side="no"])
AC_MSG_RESULT($enable_client_side)
if test "$enable_client_side" = "no"
then
 AC_DEFINE([MHD_DEBUG_TLS],[0],[Compile client side code. This will enable running some test cases.])
 MSG_CLIENT_SIDE="disabled. running some test cases won't be possible"
 if test "x$gn_cv_export_symbols_regex_works" = "xyes"
 then
  MHD_LIB_LDFLAGS="$MHD_LIB_LDFLAGS -export-symbols \$(srcdir)/EXPORT.sym"
 fi
else
 AC_DEFINE([MHD_DEBUG_TLS],[1],[Compile client side code. This will enable running some test cases.])
 MSG_CLIENT_SIDE="enabled" 
fi
AM_CONDITIONAL(MHD_DEBUG_TLS, test "$enable_client_side" != "no")


# Symbols required by GNU_TLS 
AC_DEFINE([GNULIB_GC_RANDOM],[1],[GNULIB_GC_RANDOM])


# gcov compilation
use_gcov=no
AC_ARG_ENABLE([coverage], 
   AS_HELP_STRING([--enable-coverage],
                  [compile the library with code coverage support (default is NO)]),
                  [use_gcov=yes], [use_gcov=no])
AM_CONDITIONAL([USE_COVERAGE], [test "x$use_gcov" = "xyes"])


AC_SUBST(MHD_LIB_LDFLAGS)

# for pkg-config
AC_SUBST(MHD_LIBDEPS)

AC_SUBST(CPPFLAGS)
AC_SUBST(LIBS)
AC_SUBST(LDFLAGS)
AC_SUBST(EXT_LIB_PATH)
AC_SUBST(EXT_LIBS)

AC_CONFIG_FILES([ 
libmicrohttpd.pc
Makefile
contrib/Makefile
doc/Makefile
m4/Makefile
src/Makefile
src/include/Makefile
src/daemon/Makefile
src/daemon/https/Makefile
src/daemon/https/tls/Makefile
src/daemon/https/x509/Makefile
src/daemon/https/lgl/Makefile
src/daemon/https/minitasn1/Makefile
src/examples/Makefile
src/testcurl/Makefile
src/testcurl/https/Makefile
src/testzzuf/Makefile])
AC_OUTPUT

AM_CONDITIONAL(ENABLE_MINITASN1,  [test -n " " ] )
AM_CONDITIONAL(ENABLE_OPENSSL,  [test -n "" ] )
AM_CONDITIONAL(HAVE_LD_OUTPUT_DEF,  [test -n "" ] )
AM_CONDITIONAL(HAVE_LD_VERSION_SCRIPT,  [test -n "" ] )

# Finally: summary
if test "$curl" != 1 ; then
 MSG_CURL="no, many unit tests will not run"
else
 MSG_CURL="yes"
fi

if test "$gcrypt" != true
then
 MSG_GCRYPT="no, HTTPS will not be built"
else
 MSG_GCRYPT="yes"
fi

AC_MSG_NOTICE([Configuration Summary:  
  Operating System:  ${host_os}
  Target directory:  ${prefix}
  Messages:          ${enable_messages}
  libgcrypt:         ${MSG_GCRYPT}
  libcurl (testing): ${MSG_CURL}
  HTTPS support:     ${enable_https}
])

if test "$enable_https" = "yes"
then
 AC_MSG_NOTICE([HTTPS subsystem configuration:
  Client code dep.:  ${MSG_CLIENT_SIDE}    		
  License         :  LGPL only
 ])
else
 AC_MSG_NOTICE([
  License         :  LGPL or eCos
])
fi  


