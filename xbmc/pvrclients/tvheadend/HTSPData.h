#pragma once

/*
 *      Copyright (C) 2005-2010 Team XBMC
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
#include "HTSPSession.h"

class cHTSPData : public cThread
{
public:
  cHTSPData();
  ~cHTSPData();

  bool Open(CStdString hostname, int port, CStdString user, CStdString pass, long timeout);
  void Close();
  bool CheckConnection();

  htsmsg_t* ReadResult(htsmsg_t* m);
  int GetProtocol()   { return m_session.GetProtocol(); }
  CStdString GetServerName() { return m_session.GetServerName(); }
  CStdString GetVersion()    { return m_session.GetVersion(); }
  bool GetDriveSpace(long long *total, long long *used);
  bool GetTime(time_t *localTime, int *gmtOffset);
  int GetNumChannels();
  int GetNumRecordings();
  PVR_ERROR RequestChannelList(PVRHANDLE handle, int radio);
  PVR_ERROR RequestEPGForChannel(PVRHANDLE handle, const PVR_CHANNEL &channel, time_t start, time_t end);
  PVR_ERROR RequestRecordingsList(PVRHANDLE handle);
  PVR_ERROR DeleteRecording(const PVR_RECORDINGINFO &recinfo);
  PVR_ERROR AddTimer(const PVR_TIMERINFO &timerinfo);
  PVR_ERROR UpdateTimer(const PVR_TIMERINFO &timerinfo);
  PVR_ERROR RenameRecording(const PVR_RECORDINGINFO &recinfo, const char* newname);

  int GetNumTimers();
  PVR_ERROR RequestTimerList(PVRHANDLE handle);
  PVR_ERROR DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force);

protected:
  virtual void Action(void);

private:
  void GetGmtOffset(void);

  struct SMessage
  {
    cCondWait * event;
    htsmsg_t  * msg;
  };
  typedef std::map<int, SMessage> SMessages;

  SChannels GetChannels();
  SChannels GetChannels(int tag);
  SChannels GetChannels(STag &tag);
  STags GetTags();
  bool GetEvent(SEvent& event, uint32_t id);
  SRecordings GetDVREntries(bool recorded, bool scheduled);

  cHTSPSession    m_session;
  cCondWait       m_started;
  cMutex          m_Mutex;
  SChannels       m_channels;
  STags           m_tags;
  SEvents         m_events;
  SMessages       m_queue;
  SRecordings     m_recordings;

  bool            m_bGotGmtOffset;
  int             m_iGmtOffset;
};
