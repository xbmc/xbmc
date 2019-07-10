/*
 *  Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *
 *  Copyright (C) 2002-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <string>
#include <vector>

#include <netinet/in.h>
#include <sys/socket.h>

#include "PlatformDefs.h"

class CUdpClient : CThread
{
public:
  CUdpClient();
  ~CUdpClient(void) override;

protected:

  bool Create();
  void Destroy();

  void OnStartup() override;
  void Process() override;

  bool Broadcast(int aPort, const std::string& aMessage);
  bool Send(const std::string& aIpAddress, int aPort, const std::string& aMessage);
  bool Send(struct sockaddr_in aAddress, const std::string& aMessage);
  bool Send(struct sockaddr_in aAddress, unsigned char* pMessage, DWORD dwSize);

  virtual void OnMessage(struct sockaddr_in& aRemoteAddress, const std::string& aMessage, unsigned char* pMessage, DWORD dwMessageLength){};

protected:

  struct UdpCommand
  {
    struct sockaddr_in address;
    std::string message;
    unsigned char* binary;
    DWORD binarySize;
  };

  bool DispatchNextCommand();

  SOCKET client_socket;

  std::vector<UdpCommand> commands;
  typedef std::vector<UdpCommand> ::iterator COMMANDITERATOR;

  CCriticalSection critical_section;
};
