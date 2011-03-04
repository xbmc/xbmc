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

#pragma once
#include "pvrclient-vdrVNSI_os.h"
#include "StdString.h"

#include <algorithm>
#include <vector>
#include <map>

class cResponsePacket;
class cRequestPacket;

class cVNSISession
{
public:
  cVNSISession();
  ~cVNSISession();

  bool              Open(const CStdString& hostname, int port, long timeout, const char *name = NULL);
  void              Close();
  void              Abort();

  cResponsePacket*  ReadMessage(int timeout = 20);
  bool              SendMessage(cRequestPacket* vrp);
  int               sendData(void* bufR, size_t count);
  int               readData(uint8_t* buffer, int totalBytes, int TimeOut = 2);

  cResponsePacket*  ReadResult(cRequestPacket* vrp, bool sequence = true);
  bool              ReadSuccess(cRequestPacket* m, bool sequence = true);
  int               GetProtocol()   { return m_protocol; }
  const CStdString& GetServerName() { return m_server; }
  const CStdString& GetVersion()    { return m_version; }

private:
  SOCKET      m_fd;
  int         m_protocol;
  CStdString  m_server;
  CStdString  m_version;

  struct {
        uint32_t opCodeID;
        uint32_t streamID;
        uint32_t duration;
        uint8_t pts[sizeof(int64_t)];
        uint8_t dts[sizeof(int64_t)];
        uint32_t userDataLength;
  } m_streamPacketHeader;

  struct {
        uint32_t requestID;
        uint32_t userDataLength;
  } m_responsePacketHeader;
};
