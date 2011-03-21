#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "PVRClient.h"
#include "threads/CriticalSection.h"

#include <vector>
#include <deque>

typedef std::map< long, boost::shared_ptr<CPVRClient> >           CLIENTMAP;
typedef std::map< long, boost::shared_ptr<CPVRClient> >::iterator CLIENTMAPITR;
typedef std::map< long, PVR_SERVERPROPS >                         CLIENTPROPS;
typedef std::map< long, PVR_STREAMPROPS >                         STREAMPROPS;

#define XBMC_VIRTUAL_CLIENTID -1

class CPVRClients : IPVRClientCallback,
                    public ADDON::IAddonMgrCallback
{
public:
  CPVRClients(void);
  virtual ~CPVRClients(void);

  /*!
   * @brief Get the ID of the first client.
   * @return The ID of the first client or -1 if no clients are active;
   */
  int GetFirstID(void);

  /*!
   * @brief Try to load and initialise all clients.
   * @param iMaxTime Maximum time to try to load clients in seconds. Use 0 to keep trying until m_bStop becomes true.
   * @return True if all clients were loaded, false otherwise.
   */
  bool TryLoadClients(int iMaxTime = 0);

  /*!
   * @brief Stop a client.
   * @param addon The client to stop.
   * @param bRestart If true, restart the client.
   * @return True if the client was found, false otherwise.
   */
  bool StopClient(ADDON::AddonPtr client, bool bRestart);

  /*!
   * @brief Open a stream on the given channel.
   * @param tag The channel to start playing.
   * @return True if the stream was opened successfully, false otherwise.
   */
  bool OpenLiveStream(const CPVRChannel &tag);
  bool CloseLiveStream(void);

  /*!
   * @brief Open a stream from the given recording.
   * @param tag The recording to start playing.
   * @return True if the stream was opened successfully, false otherwise.
   */
  bool OpenRecordedStream(const CPVRRecording &tag);
  bool CloseRecordedStream(void);

  /*!
   * @brief Close a PVR stream.
   */
  void CloseStream(void);

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
  void DemuxReset(void);

  /*!
   * @brief Abort any internal reading that might be stalling main thread.
   *        NOTICE - this can be called from another thread.
   */
  void DemuxAbort(void);

  /*!
   * @brief Flush the demuxer. If any data is kept in buffers, this should be freed now.
   */
  void DemuxFlush(void);

  /*!
   * @brief Read the stream from the demuxer
   * @return An allocated demuxer packet
   */
  DemuxPacket* ReadDemuxStream(void);

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

  const CStdString GetClientName(int iClientId);
  const CStdString GetStreamURL(const CPVRChannel &tag);
  int GetSignalLevel(void) const;
  int GetSNR(void) const;
  bool HasClients(void) const;
  bool HasTimerSupport(int iClientId);
  bool IsEncrypted(void) const;
  bool IsPlaying(void) const;
  bool AllClientsLoaded(void) const;
  bool IsReadingLiveStream(void) const;
  bool SwitchChannel(const CPVRChannel &channel);

  int GetTimers(CPVRTimers *timers);
  bool AddTimer(const CPVRTimerInfoTag &timer, PVR_ERROR *error);
  bool UpdateTimer(const CPVRTimerInfoTag &timer, PVR_ERROR *error);
  bool DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce, PVR_ERROR *error);
  bool RenameTimer(const CPVRTimerInfoTag &timer, const CStdString &strNewName, PVR_ERROR *error);

  int GetRecordings(CPVRRecordings *recordings);
  bool RenameRecording(const CPVRRecording &recording, const CStdString &strNewName, PVR_ERROR *error);
  bool DeleteRecording(const CPVRRecording &recording, PVR_ERROR *error);

  bool GetEPGForChannel(const CPVRChannel &channel, CPVREpg *epg, time_t start, time_t end, PVR_ERROR *error);
  int GetChannels(CPVRChannelGroupInternal *group, PVR_ERROR *error);

  int GetClients(std::map<long, CStdString> *clients);

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
   * @brief Check whether the current channel can be recorded instantly.
   * @return True if it can, false otherwise.
   */
  bool CanRecordInstantly(void);

  /*!
   * @brief Check whether there are any active clients.
   * @return True if at least one client is active.
   */
  bool HasActiveClients(void);

  /*!
   * @brief Callback function from client to inform about changed timers, channels, recordings or epg.
   * @param clientID The ID of the client that sends an update.
   * @param clientEvent The event that just happened.
   * @param strMessage The passed message.
   */
  void OnClientMessage(const int iClientId, const PVR_EVENT clientEvent, const char* strMessage);

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
   * @brief Check if a TV channel is playing.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingTV(void) const;

  /*!
   * @brief Check if a radio channel is playing.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingRadio(void) const;

  /*!
   * @brief Check if a recording is playing.
   * @return True if it's playing, false otherwise.
   */
  bool IsPlayingRecording(void) const;

  bool IsRunningChannelScan(void) const;

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
   * @brief Get the input format name of the current playing stream content.
   * @return A pointer to the properties or NULL if no stream is playing.
   */
  CStdString GetCurrentInputFormat(void) const;

  /*!
   * @brief Start an instant recording on the current channel.
   * @param bOnOff Activate the recording if true, deactivate if false.
   * @return True if the recording was started, false otherwise.
   */
  bool StartRecordingOnPlayingChannel(bool bOnOff);

  /*!
   * @brief Check whether there is an active recording on the current channel.
   * @return True if there is, false otherwise.
   */
  bool IsRecordingOnPlayingChannel(void) const;

  void Unload(void);

  const char *CharInfoVideoBR(void) const;
  const char *CharInfoAudioBR(void) const;
  const char *CharInfoDolbyBR(void) const;
  const char *CharInfoSignal(void) const;
  const char *CharInfoSNR(void) const;
  const char *CharInfoBER(void) const;
  const char *CharInfoUNC(void) const;
  const char *CharInfoFrontendName(void) const;
  const char *CharInfoFrontendStatus(void) const;
  const char *CharInfoBackendName(void) const;
  const char *CharInfoBackendNumber(void);
  const char *CharInfoTotalDiskSpace(void);
  const char *CharInfoEncryption(void) const;
  const char *CharInfoBackendVersion(void) const;
  const char *CharInfoBackendHost(void) const;
  const char *CharInfoBackendDiskspace(void) const;
  const char *CharInfoBackendChannels(void) const;
  const char *CharInfoBackendTimers(void) const;
  const char *CharInfoBackendRecordings(void) const;
  const char *CharInfoPlayingClientName(void) const;

  bool GetPlayingChannel(CPVRChannel *channel) const;
  bool GetPlayingRecording(CPVRRecording *recording) const;
  int GetPlayingClientID(void) const;
  bool IsValidClient(int iClientId);
  bool ClientLoaded(const CStdString &strClientId);

  /*!
   * @brief Update the signal quality info.
   */
  void UpdateSignalQuality(void);

