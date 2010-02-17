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

typedef std::map< long, boost::shared_ptr<CPVRClient> >           CLIENTMAP;
typedef std::map< long, boost::shared_ptr<CPVRClient> >::iterator CLIENTMAPITR;
typedef std::map< long, PVR_SERVERPROPS >       CLIENTPROPS;
typedef std::map< long, PVR_STREAMPROPS >       STREAMPROPS;

class CPVRManager : IPVRClientCallback
                  , public ADDON::IAddonMgrCallback
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
  void OnClientMessage(const long clientID, const PVR_EVENT clientEvent, const char* msg);

  /*--- GUIInfoManager functions ---*/
  void SyncInfo();
  const char* TranslateCharInfo(DWORD dwInfo);
  int TranslateIntInfo(DWORD dwInfo);
  bool TranslateBoolInfo(DWORD dwInfo);

  /*--- General functions ---*/
  void ResetDatabase();
  void ResetEPG();
  bool IsPlayingTV();
  bool IsPlayingRadio();
  bool IsPlayingRecording();
  PVR_SERVERPROPS *GetCurrentClientProps();
  PVR_STREAMPROPS *GetCurrentStreamProps();
  PVR_SERVERPROPS *GetClientProps(int clientID) { return &m_clientsProps[clientID]; }
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
  CStdString GetLiveStreamURL(const cPVRChannelInfoTag* tag);
  void CloseStream();
  int ReadStream(BYTE* buf, int buf_size);
  bool SeekTimeRequired();
  int SeekTimeStep(bool bPlus, bool bLargeStep, __int64 curTime);
  bool SeekTime(int iTimeInMsec, int *iRetTimeInMsec);
  __int64 SeekStream(__int64 pos, int whence=SEEK_SET);
  __int64 LengthStream(void);
  bool UpdateItem(CFileItem& item);
  bool ChannelSwitch(unsigned int channel);
  bool ChannelUp(unsigned int *newchannel, bool preview = false);
  bool ChannelDown(unsigned int *newchannel, bool preview = false);
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
  STREAMPROPS         m_streamProps;
  CTVDatabase         m_database;
  CRITICAL_SECTION    m_critSection;
  bool                m_bFirstStart;            /* Is set if this is first startup of PVRManager */

  /*--- GUIInfoManager information data ---*/
  DWORD               m_infoToggleStart;        /* Time to toogle pvr infos like in System info */
  unsigned int        m_infoToggleCurrent;      /* The current item showed by the GUIInfoManager */
  DWORD               m_recordingToggleStart;   /* Time to toogle currently running pvr recordings */
  unsigned int        m_recordingToggleCurrent; /* The current item showed by the GUIInfoManager */

  CStdString          m_nextRecordingDateTime;
  CStdString          m_nextRecordingChannel;
  CStdString          m_nextRecordingTitle;
  std::vector<CStdString> m_nowRecordingDateTime;
  std::vector<CStdString> m_nowRecordingChannel;
  std::vector<CStdString> m_nowRecordingTitle;
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
  CStdString          m_playingClientName;
  bool                m_isRecording;
  bool                m_hasRecordings;
  bool                m_hasTimers;

  /*--- Thread Update Timers ---*/
  int                 m_LastTVChannelCheck;
  int                 m_LastRadioChannelCheck;
  int                 m_LastRecordingsCheck;
  int                 m_LastEPGUpdate;
  int                 m_LastEPGScan;

  /*--- Previous Channel data ---*/
  int                 m_PreviousChannel[2];
  int                 m_PreviousChannelIndex;
  int                 m_LastChannel;
  long                m_LastChannelChanged;

  /*--- Stream playback data ---*/
  CFileItem          *m_currentPlayingChannel;    /* The current playing channel or NULL */
  CFileItem          *m_currentPlayingRecording;  /* The current playing recording or NULL */
  int                 m_CurrentGroupID;           /* The current selected Channel group list */
  DWORD               m_scanStart;                /* Scan start time to check for non present streams */
  PVR_SIGNALQUALITY   m_qualityInfo;              /* Stream quality information */
};

extern CPVRManager g_PVRManager;
