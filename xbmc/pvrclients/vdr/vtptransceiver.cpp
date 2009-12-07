/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "vtptransceiver.h"
#include "client.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

using namespace std;

CVTPTransceiver::CVTPTransceiver()
  : m_socket(INVALID_SOCKET)
{}

CVTPTransceiver::~CVTPTransceiver()
{
  Quit();
  Close();
}

bool CVTPTransceiver::OpenStreamSocket(SOCKET& sock, struct sockaddr_in& address2)
{
  struct sockaddr_in address(address2);

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if(sock == INVALID_SOCKET)
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::OpenStreamSocket - invalid socket");
    return false;
  }

  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port        = 0;

  if(bind(sock, (struct sockaddr*) &address, sizeof(address)) == SOCKET_ERROR)
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::OpenStreamSocket - bind failed");
    return false;
  }

  socklen_t len = sizeof(address);
  if(getsockname(sock, (struct sockaddr*) &address, &len) == SOCKET_ERROR)
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::OpenStreamSocket - bind failed");
    return false;
  }

  if(listen(sock, 1) == SOCKET_ERROR)
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::OpenStreamSocket - listen failed");
    return false;
  }

  address2.sin_port = address.sin_port;

  XBMC_log(LOG_DEBUG, "CVTPTransceiver::OpenStreamSocket - listening on %s:%d", inet_ntoa(address.sin_addr), address.sin_port);
  return true;
}

bool CVTPTransceiver::AcceptStreamSocket(SOCKET& sock2)
{
  SOCKET sock;
  sock = accept(sock2, NULL, NULL);
  if(sock == INVALID_SOCKET)
  {
    XBMC_log(LOG_ERROR, "CVTPStream::Accept - failed to accept incomming connection");
    return false;
  }

  closesocket(sock2);

  int sol=1;
  // Ignore possible errors here, proceed as usual
  setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &sol, sizeof(sol));

  sock2 = sock;
  return true;
}

void CVTPTransceiver::Close()
{
  if(m_socket != INVALID_SOCKET)
    closesocket(m_socket);
}

bool CVTPTransceiver::Open(const string &host, int port)
{
  socklen_t       len;
  struct hostent *hp;

  if ((m_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_IP)) == INVALID_SOCKET)
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::Open - Can't open socket '%s:%u'", host.c_str(), port);
    return false;
  }

#ifdef _LINUX
  int flags = fcntl(m_socket, F_GETFL);
  if (fcntl(m_socket, F_SETFL, O_NONBLOCK) == -1)
#elif defined(_WIN32) || defined(_WIN64)
  u_long iMode = 1;
  if (ioctlsocket(m_socket, FIONBIO, &iMode) == -1)
#endif
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::Open - Can't set socket to non blocking mode");
    Close();
    return false;
  }

  if ((hp = gethostbyname(host.c_str())) == NULL)
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::Open - failed to resolve hostname: %s", host.c_str());
    Close();
    return false;
  }

  m_LocalAddr.sin_family = AF_INET;
  m_LocalAddr.sin_port   = 0;
  m_LocalAddr.sin_addr.s_addr = INADDR_ANY;
  if (bind(m_socket, (struct sockaddr*)&m_LocalAddr, sizeof(m_LocalAddr)) == -1)
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::Open - Can't bind requested socket '%s:%u'", host.c_str(), port);
    Close();
    return false;
  }

  m_RemoteAddr.sin_family = AF_INET;
  m_RemoteAddr.sin_port   = htons(port);
  memcpy(&m_RemoteAddr.sin_addr.s_addr, hp->h_addr_list[0], hp->h_length);
  if (connect(m_socket, (struct sockaddr*)&m_RemoteAddr, sizeof(m_RemoteAddr)) < 0)
  {
    fd_set         set_r, set_w, set_e;
    timeval        timeout;

    timeout.tv_sec  = m_iConnectTimeout;
    timeout.tv_usec = 0;

    // fill with new data
    FD_ZERO(&set_r);
    FD_ZERO(&set_w);
    FD_ZERO(&set_e);
    FD_SET(m_socket, &set_r);
    FD_SET(m_socket, &set_w);
    FD_SET(m_socket, &set_e);
    int result = select(FD_SETSIZE, &set_r, &set_w, &set_e, &timeout);
    if (result < 0)
    {
      XBMC_log(LOG_ERROR, "CVTPTransceiver::Open - select failed '%s:%u'", host.c_str(), port);
      Close();
      return false;
    }
    else if (result == 0)
    {
      XBMC_log(LOG_ERROR, "CVTPTransceiver::Open - connect timed out '%s:%u'", host.c_str(), port);
      Close();
      return false;
    }
    else if (!IsConnected(m_socket, &set_r, &set_w, &set_e))
    {
      XBMC_log(LOG_ERROR, "CVTPTransceiver::Open - failed to connect to IP '%d.%d.%d.%d",
                        (ntohl(m_RemoteAddr.sin_addr.s_addr) >> 24) & 0xff,
                        (ntohl(m_RemoteAddr.sin_addr.s_addr) >> 16) & 0xff,
                        (ntohl(m_RemoteAddr.sin_addr.s_addr) >> 8) & 0xff,
                        (ntohl(m_RemoteAddr.sin_addr.s_addr) & 0xff));
      Close();
      return false;
    }
  }

