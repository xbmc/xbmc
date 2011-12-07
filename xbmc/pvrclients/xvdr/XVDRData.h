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

#include "XVDRSession.h"
#include "thread.h"
#include "client.h"

#include <string>
#include <map>

using namespace ADDON;

class cResponsePacket;
class cRequestPacket;

class cXVDRData : public cXVDRSession, public cThread
{
public:

  cXVDRData();
  virtual ~cXVDRData();

  bool        Open(const std::string& hostname, int port, const char* name = NULL);
  bool        Login();
  void        Abort();

  bool        SupportChannelScan();
  bool        EnableStatusInterface(bool onOff);
  bool        SetUpdateChannels(uint8_t method);
  bool        GetDriveSpace(long long *total, long long *used);

  int         GetChannelsCount();
  bool        GetChannelsList(PVR_HANDLE handle, bool radio = false);
  bool        GetEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end);

  int         GetChannelGroupCount(bool automatic);
  bool        GetChannelGroupList(PVR_HANDLE handle, bool bRadio);
  bool        GetChannelGroupMembers(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group);

  bool        GetTimersList(PVR_HANDLE handle);
  int         GetTimersCount();
  PVR_ERROR   AddTimer(const PVR_TIMER &timerinfo);
  PVR_ERROR   GetTimerInfo(unsigned int timernumber, PVR_TIMER &tag);
  PVR_ERROR   DeleteTimer(const PVR_TIMER &timerinfo, bool force = false);
  PVR_ERROR   RenameTimer(const PVR_TIMER &timerinfo, const char *newname);
  PVR_ERROR   UpdateTimer(const PVR_TIMER &timerinfo);

  int         GetRecordingsCount();
  PVR_ERROR   GetRecordingsList(PVR_HANDLE handle);
  PVR_ERROR   RenameRecording(const PVR_RECORDING& recinfo, const char* newname);
  PVR_ERROR   DeleteRecording(const PVR_RECORDING& recinfo);

  cResponsePacket*  ReadResult(cRequestPacket* vrp);

protected:

  virtual void Action(void);
  virtual bool OnResponsePacket(cResponsePacket *pkt);

  void SignalConnectionLost();
  void OnDisconnect();
  void OnReconnect();

private:

  bool SendPing();

  struct SMessage
  {
    cCondWait       *event;
    cResponsePacket *pkt;
  };
  typedef std::map<int, SMessage> SMessages;

  cMutex          m_Mutex;
  SMessages       m_queue;
  std::string     m_videodir;
  bool            m_aborting;
};
