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
#include "EPG.h"
#include "URL.h"

typedef enum {
  GET_EPG_FOR_CHANNEL = 1
} myth_task_t;

typedef std::queue<std::pair<int, std::string> > myth_event_queue;

class CCMythSession;
class CCriticalSection;
class DllLibCMyth;

class PVRClientMythTv : public IPVRClient
                      , private XFILE::CCMythSession::IEventListener
                      , private CThread
{
public:
  PVRClientMythTv(DWORD sourceID, IPVRClientCallback *callback);
  ~PVRClientMythTv();

  void Release();

  virtual void OnEvent(int event, const std::string& data);

  virtual void Process();
  virtual void OnStartup();
  virtual void OnExit();

  /* server */
  virtual DWORD GetID() { return m_clientID; };
  virtual void SetConnString(CURL connString) { m_connString = connString; };
  virtual void Connect();
  virtual PVRCLIENT_PROPS GetProperties();
  virtual bool IsUp();
  virtual bool GetDriveSpace(long long *total, long long *used);

  /* bouquets */
  // NB: no bouquet support in mythTV
  virtual int GetBouquetForChannel(char* chanName) { return 0; };
  virtual const char* GetBouquetName(int bouquetID)   { return "mythTV"; };
  virtual const char* GetBouquetIcon(int bouquetID)   { return ""; };
  virtual const char* GetBouquetGenre(int bouquetID)  { return ""; };

  /* channels */
  virtual int  GetNumChannels();
  virtual int  GetChannelList(PVRCLIENT_CHANNEL* chanList);

  virtual bool GetEPGDataEnd(CDateTime &end);
  virtual void GetEPGForChannel(int bouquet, int channel);

  /* scheduled recordings */
  virtual bool GetTimers(CFileItemList* results);
  virtual bool GetRecordingSchedules(CFileItemList* results);
  virtual bool GetConflicting(CFileItemList* results);

  /* recordings completed/started */
  virtual bool GetRecordings(CFileItemList* results);

  /* individual programme operations */

private:
  CStdString GetValue(char* str)           { return m_session->GetValue(str); };
  int        GetValue(int integer)         { return m_session->GetValue(integer); };
  CDateTime  GetValue(cmyth_timestamp_t t) { return m_session->GetValue(t); };

  void       AddTask(myth_task_t task, std::string &data);
  bool       UpdateRecording(CFileItem &item, cmyth_proginfo_t info);
  int        GetRecordingStatus(cmyth_proginfo_t prog);
  PVRCLIENT_CHANNEL GetXBMCChannel(cmyth_channel_t channel);
  CEPGInfoTag       FillProgrammeTag(cmyth_proginfo_t programme);

  // myth sessions helpers
  bool       GetControl();
  bool       GetLibrary();
  bool       GetDB();

  void GetEPGForChannelTask(CStdString chan);

  bool m_isRunning;
  DWORD m_clientID;
  IPVRClientCallback*   m_manager;

  XFILE::CCMythSession* m_session;
  DllLibCMyth*          m_dll;
  cmyth_conn_t         m_control;
  CURL                  m_connString;
  cmyth_database_t      m_database;
  cmyth_recorder_t      m_recorder;
  cmyth_proginfo_t      m_program;

  static XFILE::CCMythSession* m_mythEventSession;

  static myth_event_queue   m_thingsToDo;
  static CCriticalSection   m_thingsToDoSection;
};