#ifdef _LINUX
  if (fcntl(m_socket, F_SETFL, flags) == -1)
#elif defined(_WIN32) || defined(_WIN64)
  iMode = 0;
  if (ioctlsocket(m_socket, FIONBIO, &iMode) == -1)
#endif
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::Open - Can't set socket initial condition");
    Close();
    return false;
  }

  len = sizeof(struct sockaddr_in);
  if (getpeername(m_socket, (struct sockaddr*)&m_RemoteAddr, &len) == -1)
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::Open - Can't get the name of the peer socket '%s:%u'", host.c_str(), port);
    Close();
    return false;
  }

  len = sizeof(struct sockaddr_in);
  if (getsockname(m_socket, (struct sockaddr*)&m_LocalAddr, &len) == -1)
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::Open - Can't get the socket name '%s:%u'", host.c_str(), port);
    Close();
    return false;
  }

  // VTP Server will send a greeting
  string line;
  int    code;
  ReadResponse(code, line);

  if(!SendCommand("CAPS TS", code, line))
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::Open - server is unable to provide mpeg-ts");
    return false;
  }

  XBMC_log(LOG_INFO, "CVTPTransceiver::Open - server greeting: %s", line.c_str());
  return true;
}

#if defined(_WIN32) || defined(_WIN64)
bool CVTPTransceiver::IsConnected(SOCKET socket, fd_set *rd, fd_set *wr, fd_set *ex)
{
  WSASetLastError(0);
  if (!FD_ISSET(socket, rd) && !FD_ISSET(socket, wr))
    return false;
  if (FD_ISSET(socket, ex))
    return false;
  return true;
}
#else
bool CVTPTransceiver::IsConnected(SOCKET socket, fd_set *rd, fd_set *wr, fd_set *ex)
{
  int err;
  socklen_t len = sizeof(err);

  errno = 0;             /* assume no error */
  if (!FD_ISSET(socket, rd ) && !FD_ISSET(socket, wr))
    return false;
  if (getsockopt(socket, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
    return false;
  errno = err;           /* in case we're not connected */
  return err == 0;
}
#endif

bool CVTPTransceiver::IsOpen()
{
  return !(m_socket == INVALID_SOCKET);
}

bool CVTPTransceiver::ReadResponse(int &code, string &line)
{
  vector<string> lines;
  if(ReadResponse(code, lines))
  {
    line = lines[lines.size()-1];
    return true;
  }
  return false;
}

bool CVTPTransceiver::ReadResponse(int &code, vector<string> &lines)
{
  fd_set         set_r, set_e;
  timeval        timeout;
  int            result;
  int            retries = 5;
  char           buffer[2048];
  char           cont = 0;
  string         line;
  size_t         pos1 = 0, pos2 = 0, pos3 = 0;

  while(true)
  {
    while( (pos1 = line.find("\r\n", pos3)) != std::string::npos)
    {
      if(sscanf(line.c_str(), "%d%c", &code, &cont) != 2)
      {
        XBMC_log(LOG_DEBUG, "CVTPTransceiver::ReadResponse - unknown line format: %s", line.c_str());
        line.erase(0, pos1 + 2);
        continue;
      }

      pos2 = line.find(cont);

      lines.push_back(line.substr(pos2+1, pos1-pos2-1));

      line.erase(0, pos1 + 2);
      pos3 = 0;
    }

    // we only need to recheck 1 byte
    if(line.size() > 0)
      pos3 = line.size() - 1;
    else
      pos3 = 0;

    if(cont == ' ')
      break;

    //TODO set 10 seconds timeout value??
    timeout.tv_sec  = 10;
    timeout.tv_usec = 0;

    // fill with new data
    FD_ZERO(&set_r);
    FD_ZERO(&set_e);
    FD_SET(m_socket, &set_r);
    FD_SET(m_socket, &set_e);
    result = select(FD_SETSIZE, &set_r, NULL, &set_e, &timeout);
    if(result < 0)
    {
      XBMC_log(LOG_DEBUG, "CVTPTransceiver::ReadResponse - select failed");
      m_socket = INVALID_SOCKET;
      return false;
    }

    if(result == 0)
    {
      XBMC_log(LOG_DEBUG, "CVTPTransceiver::ReadResponse - timeout waiting for response, retrying...");
      if (retries != 0) {
          retries--;
      continue;
    }
      else {
          m_socket = INVALID_SOCKET;
          return false;
      }
    }

    result = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
    if(result < 0)
    {
      XBMC_log(LOG_DEBUG, "CVTPTransceiver::ReadResponse - recv failed");
      m_socket = INVALID_SOCKET;
      return false;
    }
    buffer[result] = 0;

    line.append(buffer);
  }

  return true;
}

bool CVTPTransceiver::SendCommand(const string &command)
{
  fd_set set_w, set_e;
  struct timeval tv;
  int  result;
  char buffer[1024];
  int  len;

  len = sprintf(buffer, "%s\r\n", command.c_str());

  // fill with new data
  tv.tv_sec  = 0;
  tv.tv_usec = 0;

  FD_ZERO(&set_w);
  FD_ZERO(&set_e);
  FD_SET(m_socket, &set_w);
  FD_SET(m_socket, &set_e);
  result = select(FD_SETSIZE, &set_w, NULL, &set_e, &tv);
  if(result < 0)
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::SendCommand - select failed");
    m_socket = INVALID_SOCKET;
    return false;
  }
  if (FD_ISSET(m_socket, &set_w))
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::SendCommand - failed to send data");
    m_socket = INVALID_SOCKET;
    return false;
  }
  if(send(m_socket, buffer, len, 0) != len)
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::SendCommand - failed to send data");
    m_socket = INVALID_SOCKET;
    return false;
  }
  return true;
}

