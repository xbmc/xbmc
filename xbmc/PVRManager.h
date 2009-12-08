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

#include "utils/Thread.h"
#include "utils/Addon.h"
#include "FileItem.h"
#include "TVDatabase.h"
#include "pvrclients/PVRClient.h"
#include "utils/AddonManager.h"
#include "utils/PVRChannels.h"
#include "utils/PVRRecordings.h"
#include "utils/PVRTimers.h"

#include <vector>
#include <deque>

typedef std::map< long, CPVRClient* >           CLIENTMAP;
typedef std::map< long, CPVRClient* >::iterator CLIENTMAPITR;
typedef std::map< long, PVR_SERVERPROPS >       CLIENTPROPS;

class CPVRTimeshiftRcvr : private CThread
{
public:
  CPVRTimeshiftRcvr();
  ~CPVRTimeshiftRcvr();

  /* Thread handling */
  void Process();
  bool StartReceiver(CPVRClient *client);
  void StopReceiver();
  int WriteBuffer(BYTE* buf, int buf_size);
  __int64 GetMaxSize();
  __int64 GetWritten();
  __int64 TimeToPos(DWORD time, DWORD *timeRet, bool *wrapback);
  DWORD GetDuration();
  DWORD GetTimeTotal();
  const char* GetDurationString();

private:
  typedef struct STimestamp
  {
    __int64 pos;
    DWORD time;
  } STimestamp;

  std::deque<STimestamp>  m_Timestamps;
  CRITICAL_SECTION        m_critSection;
  CPVRClient             *m_client;         // pointer to a enabled client interface
  XFILE::CFile           *m_pFile;          // Stream cache file
  __int64                 m_position;       // Current cache file write position
  __int64                 m_written;        // Total Bytes written to cache file
  __int64                 m_MaxSize;        // Maximum size after cache wraparound
  __int64                 m_MaxSizeStatic;  // The maximum size for cache from settings
  DWORD                   m_Started;
  CStdString              m_DurationStr;
  uint8_t                 buf[32768];       // temporary buffer for client read
};

