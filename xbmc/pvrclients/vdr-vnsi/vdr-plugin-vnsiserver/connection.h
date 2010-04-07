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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <vdr/thread.h>
#include <vdr/receiver.h>

#include "config.h"
#include "cxsocket.h"

class cServer;
class cChannel;
class cDevice;
class cLiveStreamer;
class cResponsePacket;
class cRecPlayer;
class cCmdControl;

class cConnection : public cThread
{
private:
  friend class cCmdControl;

  unsigned int    m_Id;
  cxSocket        m_socket;
  cServer        *m_server;
  bool            m_loggedIn;
  cLiveStreamer  *m_Streamer;
  const cChannel *m_Channel;
  bool            m_isStreaming;
  FILE           *m_NetLogFile;
  cString         m_ClientAddress;
  cRecPlayer     *m_RecPlayer;

protected:
  virtual void Action(void);

public:
  cConnection(cServer *server, int fd, unsigned int id, const char *ClientAdr);
  virtual ~cConnection();

  unsigned int GetID() { return m_Id; }
  void SetLoggedIn(bool yesNo) { m_loggedIn = yesNo; }
  void EnableNetLog(bool yesNo, const char* ClientName = "");
  cxSocket *GetSocket() { return &m_socket; }
  bool StartChannelStreaming(const cChannel *channel, cResponsePacket *resp);
  void StopChannelStreaming();
  bool IsStreaming() { return m_isStreaming; }
  cRecPlayer *GetRecPlayer() { return m_RecPlayer; }
};

#endif /* CONNECTION_H */
