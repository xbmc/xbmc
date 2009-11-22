#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include <string>
#include <vector>

#include "pvrclient-vdr_os.h"

class CVTPTransceiver
{
public:
  CVTPTransceiver();
  ~CVTPTransceiver();
  bool Open(const std::string &host, int port);
  void Close();
  bool IsConnected(SOCKET socket, fd_set *rd, fd_set *wr, fd_set *ex);

  bool ReadResponse(int &code, std::string &line);
  bool ReadResponse(int &code, std::vector<std::string> &lines);

  bool SendCommand(const std::string &command);
  bool SendCommand(const std::string &command, int &code, std::string line);
  bool SendCommand(const std::string &command, int &code, std::vector<std::string> &lines);

  bool   SetChannel(unsigned int channel);
  SOCKET GetStreamLive(unsigned int channel);
  SOCKET GetStreamRecording(int recording, uint64_t *size, uint32_t *frames);
  SOCKET GetStreamData();
  void   AbortStreamLive();
  void   AbortStreamRecording();
  void   AbortStreamData();
  bool   CanStreamLive(int channel);
  bool   IsOpen();
  bool   SuspendServer();
  bool   Quit();

private:
  struct sockaddr_in m_LocalAddr;
  struct sockaddr_in m_RemoteAddr;

  bool   OpenStreamSocket(SOCKET& socket, struct sockaddr_in& address);
  bool   AcceptStreamSocket(SOCKET& socket);

  SOCKET m_socket;
};
