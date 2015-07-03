#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/CriticalSection.h"
#include "threads/Thread.h"
#include "utils/Observer.h"
#include "PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/recordings/PVRRecording.h"
#include "addons/AddonDatabase.h"

#include <vector>
#include <deque>

namespace EPG
{
  class CEpg;
}

namespace PVR
{
  class CPVRGUIInfo;

  typedef std::shared_ptr<CPVRClient> PVR_CLIENT;
  typedef std::map< int, PVR_CLIENT >                 PVR_CLIENTMAP;
  typedef std::map< int, PVR_CLIENT >::iterator       PVR_CLIENTMAP_ITR;
  typedef std::map< int, PVR_CLIENT >::const_iterator PVR_CLIENTMAP_CITR;
  typedef std::map< int, PVR_STREAM_PROPERTIES >      STREAMPROPS;

  /**
   * Holds generic data about a backend (number of channels etc.)
   */
  struct SBackend
  {
    std::string name;
    std::string version;
    std::string host;
    int         numTimers = 0;
    int         numRecordings = 0;
    int         numDeletedRecordings = 0;
    int         numChannels = 0;
    long long   diskUsed = 0;
    long long   diskTotal = 0;
  };

  class CPVRClients : public ADDON::IAddonMgrCallback,
                      public Observer,
                      private CThread
  {
  public:
    CPVRClients(void);
    virtual ~CPVRClients(void);

    /*!
     * @brief Checks whether an add-on is loaded by the pvr manager
     * @param strAddonId The add-on id to check
     * @return True when in use, false otherwise
     */
    bool IsInUse(const std::string& strAddonId) const;

    /*!
     * @brief Start the backend info updater thread.
     */
    void Start(void);

    /*!
     * @brief Stop the backend info updater thread.
     */
    void Stop(void);

    /*! @name Backend methods */
    //@{

    /*!
     * @brief Check whether a client ID points to a valid and connected add-on.
     * @param iClientId The client ID.
     * @return True when the client ID is valid and connected, false otherwise.
     */
    bool IsConnectedClient(int iClientId) const;

    bool IsConnectedClient(const ADDON::AddonPtr addon);

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
    int ConnectedClientAmount(void) const;

    /*!
     * @brief Check whether there are any connected clients.
     * @return True if at least one client is connected.
     */
    bool HasConnectedClients(void) const;

    /*!
     * @brief Get the friendly name for the client with the given id.
     * @param iClientId The id of the client.
     * @param strName The friendly name of the client or an empty string when it wasn't found.
     * @return True if the client was found, false otherwise.
     */
    bool GetClientName(int iClientId, std::string &strName) const;

    /*!
     * @brief Returns properties about all connected clients
     * @return the properties
     */
    std::vector<SBackend> GetBackendProperties() const;

    /*!
     * Get the add-on ID of the client
     * @param iClientId The db id of the client
     * @return The add-on id
     */
    std::string GetClientAddonId(int iClientId) const;

    /*!
     * @return The client ID of the client that is currently playing a stream or -1 if no client is playing.
     */
    int GetPlayingClientID(void) const;

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
    const std::string GetPlayingClientName(void) const;

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
    int64_t GetStreamLength(void);

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
     * @brief (Un)Pause a PVR stream (only called when timeshifting is supported)
     */
    void PauseStream(bool bPaused);

    /*!
     * @brief Check whether it is possible to pause the currently playing livetv or recording stream
     */
    bool CanPauseStream(void) const;

    /*!
     * @brief Check whether it is possible to seek the currently playing livetv or recording stream
     */
    bool CanSeekStream(void) const;

    /*!
     * @brief Get the input format name of the current playing stream content.
     * @return A pointer to the properties or NULL if no stream is playing.
     */
    std::string GetCurrentInputFormat(void) const;

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
     * @param channel The channel to start playing.
     * @param bIsSwitchingChannel True when switching channels, false otherwise.
     * @return True if the stream was opened successfully, false otherwise.
     */
    bool OpenStream(const CPVRChannelPtr &channel, bool bIsSwitchingChannel);

    /*!
     * @brief Get the URL for the stream to the given channel.
     * @param channel The channel to get the stream url for.
     * @return The requested stream url or an empty string if it wasn't found.
     */
    std::string GetStreamURL(const CPVRChannelPtr &channel);

    /*!
     * @brief Switch an opened live tv stream to another channel.
     * @param channel The channel to switch to.
     * @return True if the switch was successfull, false otherwise.
     */
    bool SwitchChannel(const CPVRChannelPtr &channel);

    /*!
     * @brief Get the channel that is currently playing.
     * @return the channel that is currently playing, NULL otherwise.
     */
    CPVRChannelPtr GetPlayingChannel() const;

    /*!
     * @return True if a recording is playing, false otherwise.
     */
    bool IsPlayingRecording(void) const;

    /*!
     * @brief Open a stream from the given recording.
     * @param tag The recording to start playing.
     * @return True if the stream was opened successfully, false otherwise.
     */
    bool OpenStream(const CPVRRecordingPtr &tag);

    /*!
     * @brief Get the recordings that is currently playing.
     * @return The recording that is currently playing, NULL otherwise.
     */
    CPVRRecordingPtr GetPlayingRecording(void) const;

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
    PVR_ERROR GetTimers(CPVRTimers *timers);

    /*!
     * @brief Add a new timer to a backend.
     * @param timer The timer to add.
     * @param error An error if it occured.
     * @return True if the timer was added successfully, false otherwise.
     */
    PVR_ERROR AddTimer(const CPVRTimerInfoTag &timer);

    /*!
     * @brief Update a timer on the backend.
     * @param timer The timer to update.
     * @param error An error if it occured.
     * @return True if the timer was updated successfully, false otherwise.
     */
    PVR_ERROR UpdateTimer(const CPVRTimerInfoTag &timer);

    /*!
     * @brief Delete a timer from the backend.
     * @param timer The timer to delete.
     * @param bForce Also delete when currently recording if true.
     * @param bDeleteSchedule Also delete schedule instead of single timer.
     * @param error An error if it occured.
     * @return True if the timer was deleted successfully, false otherwise.
     */
    PVR_ERROR DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce, bool bDeleteSchedule);

