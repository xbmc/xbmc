#pragma once
/*
 *  Networking
 *  Copyright (C) 2007-2008 Andreas Öman
 *  Copyright (C) 2011 Team XBMC
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#if defined _MSC_VER || defined(_WIN32) || defined(_WIN64)
#ifndef __WINDOWS__
#define __WINDOWS__
#endif
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR   (-1)
#endif

#if defined(__WINDOWS__)
#include "windows/net_winsock.h"
#else
#include "linux/net_posix.h"
#endif

#include <sys/types.h>
#include <stdint.h>


/*!
	\brief	used to establish a connection to a host on a specified a port,
			provides an error description if something went wrong
	\return	valid socket_t file descriptor on success, or INVALID_SOCKET / SOCKET_ERROR on failure
	\param	szHostname	host name to connect to
	\param	nPort	port used for connection
	\param	szErrbuf	error buffer
	\param	nErrbufSize	error buffer size
	\param	nTimeout	timeout
 */
socket_t tcp_connect(const char *szHostname, int nPort, char *szErrbuf,
      size_t nErrbufSize, int nTimeout);

/*!
	\brief	used to establish a non-blocking connection to a specified address information,
			provides an error description if something went wrong
	\return	0 on success, or SOCKET_ERROR on failure
	\param	addr	points to a valid address information
	\param	fdSock	valid socket_t file descriptor obtained from the given address information
	\param	szErrbuf	error buffer
	\param	nErrbufSize	error buffer size
	\param	nTimeout	timeout
 */
int tcp_connect_addr_socket_nonblocking(struct addrinfo* addr, socket_t fdSock,
      char *szErrbuf, size_t nErrbufSize, int nTimeout);

/*!
	\brief	used to read data from a socket opened with tcp_connect
	\return	0 on success, otherwise an error number
	\param	fdSock	valid socket_t file descriptor obtained with tcp_connect
	\param	buf	points to buffer which will receive the data
	\param	nLen	length of the buffer
 */
int tcp_read(socket_t fdSock, void *buf, size_t nLen);

/*!
	\brief	used to read data from a socket opened with tcp_connect
	\return	0 on success, otherwise an error number
	\param	fdSock	valid socket_t file descriptor obtained with tcp_connect
	\param	buf	points to buffer which will receive the data
	\param	nLen	length of the buffer
	\param	nTimeout	timeout
 */
int tcp_read_timeout(socket_t fdSock, void *buf, size_t nLen, int nTimeout);

/*!
	\brief	used to close a socket connection
	\param	fdSock	valid socket_t file descriptor obtained with tcp_connect
 */
void tcp_close(socket_t fdSock);
