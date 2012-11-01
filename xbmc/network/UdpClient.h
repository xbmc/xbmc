#pragma once

/*
* XBMC Media Center
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "utils/StdString.h"
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

  bool Broadcast(int aPort, CStdString& aMessage);
  bool Send(CStdString aIpAddress, int aPort, CStdString& aMessage);
  bool Send(SOCKADDR_IN aAddress, CStdString& aMessage);
  bool Send(SOCKADDR_IN aAddress, LPBYTE pMessage, DWORD dwSize);

  virtual void OnMessage(SOCKADDR_IN& aRemoteAddress, CStdString& aMessage, LPBYTE pMessage, DWORD dwMessageLength){};

protected:

  struct UdpCommand
  {
    SOCKADDR_IN address;
    CStdString message;
    LPBYTE binary;
    DWORD binarySize;
  };

  bool DispatchNextCommand();

  SOCKET client_socket;

  std::vector<UdpCommand> commands;
  typedef std::vector<UdpCommand> ::iterator COMMANDITERATOR;

  CCriticalSection critical_section;
};
