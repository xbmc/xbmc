/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAS_DS_PLAYER

#include "DSSocket.h"
#include "utils/log.h"

using namespace std;

/* Master defines for client control */
#define RECEIVE_TIMEOUT 6 //sec

CDSSocket::CDSSocket(const enum SocketFamily family, const enum SocketDomain domain, const enum SocketType type, const enum SocketProtocol protocol)
{
  _sd = INVALID_SOCKET;
  _family = family;
  _domain = domain;
  _type = type;
  _protocol = protocol;
  memset (&_sockaddr, 0, sizeof( _sockaddr ) );
}


CDSSocket::CDSSocket()
{
  // Default constructor, default settings
  _sd = INVALID_SOCKET;
  _family = af_inet;
  _domain = pf_inet;
  _type = sock_stream;
  _protocol = tcp;
  memset (&_sockaddr, 0, sizeof( _sockaddr ) );
}


CDSSocket::~CDSSocket()
{
  close();
}

bool CDSSocket::setHostname(const std::string& host)
{
  if (isalpha(host.c_str()[0]))
  {
    // host address is a name
    struct hostent *he = NULL;
    if ((he = gethostbyname( host.c_str() )) == 0)
    {
      errormessage( getLastError(), "Socket::setHostname");
      return false;
    }

    _sockaddr.sin_addr = *((in_addr *) he->h_addr);
  }
  else
  {
    _sockaddr.sin_addr.s_addr = inet_addr(host.c_str());
  }
  return true;
}

bool CDSSocket::close()
{
  if (is_valid())
  {
    if (_sd != SOCKET_ERROR)
#ifdef TARGET_WINDOWS
      closesocket(_sd);
#else
      ::close(_sd);
#endif
    _sd = INVALID_SOCKET;
    osCleanup();
    return true;
  }
  return false;
}

bool CDSSocket::create()
{
  if( is_valid() )
  {
    close();
  }

  if(!osInit())
  {
    return false;
  }

  _sd = socket(_family, _type, _protocol );
  //0 indicates that the default protocol for the type selected is to be used.
  //For example, IPPROTO_TCP is chosen for the protocol if the type  was set to
  //SOCK_STREAM and the address family is AF_INET.

  if (_sd == INVALID_SOCKET)
  {
    errormessage( getLastError(), "Socket::create" );
    return false;
  }

  return true;
}


bool CDSSocket::bind(const unsigned short port)
{

  if (!is_valid())
  {
    return false;
  }

  _sockaddr.sin_family = (sa_family_t) _family;
  _sockaddr.sin_addr.s_addr = INADDR_ANY;  //listen to all
  _sockaddr.sin_port = htons( port );

  int bind_return = ::bind(_sd, (sockaddr*)(&_sockaddr), sizeof(_sockaddr));

  if ( bind_return == -1 )
  {
    errormessage( getLastError(), "Socket::bind" );
    return false;
  }

  return true;
}


bool CDSSocket::listen() const
{

  if (!is_valid())
  {
    return false;
  }

  int listen_return = ::listen (_sd, SOMAXCONN);
  //This is defined as 5 in winsock.h, and 0x7FFFFFFF in winsock2.h.
  //linux 128//MAXCONNECTIONS =1

  if (listen_return == -1)
  {
    errormessage( getLastError(), "Socket::listen" );
    return false;
  }

  return true;
}


bool CDSSocket::accept(CDSSocket& new_socket) const
{
  if (!is_valid())
  {
    return false;
  }

  socklen_t addr_length = sizeof( _sockaddr );
  new_socket._sd = ::accept(_sd, const_cast<sockaddr*>( (const sockaddr*) &_sockaddr), &addr_length );

  if (new_socket._sd == INVALID_SOCKET)
  {
    errormessage( getLastError(), "Socket::accept" );
    return false;
  }

  return true;
}


int CDSSocket::send(const std::string& data)
{
  return CDSSocket::send((const char*)data.c_str(), (const unsigned int)data.size());
}


int CDSSocket::send(const char* data, const unsigned int len)
{
  fd_set set_w, set_e;
  struct timeval tv;
  int  result;

  if (!is_valid())
  {
    return 0;
  }

  // fill with new data
  tv.tv_sec  = 0;
  tv.tv_usec = 0;

  FD_ZERO(&set_w);
  FD_ZERO(&set_e);
  FD_SET(_sd, &set_w);
  FD_SET(_sd, &set_e);

  result = select(FD_SETSIZE, &set_w, NULL, &set_e, &tv);

  if (result < 0)
  {
    CLog::Log(LOGERROR, "%s Socket::send  - select failed", __FUNCTION__);
    _sd = INVALID_SOCKET;
    return 0;
  }
  if (FD_ISSET(_sd, &set_w))
  {
    CLog::Log(LOGERROR, "%s Socket::send  - failed to send data", __FUNCTION__);
    _sd = INVALID_SOCKET;
    return 0;
  }

  int status = ::send(_sd, data, len, 0 );

  if (status == -1)
  {
    errormessage( getLastError(), "Socket::send");
    CLog::Log(LOGERROR, "%s Socket::send  - failed to send data", __FUNCTION__);
    _sd = INVALID_SOCKET;
    return 0;
  }
  return status;
}


