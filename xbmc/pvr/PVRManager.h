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

#include "FileItem.h"
#include "PVRDatabase.h"
#include "addons/Addon.h"
#include "addons/PVRClient.h"
#include "addons/AddonManager.h"
#include "threads/Thread.h"

#include <vector>
#include <deque>

class CPVRChannelGroup;
class CPVRRecordings;
class CPVRTimers;

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

  /*! \name Startup functions
   */
  void Start();
  void Stop();
  void Restart(void);

  /*! \name External Client access functions
   */
  unsigned int GetFirstClientID();
  CLIENTMAP* Clients() { return &m_clients; }
  CPVRDatabase *GetTVDatabase() { return &m_database; }

  /*! \name Addon related functions
   */
  bool RequestRestart(ADDON::AddonPtr addon, bool datachanged);
  bool RequestRemoval(ADDON::AddonPtr addon);
  void OnClientMessage(const int clientID, const PVR_EVENT clientEvent, const char* msg);

  /*! \name GUIInfoManager functions
   */
  void UpdateRecordingsCache();
  const char* TranslateCharInfo(DWORD dwInfo);
  int TranslateIntInfo(DWORD dwInfo);
  bool TranslateBoolInfo(DWORD dwInfo);

  /*! \name General functions
   */

  /*! \brief Open a selection Dialog and start a channelscan on
   selected Client.
   */
  void StartChannelScan();

  /*! \brief Get info about a running channel scan
   \return true if scan is running
   */
  bool ChannelScanRunning() { return m_bChannelScanRunning; }

  /*! \brief Set the TV Database to it's initial state and delete all
   the data inside.
   */
  void ResetDatabase();

  /*! \brief Set the EPG data inside TV Database to it's initial state
   and reload it from Clients.
   */
  void ResetEPG();

  /*! \brief Returns true if a tv channel is playing
   \return true during TV playback
   */
  bool IsPlayingTV();

  /*! \brief Returns true if a radio channel is playing
   \return true during radio playback
   */
  bool IsPlayingRadio();

  /*! \brief Returns true if a recording is playing over the client
   \return true during recording playback
   */
  bool IsPlayingRecording();

  /*! \brief Returns the properties of the current playing client
   \return pointer to properties (NULL if no stream is playing)
   */
  PVR_SERVERPROPS *GetCurrentClientProps();

  /*! \brief Return the current playing client identifier
   \return the identifier or -1 if no playing item ist present
   */
  int GetCurrentPlayingClientID();

  /*! \brief Returns the properties of the given client identifier
   \param clientID The identifier of the client
   \return pointer to properties (NULL if no stream is playing)
   */
  PVR_SERVERPROPS *GetClientProps(int clientID) { return &m_clientsProps[clientID]; }

  /*! \brief Returns the properties of the current playing stream content
   \return pointer to properties (NULL if no stream is playing)
   */
  PVR_STREAMPROPS *GetCurrentStreamProps();

  /*! \brief Returns the current playing file item
   \return pointer to file item class (NULL if no stream is playing)
   */
  CFileItem *GetCurrentPlayingItem();

  /*! \brief Get the input format name of the current playing channel
   \return the name of the input format or empty if unknown
   */
  CStdString GetCurrentInputFormat();

  /*! \brief Returns the current playing channel number
   \param number Address to integer value to write playing channel number
   \param radio Address to boolean value to set it true if it is a radio channel
   \return true if channel is playing
   */
  bool GetCurrentChannel(int *number, bool *radio);

  bool GetCurrentChannel(const CPVRChannel *channel);

   /*! \brief Returns if a minimum one client is active
   \return true if minimum one client is started
   */
  bool HaveActiveClients();

   /*! \brief Returns the presence of PVR specific Menu entries
   \param clientID identifier of the client to ask or < 0 for playing channel
   \return true if menu hooks are present
   */
  bool HaveMenuHooks(int clientID);

   /*! \brief Open selection and progress pvr actions
   \param clientID identifier to process
   */
  void ProcessMenuHooks(int clientID);

  /*! \brief Returns the previous selected channel
   \return the number of the previous channel or -1 if no channel was selected before
   */
  int GetPreviousChannel();

  /*! \brief Get the possibility to start a recording of the current playing
   channel.
   \return true if a recording can be started
   */
  bool CanRecordInstantly();

  /*! \brief Get the presence of timers.
   \return true if timers are present
   */
  bool HasTimer() { return m_hasTimers; }

  /*! \brief Get the presence of a running recording.
   \return true if a recording is running
   */
  bool IsRecording() { return m_isRecording; }

  /*! \brief Get the presence of a running recording on current playing channel.
   \return true if a recording is running
   */
  bool IsRecordingOnPlayingChannel();

  /*! \brief Start a instant recording on playing channel
   \return true if it success
   */
  bool StartRecordingOnPlayingChannel(bool bOnOff);

  /*! \brief Set the current playing group ID, used to load the right channel
   lists.
   */
  void SetPlayingGroup(int GroupId);

  /*! \brief Get the current playing group ID, used to load the
    right channel lists
   \return current playing group identifier
   */
  int GetPlayingGroup();

  /*! \brief Trigger a recordings list update
   */
  void TriggerRecordingsUpdate(bool force=true);

  /*! \brief Trigger a timer list update
   */
  void TriggerTimersUpdate(bool force=true);


  /*! \name Stream reading functions
   PVR Client internal input stream access, is used if
   inside PVR_SERVERPROPS the HandleInputStream is true
   */

  /*! \brief Open the Channel stream on the given channel info tag
   \return true if opening was succesfull
   */
  bool OpenLiveStream(const CPVRChannel* tag);

  /*! \brief Open a recording by a index number passed to this function.
   \return true if opening was succesfull
   */
  bool OpenRecordedStream(const CPVRRecordingInfoTag* tag);

  /*! \brief Returns runtime generated stream URL
   Returns a during runtime generated stream URL from the PVR Client.
   Backends like Mediaportal generates the URL for the RTSP streams
   during opening.
   \return Stream URL
   */
  CStdString GetLiveStreamURL(const CPVRChannel* tag);

  /*! \brief Close the stream on the PVR Client.
   */
  void CloseStream();

  /*! \brief Read the stream
   Read the stream to the buffer pointer passed defined by "buf" and
   a maximum site passed with "buf_size".
   \return the amount of readed bytes is returned
   */
  int ReadStream(void* lpBuf, int64_t uiBufSize);

  /*! \brief Reset the client demuxer
   */
  void DemuxReset();

  /*! \brief Aborts any internal reading that might be stalling main thread
   * NOTICE - this can be called from another thread
   */
  void DemuxAbort();

  /*! \brief Flush the demuxer, if any data is kept in buffers, this should be freed now
   */
  void DemuxFlush();

  /*! \brief Read the stream from demuxer
   Read the stream from pvr client own demuxer
   \return a allocated demuxer packet
   */
  DemuxPacket* ReadDemuxStream();

  /*! \brief Return the filesize of the current running stream.
   Limited to recordings playback at the moment.
   \return the size of the actual running stream
   */
  int64_t LengthStream(void);

  /*! \brief Seek to a position inside stream
   It is currently limited to playback of recordings, no live tv seek is possible.
   \param pos the position to seek to
   \param whence how we want to seek ("new position=pos", or "new position=pos+actual postion" or "new position=filesize-pos
   \return the new position inside stream
   */
  int64_t SeekStream(int64_t iFilePosition, int iWhence = SEEK_SET);

  /*! \brief Get the current playing position in stream
   \return the current position inside stream
   */
  int64_t GetStreamPosition(void);

  /*! \brief Update the current running channel
   It is called during a channel  change to refresh the global file item.
   \param item the file item with the current running item
   \return true if success
   */
  bool UpdateItem(CFileItem& item);

  /*! \brief Switch to a channel by the given number
   Used only for live TV channels
   \param channel the channel number to switch
   \return true if success
   */
  bool ChannelSwitch(unsigned int channel);

  /*! \brief Switch to the next channel in list
   It switch to the next channel and return the new channel to
   the pointer passed by this function, if preview is true no client
   channel switch is performed, only the data of the new channel is
   loaded, is used to show new event information in fast channel zapping.
   \param newchannel pointer to store the new selected channel number
   \param preview if set the channel is only switched inside XBMC without client action
   \return true if success
   */
  bool ChannelUp(unsigned int *newchannel, bool preview = false);

  /*! \brief Switch the to previous channel in list
   It switch to the previous channel and return the new channel to
   the pointer passed by this function, if preview is true no client
   channel switch is performed, only the data of the new channel is
   loaded, is used to show new event information in fast channel zapping.
   \param newchannel pointer to store the new selected channel number
   \param preview if set the channel is only switched inside XBMC without client action
   \return true if success
   */
  bool ChannelDown(unsigned int *newchannel, bool preview = false);

  /*! \brief Returns the duration of the current playing channel
   Used only for live TV channels
   \return duration in milliseconds or NULL if no channel is playing.
   */
  int GetTotalTime();

  /*! \brief Returns the current position from 0 in milliseconds
   Used only for live TV channels
   \return position in milliseconds or NULL if no channel is playing.
   */
  int GetStartTime();

