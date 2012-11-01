#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>
#include <netinet/in.h>

#include "system.h" // for SOCKET

//#define VTP_STANDALONE

class CVTPSession
{
public:
  CVTPSession();
  ~CVTPSession();
  bool Open(const std::string &host, int port);
  void Close();

  bool ReadResponse(int &code, std::string &line);
  bool ReadResponse(int &code, std::vector<std::string> &lines);

  bool SendCommand(const std::string &command);
  bool SendCommand(const std::string &command, int &code, std::string line);
  bool SendCommand(const std::string &command, int &code, std::vector<std::string> &lines);

  struct Channel
  {
    int         index;
    std::string name;
    std::string network;
  };

  bool   GetChannels(std::vector<Channel> &channels);

  SOCKET   GetStreamLive(int channel);
  void   AbortStreamLive();
  bool     CanStreamLive(int channel);

private:
  bool   OpenStreamSocket(SOCKET& socket, struct sockaddr_in& address);
  bool AcceptStreamSocket(SOCKET& socket);

  SOCKET m_socket;
};

