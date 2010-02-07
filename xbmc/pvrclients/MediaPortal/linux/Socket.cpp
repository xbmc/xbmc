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
 * @file	linux/Socket.cpp
 * @brief	Implementation of the Socket class for Linux
 * @author	Marcel Groothuis
 *
 * Socket support is platform dependent. This file includes the right
 * platform specific header file and defines the platform independent
 * interface
 */

#if _LINUX

#include "../Socket.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>	//gethostbyname

#include "../../../addons/include/xbmc_pvr_types.h"
#include "../../../addons/include/xbmc_addon_lib++.h"
#include "../pvrclient-mediaportal_os.h"

using namespace std;

Socket::Socket(const enum SocketFamily family, const enum SocketDomain domain, const enum SocketType type, const enum SocketProtocol protocol)
{
	_sd = 0;
	_family = family;
	_domain = domain;
	_type = type;
	_protocol = protocol;
	memset (&_sockaddr, 0, sizeof( _sockaddr ) );
}


Socket::Socket()
{
	// Default constructor, default settings
	_sd = 0;
	_family = af_inet;
	_domain = pf_inet;
	_type = sock_stream;
	_protocol = tcp;
	memset (&_sockaddr, 0, sizeof( _sockaddr ) );
}


Socket::~Socket()
{
	close();
}

bool Socket::setHostname ( const std::string host )
{
	if (isalpha(host.c_str()[0]))
	{	// host address is a name
		hostent *he = NULL;
		if ((he = gethostbyname( host.c_str() )) == 0)
		{
			XBMC_log(LOG_ERROR, "Socket::setHostname gethostbyname failed with h_errno=%i\n", h_errno);
			return false;
		}

		_sockaddr.sin_addr = *((in_addr *) he->h_addr);
	}
	else
	{	// host addres is an ip address
		_sockaddr.sin_addr.s_addr = inet_addr(host.c_str());
		
		if ( _sockaddr.sin_addr.s_addr == INADDR_NONE )
		{
			XBMC_log(LOG_ERROR, "Socket::setHostname inet_addr error: No valid address\n");
			return false;
		}
	}
	return true;
};


bool Socket::close()
{
	if ( is_valid() )
	{
		::close(_sd);
		return true;
	}
	return false;
}

bool Socket::create()
{
	if ( !is_valid() )
	{
		XBMC_log(LOG_ERROR, "is !not valid\n");
		return false;
	}

	_sd = socket(_family, _type, _protocol );
	if (_sd == INVALID_SOCKET)
	{
		errormessage(errno, "Socket::create()" );
		return false;
	}

	return true;
}


bool Socket::bind ( const unsigned short port )
{
	if ( !is_valid() )
	{
		XBMC_log(LOG_ERROR, "is !not valid\n");
		return false;
	}

	_sockaddr.sin_family = _family;
	_sockaddr.sin_addr.s_addr = INADDR_ANY;
	_sockaddr.sin_port = htons( port );

	int bind_return = ::bind(_sd, (struct sockaddr *) &_sockaddr, sizeof(_sockaddr));

	if ( bind_return == -1 )
	{
		Socket::errormessage(errno, "Socket::bind" );
		return false;
	}

	return true;
}


bool Socket::listen() const
{
	if ( !is_valid() )
	{
		XBMC_log(LOG_ERROR, "is !not valid\n");
		return false;
	}

	int listen_return = ::listen (_sd, MAXCONNECTIONS);

	if ( listen_return == -1 )
	{
		XBMC_log(LOG_ERROR, "Socket::listen returned %i, Errno=%i\n", listen_return, errno);
		return false;
	}

	return true;
}


bool Socket::accept ( Socket& new_socket ) const
{
	if ( !is_valid() )
	{
		XBMC_log(LOG_ERROR, "is !not valid\n");
		return false;
	}

	int addr_length = sizeof( _sockaddr );
	new_socket._sd = ::accept(_sd, ( struct sockaddr * ) &_sockaddr, ( socklen_t * ) &addr_length );

	if ( new_socket._sd <= 0 )
	{
		XBMC_log(LOG_ERROR, "new_socket._sd <= 0, Errno=%i\n", errno);
		return false;
	}

	return true;
}


int Socket::send ( const std::string data )
{
	if ( !is_valid() )
	{
		XBMC_log(LOG_ERROR, "is !not valid\n");
		return 0;
	}

	int status = ::send(_sd, data.c_str(), ((data.size())+1), MSG_NOSIGNAL );

	if ( status == -1 )
	{
		XBMC_log(LOG_ERROR, "Socket::send Send Error.Errno=%i\n", errno);
	}

	return status;
}


int Socket::send ( const char* data, unsigned int size )
{
	if ( !is_valid() )
	{
		XBMC_log(LOG_ERROR, "is !not valid\n");
		return 0;
	}

	int status = ::send(_sd, data, size, MSG_NOSIGNAL );

	if ( status == -1 )
	{
		XBMC_log(LOG_ERROR, "Socket::send Send Error. Errno=%i\n", errno);
	}

	return status;
}


int Socket::sendto ( const char* data, unsigned int size, bool sendcompletebuffer) const
{
	int sentbytes = 0;
	int i;

	do
	{
		i = ::sendto(_sd, data, size, 0, (struct sockaddr*) &_sockaddr, sizeof( _sockaddr ) );

		if ( i <= 0 )
		{
			XBMC_log(LOG_ERROR, "Socket::sendto error");
			errormessage( errno, "Socket::sendto" );
			return i;
		}
		sentbytes += i;
	} while ( (sentbytes < (int) size) && (sendcompletebuffer == true));

	return i;
}