private:
  int AddClientToDb(const CStdString &strClientId, const CStdString &strName);
  int ReadLiveStream(void* lpBuf, int64_t uiBufSize);
  int ReadRecordedStream(void* lpBuf, int64_t uiBufSize);
  bool GetMenuHooks(int iClientID, PVR_MENUHOOKS *hooks);

  /*!
   * @brief Load and initialise all clients.
   * @return True if any clients were loaded, false otherwise.
   */
  bool LoadClients(void);

  /*!
   * @brief Reset the signal quality data to the initial values.
   */
  void ResetQualityData(void);

  int GetActiveClients(CLIENTMAP *clients);

  const CPVRChannel *   m_currentChannel;
  const CPVRRecording * m_currentRecording;
  bool                  m_bAllClientsLoaded; /*!< true if all clients are loaded, false otherwise */
  CCriticalSection      m_critSection;
  CLIENTMAP             m_clientMap;
  CLIENTPROPS           m_clientsProps;      /*!< store the properties of each client locally */
  PVR_SIGNALQUALITY     m_qualityInfo;       /*!< stream quality information */
  bool                  m_bChannelScanRunning;      /*!< true if a channel scan is currently running, false otherwise */
  STREAMPROPS           m_streamProps;              /*!< the current stream's properties */

  CStdString            m_strPlayingClientName;
  CStdString            m_strBackendName;
  CStdString            m_strBackendVersion;
  CStdString            m_strBackendHost;
  CStdString            m_strBackendDiskspace;
  CStdString            m_strBackendTimers;
  CStdString            m_strBackendRecordings;
  CStdString            m_strBackendChannels;
  CStdString            m_strTotalDiskspace;

  unsigned int          m_iInfoToggleStart;
  unsigned int          m_iInfoToggleCurrent;

  DWORD                 m_scanStart;                /* Scan start time to check for non present streams */
};