int CDSSocket::sendto(const char* data, unsigned int size, bool sendcompletebuffer)
{
  int sentbytes = 0;
  int i;

  do
  {
    i = ::sendto(_sd, data, size, 0, (const struct sockaddr*) &_sockaddr, sizeof( _sockaddr ) );

    if (i <= 0)
    {
      errormessage( getLastError(), "Socket::sendto");
      osCleanup();
      return i;
    }
    sentbytes += i;
  } while ( (sentbytes < (int) size) && (sendcompletebuffer == true));

  return i;
}


int CDSSocket::receive(std::string& data, unsigned int minpacketsize) const
{
  char * buf = NULL;
  int status = 0;

  if (!is_valid())
  {
    return 0;
  }

  buf = new char [ minpacketsize + 1 ];
  memset ( buf, 0, minpacketsize + 1 );

  status = receive( buf, minpacketsize, minpacketsize );

  data = buf;

  delete[] buf;
  return status;
}


//Receive until error or \n
bool CDSSocket::ReadLine(string& line)
{
  fd_set         set_r, set_e;
  timeval        timeout;
  int            retries = 6;
  char           buffer[2048];

  if (!is_valid())
    return false;

  while (true)
  {
    size_t pos1 = line.find("\r\n", 0);
    if (pos1 != std::string::npos)
    {
      line.erase(pos1, string::npos);
      return true;
    }

    timeout.tv_sec  = RECEIVE_TIMEOUT;
    timeout.tv_usec = 0;

    // fill with new data
    FD_ZERO(&set_r);
    FD_ZERO(&set_e);
    FD_SET(_sd, &set_r);
    FD_SET(_sd, &set_e);
    int result = select(FD_SETSIZE, &set_r, NULL, &set_e, &timeout);

    if (result < 0)
    {
      CLog::Log(LOGDEBUG, "%s select failed", __FUNCTION__);
      errormessage(getLastError(), __FUNCTION__);
      _sd = INVALID_SOCKET;
      return false;
    }

    if (result == 0)
    {
      if (retries != 0)
      {
         CLog::Log(LOGDEBUG, "%s timeout waiting for response, retrying... (%i)", __FUNCTION__, retries);
         retries--;
        continue;
      } else {
         CLog::Log(LOGDEBUG, "%s timeout waiting for response. Aborting after 10 retries.", __FUNCTION__);
         return false;
      }
    }

    result = recv(_sd, buffer, sizeof(buffer) - 1, 0);
    if (result < 0)
    {
      CLog::Log(LOGDEBUG, "%s recv failed", __FUNCTION__);
      errormessage(getLastError(), __FUNCTION__);
      _sd = INVALID_SOCKET;
      return false;
    }
    buffer[result] = 0;

    line.append(buffer);
  }

  return true;
}


int CDSSocket::receive(std::string& data) const
{
  char buf[MAXRECV + 1];
  int status = 0;

  if ( !is_valid() )
  {
    return 0;
  }

  memset ( buf, 0, MAXRECV + 1 );
  status = receive( buf, MAXRECV, 0 );
  data = buf;

  return status;
}

int CDSSocket::receive(char* data, const unsigned int buffersize, const unsigned int minpacketsize) const
{
  unsigned int receivedsize = 0;

  if ( !is_valid() )
  {
    return 0;
  }

  while ( (receivedsize <= minpacketsize) && (receivedsize < buffersize) )
  {
    int status = ::recv(_sd, data+receivedsize, (buffersize - receivedsize), 0 );

    if ( status == SOCKET_ERROR )
    {
      errormessage( getLastError(), "Socket::receive" );
      return status;
    }

    receivedsize += status;
  }

  return receivedsize;
}


int CDSSocket::recvfrom(char* data, const int buffersize, struct sockaddr* from, socklen_t* fromlen) const
{
  int status = ::recvfrom(_sd, data, buffersize, 0, from, fromlen);

  return status;
}


