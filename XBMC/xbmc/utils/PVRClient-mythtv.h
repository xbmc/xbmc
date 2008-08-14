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

#include "Thread.h"
#include "IPVRClient.h"
#include "../FileSystem/CMythSession.h"
#include "../FileSystem/DllLibCMyth.h"
#include "EPGInfoTag.h"

class CCMythSession;
class CCriticalSection;
class DllLibCMyth;

class PVRClientMythTv : public IPVRClient, IEventListener, CThread
{
public:
  PVRClientMythTv(DWORD sourceID, IPVRClientCallback *callback);
  ~PVRClientMythTv();

  void Release();

  virtual void Process();

  /* server status */
  virtual bool IsUp() { return true; }; ///
  virtual bool GetDriveSpace(long long *total, long long *used);

  /* channels */
  virtual int  GetNumChannels();
  virtual void GetChannelList(CFileItemList &channels);

  virtual bool GetEPGDataEnd(CDateTime &end) { return true; }; ///
  virtual void GetEPGForChannel(int bouquet, int channel, CFileItemList &channelData);

  /* scheduled recordings */
  virtual bool GetRecordingSchedules(CFileItemList &results);
  virtual bool GetUpcomingRecordings(CFileItemList &results);
  virtual bool GetConflicting(CFileItemList &results);

  /* recordings completed/started */
  virtual bool GetAllRecordings(CFileItemList &results);

  /* individual programme operations */

private:
  CStdString GetValue(char* str)           { return m_session->GetValue(str); }
  int        GetValue(int integer)         { return m_session->GetValue(integer); }
  CDateTime  GetValue(cmyth_timestamp_t t) { return m_session->GetValue(t); }
  CEPGInfoTag FillProgrammeTag(cmyth_proginfo_t programme);
  bool       UpdateRecording(CFileItem &item, cmyth_proginfo_t info);
  int        GetRecordingStatus(cmyth_proginfo_t prog);

  bool m_isConnected;
  DWORD m_sourceID;
  IPVRClientCallback*   m_manager;

  XFILE::CCMythSession* m_mythEvents;
  DllLibCMyth*          m_dll;
  cmyth_database_t      m_database;
  cmyth_recorder_t      m_recorder;
  cmyth_proginfo_t      m_program;
};