int Socket::receive ( std::string& data, unsigned int minpacketsize ) const
{
	char * buf = NULL;
	int status = 0;

	if ( !is_valid() )
	{
		XBMC_log(LOG_ERROR, "is !not valid\n");
		return 0;
	}

	buf = new char [ minpacketsize + 1 ];
	memset ( buf, 0, minpacketsize + 1 );

	status = receive( buf, minpacketsize, minpacketsize );

	data = buf;

	delete[] buf;
	return status;
}


int Socket::receive ( std::string& data) const
{
	char buf[MAXRECV + 1];
	int status = 0;

	if ( !is_valid() )
	{
		XBMC_log(LOG_ERROR, "is !not valid\n");
		return 0;
	}

	memset ( buf, 0, MAXRECV + 1 );

	status = receive( buf, MAXRECV, 0 );

	data = buf;

	return status;
}


int Socket::receive ( char* data, const unsigned int buffersize, const unsigned int minpacketsize ) const
{
	unsigned int receivedsize = 0;
	int status = 0;

	if ( !is_valid() )
	{
		XBMC_log(LOG_ERROR, "is !not valid\n");
		return 0;
	}

	while ( (receivedsize <= minpacketsize) && (receivedsize < buffersize) )
	{
		status = ::recv(_sd, data+receivedsize, (buffersize - receivedsize), 0 );

		if ( status == SOCKET_ERROR )
		{
			XBMC_log(LOG_ERROR, "Socket::receive SOCKET_ERROR. Errno = %i\n", errno);
			return status;
		}

		receivedsize += status;
	}

	return receivedsize;
}


int Socket::recvfrom ( char* data, const int buffersize, const int minpacketsize, struct sockaddr* from, socklen_t* fromlen) const
{
	int status = ::recvfrom(_sd, data, buffersize, 0, from, fromlen);

	return status;
}


bool Socket::connect ( const std::string host, const unsigned short port )
{
	if ( !is_valid() )
	{
		XBMC_log(LOG_ERROR, "is !not valid\n");
		return false;
	}

	_sockaddr.sin_family = _family;
	_sockaddr.sin_port = htons ( port );

	if ( !setHostname( host ) )
	{
		return false;
	};

	int status = ::connect ( _sd, ( sockaddr * ) &_sockaddr, sizeof ( _sockaddr ) );

	if ( status == SOCKET_ERROR )
	{
		XBMC_log(LOG_ERROR, "Socket::connect connection failed. Errno = %i\n", errno);
		return false;
	}

	return true;
}


bool Socket::set_non_blocking ( const bool b )
{
	int opts;

	opts = fcntl(_sd, F_GETFL);

	if ( opts < 0 )
	{
		return false;
	}

	if ( b )
		opts = ( opts | O_NONBLOCK );
	else
		opts = ( opts & ~O_NONBLOCK );

	if(fcntl (_sd , F_SETFL, opts) == -1)
	{
		return false;
	} else {
		return true;
	}
}


bool Socket::is_valid() const
{
	return (_sd != -1);
};


void Socket::errormessage( int errnum, const char* functionname) const
{
	const char* errmsg = NULL;

	switch ( errnum )
	{
	case EAGAIN: //same as EWOULDBLOCK
		errmsg = "EAGAIN: The socket is marked non-blocking and the requested operation would block";
		break;
	case EBADF:
		errmsg = "EBADF: An invalid descriptor was specified";
		break;
	case ECONNRESET:
		errmsg = "ECONNRESET: Connection reset by peer";
		break;
	case EDESTADDRREQ:
		errmsg = "EDESTADDRREQ: The socket is not in connection mode and no peer address is set";
		break;
	case EFAULT:
		errmsg = "EFAULT: An invalid userspace address was specified for a parameter";
		break;
	case EINTR:
		errmsg = "EINTR: A signal occurred before data was transmitted";
		break;
	case EINVAL:
		errmsg = "EINVAL: Invalid argument passed";
		break;
	case ENOTSOCK:
		errmsg = "ENOTSOCK: The argument is not a valid socket";
		break;
	case EMSGSIZE:
		errmsg = "EMSGSIZE: The socket requires that message be sent atomically, and the size of the message to be sent made this impossible";
		break;
	case ENOBUFS:
		errmsg = "ENOBUFS: The output queue for a network interface was full";
		break;
	case ENOMEM:
		errmsg = "ENOMEM: No memory available";
		break;
	case EPIPE:
		errmsg = "EPIPE: The local end has been shut down on a connection oriented socket";
		break;
	case EPROTONOSUPPORT:
		errmsg = "EPROTONOSUPPORT: The protocol type or the specified protocol is not supported within this domain";
		break;
	case EAFNOSUPPORT:
		errmsg = "EAFNOSUPPORT: The implementation does not support the specified address family";
		break;
	case ENFILE:
		errmsg = "ENFILE: Not enough kernel memory to allocate a new socket structure";
		break;
	case EMFILE:
		errmsg = "EMFILE: Process file table overflow";
		break;
	case EACCES:
		errmsg = "EACCES: Permission to create a socket of the specified type and/or protocol is denied";
		break;
	case ECONNREFUSED:
		errmsg = "ECONNREFUSED: A remote host refused to allow the network connection (typically because it is not running the requested service)";
		break;
	case ENOTCONN:
		errmsg = "ENOTCONN: The socket is associated with a connection-oriented protocol and has not been connected";
		break;
	//case E:
	//	errmsg = "";
	//	break;
	default:
		break;
	}
	XBMC_log(LOG_ERROR, "%s: (errno=%i) %s\n", functionname, errnum, errmsg);
}

#endif //LINUX
