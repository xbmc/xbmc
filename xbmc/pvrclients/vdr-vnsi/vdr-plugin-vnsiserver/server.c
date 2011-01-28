/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#include <netdb.h>
#include <poll.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <vdr/plugin.h>

#include "server.h"
#include "connection.h"

unsigned int cServer::m_IdCnt = 0;

class cAllowedHosts : public cSVDRPhosts
{
public:
  cAllowedHosts(const cString& AllowedHostsFile)
  {
    if (!Load(AllowedHostsFile, true, true))
    {
      isyslog("VNSI-Error: Invalid or missing '%s'. falling back to 'svdrphosts.conf'.", *AllowedHostsFile);
      cString Base = cString::sprintf("%s/../svdrphosts.conf", *VNSIServerConfig.ConfigDirectory);
      if (!Load(Base, true, true))
      {
        esyslog("VNSI-Error: Invalid or missing %s. Adding 127.0.0.1 to list of allowed hosts.", *Base);
        cSVDRPhost *localhost = new cSVDRPhost;
        if (localhost->Parse("127.0.0.1"))
          Add(localhost);
        else
          delete localhost;
      }
    }
  }
};

cServer::cServer(int listenPort) : cThread("VDR VNSI Server")
{
  m_ServerPort  = listenPort;
  m_ServerId    = time(NULL) ^ getpid();

  if(*VNSIServerConfig.ConfigDirectory)
  {
    m_AllowedHostsFile = cString::sprintf("%s/" ALLOWED_HOSTS_FILE, *VNSIServerConfig.ConfigDirectory);
  }
  else
  {
    esyslog("VNSI-Error: cServer: missing ConfigDirectory!");
    m_AllowedHostsFile = cString::sprintf("/video/" ALLOWED_HOSTS_FILE);
  }

  m_ServerFD = socket(AF_INET, SOCK_STREAM, 0);
  if(m_ServerFD == -1)
    return;

  fcntl(m_ServerFD, F_SETFD, fcntl(m_ServerFD, F_GETFD) | FD_CLOEXEC);

  int one = 1;
  setsockopt(m_ServerFD, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

  struct sockaddr_in s;
  memset(&s, 0, sizeof(s));
  s.sin_family = AF_INET;
  s.sin_port = htons(m_ServerPort);

  int x = bind(m_ServerFD, (struct sockaddr *)&s, sizeof(s));
  if (x < 0)
  {
    close(m_ServerFD);
    isyslog("VNSI: Unable to start VNSI Server, port already in use ?");
    m_ServerFD = -1;
    return;
  }

  listen(m_ServerFD, 10);

  Start();

  isyslog("VNSI: VNSI Server started");
  isyslog("VNSI: Channel streaming timeout: %i seconds", VNSIServerConfig.stream_timeout);
  return;
}

cServer::~cServer()
{
  Cancel(-1);
  for (Connections::iterator i = m_Connections.begin(); i != m_Connections.end(); i++)
  {
    delete (*i);
  }
  m_Connections.erase(m_Connections.begin(), m_Connections.end());
  Cancel();
  isyslog("VNSI: VNSI Server stopped");
}

void cServer::NewClientConnected(int fd)
{
  char buf[64];
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);

  if (getpeername(fd, (struct sockaddr *)&sin, &len))
  {
    esyslog("VNSI-Error: getpeername() failed, dropping new incoming connection %d", m_IdCnt);
    close(fd);
    return;
  }

  cAllowedHosts AllowedHosts(m_AllowedHostsFile);
  if (!AllowedHosts.Acceptable(sin.sin_addr.s_addr))
  {
    esyslog("VNSI: Address not allowed to connect (%s)", *m_AllowedHostsFile);
    close(fd);
    return;
  }

  if (fcntl(fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK) == -1)
  {
    esyslog("VNSI-Error: Error setting control socket to nonblocking mode");
    close(fd);
    return;
  }

  int val = 1;
  setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val));

  val = 30;
  setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &val, sizeof(val));

  val = 15;
  setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &val, sizeof(val));

  val = 5;
  setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &val, sizeof(val));

  val = 1;
  setsockopt(fd, SOL_TCP, TCP_NODELAY, &val, sizeof(val));

  isyslog("VNSI: Client with ID %d connected: %s", m_IdCnt, cxSocket::ip2txt(sin.sin_addr.s_addr, sin.sin_port, buf));
  cConnection *connection = new cConnection(this, fd, m_IdCnt, cxSocket::ip2txt(sin.sin_addr.s_addr, sin.sin_port, buf));
  m_Connections.push_back(connection);
  m_IdCnt++;
}

void cServer::Action(void)
{
  fd_set fds;
  struct timeval tv;
    
  while (Running())
  {
    FD_ZERO(&fds);
    FD_SET(m_ServerFD, &fds);
    
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    
    int r = select(m_ServerFD + 1, &fds, NULL, NULL, &tv);
    if (r == -1)
    {
      esyslog("VNSI-Error: failed during select");
      continue;
    }
    if (r == 0)
    {
      for (Connections::iterator i = m_Connections.begin(); i != m_Connections.end();)
      {
        if (!(*i)->Active())
        {
          isyslog("VNSI: Client with ID %u seems to be disconnected, removing from client list", (*i)->GetID());
          delete (*i);
          i = m_Connections.erase(i);
        }
	else {
	  i++;
	}
      }
      continue;
    }

    int fd = accept(m_ServerFD, 0, 0);
    if (fd >= 0)
    {
      NewClientConnected(fd);
    }
    else
    {
      esyslog("VNSI-Error: accept failed");
    }
  }
  return;
}