    /*!
     * @brief Rename a timer on the backend.
     * @param timer The timer to rename.
     * @param strNewName The new name.
     * @param error An error if it occured.
     * @return True if the timer was renamed successfully, false otherwise.
     */
    PVR_ERROR RenameTimer(const CPVRTimerInfoTag &timer, const std::string &strNewName);

    /*!
     * @brief Get all supported timer types.
     * @param results The container to store the result in.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVR_ERROR GetTimerTypes(CPVRTimerTypes& results) const;

    /*!
     * @brief Get all timer types supported by a certain client.
     * @param iClientId The id of the client.
     * @param results The container to store the result in.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVR_ERROR GetTimerTypes(CPVRTimerTypes& results, int iClientId) const;

    //@}

    /*! @name Recording methods */
    //@{

    /*!
     * @brief Check whether a client supports recordings.
     * @param iClientId The id of the client to check.
     * @return True if the supports recordings, false otherwise.
     */
    bool SupportsRecordings(int iClientId) const;

    /*!
     * @brief Check whether a client supports undelete of recordings.
     * @param iClientId The id of the client to check.
     * @return True if the supports undeleted of recordings, false otherwise.
     */
    bool SupportsRecordingsUndelete(int iClientId) const;

    /*!
     * @brief Get all recordings from clients
     * @param recordings Store the recordings in this container.
     * @param deleted Return deleted recordings
     * @return The amount of recordings that were added.
     */
    PVR_ERROR GetRecordings(CPVRRecordings *recordings, bool deleted);

    /*!
     * @brief Rename a recordings on the backend.
     * @param recording The recordings to rename.
     * @param error An error if it occured.
     * @return True if the recording was renamed successfully, false otherwise.
     */
    PVR_ERROR RenameRecording(const CPVRRecording &recording);

    /*!
     * @brief Delete a recording from the backend.
     * @param recording The recording to delete.
     * @param error An error if it occured.
     * @return True if the recordings was deleted successfully, false otherwise.
     */
    PVR_ERROR DeleteRecording(const CPVRRecording &recording);

    /*!
     * @brief Undelete a recording from the backend.
     * @param recording The recording to undelete.
     * @param error An error if it occured.
     * @return True if the recording was undeleted successfully, false otherwise.
     */
    PVR_ERROR UndeleteRecording(const CPVRRecording &recording);

    /*!
     * @brief Delete all recordings permanent which in the deleted folder on the backend.
     * @return PVR_ERROR_NO_ERROR if the recordings has been deleted successfully.
     */
    PVR_ERROR DeleteAllRecordingsFromTrash();

    /*!
     * @brief Set play count of a recording on the backend.
     * @param recording The recording to set the play count.
     * @param count Play count.
     * @param error An error if it occured.
     * @return True if the recording's play count was set successfully, false otherwise.
     */
    bool SetRecordingPlayCount(const CPVRRecording &recording, int count, PVR_ERROR *error);

