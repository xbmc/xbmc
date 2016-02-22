/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2010, 2011 Alexander Pipelka
 *
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
#include <vdr/shutdown.h>

#include "vnsiserver.h"
#include "vnsiclient.h"

unsigned int cVNSIServer::m_IdCnt = 0;

class cAllowedHosts : public cSVDRPhosts
{
public:
  cAllowedHosts(const cString& AllowedHostsFile)
  {
    if (!Load(AllowedHostsFile, true, true))
    {
      ERRORLOG("Invalid or missing '%s'. falling back to 'svdrphosts.conf'.", *AllowedHostsFile);
      cString Base = cString::sprintf("%s/../svdrphosts.conf", *VNSIServerConfig.ConfigDirectory);
      if (!Load(Base, true, true))
      {
        ERRORLOG("Invalid or missing %s. Adding 127.0.0.1 to list of allowed hosts.", *Base);
        cSVDRPhost *localhost = new cSVDRPhost;
        if (localhost->Parse("127.0.0.1"))
          Add(localhost);
        else
          delete localhost;
      }
    }
  }
};

cVNSIServer::cVNSIServer(int listenPort) : cThread("VDR VNSI Server")
{
  m_ServerPort  = listenPort;

  if(*VNSIServerConfig.ConfigDirectory)
  {
    m_AllowedHostsFile = cString::sprintf("%s/" ALLOWED_HOSTS_FILE, *VNSIServerConfig.ConfigDirectory);
  }
  else
  {
    ERRORLOG("cVNSIServer: missing ConfigDirectory!");
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
    INFOLOG("Unable to start VNSI Server, port already in use ?");
    m_ServerFD = -1;
    return;
  }

  listen(m_ServerFD, 10);

  Start();

  INFOLOG("VNSI Server started");
  INFOLOG("Channel streaming timeout: %i seconds", VNSIServerConfig.stream_timeout);
  return;
}

cVNSIServer::~cVNSIServer()
{
  Cancel(-1);
  for (ClientList::iterator i = m_clients.begin(); i != m_clients.end(); i++)
  {
    delete (*i);
  }
  m_clients.erase(m_clients.begin(), m_clients.end());
  Cancel();
  INFOLOG("VNSI Server stopped");
}

void cVNSIServer::NewClientConnected(int fd)
{
  char buf[64];
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);

  if (getpeername(fd, (struct sockaddr *)&sin, &len))
  {
    ERRORLOG("getpeername() failed, dropping new incoming connection %d", m_IdCnt);
    close(fd);
    return;
  }

  cAllowedHosts AllowedHosts(m_AllowedHostsFile);
  if (!AllowedHosts.Acceptable(sin.sin_addr.s_addr))
  {
    ERRORLOG("Address not allowed to connect (%s)", *m_AllowedHostsFile);
    close(fd);
    return;
  }

  if (fcntl(fd, F_SETFL, fcntl (fd, F_GETFL) | O_NONBLOCK) == -1)
  {
    ERRORLOG("Error setting control socket to nonblocking mode");
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

  INFOLOG("Client with ID %d connected: %s", m_IdCnt, cxSocket::ip2txt(sin.sin_addr.s_addr, sin.sin_port, buf));
  cVNSIClient *connection = new cVNSIClient(fd, m_IdCnt, cxSocket::ip2txt(sin.sin_addr.s_addr, sin.sin_port, buf));
  m_clients.push_back(connection);
  m_IdCnt++;
}

void cVNSIServer::Action(void)
{
  fd_set fds;
  struct timeval tv;

  // get initial state of the recordings
  int recState = -1;
  Recordings.StateChanged(recState);

  // get initial state of the timers
  int timerState = -1;
  Timers.Modified(timerState);

  while (Running())
  {
    FD_ZERO(&fds);
    FD_SET(m_ServerFD, &fds);

    tv.tv_sec = 0;
    tv.tv_usec = 250*1000;

    int r = select(m_ServerFD + 1, &fds, NULL, NULL, &tv);
    if (r == -1)
    {
      ERRORLOG("failed during select");
      continue;
    }
    if (r == 0)
    {
      // remove disconnected clients
      for (ClientList::iterator i = m_clients.begin(); i != m_clients.end();)
      {
        if (!(*i)->Active())
        {
          INFOLOG("Client with ID %u seems to be disconnected, removing from client list", (*i)->GetID());
          delete (*i);
          i = m_clients.erase(i);
        }
        else {
          i++;
        }
      }

      // trigger clients to reload the modified channel list
      if(m_clients.size() > 0)
      {
        Channels.Lock(false);
        if(Channels.Modified() != 0)
        {
          INFOLOG("Requesting clients to reload channel list");
          for (ClientList::iterator i = m_clients.begin(); i != m_clients.end(); i++)
            (*i)->ChannelChange();
        }
        Channels.Unlock();
      }

      // reset inactivity timeout as long as there are clients connected
      if(m_clients.size() > 0) {
        ShutdownHandler.SetUserInactiveTimeout();
      }

      // update recordings
      if(Recordings.StateChanged(recState))
      {
        INFOLOG("Recordings state changed (%i)", recState);
        INFOLOG("Requesting clients to reload recordings list");
        for (ClientList::iterator i = m_clients.begin(); i != m_clients.end(); i++)
          (*i)->RecordingsChange();
      }

      // update timers
      if(Timers.Modified(timerState))
      {
        INFOLOG("Timers state changed (%i)", timerState);
        INFOLOG("Requesting clients to reload timers");
        for (ClientList::iterator i = m_clients.begin(); i != m_clients.end(); i++)
        {
         (*i)->TimerChange();
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
      ERRORLOG("accept failed");
    }
  }
  return;
}
