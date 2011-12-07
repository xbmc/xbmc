#pragma once
/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *      Copyright (C) 2011 Alexander Pipelka
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

#include <stdint.h>
#include <string>

extern "C" {
#include "libTcpSocket/os-dependent_socket.h"
}

class cResponsePacket;
class cRequestPacket;

class cXVDRSession
{
public:
  cXVDRSession();
  virtual ~cXVDRSession();

  virtual bool      Open(const std::string& hostname, int port, const char *name = NULL);
  virtual bool      Login();
  virtual void      Close();
  virtual void      Abort();

  cResponsePacket*  ReadMessage();
  bool              SendMessage(cRequestPacket* vrp);

  cResponsePacket*  ReadResult(cRequestPacket* vrp);
  bool              ReadSuccess(cRequestPacket* vrp, uint32_t& rc);
  bool              ReadSuccess(cRequestPacket* vrp);

  int                GetProtocol()   { return m_protocol; }
  const std::string& GetServerName() { return m_server; }
  const std::string& GetVersion()    { return m_version; }

protected:

  void SleepMs(int ms);

  bool TryReconnect();
  bool IsOpen() { return m_fd != INVALID_SOCKET; }

  virtual void OnDisconnect();
  virtual void OnReconnect();

  virtual void SignalConnectionLost();
  bool ConnectionLost() { return m_connectionLost; }

  std::string     m_hostname;
  int             m_port;
  std::string     m_name;

private:

  bool readData(uint8_t* buffer, int totalBytes);

  socket_t    m_fd;
  int         m_protocol;
  std::string m_server;
  std::string m_version;
  bool        m_connectionLost;

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
