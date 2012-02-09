#pragma once

/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "HTSPTypes.h"
#include "../../../lib/platform/threads/mutex.h"

extern "C" {
#include "libhts/net.h"
#include "libhts/htsmsg.h"
}

namespace PLATFORM
{
  class CTcpConnection;
}

class CHTSPConnection
{
public:
  CHTSPConnection();
  ~CHTSPConnection();

  bool        Connect(void);
  void        Close();
  void        Abort();
  bool        IsConnected(void) const { return m_bIsConnected; }
  int         GetProtocol() const { return m_iProtocol; }
  const char *GetServerName() const { return m_strServerName.c_str(); }
  const char *GetVersion() const { return m_strVersion.c_str(); }

  htsmsg_t *  ReadMessage(int iInitialTimeout = 10000, int iDatapacketTimeout = 10000);
  bool        TransmitMessage(htsmsg_t* m);
  htsmsg_t *  ReadResult (htsmsg_t* m, bool sequence = true);
  bool        ReadSuccess(htsmsg_t* m, bool sequence = true, std::string action = "");

private:
  bool SendGreeting(void);
  bool Auth(void);

  PLATFORM::CMutex          m_mutex;
  PLATFORM::CTcpConnection* m_socket;
  void*                     m_challenge;
  int                       m_iChallengeLength;
  int                       m_iProtocol;
  int                       m_iPortnumber;
  int                       m_iConnectTimeout;
  std::string               m_strServerName;
  std::string               m_strUsername;
  std::string               m_strPassword;
  std::string               m_strVersion;
  std::string               m_strHostname;
  bool                      m_bIsConnected;

  std::deque<htsmsg_t*>     m_queue;
  const unsigned int        m_iQueueSize;
};
