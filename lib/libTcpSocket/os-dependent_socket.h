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
 * @brief Establish a TCP connection to the specified host and portnumber.
 * @param szHostname The name or ip address of the host to connect to.
 * @param nPort The port number to connect to.
 * @param szErrbuf Buffer to write an error message to.
 * @param nErrbufSize The size of the error buffer.
 * @param nTimeout The connection timeout in milliseconds.
 * @return valid socket_t file descriptor on success, or INVALID_SOCKET / SOCKET_ERROR on failure.
 */
socket_t tcp_connect(const char *szHostname, int nPort, char *szErrbuf,
      size_t nErrbufSize, int nTimeout);

/*!
 * @brief Read data from a socket opened with tcp_connect.
 * @param fdSock The socket to read from.
 * @param buf The buffer to write the received data to.
 * @param nLen The length of the buffer.
 * @return 0 on success, or the error number on error.
 */
int tcp_read(socket_t fdSock, void *buf, size_t nLen);

/*!
 * @brief Read from a socket opened with tcp_connect.
 * @param fdSock The socket to read from.
 * @param buf The buffer to write the received data to.
 * @param nLen The length of the buffer.
 * @param nTimeout The timeout in milliseconds.
 * @return 0 on success, or the error number on error.
 */
int tcp_read_timeout(socket_t fdSock, void *buf, size_t nLen, int nTimeout);

/*!
 * @brief Close a socket connection opened with tcp_connect.
 * @param fdSock The socket to close.
 */
void tcp_close(socket_t fdSock);
