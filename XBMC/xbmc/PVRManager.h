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
#include "pvrclients/IPVRClient.h"
#include "utils/AddonManager.h"
#include "utils/PVRChannels.h"
#include "utils/PVRRecordings.h"
#include "utils/PVRTimers.h"

#include <vector>
#include <deque>

typedef std::map< long, IPVRClient* >           CLIENTMAP;
typedef std::map< long, IPVRClient* >::iterator CLIENTMAPITR;
typedef std::map< long, PVR_SERVERPROPS >       CLIENTPROPS;

class CPVRTimeshiftRcvr : private CThread
{
public:
  CPVRTimeshiftRcvr();
  ~CPVRTimeshiftRcvr();

  /* Thread handling */
  void Process();
  void SetClient(IPVRClient *client);
  bool StartReceiver(IPVRClient *client);
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
  IPVRClient             *m_client;         // pointer to a enabled client interface
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

  void Start();
  void Stop();
  bool LoadClients();
  void GetClientProperties(); // call GetClientProperties(long clientID) for each client connected
  void GetClientProperties(long clientID); // request the PVR_SERVERPROPS struct from each client

  /* Synchronize Thread */
  virtual void Process();

  /* Manager access */
  static void RemoveInstance();
  static void ReleaseInstance();
  static bool IsInstantiated() { return m_instance != NULL; }

  static CPVRManager* GetInstance();
  unsigned long GetFirstClientID();
  static CLIENTMAP* Clients() { return &m_clients; }
  CTVDatabase *GetTVDatabase() { return &m_database; }

  /* addon specific */
  bool RequestRestart(const ADDON::CAddon* addon, bool datachanged);
  bool RequestRemoval(const ADDON::CAddon* addon);
  ADDON_STATUS SetSetting(const ADDON::CAddon* addon, const char *settingName, const void *settingValue);



  /* Event handling */
  void	      OnClientMessage(const long clientID, const PVR_EVENT clientEvent, const char* msg);
  const char* TranslateInfo(DWORD dwInfo);
  static bool HasTimer() { return m_hasTimers;  }
  static bool IsRecording() { return m_isRecording; }
  bool        IsRecording(unsigned int channel, bool radio = false);
  static bool IsPlayingTV();
  static bool IsPlayingRadio();

  int GetGroupList(CFileItemList* results);
  void AddGroup(const CStdString &newname);
  bool RenameGroup(unsigned int GroupId, const CStdString &newname);
  bool DeleteGroup(unsigned int GroupId);
  bool ChannelToGroup(unsigned int number, unsigned int GroupId, bool radio = false);
  int GetPrevGroupID(int current_group_id);
  int GetNextGroupID(int current_group_id);
  CStdString GetGroupName(int GroupId);
  int GetFirstChannelForGroupID(int GroupId, bool radio = false);

  /* Backend Channel handling */
  bool AddBackendChannel(const CFileItem &item);
  bool DeleteBackendChannel(unsigned int index);
  bool RenameBackendChannel(unsigned int index, CStdString &newname);
  bool MoveBackendChannel(unsigned int index, unsigned int newindex);
  bool UpdateBackendChannel(const CFileItem &item);

  /* Live stream handling */
    bool PauseLiveStream(bool DoPause, double dTime);
  int GetTotalTime();
  int GetStartTime();
  void SetPlayingGroup(int GroupId);
  int GetPlayingGroup();

  /* Recorded stream handling */
  bool RecordChannel(unsigned int channel, bool bOnOff, bool radio = false);

  void                SetCurrentPlayingProgram(CFileItem& item);
  void                SyncInfo(); // synchronize InfoManager related stuff









  /* General functions */
  PVR_SERVERPROPS *GetCurrentClientProps();
  CFileItem *GetCurrentPlayingItem();
  bool GetCurrentChannel(int *number, bool *radio);
  bool HaveActiveClients();
  int GetPreviousChannel();

  /* Stream reading functions */
  bool OpenLiveStream(unsigned int channel, bool radio = false);
  bool OpenRecordedStream(unsigned int recording);
  void CloseStream();
  int ReadStream(BYTE* buf, int buf_size);
  __int64 SeekStream(__int64 pos, int whence=SEEK_SET);
  __int64 LengthStream(void);
  bool UpdateItem(CFileItem& item);
  bool ChannelSwitch(unsigned int channel);
  bool ChannelUp(unsigned int *newchannel);
  bool ChannelDown(unsigned int *newchannel);

  /* Stream demux functions */
  bool OpenDemux(PVRDEMUXHANDLE handle);
  void DisposeDemux();
  void ResetDemux();
  void FlushDemux();
  void AbortDemux();
  void SetDemuxSpeed(int iSpeed);
  demux_packet_t* ReadDemux();
  bool SeekDemuxTime(int time, bool backwords, double* startpts);
  int GetDemuxStreamLength();

protected:

private:
  static CPVRManager   *m_instance;
  static CLIENTMAP      m_clients; // pointer to each enabled client's interface
  CLIENTPROPS           m_clientsProps; // store the properties of each client locally
  DWORD                 m_infoToggleStart;
  unsigned int          m_infoToggleCurrent;
  CTVDatabase           m_database;

  static bool         m_isRecording;
  static bool         m_hasRecordings;
  static bool         m_hasTimers;

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

  int                 m_CurrentChannelID;
  int                 m_CurrentGroupID;

  CHANNELGROUPS_DATA  m_channel_group;

  CRITICAL_SECTION    m_critSection;

  DWORD               m_scanStart;

  static CFileItem   *m_currentPlayingChannel;
  static CFileItem   *m_currentPlayingRecording;
  int                 m_PreviousChannel[2];
  int                 m_PreviousChannelIndex;
  DWORD               m_LastChannelChanged;
  int                 m_LastChannel;
  
  /*--- Timeshift data ---*/
  bool                CreateInternalTimeshift();
  bool                m_timeshiftExt;             /* True if external Timeshift is possible and active */
  bool                m_timeshiftInt;             /* True if internal Timeshift is possible and active */
  DWORD               m_playbackStarted;          /* Time where the playback was started */
  bool                m_bPaused;                  /* True if stream is paused */
  XFILE::CFile       *m_pTimeshiftFile;           /* File class to read buffer file */
  CPVRTimeshiftRcvr  *m_TimeshiftReceiver;        /* The Thread based Receiver to fill buffer file */
  DWORD               m_timeshiftTimePause;       /* Time in ms where the stream was paused */
  int                 m_timeshiftTimeDiff;        /* Difference between current position and true position in sec. */
  __int64             m_timeshiftCurrWrapAround;  /* Bytes readed during current wrap around */
  __int64             m_timeshiftLastWrapAround;  /* Bytes readed during last wrap around */
};