bool CVTPTransceiver::SendCommand(const string &command, int &code, string line)
{
  vector<string> lines;
  if(SendCommand(command, code, lines))
  {
    line = lines[lines.size()-1];
    return true;
  }
  return false;
}

bool CVTPTransceiver::SendCommand(const string &command, int &code, vector<string> &lines)
{
  if(!SendCommand(command))
    return false;

  if(!ReadResponse(code, lines))
    return false;

  if(code < 200 || code > 299)
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::SendCommand - Failed with code: %d (%s)", code, lines[lines.size()-1].c_str());
    return false;
  }

  return true;
}

bool CVTPTransceiver::SetChannel(unsigned int channel)
{
  char        buffer[64];
  string      result;
  int         code;

  sprintf(buffer, "TUNE %d", channel);
  if(!SendCommand(buffer, code, result))
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::SetChannel - server is unable to tune to said channel '%i'", channel);
		return false;
	}
	return true;
}

SOCKET CVTPTransceiver::GetStreamLive(unsigned int channel)
{
  sockaddr_in address;
  SOCKET      sock;
  socklen_t   len = sizeof(address);
  char        buffer[1024];
  string      result;
  int         code;

  sprintf(buffer, "PROV %d %d", 100, channel);
  if(!SendCommand(buffer, code, result))
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::GetStreamLive - server is unable to provide channel '%i'", channel);
    return INVALID_SOCKET;
  }

  sprintf(buffer, "TUNE %d", channel);
  if(!SendCommand(buffer, code, result))
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::GetStreamLive - server is unable to tune to said channel '%i'", channel);
    return INVALID_SOCKET;
  }

  if(getsockname(m_socket, (struct sockaddr*) &address, &len) == SOCKET_ERROR)
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::GetStreamLive - getsockname failed");
    return INVALID_SOCKET;
  }

  XBMC_log(LOG_DEBUG, "CVTPTransceiver::GetStreamLive - local address %s:%d", inet_ntoa(address.sin_addr), ntohs(address.sin_port) );

  if(!OpenStreamSocket(sock, address))
    return INVALID_SOCKET;

  int port = ntohs(address.sin_port);
  int addr = ntohl(address.sin_addr.s_addr);

  sprintf(buffer, "PORT 0 %d,%d,%d,%d,%d,%d"
                , (addr & 0xFF000000)>>24
                , (addr & 0x00FF0000)>>16
                , (addr & 0x0000FF00)>>8
                , (addr & 0x000000FF)>>0
                , (port & 0xFF00)>>8
                , (port & 0x00FF)>>0);

  if(!SendCommand(buffer, code, result))
    return NULL;

  if(!AcceptStreamSocket(sock))
  {
    closesocket(sock);
    return INVALID_SOCKET;
  }
  return sock;
}

