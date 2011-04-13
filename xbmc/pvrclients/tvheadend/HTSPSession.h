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

class cHTSPSession
{
public:
  cHTSPSession();
  ~cHTSPSession();

  bool      Connect(const std::string& hostname, int port);
  bool      IsConnected() { return m_connected; }
  void      Close(bool bForce = false);
  void      Abort();
  bool      Auth(const std::string& username, const std::string& password);

  htsmsg_t* ReadMessage(int timeout = 10000);
  bool      SendMessage(htsmsg_t* m);

  htsmsg_t* ReadResult (htsmsg_t* m, bool sequence = true);
  bool      ReadSuccess(htsmsg_t* m, bool sequence = true, std::string action = "");

  bool      SendSubscribe  (int subscription, int channel);
  bool      SendUnsubscribe(int subscription);
  bool      SendEnableAsync();
  bool      GetEvent(SEvent& event, uint32_t id);

  int         GetProtocol()   { return m_protocol; }
  const char* GetServerName() { return m_server.c_str(); }
  const char* GetVersion()    { return m_version.c_str(); }
  unsigned    AddSequence()   { return ++m_seq; }

  void      EnableNotifications(bool bSetTo = true) { m_bSendNotifications = bSetTo; }
  bool      SendNotifications(void) { return m_bSendNotifications; }

  static bool ParseEvent         (htsmsg_t* msg, uint32_t id, SEvent &event);
  static void ParseChannelUpdate (htsmsg_t* msg, SChannels &channels);
  static void ParseChannelRemove (htsmsg_t* msg, SChannels &channels);
  static void ParseTagUpdate     (htsmsg_t* msg, STags &tags);
  static void ParseTagRemove     (htsmsg_t* msg, STags &tags);
  static bool ParseQueueStatus   (htsmsg_t* msg, SQueueStatus &queue);
  static bool ParseSignalStatus  (htsmsg_t* msg, SQuality &quality);
  static bool ParseSourceInfo    (htsmsg_t* msg, SSourceInfo &si);
  static void ParseDVREntryUpdate(htsmsg_t* msg, SRecordings &recordings, bool bNotify = false);
  static void ParseDVREntryDelete(htsmsg_t* msg, SRecordings &recordings, bool bNotify = false);

private:
  SOCKET                m_fd;
  unsigned              m_seq;
  void*                 m_challenge;
  int                   m_challenge_len;
  int                   m_protocol;
  int                   m_iRefCount;
  std::string           m_server;
  std::string           m_version;
  bool                  m_connected;
  bool                  m_bSendNotifications;

  std::deque<htsmsg_t*> m_queue;
  const unsigned int    m_queue_size;
};