protected:
  virtual void Process();

private:
  /*--- Handling functions ---*/
  bool LoadClients();
  void GetClientProperties();                   /*! \brief call GetClientProperties(long clientID) for each client connected */
  void GetClientProperties(int clientID);      /*! \brief request the PVR_SERVERPROPS struct from each client */
  void SaveCurrentChannelSettings();            /*! \brief Write the current Video and Audio settings of
                                                 playing channel to the TV Database */
  void LoadCurrentChannelSettings();            /*! \brief Read and set the Video and Audio settings of
                                                 playing channel from the TV Database */
  void ResetQualityData();                      /*! \brief Reset the Signal Quality data structure to initial values */
  bool ContinueLastChannel();
  void Reset(void);

  /*--- General PVRManager data ---*/
  CLIENTMAP           m_clients;                /* pointer to each enabled client's interface */
  CLIENTPROPS         m_clientsProps;           /* store the properties of each client locally */
  STREAMPROPS         m_streamProps;
  CPVRDatabase        m_database;
  CCriticalSection    m_critSection;
  bool                m_bFirstStart;            /* Is set if this is first startup of PVRManager */
  bool                m_bChannelScanRunning;    /* Is set if a channel scan is running */

  /*--- GUIInfoManager information data ---*/
  DWORD               m_infoToggleStart;        /* Time to toogle pvr infos like in System info */
  unsigned int        m_infoToggleCurrent;      /* The current item showed by the GUIInfoManager */
  DWORD               m_recordingToggleStart;   /* Time to toogle currently running pvr recordings */
  unsigned int        m_recordingToggleCurrent; /* The current item showed by the GUIInfoManager */

  std::vector<CPVRTimerInfoTag *> m_NowRecording;
  CPVRTimerInfoTag *  m_NextRecording;
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
  int                 m_LastChannelCheck;
  int                 m_LastRecordingsCheck;
  int                 m_LastTimersCheck;
  int                 m_LastEPGUpdate;
  int                 m_LastEPGScan;

  /*--- Previous Channel data ---*/
  int                 m_PreviousChannel[2];
  int                 m_PreviousChannelIndex;
  int                 m_LastChannel;
  unsigned int        m_LastChannelChanged;

  /*--- Stream playback data ---*/
  CFileItem          *m_currentPlayingChannel;    /* The current playing channel or NULL */
  CFileItem          *m_currentPlayingRecording;  /* The current playing recording or NULL */
  int                 m_CurrentGroupID;           /* The current selected Channel group list */
  DWORD               m_scanStart;                /* Scan start time to check for non present streams */
  PVR_SIGNALQUALITY   m_qualityInfo;              /* Stream quality information */
};

extern CPVRManager g_PVRManager;