bool CDSSocket::connect(const std::string& host, const unsigned short port)
{
  if ( !is_valid() )
  {
    return false;
  }

  _sockaddr.sin_family = (sa_family_t) _family;
  _sockaddr.sin_port = htons ( port );

  if ( !setHostname( host ) )
  {
    CLog::Log(LOGERROR, "%s Socket::setHostname(%s) failed.", __FUNCTION__, host.c_str());
    return false;
  }

  int status = ::connect ( _sd, reinterpret_cast<sockaddr*>(&_sockaddr), sizeof ( _sockaddr ) );

  if ( status == SOCKET_ERROR )
  {
    CLog::Log(LOGERROR, "%s Socket::connect %s:%u", __FUNCTION__, host.c_str(), port);
    errormessage( getLastError(), "Socket::connect" );
    return false;
  }

  return true;
}

bool CDSSocket::reconnect()
{
  if ( _sd != INVALID_SOCKET )
  {
    return true;
  }

  if( !create() )
    return false;

  int status = ::connect ( _sd, reinterpret_cast<sockaddr*>(&_sockaddr), sizeof ( _sockaddr ) );

  if ( status == SOCKET_ERROR )
  {
    errormessage( getLastError(), "Socket::connect" );
    return false;
  }

  return true;
}

bool CDSSocket::is_valid() const
{
  return (_sd != INVALID_SOCKET);
}


bool CDSSocket::set_non_blocking(const bool b)
{
  u_long iMode;

  if ( b )
    iMode = 1;  // enable non_blocking
  else
    iMode = 0;  // disable non_blocking

  if (ioctlsocket(_sd, FIONBIO, &iMode) == -1)
  {
    CLog::Log(LOGERROR, "%s Socket::set_non_blocking - Can't set socket condition to: %i", __FUNCTION__, iMode);
    return false;
  }

  return true;
}

void CDSSocket::errormessage(int errnum, const char* functionname) const
{
  const char* errmsg = NULL;

  switch (errnum)
  {
  case WSANOTINITIALISED:
    errmsg = "A successful WSAStartup call must occur before using this function.";
    break;
  case WSAENETDOWN:
    errmsg = "The network subsystem or the associated service provider has failed";
    break;
  case WSA_NOT_ENOUGH_MEMORY:
    errmsg = "Insufficient memory available";
    break;
  case WSA_INVALID_PARAMETER:
    errmsg = "One or more parameters are invalid";
    break;
  case WSA_OPERATION_ABORTED:
    errmsg = "Overlapped operation aborted";
    break;
  case WSAEINTR:
    errmsg = "Interrupted function call";
    break;
  case WSAEBADF:
    errmsg = "File handle is not valid";
    break;
  case WSAEACCES:
    errmsg = "Permission denied";
    break;
  case WSAEFAULT:
    errmsg = "Bad address";
    break;
  case WSAEINVAL:
    errmsg = "Invalid argument";
    break;
  case WSAENOTSOCK:
    errmsg = "Socket operation on nonsocket";
    break;
  case WSAEDESTADDRREQ:
    errmsg = "Destination address required";
    break;
  case WSAEMSGSIZE:
    errmsg = "Message too long";
    break;
  case WSAEPROTOTYPE:
    errmsg = "Protocol wrong type for socket";
    break;
  case WSAENOPROTOOPT:
    errmsg = "Bad protocol option";
    break;
  case WSAEPFNOSUPPORT:
    errmsg = "Protocol family not supported";
    break;
  case WSAEAFNOSUPPORT:
    errmsg = "Address family not supported by protocol family";
    break;
  case WSAEADDRINUSE:
    errmsg = "Address already in use";
    break;
  case WSAECONNRESET:
    errmsg = "Connection reset by peer";
    break;
  case WSAHOST_NOT_FOUND:
    errmsg = "Authoritative answer host not found";
    break;
  case WSATRY_AGAIN:
    errmsg = "Nonauthoritative host not found, or server failure";
    break;
  case WSAEISCONN:
    errmsg = "Socket is already connected";
    break;
  case WSAETIMEDOUT:
    errmsg = "Connection timed out";
    break;
  case WSAECONNREFUSED:
    errmsg = "Connection refused";
    break;
  case WSANO_DATA:
    errmsg = "Valid name, no data record of requested type";
    break;
  default:
    errmsg = "WSA Error";
  }
  CLog::Log(LOGERROR, "%s %s: (Winsock error=%i) %s", __FUNCTION__, functionname, errnum, errmsg);
}

int CDSSocket::getLastError() const
{
  return WSAGetLastError();
}

int CDSSocket::win_usage_count = 0; //Declared static in Socket class

bool CDSSocket::osInit()
{
  win_usage_count++;
  // initialize winsock:
  if (WSAStartup(MAKEWORD(2,2),&_wsaData) != 0)
  {
    return false;
  }

  WORD wVersionRequested = MAKEWORD(2,2);

  // check version
  if (_wsaData.wVersion != wVersionRequested)
  {
    return false;
  }

  return true;
}

void CDSSocket::osCleanup()
{
  win_usage_count--;
  if(win_usage_count == 0)
  {
    WSACleanup();
  }
}

#endif
