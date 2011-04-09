/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
/* "groupsock" interface
 * Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
 * Common include files, typically used for networking
 */

#ifndef _NET_COMMON_H
#define _NET_COMMON_H

#if defined(IMN_PIM)
#include "IMN_PIMNetCommon.h"

#elif defined(__WIN32__) || defined(_WIN32) || defined(_WIN32_WCE)
/* Windows */
#if defined(WINNT) || defined(_WINNT) || defined(__BORLANDC__) || defined(__MINGW32__) || defined(_WIN32_WCE)
#define _MSWSOCK_
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <windows.h>
#include <string.h>

#define closeSocket closesocket
#undef EWOULDBLOCK //MG
#define EWOULDBLOCK WSAEWOULDBLOCK
#undef EINPROGRESS //MG
#define EINPROGRESS WSAEWOULDBLOCK
#undef EAGAIN //MG
#define EAGAIN WSAEWOULDBLOCK
#undef EINTR //MG
#define EINTR WSAEINTR

#if defined(_WIN32_WCE)
#define NO_STRSTREAM 1
#endif

/* Definitions of size-specific types: */
typedef __int64 int64_t;
typedef unsigned __int64 u_int64_t;
typedef unsigned u_int32_t;
typedef unsigned short u_int16_t;
typedef unsigned char u_int8_t;

#elif defined(VXWORKS)
/* VxWorks */
#include <time.h>
#include <timers.h>
#include <sys/times.h>
#include <sockLib.h>
#include <hostLib.h>
#include <resolvLib.h>
#include <ioLib.h>

typedef unsigned int u_int32_t;
typedef unsigned short u_int16_t;
typedef unsigned char u_int8_t;

#else
/* Unix */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <ctype.h>
#if defined(_QNX4)
#include <sys/select.h>
#include <unix.h>
#endif

#define closeSocket close

#ifdef SOLARIS
#define u_int64_t uint64_t
#define u_int32_t uint32_t
#define u_int16_t uint16_t
#define u_int8_t uint8_t
#endif
#endif

#ifndef SOCKLEN_T
#define SOCKLEN_T int
#endif

#endif