    /*!
     * @brief Set the last watched position of a recording on the backend.
     * @param recording The recording.
     * @param position The last watched position in seconds
     * @param error An error if it occured.
     * @return True if the last played position was updated successfully, false otherwise
    */
    bool SetRecordingLastPlayedPosition(const CPVRRecording &recording, int lastplayedposition, PVR_ERROR *error);

    /*!
    * @brief Retrieve the last watched position of a recording on the backend.
    * @param recording The recording.
    * @return The last watched position in seconds
    */
    int GetRecordingLastPlayedPosition(const CPVRRecording &recording);

    /*!
    * @brief Retrieve the edit decision list (EDL) from the backend.
    * @param recording The recording.
    * @return The edit decision list (empty on error).
    */
    std::vector<PVR_EDL_ENTRY> GetRecordingEdl(const CPVRRecording &recording);

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
    bool SupportsEPG(int iClientId) const;

    /*!
     * @brief Get the EPG table for a channel.
     * @param channel The channel to get the EPG table for.
     * @param epg Store the EPG in this container.
     * @param start Get entries after this start time.
     * @param end Get entries before this end time.
     * @param error An error if it occured.
     * @return True if the EPG was transfered successfully, false otherwise.
     */
    PVR_ERROR GetEPGForChannel(const CPVRChannelPtr &channel, EPG::CEpg *epg, time_t start, time_t end);

    //@}

    /*! @name Channel methods */
    //@{

    /*!
     * @brief Get all channels from backends.
     * @param group The container to store the channels in.
     * @param error An error if it occured.
     * @return The amount of channels that were added.
     */
    PVR_ERROR GetChannels(CPVRChannelGroupInternal *group);

    /*!
     * @brief Check whether a client supports channel groups.
     * @param iClientId The id of the client to check.
     * @return True if the supports channel groups, false otherwise.
     */
    bool SupportsChannelGroups(int iClientId) const;

    /*!
     * @brief Get all channel groups from backends.
     * @param groups Store the channel groups in this container.
     * @param error An error if it occured.
     * @return The amount of groups that were added.
     */
    PVR_ERROR GetChannelGroups(CPVRChannelGroups *groups);

    /*!
     * @brief Get all group members of a channel group.
     * @param group The group to get the member for.
     * @param error An error if it occured.
     * @return The amount of channels that were added.
     */
    PVR_ERROR GetChannelGroupMembers(CPVRChannelGroup *group);

    //@}

    /*! @name Menu hook methods */
    //@{

    /*!
     * @brief Check whether a client has any PVR specific menu entries.
     * @param iClientId The ID of the client to get the menu entries for. Get the menu for the active channel if iClientId < 0.
     * @return True if the client has any menu hooks, false otherwise.
     */
    bool HasMenuHooks(int iClientId, PVR_MENUHOOK_CAT cat);

    /*!
     * @brief Open selection and progress PVR actions.
     * @param iClientId The ID of the client to process the menu entries for. Process the menu entries for the active channel if iClientId < 0.
     * @param item The selected file item for which the hook was called.
     */
    void ProcessMenuHooks(int iClientID, PVR_MENUHOOK_CAT cat, const CFileItem *item);

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

    /*!
     * @return All clients that support channel scanning.
     */
    std::vector<PVR_CLIENT> GetClientsSupportingChannelScan(void) const;

    //@}

    /*! @name Channel settings methods */
    //@{

    /*!
     * @return All clients that support channel settings inside addon.
     */
    std::vector<PVR_CLIENT> GetClientsSupportingChannelSettings(bool bRadio) const;

    /*!
     * @brief Open addon settings dialog to add a channel
     * @param channel The channel to edit.
     * @return True if the edit was successfull, false otherwise.
     */
    bool OpenDialogChannelAdd(const CPVRChannelPtr &channel);

    /*!
     * @brief Open addon settings dialog to related channel
     * @param channel The channel to edit.
     * @return True if the edit was successfull, false otherwise.
     */
    bool OpenDialogChannelSettings(const CPVRChannelPtr &channel);

    /*!
     * @brief Inform addon to delete channel
     * @param channel The channel to delete.
     * @return True if it was successfull, false otherwise.
     */
    bool DeleteChannel(const CPVRChannelPtr &channel);

    /*!
     * @brief Request the client to rename given channel
     * @param channel The channel to rename
     * @return True if the edit was successfull, false otherwise.
     */
    bool RenameChannel(const CPVRChannelPtr &channel);

    //@}

    void Notify(const Observable &obs, const ObservableMessage msg);

    bool GetClient(const std::string &strId, ADDON::AddonPtr &addon) const;