SOCKET CVTPTransceiver::GetStreamRecording(int recording, uint64_t *size, uint32_t *frames)
{
    sockaddr_in address;
    SOCKET      sock;
    socklen_t   len = sizeof(address);
    char        buffer[1024];
    string      result;
    int         code;
    vector<string> lines;

    sprintf(buffer, "PLAY %d", recording);
    if (!SendCommand(buffer, code, lines))
    {
      return INVALID_SOCKET;
    }

    vector<string>::iterator it = lines.begin();
    string& data(*it);

    *size = atoll(data.c_str());
    data.erase(0,data.find(" ", 0)+1);
    *frames = atol(data.c_str());

    if(getsockname(m_socket, (struct sockaddr*) &address, &len) == SOCKET_ERROR)
    {
      XBMC_log(LOG_ERROR, "CVTPTransceiver::GetStreamRecording - getsockname failed");
      return INVALID_SOCKET;
    }

    XBMC_log(LOG_DEBUG, "CVTPTransceiver::GetStreamRecording - local address %s:%d", inet_ntoa(address.sin_addr), ntohs(address.sin_port) );

    if(!OpenStreamSocket(sock, address))
      return INVALID_SOCKET;

    int port = ntohs(address.sin_port);
    int addr = ntohl(address.sin_addr.s_addr);

    sprintf(buffer, "PORT 1 %d,%d,%d,%d,%d,%d"
                  , (addr & 0xFF000000)>>24
                  , (addr & 0x00FF0000)>>16
                  , (addr & 0x0000FF00)>>8
                  , (addr & 0x000000FF)>>0
                  , (port & 0xFF00)>>8
                  , (port & 0x00FF)>>0);

    if(!SendCommand(buffer, code, result))
      return INVALID_SOCKET;


    if(!AcceptStreamSocket(sock))
    {
      closesocket(sock);
      return INVALID_SOCKET;
    }
    return sock;
}

SOCKET CVTPTransceiver::GetStreamData()
{
  sockaddr_in address;
  SOCKET      sock;
  socklen_t   len = sizeof(address);
  char        buffer[1024];
  string      result;
  int         code;

  if(getsockname(m_socket, (struct sockaddr*) &address, &len) == SOCKET_ERROR)
  {
    XBMC_log(LOG_ERROR, "VTPTransceiver::GetStreamData - getsockname failed");
    return INVALID_SOCKET;
  }

  XBMC_log(LOG_DEBUG, "VTPTransceiver::GetStreamData - local address %s:%d", inet_ntoa(address.sin_addr), ntohs(address.sin_port) );

  if(!OpenStreamSocket(sock, address))
    return INVALID_SOCKET;

  int port = ntohs(address.sin_port);
  int addr = ntohl(address.sin_addr.s_addr);

  sprintf(buffer, "PORT 3 %d,%d,%d,%d,%d,%d"
                , (addr & 0xFF000000)>>24
                , (addr & 0x00FF0000)>>16
                , (addr & 0x0000FF00)>>8
                , (addr & 0x000000FF)>>0
                , (port & 0xFF00)>>8
                , (port & 0x00FF)>>0);

  if(!SendCommand(buffer, code, result))
    return INVALID_SOCKET;

  if(!AcceptStreamSocket(sock))
  {
    closesocket(sock);
    return INVALID_SOCKET;
  }
  return sock;
}

void CVTPTransceiver::AbortStreamLive()
{
  if(m_socket == INVALID_SOCKET)
    return;

  string line;
  int    code;
  if(SendCommand("ABRT 0", code, line))
    return;
  XBMC_log(LOG_ERROR, "CVTPTransceiver::AbortStreamLive - failed");
}

void CVTPTransceiver::AbortStreamRecording()
{
  if(m_socket == INVALID_SOCKET)
    return;

  string line;
  int    code;
  if(SendCommand("ABRT 1", code, line))
    return;
  XBMC_log(LOG_ERROR, "CVTPTransceiver::AbortStreamRecording - failed");
}

void CVTPTransceiver::AbortStreamData()
{
  if(m_socket == INVALID_SOCKET)
    return;

  string line;
  int    code;
  if(SendCommand("ABRT 3", code, line))
    return;
  XBMC_log(LOG_ERROR, "VTPTransceiver::AbortStreamData - failed");
}

bool CVTPTransceiver::CanStreamLive(int channel)
{
  if(m_socket == INVALID_SOCKET)
    return false;

  char   buffer[64];
  string line;
  int    code;

  sprintf(buffer, "PROV %d %d", -1, channel);
  if(!SendCommand(buffer, code, line))
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::CanStreamLive - server is unable to provide channel %d", channel);
    return false;
  }
  return true;
}

bool CVTPTransceiver::SuspendServer()
{
  vector<string> lines;
  int            code;
  bool           ret;

  ret = SendCommand("SUSP", code, lines);

  if (!ret || code != 220)
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::SuspendServer: Couldn't suspend server");
    return false;
  }
  return true;
}

bool CVTPTransceiver::Quit()
{
  vector<string> lines;
  int            code;
  bool           ret;

  ret = SendCommand("QUIT", code, lines);

  if (!ret || code != 221)
  {
    XBMC_log(LOG_ERROR, "CVTPTransceiver::Quit: Couldn't quit command connection");
    return false;
  }
  return true;
}
