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
 * \brief	Socket access under Linux
 * \file	linux/Socket.h
 *
 **/

#ifndef __LIN_SOCKET
#define __LIN_SOCKET

#if defined CONFIG_CT_LINUX || defined CONFIG_CT_LXRT || defined CONFIG_CT_RTAI
	#include <sys/types.h> 	/* for socket,connect */
	#include <sys/socket.h>	/* for socket,connect */
	#include <sys/un.h>	/* for Unix socket */
	#include <arpa/inet.h>	/* for inet_pton */
	#include <netdb.h>	/* for gethostbyname */
	#include <netinet/in.h>	/* for htons */
	#include <unistd.h>	/* for read, write, close */
	#include <string>	/* for std::string */

	typedef int SOCKET;
	typedef sockaddr SOCKADDR;
	typedef sockaddr_in SOCKADDR_IN;

	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
#endif /* CONFIG_CT_LINUX, LXRT, RTAI */
#endif /* __LIN_SOCKET */

