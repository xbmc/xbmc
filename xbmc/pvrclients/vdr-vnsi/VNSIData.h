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

#include "client.h"
#include "thread.h"
#include "VNSISession.h"

class cResponsePacket;
class cRequestPacket;

class cVNSIData : public cThread
{
public:
  cVNSIData();
  ~cVNSIData();

  bool Open(const CStdString& hostname, int port);
  void Close();
  bool CheckConnection();

  cResponsePacket*  ReadResult(cRequestPacket* vrp);
  int         GetProtocol()   { return m_session.GetProtocol(); }
  const CStdString& GetServerName() { return m_session.GetServerName(); }
  const CStdString& GetVersion()    { return m_session.GetVersion(); }
  bool        SupportChannelScan();
  bool        EnableStatusInterface(bool onOff);
  bool        EnableOSDInterface(bool onOff);
  bool        GetTime(time_t *localTime, int *gmtOffset);
  bool        GetDriveSpace(long long *total, long long *used);
  int         GetGroupsCount();
  int         GetChannelsCount();
  bool        GetGroupsList(PVRHANDLE handle, bool radio = false);
  bool        GetChannelsList(PVRHANDLE handle, bool radio = false);
  bool        GetEPGForChannel(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end);
  bool        GetTimersList(PVRHANDLE handle);

  int         GetTimersCount();
  PVR_ERROR   AddTimer(const PVR_TIMERINFO &timerinfo);
  PVR_ERROR   GetTimerInfo(unsigned int timernumber, PVR_TIMERINFO &tag);
  PVR_ERROR   DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force = false);
  PVR_ERROR   RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname);
  PVR_ERROR   UpdateTimer(const PVR_TIMERINFO &timerinfo);

  int         GetRecordingsCount();
  PVR_ERROR   GetRecordingsList(PVRHANDLE handle);
  const CStdString& GetRecordingPath(uint32_t index);
  PVR_ERROR   DeleteRecording(const CStdString& path);


protected:
  virtual void Action(void);

private:
  bool readData(uint8_t* buffer, int totalBytes);

  struct SMessage
  {
    cCondWait       *event;
    cResponsePacket *pkt;
  };
  typedef std::map<int, SMessage> SMessages;
  typedef std::vector<CStdString> RecordPaths;

  cVNSISession    m_session;
  cMutex          m_Mutex;
  SMessages       m_queue;
  RecordPaths     m_RecordsPaths;
  CStdString      m_videodir;
  int             m_recIndex;
};
