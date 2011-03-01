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
#include "windows/GUIWindowPVRCommon.h"

#include <vector>
#include <deque>

class CPVRChannelGroupsContainer;
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
private:
  /*!
   * @brief Create a new CPVRManager instance, which handles all PVR related operations in XBMC.
   */
  CPVRManager(void);

public:
  /*!
   * @brief Stop the PVRManager and destroy all objects it created.
   */
  virtual ~CPVRManager(void);

  /*!
   * @brief Get the instance of the PVRManager.
   * @return The PVRManager instance.
   */
  static CPVRManager *Get(void);

  /*!
   * @brief Get the channel groups container.
   * @return The groups container.
   */
  static CPVRChannelGroupsContainer *GetChannelGroups(void);

  /*!
   * @brief Get the EPG container.
   * @return The EPG container.
   */
  static CPVREpgContainer *GetEpg(void);

  /*!
   * @brief Get the recordings container.
   * @return The recordings container.
   */
  static CPVRRecordings *GetRecordings(void);

  /*!
   * @brief Get the timers container.
   * @return The timers container.
   */
  static CPVRTimers *GetTimers(void);

  /*!
   * @brief Clean up and destroy the PVRManager.
   */
  static void Destroy(void);

  /** @name Startup function */
  //@{

  /*!
   * @brief Start the PVRManager
   */
  void Start(void);

private:

  /*!
   * @brief Stop the PVRManager and destroy all objects it created.
   */
  void Stop(void);
  //@}

