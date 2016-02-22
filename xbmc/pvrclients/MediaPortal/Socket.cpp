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
#include "libXBMC_addon.h"
#include "utils.h"
#include <string>
#include "os-dependent.h"
#include "client.h"
#include "Socket.h"

using namespace std;
using namespace ADDON;
using namespace MPTV;

namespace MPTV
{

/* Master defines for client control */
#define RECEIVE_TIMEOUT 6 //sec

Socket::Socket(const enum SocketFamily family, const enum SocketDomain domain, const enum SocketType type, const enum SocketProtocol protocol)
{
  _sd = INVALID_SOCKET;
  _family = family;
  _domain = domain;
  _type = type;
  _protocol = protocol;
  memset (&_sockaddr, 0, sizeof( _sockaddr ) );
}


Socket::Socket()
{
  // Default constructor, default settings
  _sd = INVALID_SOCKET;
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

bool Socket::close()
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

bool Socket::create()
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


bool Socket::bind ( const unsigned short port )
{

  if (!is_valid())
  {
    return false;
  }

  _sockaddr.sin_family = _family;
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


bool Socket::listen() const
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


bool Socket::accept ( Socket& new_socket ) const
{
  if (!is_valid())
  {
    return false;
  }

  socklen_t addr_length = sizeof( _sockaddr );
  new_socket._sd = ::accept(_sd, const_cast<sockaddr*>( (const sockaddr*) &_sockaddr), &addr_length );

  if (new_socket._sd <= 0)
  {
    errormessage( getLastError(), "Socket::accept" );
    return false;
  }

  return true;
}


int Socket::send ( const std::string data )
{
  if (!is_valid())
  {
    return 0;
  }

  int status = Socket::send( (const char*) data.c_str(), (const unsigned int) data.size());

  return status;
}


int Socket::send ( const char* data, const unsigned int len )
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
    XBMC->Log(LOG_ERROR, "Socket::send  - select failed");
    _sd = INVALID_SOCKET;
    return 0;
  }
  if (FD_ISSET(_sd, &set_w))
  {
    XBMC->Log(LOG_ERROR, "Socket::send  - failed to send data");
    _sd = INVALID_SOCKET;
    return 0;
  }

  int status = ::send(_sd, data, len, 0 );

  if (status == -1)
  {
    errormessage( getLastError(), "Socket::send");
    XBMC->Log(LOG_ERROR, "Socket::send  - failed to send data");
    _sd = INVALID_SOCKET;
  }
  return status;
}


int Socket::sendto ( const char* data, unsigned int size, bool sendcompletebuffer)
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


int Socket::receive ( std::string& data, unsigned int minpacketsize ) const
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
bool Socket::ReadResponse (int &code, vector<string> &lines)
{
  fd_set         set_r, set_e;
  timeval        timeout;
  int            result;
  int            retries = 6;
  char           buffer[2048];
  char           cont = 0;
  string         line;
  size_t         pos1 = 0, pos2 = 0, pos3 = 0;

  code = 0;

  while (true)
  {
    while ((pos1 = line.find("\r\n", pos3)) != std::string::npos)
    {
      pos2 = line.find(cont);

      lines.push_back(line.substr(pos2+1, pos1-pos2-1));

      line.erase(0, pos1 + 2);
      pos3 = 0;
      return true;
    }

    // we only need to recheck 1 byte
    if (line.size() > 0)
    {
      pos3 = line.size() - 1;
    }
    else
    {
      pos3 = 0;
    }

    if (cont == ' ')
    {
      break;
    }

    timeout.tv_sec  = RECEIVE_TIMEOUT;
    timeout.tv_usec = 0;

    // fill with new data
    FD_ZERO(&set_r);
    FD_ZERO(&set_e);
    FD_SET(_sd, &set_r);
    FD_SET(_sd, &set_e);
    result = select(FD_SETSIZE, &set_r, NULL, &set_e, &timeout);

    if (result < 0)
    {
      XBMC->Log(LOG_DEBUG, "CVTPTransceiver::ReadResponse - select failed");
      lines.push_back("ERROR: Select failed");
      code = 1; //error
      _sd = INVALID_SOCKET;
      return false;
    }

    if (result == 0)
    {
      if (retries != 0)
      {
         XBMC->Log(LOG_DEBUG, "CVTPTransceiver::ReadResponse - timeout waiting for response, retrying... (%i)", retries);
         retries--;
        continue;
      } else {
         XBMC->Log(LOG_DEBUG, "CVTPTransceiver::ReadResponse - timeout waiting for response. Failed after 10 retries.");
         lines.push_back("ERROR: Failed after 10 retries");
         code = 1; //error
        _sd = INVALID_SOCKET;
         return false;
      }
    }

    result = recv(_sd, buffer, sizeof(buffer) - 1, 0);
    if (result < 0)
    {
      XBMC->Log(LOG_DEBUG, "CVTPTransceiver::ReadResponse - recv failed");
      lines.push_back("ERROR: Recv failed");
      code = 1; //error
      _sd = INVALID_SOCKET;
      return false;
    }
    buffer[result] = 0;

    line.append(buffer);
  }

  return true;
}

int Socket::receive ( std::string& data) const
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

int Socket::receive ( char* data, const unsigned int buffersize, const unsigned int minpacketsize ) const
{

  unsigned int receivedsize = 0;
  int status = 0;

  if ( !is_valid() )
  {
    return 0;
  }

  while ( (receivedsize <= minpacketsize) && (receivedsize < buffersize) )
  {
    status = ::recv(_sd, data+receivedsize, (buffersize - receivedsize), 0 );

    if ( status == SOCKET_ERROR )
    {
      errormessage( getLastError(), "Socket::receive" );
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
    return false;
  }

  _sockaddr.sin_family = _family;
  _sockaddr.sin_port = htons ( port );

  if ( !setHostname( host ) )
  {
    XBMC->Log(LOG_ERROR, "Socket::setHostname(%s) failed.\n", host.c_str());
    return false;
  }

  int status = ::connect ( _sd, reinterpret_cast<sockaddr*>(&_sockaddr), sizeof ( _sockaddr ) );

  if ( status == SOCKET_ERROR )
  {
    XBMC->Log(LOG_ERROR, "Socket::connect %s:%u\n", host.c_str(), port);
    errormessage( getLastError(), "Socket::connect" );
    return false;
  }

  return true;
}

bool Socket::reconnect()
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

bool Socket::is_valid() const
{
  return (_sd != INVALID_SOCKET);
}

#if defined(TARGET_WINDOWS)
bool Socket::set_non_blocking ( const bool b )
{
  u_long iMode;

  if ( b )
    iMode = 1;  // enable non_blocking
  else
    iMode = 0;  // disable non_blocking

  if (ioctlsocket(_sd, FIONBIO, &iMode) == -1)
  {
    XBMC->Log(LOG_ERROR, "Socket::set_non_blocking - Can't set socket condition to: %i", iMode);
    return false;
  }

  return true;
}

void Socket::errormessage( int errnum, const char* functionname) const
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
  XBMC->Log(LOG_ERROR, "%s: (Winsock error=%i) %s\n", functionname, errnum, errmsg);
}

int Socket::getLastError() const
{
  return WSAGetLastError();
}

int Socket::win_usage_count = 0; //Declared static in Socket class

bool Socket::osInit()
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

void Socket::osCleanup()
{
  win_usage_count--;
  if(win_usage_count == 0)
  {
    WSACleanup();
  }
}

#elif defined TARGET_LINUX
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
    XBMC->Log(LOG_ERROR, "Socket::set_non_blocking - Can't set socket flags to: %i", opts);
    return false;
  }
  return true;
}

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
  XBMC->Log(LOG_ERROR, "%s: (errno=%i) %s\n", functionname, errnum, errmsg);
}

int Socket::getLastError() const
{
  return errno;
}

bool Socket::osInit()
{
  // Not needed for Linux
  return true;
}

void Socket::osCleanup()
{
  // Not needed for Linux
}
#endif //TARGET_WINDOWS || TARGET_LINUX

} //namespace MPTV