    bool SupportsChannelScan(int iClientId) const;
    bool SupportsChannelSettings(int iClientId) const;
    bool SupportsLastPlayedPosition(int iClientId) const;
    bool SupportsRadio(int iClientId) const;
    bool SupportsRecordingPlayCount(int iClientId) const;
    bool SupportsRecordingEdl(int iClientId) const;
    bool SupportsTimers(int iClientId) const;
    bool SupportsTV(int iClientId) const;
    bool HandlesDemuxing(int iClientId) const;
    bool HandlesInputStream(int iClientId) const;

    bool GetPlayingClient(PVR_CLIENT &client) const;

    std::string GetBackendHostnameByClientId(int iClientId) const;

    time_t GetPlayingTime() const;
    time_t GetBufferTimeStart() const;
    time_t GetBufferTimeEnd() const;

    /**
     * Called by OnEnable() and OnDisable() to check if the manager should be restarted
     * @return True if it should be restarted, false otherwise
     */
    bool RestartManagerOnAddonDisabled(void) const { return m_bRestartManagerOnAddonDisabled; }

    int GetClientId(const std::string& strId) const;

  private:
    /*!
     * @brief Update add-ons from the AddonManager
     * @return True when updated, false otherwise
     */
    bool UpdateAddons(void);

    /*!
     * @brief Get the menu hooks for a client.
     * @param iClientID The client to get the hooks for.
     * @param hooks The container to add the hooks to.
     * @return True if the hooks were added successfully (if any), false otherwise.
     */
    bool GetMenuHooks(int iClientID, PVR_MENUHOOK_CAT cat, PVR_MENUHOOKS *hooks);

    /*!
     * @brief Updates the backend information
     */
    void Process(void);

    /*!
     * @brief Show a dialog to guide new users who have no clients enabled.
     */
    void ShowDialogNoClientsEnabled(void);

    /*!
     * @brief Get the instance of the client.
     * @param iClientId The id of the client to get.
     * @param addon The client.
     * @return True if the client was found, false otherwise.
     */
    bool GetClient(int iClientId, PVR_CLIENT &addon) const;

    /*!
     * @brief Get the instance of the client, if it's connected.
     * @param iClientId The id of the client to get.
     * @param addon The client.
     * @return True if the client is connected, false otherwise.
     */
    bool GetConnectedClient(int iClientId, PVR_CLIENT &addon) const;

    /*!
     * @bried Get all connected clients.
     * @param clients Store the active clients in this map.
     * @return The amount of added clients.
     */
    int GetConnectedClients(PVR_CLIENTMAP &clients) const;

    /*!
     * @brief Check whether a client is registered.
     * @param client The client to check.
     * @return True if this client is registered, false otherwise.
     */
    bool IsKnownClient(const ADDON::AddonPtr client) const;

    /*!
     * @brief Check whether there are any new pvr add-ons enabled or whether any of the known clients has been disabled.
     * @param bInitialiseAllClients True to initialise all clients, false to only initialise new clients.
     * @return True if all clients were updated successfully, false otherwise.
     */
    bool UpdateAndInitialiseClients(bool bInitialiseAllClients = false);

    /*!
     * @brief Initialise and connect a client.
     * @param client The client to initialise.
     * @return The id of the client if it was created or found in the existing client map, -1 otherwise.
     */
    int RegisterClient(ADDON::AddonPtr client);

    int GetClientId(const ADDON::AddonPtr client) const;

    /*!
     * Try to automatically configure clients
     * @return True when at least one was configured
     */
    bool AutoconfigureClients(void);

    bool                  m_bChannelScanRunning;      /*!< true when a channel scan is currently running, false otherwise */
    bool                  m_bIsSwitchingChannels;        /*!< true while switching channels */
    int                   m_playingClientId;          /*!< the ID of the client that is currently playing */
    bool                  m_bIsPlayingLiveTV;
    bool                  m_bIsPlayingRecording;
    DWORD                 m_scanStart;                /*!< scan start time to check for non present streams */
    std::string           m_strPlayingClientName;     /*!< the name client that is currenty playing a stream or an empty string if nothing is playing */
    ADDON::VECADDONS      m_addons;
    PVR_CLIENTMAP         m_clientMap;                /*!< a map of all known clients */
    STREAMPROPS           m_streamProps;              /*!< the current stream's properties */
    bool                  m_bNoAddonWarningDisplayed; /*!< true when a warning was displayed that no add-ons were found, false otherwise */
    CCriticalSection      m_critSection;
    std::map<int, time_t> m_connectionAttempts;       /*!< last connection attempt per add-on */
    bool                  m_bRestartManagerOnAddonDisabled; /*!< true to restart the manager when an add-on is enabled/disabled */
    std::map<std::string, int> m_addonNameIds; /*!< map add-on names to IDs */
  };
}