public:

  /** @name External client access functions */
  //@{

  /*!
   * @brief Get the ID of the first client.
   * @return The ID of the first client.
   */
  unsigned int GetFirstClientID(void);

  /*!
   * @brief Get a pointer to the clients.
   * @return The clients.
   */
  CLIENTMAP *Clients(void) { return &m_clients; }

  /*!
   * @brief Get the TV database.
   * @return The TV database.
   */
  CPVRDatabase *GetTVDatabase(void) { return &m_database; }

  //@}

  /** @name Add-on functions */
  //@{

  /*!
   * @brief Restart a single client add-on.
   * @param addon The add-on to restart.
   * @param bDataChanged True if the client's data changed, false otherwise (unused).
   * @return True if the client was found and restarted, false otherwise.
   */
  bool RequestRestart(ADDON::AddonPtr addon, bool bDataChanged);

  /*!
   * @brief Remove a single client add-on.
   * @param addon The add-on to remove.
   * @return True if the client was found and removed, false otherwise.
   */
  bool RequestRemoval(ADDON::AddonPtr addon);

  /*!
   * @brief Callback function from client to inform about changed timers, channels, recordings or epg.
   * @param clientID The ID of the client that sends an update.
   * @param clientEvent The event that just happened.
   * @param strMessage The passed message.
   */
  void OnClientMessage(const int iClientId, const PVR_EVENT clientEvent, const char* strMessage);

  //@}

  /** @name GUIInfoManager functions */
  //@{

  /*!
   * @brief Updates the recordings and the "now" and "next" timers.
   */
  void UpdateRecordingsCache(void);

  /*!
   * @brief Get a GUIInfoManager character string.
   * @param dwInfo The string to get.
   * @return The requested string or an empty one if it wasn't found.
   */
  const char* TranslateCharInfo(DWORD dwInfo);

  /*!
   * @brief Get a GUIInfoManager integer.
   * @param dwInfo The integer to get.
   * @return The requested integer or 0 if it wasn't found.
   */
  int TranslateIntInfo(DWORD dwInfo);

  /*!
   * @brief Get a GUIInfoManager boolean.
   * @param dwInfo The boolean to get.
   * @return The requested boolean or false if it wasn't found.
   */
  bool TranslateBoolInfo(DWORD dwInfo);

  //@}

  /** @name General functions */
  //@{

  /*!
   * @brief Open a selection dialog and start a channel scan on the selected client.
   */
  void StartChannelScan(void);

  /*!
   * @brief Check whether a channel scan is running.
   * @return True if it's running, false otherwise.
   */
  bool ChannelScanRunning(void) { return m_bChannelScanRunning; }

  /*!
   * @brief Reset the TV database to it's initial state and delete all the data inside.
   * @param bShowProgress True to show a progress bar, false otherwise.
   */
  void ResetDatabase(bool bShowProgress = true);

  /*!
   * @brief Delete all EPG data from the database and reload it from the clients.
   */
  void ResetEPG(void);

  /*!
   * @brief Check if a TV channel is playing.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingTV(void);

  /*!
   * @brief Check if a radio channel is playing.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingRadio(void);

  /*!
   * @brief Check if a recording is playing.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingRecording(void);

  /*!
   * @brief Check if a TV channel, radio channel or recording is playing.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlaying(void);

  /*!
   * @brief Get the properties of the current playing client.
   * @return A pointer to the properties or NULL if no stream is playing.
   */
  PVR_SERVERPROPS *GetCurrentClientProperties(void);

  /*!
   * @brief Get the ID of the client that is currently being used to play.
   * @return The requested ID or -1 if no PVR item is currently being played.
   */
  int GetCurrentPlayingClientID(void);

  /*!
   * @brief Get the properties for a specific client.
   * @param clientID The ID of the client.
   * @return A pointer to the properties or NULL if no stream is playing.
   */
  PVR_SERVERPROPS *GetClientProperties(int iClientId) { return &m_clientsProps[iClientId]; }

  /*!
   * @brief Get the properties of the current playing stream content.
   * @return A pointer to the properties or NULL if no stream is playing.
   */
  PVR_STREAMPROPS *GetCurrentStreamProperties(void);

  /*!
   * @brief Get the file that is currently being played.
   * @return A pointer to the file or NULL if no stream is being played.
   */
  CFileItem *GetCurrentPlayingItem(void);

  /*!
   * @brief Get the input format name of the current playing stream content.
   * @return A pointer to the properties or NULL if no stream is playing.
   */
  CStdString GetCurrentInputFormat(void);

  /*!
   * @brief Return the channel that is currently playing.
   * @param channel The channel or NULL if none is playing.
   * @return True if a channel is playing, false otherwise.
   */
  bool GetCurrentChannel(const CPVRChannel *channel);

  /*!
   * @brief Check whether the PVRManager has fully started.
   * @return True if started, false otherwise.
   */
  bool IsStarted(void) const { return m_bLoaded; }

  /*!
   * @brief Check whether there are any active clients.
   * @return True if at least one client is active.
   */
  bool HasActiveClients(void);

  /*!
   * @brief Check whether a client has any PVR specific menu entries.
   * @param iClientId The ID of the client to get the menu entries for. Get the menu for the active channel if iClientId < 0.
   * @return True if the client has any menu hooks, false otherwise.
   */
  bool HasMenuHooks(int iClientId);

  /*!
   * @brief Open selection and progress PVR actions.
   * @param iClientId The ID of the client to process the menu entries for. Process the menu entries for the active channel if iClientId < 0.
   */
  void ProcessMenuHooks(int iClientID);

  /*!
   * @brief Get the channel number of the previously selected channel.
   * @return The requested channel number or -1 if it wasn't found.
   */
  int GetPreviousChannel();

  /*!
   * @brief Check whether the current channel can be recorded instantly.
   * @return True if it can, false otherwise.
   */
  bool CanRecordInstantly();

  /*!
   * @brief Check whether there are active timers.
   * @return True if there are active timers, false otherwise.
   */
  bool HasTimer() { return m_hasTimers; }

  /*!
   * @brief Check whether there are active recordings.
   * @return True if there are active recordings, false otherwise.
   */
  bool IsRecording() { return m_isRecording; }

  /*!
   * @brief Check whether there is an active recording on the current channel.
   * @return True if there is, false otherwise.
   */
  bool IsRecordingOnPlayingChannel();

  /*!
   * @brief Start an instant recording on the current channel.
   * @param bOnOff Activate the recording if true, deactivate if false.
   * @return True if the recording was started, false otherwise.
   */
  bool StartRecordingOnPlayingChannel(bool bOnOff);

  /*!
   * @brief Set the current playing group, used to load the right channel.
   * @param group The new group.
   */
  void SetPlayingGroup(const CPVRChannelGroup *group) { m_currentGroup = group; }

  /*!
   * @brief Get the current playing group, used to load the right channel.
   * @return The current group or the group containing all channels if it's not set.
   */
  const CPVRChannelGroup *GetPlayingGroup();

  /*!
   * @brief Let the background thread update the recordings list.
   */
  void TriggerRecordingsUpdate(void);

  /*!
   * @brief Let the background thread update the timer list.
   */
  void TriggerTimersUpdate(void);

  /*!
   * @brief Let the background thread update the channel list.
   */
  void TriggerChannelsUpdate(void);

  //@}

  /** @name Stream reading functions */
  //@{

  /*!
   * @brief Open a stream on the given channel.
   * @param channel The channel to start playing.
   * @return True if the stream was opened successfully, false otherwise.
   */
  bool OpenLiveStream(const CPVRChannel *channel);

  /*!
   * @brief Open a stream from the given recording.
   * @param recording The recording to start playing.
   * @return True if the stream was opened successfully, false otherwise.
   */
  bool OpenRecordedStream(const CPVRRecordingInfoTag *recording);

  /*!
   * @brief Get a stream URL from the PVR Client.
   *
   * Get a stream URL from the PVR Client.
   * Backends like Mediaportal generate the URL to open RTSP streams.
   *
   * @param channel The channel to get the URL for.
   * @return The requested URL or an empty string.
   */
  CStdString GetLiveStreamURL(const CPVRChannel *channel);

  /*!
   * @brief Close a PVR stream.
   */
  void CloseStream();

  /*!
   * @brief Read from an open stream.
   * @param lpBuf Target buffer.
   * @param uiBufSize The size of the buffer.
   * @return The amount of bytes that was added.
   */
  int ReadStream(void* lpBuf, int64_t uiBufSize);

  /*!
   * @brief Reset the demuxer.
   */
  void DemuxReset();

  /*!
   * @brief Abort any internal reading that might be stalling main thread.
   *        NOTICE - this can be called from another thread.
   */
  void DemuxAbort();

  /*!
   * @brief Flush the demuxer. If any data is kept in buffers, this should be freed now.
   */
  void DemuxFlush();

  /*!
   * @brief Read the stream from the demuxer
   * @return An allocated demuxer packet
   */
  DemuxPacket* ReadDemuxStream();

  /*!
   * @brief Return the filesize of the currently running stream.
   *        Limited to recordings playback at the moment.
   * @return The size of the stream.
   */
  int64_t LengthStream(void);

  /*!
   * @brief Seek to a position in a stream.
   *        Limited to recordings playback at the moment.
   * @param iFilePosition The position to seek to.
   * @param iWhence Specify how to seek ("new position=pos", "new position=pos+actual postion" or "new position=filesize-pos")
   * @return The new stream position.
   */
  int64_t SeekStream(int64_t iFilePosition, int iWhence = SEEK_SET);

  /*!
   * @brief Get the currently playing position in a stream.
   * @return The current position.
   */
  int64_t GetStreamPosition(void);

  /*!
   * @brief Update the channel that is currently active.
   * @param item The new channel.
   * @return True if it was updated correctly, false otherwise.
   */
  bool UpdateItem(CFileItem& item);

  /*!
   * @brief Switch to a channel given it's channel number.
   * @param channel The channel number to switch to.
   * @return True if the channel was switched, false otherwise.
   */
  bool ChannelSwitch(unsigned int channel);

  /*!
   * @brief Switch to the next channel in this group.
   * @param newchannel The new channel number after the switch.
   * @param preview If true, don't do the actual switch but just update channel pointers.
   *                Used to display event info while doing "fast channel switching"
   * @return True if the channel was switched, false otherwise.
   */
  bool ChannelUp(unsigned int *newchannel, bool preview = false);

  /*!
   * @brief Switch to the previous channel in this group.
   * @param newchannel The new channel number after the switch.
   * @param preview If true, don't do the actual switch but just update channel pointers.
   *                Used to display event info while doing "fast channel switching"
   * @return True if the channel was switched, false otherwise.
   */
  bool ChannelDown(unsigned int *newchannel, bool preview = false);

  /*!
   * @brief Get the total duration of the currently playing LiveTV item.
   * @return The total duration in milliseconds or NULL if no channel is playing.
   */
  int GetTotalTime();

  /*!
   * @brief Get the current position in milliseconds since the start of a LiveTV item.
   * @return The position in milliseconds or NULL if no channel is playing.
   */
  int GetStartTime();

  /*!
   * @brief Start playback on a channel.
   * @param channel The channel to start to play.
   * @param bPreview If true, open minimised.
   * @return True if playback was started, false otherwise.
   */
  bool StartPlayback(const CPVRChannel *channel, bool bPreview = false);

  /*!
   * @brief Get the currently playing EPG tag and update it if needed.
   * @return The currently playing EPG tag or NULL if there is none.
   */
  const CPVREpgInfoTag *GetPlayingTag(void);

