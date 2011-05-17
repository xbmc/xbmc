#pragma once
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

#include "VNSISession.h"
#include "thread.h"
#include "client.h"

#include <string>
#include <map>

class cResponsePacket;
class cRequestPacket;

class cVNSIData : public cVNSISession, public cThread
{
public:
  cVNSIData();
  virtual ~cVNSIData();

  bool Open(const std::string& hostname, int port, const char* name = NULL);
  bool Login();
  void Abort();

  cResponsePacket*  ReadResult(cRequestPacket* vrp);

  bool        SupportChannelScan();
  bool        EnableStatusInterface(bool onOff);
  bool        GetDriveSpace(long long *total, long long *used);

  int         GetChannelsCount();
  bool        GetChannelsList(PVR_HANDLE handle, bool radio = false);
  bool        GetEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end);

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


protected:

  virtual void Action(void);

  virtual bool onResponsePacket(cResponsePacket *pkt);

  void SignalConnectionLost();
  void OnDisconnect();
  void OnReconnect();

  bool SendPing();

private:

  struct SMessage
  {
    cCondWait       *event;
    cResponsePacket *pkt;
  };
  typedef std::map<int, SMessage> SMessages;

  //cVNSISession    m_session;
  cMutex          m_Mutex;
  SMessages       m_queue;
  std::string     m_videodir;
  bool            m_aborting;
};
