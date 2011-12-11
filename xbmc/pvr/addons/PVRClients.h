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

#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/Observer.h"
#include "PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/recordings/PVRRecording.h"

#include <vector>
#include <deque>

namespace EPG
{
  class CEpg;
}

namespace PVR
{
  class CPVRGUIInfo;

  typedef std::map< int, boost::shared_ptr<CPVRClient> >           CLIENTMAP;
  typedef std::map< int, boost::shared_ptr<CPVRClient> >::iterator CLIENTMAPITR;
  typedef std::map< int, PVR_STREAM_PROPERTIES >                   STREAMPROPS;

  #define XBMC_VIRTUAL_CLIENTID -1

  class CPVRClients : public ADDON::IAddonMgrCallback,
                      public Observer,
                      private CThread
  {
    friend class CPVRGUIInfo;

  public:
    CPVRClients(void);
    virtual ~CPVRClients(void);

    /*!
     * @brief Start the backend info updater thread.
     */
    void Start(void);

    /*!
     * @brief Stop the backend info updater thread.
     */
    void Stop(void);

    /*!
     * @brief Load the settings for the current channel from the database.
     */
    void LoadCurrentChannelSettings(void);

    /*!
     * @brief Persist the current channel settings in the database.
     */
    void SaveCurrentChannelSettings(void);

    /*! @name Backend methods */
    //@{

    /*!
     * @brief Check whether a client ID points to a valid and connected add-on.
     * @param iClientId The client ID.
     * @return True when the client ID is valid and connected, false otherwise.
     */
    bool IsConnectedClient(int iClientId);

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
     * @brief Unload all loaded add-ons and reset all class properties.
     */
    void Unload(void);

    /*!
     * @brief The ID of the first active client or -1 if no clients are active;
     */
    int GetFirstConnectedClientID(void);

    /*!
     * @return True when all clients are connected, false otherwise.
     */
    bool AllClientsConnected(void) const;

    /*!
     * @return True when at least one client is known and enabled, false otherwise.
     */
    bool HasEnabledClients(void) const;

    /*!
     * @return The amount of enabled clients.
     */
    int EnabledClientAmount(void) const;

    /*!
     * @brief Stop a client.
     * @param addon The client to stop.
     * @param bRestart If true, restart the client.
     * @return True if the client was found, false otherwise.
     */
    bool StopClient(ADDON::AddonPtr client, bool bRestart);

    /*!
     * @return The amount of connected clients.
     */
    int ConnectedClientAmount(void);

    /*!
     * @brief Check whether there are any connected clients.
     * @return True if at least one client is connected.
     */
    bool HasConnectedClients(void);

    /*!
     * @brief Get the friendly name for the client with the given id.
     * @param iClientId The id of the client.
     * @param strName The friendly name of the client or an empty string when it wasn't found.
     * @return True if the client was found, false otherwise.
     */
    bool GetClientName(int iClientId, CStdString &strName);

    /*!
     * @bried Get all connected clients.
     * @param clients Store the active clients in this map.
     * @return The amount of added clients.
     */
    int GetConnectedClients(CLIENTMAP *clients);

    /*!
     * @return The client ID of the client that is currently playing a stream or -1 if no client is playing.
     */
    int GetPlayingClientID(void) const;

    /*!
     * @brief Get the capabilities for a specific client.
     * @param clientID The ID of the client.
     * @return The add-on's capabilities.
     */
    PVR_ADDON_CAPABILITIES GetAddonCapabilities(int iClientId);

    /*!
     * @brief Get the capabilities of the current playing client.
     * @return The add-on's capabilities.
     */
    PVR_ADDON_CAPABILITIES GetCurrentAddonCapabilities(void);

    //@}

    /*! @name Stream methods */
    //@{

    /*!
     * @return True if a stream is playing, false otherwise.
     */
    bool IsPlaying(void) const;

    /*!
     * @return The friendly name of the client that is currently playing or an empty string if nothing is playing.
     */
    const CStdString GetPlayingClientName(void) const;

    /*!
     * @brief Read from an open stream.
     * @param lpBuf Target buffer.
     * @param uiBufSize The size of the buffer.
     * @return The amount of bytes that was added.
     */
    int ReadStream(void* lpBuf, int64_t uiBufSize);

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
     * @brief Close a PVR stream.
     */
    void CloseStream(void);

    /*!
     * @brief Get the properties of the current playing stream content.
     * @return A pointer to the properties or NULL if no stream is playing.
     */
    PVR_STREAM_PROPERTIES *GetCurrentStreamProperties(void);

    /*!
     * @brief Get the input format name of the current playing stream content.
     * @return A pointer to the properties or NULL if no stream is playing.
     */
    CStdString GetCurrentInputFormat(void) const;
    //@}

    /*! @name Live TV stream methods */
    //@{

    /*!
     * @return True if a live stream is playing, false otherwise.
     */
    bool IsReadingLiveStream(void) const;

    /*!
     * @return True if a TV channel is playing, false otherwise.
     */
    bool IsPlayingTV(void) const;

    /*!
     * @return True if a radio channel playing, false otherwise.
     */
    bool IsPlayingRadio(void) const;

    /*!
     * @return True if the currently playing channel is encrypted, false otherwise.
     */
    bool IsEncrypted(void) const;

    /*!
     * @brief Open a stream on the given channel.
     * @param tag The channel to start playing.
     * @return True if the stream was opened successfully, false otherwise.
     */
    bool OpenLiveStream(const CPVRChannel &tag);

    /*!
     * @brief Close an opened live stream.
     * @return True if the stream was closed successfully, false otherwise.
     */
    bool CloseLiveStream(void);

    /*!
     * @brief Get the URL for the stream to the given channel.
     * @param tag The channel to get the stream url for.
     * @return The requested stream url or an empty string if it wasn't found.
     */
    CStdString GetStreamURL(const CPVRChannel &tag);

    /*!
     * @brief Switch an opened live tv stream to another channel.
     * @param channel The channel to switch to.
     * @return True if the switch was successfull, false otherwise.
     */
    bool SwitchChannel(const CPVRChannel &channel);

    /*!
     * @brief Get the channel that is currently playing.
     * @param channel A copy of the channel that is currently playing.
     * @return True if a channel is playing, false otherwise.
     */
    bool GetPlayingChannel(CPVRChannel &channel) const;

    //@}

    /*! @name Recording stream methods */
    //@{

    /*!
     * @return True if a recording is playing, false otherwise.
     */
    bool IsPlayingRecording(void) const;

    /*!
     * @brief Open a stream from the given recording.
     * @param tag The recording to start playing.
     * @return True if the stream was opened successfully, false otherwise.
     */
    bool OpenRecordedStream(const CPVRRecording &tag);

    /*!
     * @brief Close an opened stream from a recording.
     * @return True if the stream was closed successfully, false otherwise.
     */
    bool CloseRecordedStream(void);

    /*!
     * @brief Get the recordings that is currently playing.
     * @param recording A copy of the recording that is currently playing.
     * @return True if a recording is playing, false otherwise.
     */
    bool GetPlayingRecording(CPVRRecording &recording) const;

    //@}

    /*! @name Stream demux methods */
    //@{

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
     * @brief Read the stream from the demuxer.
     * @return An allocated demuxer packet.
     */
    DemuxPacket* ReadDemuxStream(void);

    //@}

    /*! @name Signal status methods */
    //@{

    /*!
     * @brief Get the quality data for the live stream that is currently playing.
     * @param status A copy of the quality data.
     */
    void GetQualityData(PVR_SIGNAL_STATUS *status) const;

    /*!
     * @return The current signal quality level.
     */
    int GetSignalLevel(void) const;

    /*!
     * @return The current signal/noise ratio.
     */
    int GetSNR(void) const;

    //@}

    /*! @name Timer methods */
    //@{

    /*!
     * @brief Check whether a client supports timers.
     * @param iClientId The id of the client to check.
     * @return True if the supports timers, false otherwise.
     */
    bool HasTimerSupport(int iClientId);

    /*!
     * @brief Get all timers from clients
     * @param timers Store the timers in this container.
     * @return The amount of timers that were added.
     */
    int GetTimers(CPVRTimers *timers);

    /*!
     * @brief Add a new timer to a backend.
     * @param timer The timer to add.
     * @param error An error if it occured.
     * @return True if the timer was added successfully, false otherwise.
     */
    bool AddTimer(const CPVRTimerInfoTag &timer, PVR_ERROR *error);

    /*!
     * @brief Update a timer on the backend.
     * @param timer The timer to update.
     * @param error An error if it occured.
     * @return True if the timer was updated successfully, false otherwise.
     */
    bool UpdateTimer(const CPVRTimerInfoTag &timer, PVR_ERROR *error);

    /*!
     * @brief Delete a timer from the backend.
     * @param timer The timer to delete.
     * @param bForce Also delete when currently recording if true.
     * @param error An error if it occured.
     * @return True if the timer was deleted successfully, false otherwise.
     */
    bool DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce, PVR_ERROR *error);

