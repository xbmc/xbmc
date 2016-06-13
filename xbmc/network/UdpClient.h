#pragma once

/*
 *      Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *
 *      Copyright (C) 2002-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include "system.h"

class CUdpClient : CThread
{
public:
  CUdpClient();
  virtual ~CUdpClient(void);

protected:

  bool Create();
  void Destroy();

  void OnStartup();
  void Process();

  bool Broadcast(int aPort, const std::string& aMessage);
  bool Send(const std::string& aIpAddress, int aPort, const std::string& aMessage);
  bool Send(SOCKADDR_IN aAddress, const std::string& aMessage);
  bool Send(SOCKADDR_IN aAddress, LPBYTE pMessage, DWORD dwSize);

  virtual void OnMessage(SOCKADDR_IN& aRemoteAddress, const std::string& aMessage, LPBYTE pMessage, DWORD dwMessageLength){};

protected:

  struct UdpCommand
  {
    SOCKADDR_IN address;
    std::string message;
    LPBYTE binary;
    DWORD binarySize;
  };

  bool DispatchNextCommand();

  SOCKET client_socket;

  std::vector<UdpCommand> commands;
  typedef std::vector<UdpCommand> ::iterator COMMANDITERATOR;

  CCriticalSection critical_section;
};
