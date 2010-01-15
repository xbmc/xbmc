/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

/**
 * \brief	Socket access under Windows
 * \file	windows/Socket.h
 *
 *
 */
#ifndef __WIN_SOCKET_H
#define __WIN_SOCKET_H

#if defined __WINDOWS__ || defined WIN32 || defined _WINDOWS
  #ifdef _WINSOCKAPI_
  #undef _WINSOCKAPI_
  #endif
	#include <winsock2.h>
  #include <windows.h>

	#ifndef NI_MAXHOST
		#define NI_MAXHOST 1025
	#endif

	#ifndef socklen_t
		typedef int socklen_t;
	#endif
	#ifndef ipaddr_t
		typedef unsigned long ipaddr_t;
	#endif
	#ifndef port_t
		typedef unsigned short port_t;
	#endif

	#if (_WIN32_WINNT == 0x0500)
		#if defined CONFIG_SOCKET_IPV6
			// Needed for the Windows 2000 IPv6 Tech Preview.
			#include <tpipv6.h>
		#endif
		#include <wspiapi.h>
	#endif

	//#define INVALID_SOCKET -1

	#define _IOLEN64 (unsigned)
	#define _IORET64 (int)
#endif /* _WIN32PC */
#endif /* __WIN_SOCKET_H */