    /*!
     * @brief Rename a timer on the backend.
     * @param timer The timer to rename.
     * @param strNewName The new name.
     * @param error An error if it occured.
     * @return True if the timer was renamed successfully, false otherwise.
     */
    bool RenameTimer(const CPVRTimerInfoTag &timer, const CStdString &strNewName, PVR_ERROR *error);

    //@}

    /*! @name Recording methods */
    //@{

    /*!
     * @brief Check whether a client supports recordings.
     * @param iClientId The id of the client to check.
     * @return True if the supports recordings, false otherwise.
     */
    bool HasRecordingsSupport(int iClientId);

    /*!
     * @brief Get all recordings from clients
     * @param recordings Store the recordings in this container.
     * @return The amount of recordings that were added.
     */
    int GetRecordings(CPVRRecordings *recordings);

    /*!
     * @brief Rename a recordings on the backend.
     * @param recording The recordings to rename.
     * @param error An error if it occured.
     * @return True if the recording was renamed successfully, false otherwise.
     */
    bool RenameRecording(const CPVRRecording &recording, PVR_ERROR *error);

    /*!
     * @brief Delete a recording from the backend.
     * @param recording The recording to delete.
     * @param error An error if it occured.
     * @return True if the recordings was deleted successfully, false otherwise.
     */
    bool DeleteRecording(const CPVRRecording &recording, PVR_ERROR *error);