protected:
  /*!
   * @brief PVR update and control thread.
   */
  virtual void Process();

  void UpdateWindow(PVRWindow window);

private:

  const char *CharInfoNowRecordingTitle(void);
  const char *CharInfoNowRecordingChannel(void);
  const char *CharInfoNowRecordingDateTime(void);
  const char *CharInfoBackendNumber(void);
  const char *CharInfoTotalDiskSpace(void);
  const char *CharInfoNextTimer(void);
  const char *CharInfoPlayingDuration(void);
  const char *CharInfoPlayingTime(void);
  const char *CharInfoVideoBR(void);
  const char *CharInfoAudioBR(void);
  const char *CharInfoDolbyBR(void);
  const char *CharInfoSignal(void);
  const char *CharInfoSNR(void);
  const char *CharInfoBER(void);
  const char *CharInfoUNC(void);
  const char *CharInfoFrontendName(void);
  const char *CharInfoFrontendStatus(void);
  const char *CharInfoEncryption(void);

  /*!
   * @brief Reset all properties.
   */
  void ResetProperties(void);

  /*!
   * @brief Try to load and initialise all clients.
   * @param iMaxTime Maximum time to try to load clients in seconds. Use 0 to keep trying until m_bStop becomes true.
   * @return True if all clients were loaded, false otherwise.
   */
  bool TryLoadClients(int iMaxTime = 0);

  /*!
   * @brief Load and initialise all clients.
   * @return True if any clients were loaded, false otherwise.
   */
  bool LoadClients(void);

  /*!
   * @brief Stop a client.
   * @param addon The client to stop.
   * @param bRestart If true, restart the client.
   * @return True if the client was found, false otherwise.
   */
  bool StopClient(ADDON::AddonPtr client, bool bRestart);

  /*!
   * @brief Switch to the given channel.
   * @param channel The new channel.
   * @param bPreview Don't reset quality data if true.
   * @return True if the switch was successful, false otherwise.
   */
  bool PerformChannelSwitch(const CPVRChannel *channel, bool bPreview);

  /*!
   * @brief Called by ChannelUp() and ChannelDown() to perform a channel switch.
   * @param iNewChannelNumber The new channel number after the switch.
   * @param bPreview Preview window if true.
   * @param bUp Go one channel up if true, one channel down if false.
   * @return True if the switch was successful, false otherwise.
   */
  bool ChannelUpDown(unsigned int *iNewChannelNumber, bool bPreview, bool bUp);

  /*!
   * @brief Stop the EPG and PVR threads but do not remove their data.
   */
  void StopThreads(void);

  /*!
   * @brief Restart the EPG and PVR threads after they've been stopped by StopThreads()
   */
  void StartThreads(void);

  /*!
   * @brief Persist the current channel settings in the database.
   */
  void SaveCurrentChannelSettings(void);

  /*!
   * @brief Load the settings for the current channel from the database.
   */
  void LoadCurrentChannelSettings(void);

  /*!
   * @brief Reset the signal quality data to the initial values.
   */
  void ResetQualityData(void);

  /*!
   * @brief Continue playback on the last channel if it was stored in the database.
   * @return True if playback was continued, false otherwise.
   */
  bool ContinueLastChannel(void);

  /*!
   * @brief Clean up all data that was created by the PVRManager.
   */
  void Cleanup(void);

  /*!
   * @brief Update all channels.
   */
  void UpdateChannels(void);

  /*!
   * @brief Update all recordings.
   */
  void UpdateRecordings(void);

  /*!
   * @brief Update all timers.
   */
  void UpdateTimers(void);

  /** @name General PVRManager data */
  //@{
  static CPVRManager *            m_instance;                 /*!< singleton instance */
  CPVRChannelGroupsContainer *    m_channelGroups;            /*!< pointer to the channel groups container */
  CPVREpgContainer *              m_epg;                      /*!< pointer to the EPG container */
  CPVRRecordings *                m_recordings;               /*!< pointer to the recordings container */
  CPVRTimers *                    m_timers;                   /*!< pointer to the timers container */
  CLIENTMAP                       m_clients;                  /*!< pointer to each enabled client */
  CLIENTPROPS                     m_clientsProps;             /*!< store the properties of each client locally */
  STREAMPROPS                     m_streamProps;              /*!< the current stream's properties */
  CPVRDatabase                    m_database;                 /*!< the database for all PVR related data */
  CCriticalSection                m_critSection;              /*!< critical section for all changes to this class */
  bool                            m_bFirstStart;              /*!< true when the PVR manager was started first, false otherwise */
  bool                            m_bLoaded;
  bool                            m_bChannelScanRunning;      /*!< true if a channel scan is currently running, false otherwise */
  bool                            m_bAllClientsLoaded;        /*!< true if all clients are loaded, false otherwise */

  bool                            m_bTriggerChannelsUpdate;   /*!< set to true to let the background thread update the channels list */
  bool                            m_bTriggerRecordingsUpdate; /*!< set to true to let the background thread update the recordings list */
  bool                            m_bTriggerTimersUpdate;     /*!< set to true to let the background thread update the timer list */
  //@}

  /** @name GUIInfoManager data */
  //@{
  DWORD                           m_infoToggleStart;        /* Time to toogle pvr infos like in System info */
  unsigned int                    m_infoToggleCurrent;      /* The current item showed by the GUIInfoManager */
  DWORD                           m_recordingToggleStart;   /* Time to toogle currently running pvr recordings */
  unsigned int                    m_recordingToggleCurrent; /* The current item showed by the GUIInfoManager */

  std::vector<CPVRTimerInfoTag *> m_NowRecording;
  CPVRTimerInfoTag *              m_NextRecording;
  CStdString                      m_backendName;
  CStdString                      m_backendVersion;
  CStdString                      m_backendHost;
  CStdString                      m_backendDiskspace;
  CStdString                      m_backendTimers;
  CStdString                      m_backendRecordings;
  CStdString                      m_backendChannels;
  CStdString                      m_totalDiskspace;
  CStdString                      m_nextTimer;
  CStdString                      m_playingDuration;
  CStdString                      m_playingTime;
  CStdString                      m_playingClientName;
  bool                            m_isRecording;
  bool                            m_hasRecordings;
  bool                            m_hasTimers;
  //@}

  /*--- Previous Channel data ---*/
  int                             m_PreviousChannel[2];
  int                             m_PreviousChannelIndex;
  int                             m_LastChannel;
  unsigned int                    m_LastChannelChanged;

  /*--- Stream playback data ---*/
  CFileItem          *            m_currentPlayingChannel;    /* The current playing channel or NULL */
  CFileItem          *            m_currentPlayingRecording;  /* The current playing recording or NULL */
  const CPVRChannelGroup *        m_currentGroup;             /* The current selected Channel group list */
  DWORD                           m_scanStart;                /* Scan start time to check for non present streams */
  PVR_SIGNALQUALITY               m_qualityInfo;              /* Stream quality information */
};