class CPVRManager : IPVRClientCallback
                  , public ADDON::IAddonCallback
                  , private CThread
{
public:
  CPVRManager();
  ~CPVRManager();

  /*--- Startup functions ---*/
  void Start();
  void Stop();

  /*--- External Client access functions ---*/
  unsigned long GetFirstClientID();
  CLIENTMAP* Clients() { return &m_clients; }
  CTVDatabase *GetTVDatabase() { return &m_database; }

  /*--- Addon related functions ---*/
  bool RequestRestart(const ADDON::IAddon* addon, bool datachanged);
  bool RequestRemoval(const ADDON::IAddon* addon);
  ADDON_STATUS SetSetting(const ADDON::IAddon* addon, const char *settingName, const void *settingValue);
  virtual addon_settings* GetSettings(const ADDON::IAddon*) { return NULL; };
  void OnClientMessage(const long clientID, const PVR_EVENT clientEvent, const char* msg);

  /*--- GUIInfoManager functions ---*/
  void SyncInfo();
  const char* TranslateCharInfo(DWORD dwInfo);
  int TranslateIntInfo(DWORD dwInfo);
  bool TranslateBoolInfo(DWORD dwInfo);

  /*--- General functions ---*/
  bool IsPlayingTV();
  bool IsPlayingRadio();
  bool IsPlayingRecording();
  bool IsTimeshifting();
  PVR_SERVERPROPS *GetCurrentClientProps();
  CFileItem *GetCurrentPlayingItem();
  bool GetCurrentChannel(int *number, bool *radio);
  bool HaveActiveClients();
  int GetPreviousChannel();
  bool CanInstantRecording();
  bool HasTimer() { return m_hasTimers;  }
  bool IsRecording() { return m_isRecording; }
  bool IsRecordingOnPlayingChannel();
  bool StartRecordingOnPlayingChannel(bool bOnOff);
  void SetPlayingGroup(int GroupId);
  int GetPlayingGroup();
  void TriggerRecordingsUpdate(bool force=true);

  /*--- Stream reading functions ---*/
  bool OpenLiveStream(const cPVRChannelInfoTag* tag);
  bool OpenRecordedStream(const cPVRRecordingInfoTag* tag);
  void CloseStream();
  int ReadStream(BYTE* buf, int buf_size);
  bool SeekTimeRequired();
  int SeekTimeStep(bool bPlus, bool bLargeStep, __int64 curTime);
  bool SeekTime(int iTimeInMsec, int *iRetTimeInMsec);
  __int64 SeekStream(__int64 pos, int whence=SEEK_SET);
  __int64 LengthStream(void);
  bool UpdateItem(CFileItem& item);
  bool ChannelSwitch(unsigned int channel);
  bool ChannelUp(unsigned int *newchannel);
  bool ChannelDown(unsigned int *newchannel);
  int GetTotalTime();
  int GetStartTime();

protected:
  virtual void Process();

private:
  /*--- Handling functions ---*/
  bool LoadClients();
  void GetClientProperties();                   /* call GetClientProperties(long clientID) for each client connected */
  void GetClientProperties(long clientID);      /* request the PVR_SERVERPROPS struct from each client */
  void SaveCurrentChannelSettings();
  void LoadCurrentChannelSettings();
  void ResetQualityData();

  /*--- General PVRManager data ---*/
  CLIENTMAP           m_clients;                /* pointer to each enabled client's interface */
  CLIENTPROPS         m_clientsProps;           /* store the properties of each client locally */
  CTVDatabase         m_database;
  CRITICAL_SECTION    m_critSection;

  /*--- GUIInfoManager information data ---*/
  DWORD               m_infoToggleStart;        /* Time to toogle pvr infos like in System info */
  unsigned int        m_infoToggleCurrent;      /* The current item showed by the GUIInfoManager */

  CStdString          m_nextRecordingDateTime;
  CStdString          m_nextRecordingChannel;
  CStdString          m_nextRecordingTitle;
  CStdString          m_nowRecordingDateTime;
  CStdString          m_nowRecordingChannel;
  CStdString          m_nowRecordingTitle;
  CStdString          m_backendName;
  CStdString          m_backendVersion;
  CStdString          m_backendHost;
  CStdString          m_backendDiskspace;
  CStdString          m_backendTimers;
  CStdString          m_backendRecordings;
  CStdString          m_backendChannels;
  CStdString          m_totalDiskspace;
  CStdString          m_nextTimer;
  CStdString          m_playingDuration;
  CStdString          m_playingTime;
  CStdString          m_timeshiftTime;
  CStdString          m_playingClientName;
  bool                m_isRecording;
  bool                m_hasRecordings;
  bool                m_hasTimers;

  /*--- Thread Update Timers ---*/
  int                 m_LastTVChannelCheck;
  int                 m_LastRadioChannelCheck;
  int                 m_LastRecordingsCheck;

  /*--- Previous Channel data ---*/
  int                 m_PreviousChannel[2];
  int                 m_PreviousChannelIndex;

  /*--- Stream playback data ---*/
  CFileItem          *m_currentPlayingChannel;    /* The current playing channel or NULL */
  CFileItem          *m_currentPlayingRecording;  /* The current playing recording or NULL */
  int                 m_CurrentGroupID;           /* The current selected Channel group list */
  DWORD               m_scanStart;                /* Scan start time to check for non present streams */
  PVR_SIGNALQUALITY   m_qualityInfo;              /* Stream quality information */

  /*--- Timeshift data ---*/
  bool                CreateInternalTimeshift();
  bool                m_timeshiftExt;             /* True if external Timeshift is possible and active */
  bool                m_timeshiftInt;             /* True if internal Timeshift is possible and active */
  DWORD               m_playbackStarted;          /* Time where the playback was started */
  XFILE::CFile       *m_pTimeshiftFile;           /* File class to read buffer file */
  CPVRTimeshiftRcvr  *m_TimeshiftReceiver;        /* The Thread based Receiver to fill buffer file */
  __int64             m_timeshiftCurrWrapAround;  /* Bytes readed during current wrap around */
  __int64             m_timeshiftLastWrapAround;  /* Bytes readed during last wrap around */
};

extern CPVRManager g_PVRManager;