    /*!
     * @brief Check whether there is an active recording on the current channel.
     * @return True if there is, false otherwise.
     */
    bool IsRecordingOnPlayingChannel(void) const;

    /*!
     * @brief Check whether the current channel can be recorded instantly.
     * @return True if it can, false otherwise.
     */
    bool CanRecordInstantly(void);

    //@}

    /*! @name EPG methods */
    //@{

    /*!
     * @brief Check whether a client supports EPG transfer.
     * @param iClientId The id of the client to check.
     * @return True if the supports EPG transfer, false otherwise.
     */
    bool HasEPGSupport(int iClientId);

    /*!
     * @brief Get the EPG table for a channel.
     * @param channel The channel to get the EPG table for.
     * @param epg Store the EPG in this container.
     * @param start Get entries after this start time.
     * @param end Get entries before this end time.
     * @param error An error if it occured.
     * @return True if the EPG was transfered successfully, false otherwise.
     */
    bool GetEPGForChannel(const CPVRChannel &channel, EPG::CEpg *epg, time_t start, time_t end, PVR_ERROR *error);

    //@}

    /*! @name Channel methods */
    //@{

    /*!
     * @brief Get all channels from backends.
     * @param group The container to store the channels in.
     * @param error An error if it occured.
     * @return The amount of channels that were added.
     */
    int GetChannels(CPVRChannelGroupInternal *group, PVR_ERROR *error);

    /*!
     * @brief Check whether a client supports channel groups.
     * @param iClientId The id of the client to check.
     * @return True if the supports channel groups, false otherwise.
     */
    bool HasChannelGroupSupport(int iClientId);

    /*!
     * @brief Get all channel groups from backends.
     * @param groups Store the channel groups in this container.
     * @param error An error if it occured.
     * @return The amount of groups that were added.
     */
    int GetChannelGroups(CPVRChannelGroups *groups, PVR_ERROR *error);

    /*!
     * @brief Get all group members of a channel group.
     * @param group The group to get the member for.
     * @param error An error if it occured.
     * @return The amount of channels that were added.
     */
    int GetChannelGroupMembers(CPVRChannelGroup *group, PVR_ERROR *error);

    //@}

    /*! @name Menu hook methods */
    //@{

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

    //@}

    /*! @name Channel scan methods */
    //@{

    /*!
     * @return True when a channel scan is currently running, false otherwise.
     */
    bool IsRunningChannelScan(void) const;

    /*!
     * @brief Open a selection dialog and start a channel scan on the selected client.
     */
    void StartChannelScan(void);

    //@}

    void Notify(const Observable &obs, const CStdString& msg);
  private:
    /*!
     * @brief Update add-ons from the AddonManager
     * @return True when updated, false otherwise
     */
    bool UpdateAddons(void);

    /*!
     * @brief Register a client in the db if it's not been registered yet.
     * @param client The client to register.
     * @return The database id of the client or -1 if an error occured.
     */
    int AddClientToDb(const ADDON::AddonPtr client);

    /*!
     * @brief Read from a livetv stream.
     * @param lpBuf The buffer to store the data in.
     * @param uiBufSize The length to read.
     * @return The number of bytes read.
     */
    int ReadLiveStream(void* lpBuf, int64_t uiBufSize);

    /*!
     * @brief Read from a recorded tv stream.
     * @param lpBuf The buffer to store the data in.
     * @param uiBufSize The length to read.
     * @return The number of bytes read.
     */
    int ReadRecordedStream(void* lpBuf, int64_t uiBufSize);

    /*!
     * @brief Get the menu hooks for a client.
     * @param iClientID The client to get the hooks for.
     * @param hooks The container to add the hooks to.
     * @return True if the hooks were added successfully (if any), false otherwise.
     */
    bool GetMenuHooks(int iClientID, PVR_MENUHOOKS *hooks);

    /*!
     * @brief Update the signal status for the tv stream that's currently being read.
     */
    void UpdateCharInfoSignalStatus(void);

    /*!
     * @brief Reset the signal quality data to the initial values.
     */
    void ResetQualityData(PVR_SIGNAL_STATUS &qualityInfo);

    /*!
     * @brief Updates the backend information
     */
    void Process(void);

    /*!
     * @brief Show a dialog to guide new users who have no clients enabled.
     */
    void ShowDialogNoClientsEnabled(void);

    /*!
     * @brief Get the instance of the client, if it's connected.
     * @param iClientId The id of the client to get.
     * @param addon The client.
     * @return True if the client is connected, false otherwise.
     */
    bool GetConnectedClient(int iClientId, boost::shared_ptr<CPVRClient> &addon);

    /*!
     * @brief Check whether a client is registered.
     * @param client The client to check.
     * @return True if this client is registered, false otherwise.
     */
    bool IsKnownClient(const ADDON::AddonPtr client);

    /*!
     * @brief Check whether there are any new pvr add-ons enabled or whether any of the known clients has been disabled.
     * @param bInitialiseAllClients True to initialise all clients, false to only initialise new clients.
     * @return True if all clients were updated successfully, false otherwise.
     */
    bool UpdateAndInitialiseClients(bool bInitialiseAllClients = false);

    /*!
     * @brief Initialise and connect a client.
     * @param client The client to initialise.
     * @return True if the client was initialised successfully, false otherwise.
     */
    bool InitialiseClient(ADDON::AddonPtr client);

    bool                  m_bChannelScanRunning;      /*!< true when a channel scan is currently running, false otherwise */
    bool                  m_bAllClientsConnected;        /*!< true when all clients are loaded, false otherwise */
    bool                  m_bIsSwitchingChannels;        /*!< true while switching channels */
    bool                  m_bIsValidChannelSettings;  /*!< true if current channel settings are valid and can be saved */
    CPVRChannel           m_currentChannel;           /*!< the channel that is currently playing or NULL if nothing is playing */
    bool                  m_bIsPlayingLiveTV;
    CPVRRecording         m_currentRecording;         /*!< the recording that is currently playing or NULL if nothing is playing */
    bool                  m_bIsPlayingRecording;
    DWORD                 m_scanStart;                /*!< scan start time to check for non present streams */
    CStdString            m_strPlayingClientName;     /*!< the name client that is currenty playing a stream or an empty string if nothing is playing */
    ADDON::VECADDONS      m_addons;
    CLIENTMAP             m_clientMap;                /*!< a map of all known clients */
    PVR_SIGNAL_STATUS     m_qualityInfo;              /*!< stream quality information */
    STREAMPROPS           m_streamProps;              /*!< the current stream's properties */
    CCriticalSection      m_critSection;
  };
}
