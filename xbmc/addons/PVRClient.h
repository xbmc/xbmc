/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"

#include "pvr/PVRTypes.h"

namespace PVR
{
  class CPVRChannelGroups;
  class CPVRTimersContainer;
  class CPVRClientMenuHook;
  class CPVRClientMenuHooks;

  class CPVRClient;
  typedef std::shared_ptr<CPVRClient> CPVRClientPtr;

  class CPVRTimerType;
  typedef std::vector<CPVRTimerTypePtr> CPVRTimerTypes;

  #define PVR_INVALID_CLIENT_ID (-2)

  class CPVRClientCapabilities
  {
  public:
    CPVRClientCapabilities() = default;
    virtual ~CPVRClientCapabilities() = default;

    CPVRClientCapabilities(const CPVRClientCapabilities& other);
    const CPVRClientCapabilities& operator =(const CPVRClientCapabilities& other);

    const CPVRClientCapabilities& operator =(const PVR_ADDON_CAPABILITIES& addonCapabilities);

    void clear();

    /////////////////////////////////////////////////////////////////////////////////
    //
    // Channels
    //
    /////////////////////////////////////////////////////////////////////////////////

    /*!
     * @brief Check whether this add-on supports TV channels.
     * @return True if supported, false otherwise.
     */
    bool SupportsTV() const { return m_addonCapabilities && m_addonCapabilities->bSupportsTV; }

    /*!
     * @brief Check whether this add-on supports radio channels.
     * @return True if supported, false otherwise.
     */
    bool SupportsRadio() const { return m_addonCapabilities && m_addonCapabilities->bSupportsRadio; }

    /*!
     * @brief Check whether this add-on supports channel groups.
     * @return True if supported, false otherwise.
     */
    bool SupportsChannelGroups() const { return m_addonCapabilities && m_addonCapabilities->bSupportsChannelGroups; }

    /*!
     * @brief Check whether this add-on supports scanning for new channels on the backend.
     * @return True if supported, false otherwise.
     */
    bool SupportsChannelScan() const { return m_addonCapabilities && m_addonCapabilities->bSupportsChannelScan; }

    /*!
     * @brief Check whether this add-on supports the following functions: DeleteChannel, RenameChannel, DialogChannelSettings and DialogAddChannel.
     * @return True if supported, false otherwise.
     */
    bool SupportsChannelSettings() const { return m_addonCapabilities && m_addonCapabilities->bSupportsChannelSettings; }

    /*!
     * @brief Check whether this add-on supports descramble information for playing channels.
     * @return True if supported, false otherwise.
     */
    bool SupportsDescrambleInfo() const { return m_addonCapabilities && m_addonCapabilities->bSupportsDescrambleInfo; }

    /////////////////////////////////////////////////////////////////////////////////
    //
    // EPG
    //
    /////////////////////////////////////////////////////////////////////////////////

    /*!
     * @brief Check whether this add-on provides EPG information.
     * @return True if supported, false otherwise.
     */
    bool SupportsEPG() const { return m_addonCapabilities && m_addonCapabilities->bSupportsEPG; }

    /*!
     * @brief Check whether this add-on supports asynchronous transfer of epg events.
     * @return True if supported, false otherwise.
     */
    bool SupportsAsyncEPGTransfer() const { return m_addonCapabilities && m_addonCapabilities->bSupportsAsyncEPGTransfer; }

    /////////////////////////////////////////////////////////////////////////////////
    //
    // Timers
    //
    /////////////////////////////////////////////////////////////////////////////////

    /*!
     * @brief Check whether this add-on supports the creation and editing of timers.
     * @return True if supported, false otherwise.
     */
    bool SupportsTimers() const { return m_addonCapabilities && m_addonCapabilities->bSupportsTimers; }

    /////////////////////////////////////////////////////////////////////////////////
    //
    // Recordings
    //
    /////////////////////////////////////////////////////////////////////////////////

    /*!
     * @brief Check whether this add-on supports recordings.
     * @return True if supported, false otherwise.
     */
    bool SupportsRecordings() const { return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings; }

    /*!
     * @brief Check whether this add-on supports undelete of deleted recordings.
     * @return True if supported, false otherwise.
     */
    bool SupportsRecordingsUndelete() const { return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings && m_addonCapabilities->bSupportsRecordingsUndelete; }

    /*!
     * @brief Check whether this add-on supports play count for recordings.
     * @return True if supported, false otherwise.
     */
    bool SupportsRecordingsPlayCount() const { return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings && m_addonCapabilities->bSupportsRecordingPlayCount; }

    /*!
     * @brief Check whether this add-on supports store/retrieve of last played position for recordings..
     * @return True if supported, false otherwise.
     */
    bool SupportsRecordingsLastPlayedPosition() const { return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings && m_addonCapabilities->bSupportsLastPlayedPosition; }

    /*!
     * @brief Check whether this add-on supports retrieving an edit decision list for recordings.
     * @return True if supported, false otherwise.
     */
    bool SupportsRecordingsEdl() const { return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings && m_addonCapabilities->bSupportsRecordingEdl; }

    /*!
     * @brief Check whether this add-on supports retrieving an edit decision list for epg tags.
     * @return True if supported, false otherwise.
     */
    bool SupportsEpgTagEdl() const { return m_addonCapabilities && m_addonCapabilities->bSupportsEPG && m_addonCapabilities->bSupportsEPGEdl; }

    /*!
     * @brief Check whether this add-on supports renaming recordings..
     * @return True if supported, false otherwise.
     */
    bool SupportsRecordingsRename() const { return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings && m_addonCapabilities->bSupportsRecordingsRename; }

    /*!
     * @brief Check whether this add-on supports changing lifetime of recording.
     * @return True if supported, false otherwise.
     */
    bool SupportsRecordingsLifetimeChange() const { return m_addonCapabilities && m_addonCapabilities->bSupportsRecordings && m_addonCapabilities->bSupportsRecordingsLifetimeChange; }

    /*!
     * @brief Obtain a list with all possible values for recordings lifetime.
     * @param list out, the list with the values or an empty list, if lifetime is not supported.
     */
    void GetRecordingsLifetimeValues(std::vector<std::pair<std::string, int>> &list) const;

    /////////////////////////////////////////////////////////////////////////////////
    //
    // Streams
    //
    /////////////////////////////////////////////////////////////////////////////////

    /*!
     * @brief Check whether this add-on provides an input stream. false if Kodi handles the stream.
     * @return True if supported, false otherwise.
     */
    bool HandlesInputStream() const { return m_addonCapabilities && m_addonCapabilities->bHandlesInputStream; }

    /*!
     * @brief Check whether this add-on demultiplexes packets.
     * @return True if supported, false otherwise.
     */
    bool HandlesDemuxing() const { return m_addonCapabilities && m_addonCapabilities->bHandlesDemuxing; }

  private:
    void InitRecordingsLifetimeValues();

    std::unique_ptr<PVR_ADDON_CAPABILITIES> m_addonCapabilities;
    std::vector<std::pair<std::string, int>> m_recordingsLifetimeValues;
  };

  /*!
   * Interface from Kodi to a PVR add-on.
   *
   * Also translates Kodi's C++ structures to the add-on's C structures.
   */
  class CPVRClient : public ADDON::CAddonDll
  {
  public:
    explicit CPVRClient(ADDON::CAddonInfo addonInfo);
    ~CPVRClient(void) override;

    void OnPreInstall() override;
    void OnPreUnInstall() override;
    ADDON::AddonPtr GetRunningInstance() const override;

    /** @name PVR add-on methods */
    //@{

    /*!
     * @brief Initialise the instance of this add-on.
     * @param iClientId The ID of this add-on.
     */
    ADDON_STATUS Create(int iClientId);

    /*!
     * @return True when the dll for this add-on was loaded, false otherwise (e.g. unresolved symbols)
     */
    bool DllLoaded(void) const;

    /*!
     * @brief Stop this add-on instance. No more client add-on access after this call.
     */
    void Stop();

    /*!
     * @brief Continue this add-on instance. Client add-on access is okay again after this call.
     */
    void Continue();

    /*!
     * @brief Destroy the instance of this add-on.
     */
    void Destroy(void);

    /*!
     * @brief Destroy and recreate this add-on.
     */
    void ReCreate(void);

    /*!
     * @return True if this instance is initialised (ADDON_Create returned true), false otherwise.
     */
    bool ReadyToUse(void) const;

    /*!
     * @brief Gets the backend connection state.
     * @return the backend connection state.
     */
    PVR_CONNECTION_STATE GetConnectionState(void) const;

    /*!
     * @brief Sets the backend connection state.
     * @param state the new backend connection state.
     */
    void SetConnectionState(PVR_CONNECTION_STATE state);

    /*!
     * @brief Gets the backend's previous connection state.
     * @return the backend's previous  connection state.
     */
    PVR_CONNECTION_STATE GetPreviousConnectionState(void) const;

    /*!
     * @brief signal to PVRManager this client should be ignored
     * @return true if this client should be ignored
     */
    bool IgnoreClient(void) const;

    /*!
     * @return The ID of this instance.
     */
    int GetID(void) const;

    //@}
    /** @name PVR server methods */
    //@{

    /*!
     * @brief Query this add-on's capabilities.
     * @return The add-on's capabilities.
     */
    const CPVRClientCapabilities& GetClientCapabilities(void) const { return m_clientCapabilities; }

    /*!
     * @brief Get the stream properties of the stream that's currently being read.
     * @param pProperties The properties.
     * @return PVR_ERROR_NO_ERROR if the properties have been fetched successfully.
     */
    PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES *pProperties);

    /*!
     * @return The name reported by the backend.
     */
    const std::string& GetBackendName(void) const;

    /*!
     * @return The version string reported by the backend.
     */
    const std::string& GetBackendVersion(void) const;

    /*!
     * @brief the ip address or alias of the pvr backend server
     */
    const std::string& GetBackendHostname(void) const;

    /*!
     * @return The connection string reported by the backend.
     */
    const std::string& GetConnectionString(void) const;

    /*!
     * @return A friendly name for this add-on that can be used in log messages.
     */
    const std::string& GetFriendlyName(void) const;

    /*!
     * @brief Get the disk space reported by the server.
     * @param iTotal The total disk space.
     * @param iUsed The used disk space.
     * @return PVR_ERROR_NO_ERROR if the drive space has been fetched successfully.
     */
    PVR_ERROR GetDriveSpace(long long &iTotal, long long &iUsed);

    /*!
     * @brief Start a channel scan on the server.
     * @return PVR_ERROR_NO_ERROR if the channel scan has been started successfully.
     */
    PVR_ERROR StartChannelScan(void);

    /*!
     * @brief Request the client to open dialog about given channel to add
     * @param channel The channel to add
     * @return PVR_ERROR_NO_ERROR if the add has been fetched successfully.
     */
    PVR_ERROR OpenDialogChannelAdd(const CPVRChannelPtr &channel);

    /*!
     * @brief Request the client to open dialog about given channel settings
     * @param channel The channel to edit
     * @return PVR_ERROR_NO_ERROR if the edit has been fetched successfully.
     */
    PVR_ERROR OpenDialogChannelSettings(const CPVRChannelPtr &channel);

    /*!
     * @brief Request the client to delete given channel
     * @param channel The channel to delete
     * @return PVR_ERROR_NO_ERROR if the delete has been fetched successfully.
     */
    PVR_ERROR DeleteChannel(const CPVRChannelPtr &channel);

    /*!
     * @brief Request the client to rename given channel
     * @param channel The channel to rename
     * @return PVR_ERROR_NO_ERROR if the rename has been fetched successfully.
     */
    PVR_ERROR RenameChannel(const CPVRChannelPtr &channel);

    /*
     * @brief Check if an epg tag can be recorded
     * @param tag The epg tag
     * @param bIsRecordable Set to true if the tag can be recorded
     * @return PVR_ERROR_NO_ERROR if bIsRecordable has been set successfully.
     */
    PVR_ERROR IsRecordable(const CConstPVREpgInfoTagPtr &tag, bool &bIsRecordable) const;

    /*
     * @brief Check if an epg tag can be played
     * @param tag The epg tag
     * @param bIsPlayable Set to true if the tag can be played
     * @return PVR_ERROR_NO_ERROR if bIsPlayable has been set successfully.
     */
    PVR_ERROR IsPlayable(const CConstPVREpgInfoTagPtr &tag, bool &bIsPlayable) const;

    /*!
     * @brief Fill the file item for an epg tag with the properties required for playback. Values are obtained from the PVR backend.
     * @param fileItem The file item to be filled.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR FillEpgTagStreamFileItem(CFileItem &fileItem);

    //@}
    /** @name PVR EPG methods */
    //@{

    /*!
     * @brief Request an EPG table for a channel from the client.
     * @param channel The channel to get the EPG table for.
     * @param epg The table to write the data to.
     * @param start The start time to use.
     * @param end The end time to use.
     * @param bSaveInDb If true, tell the callback method to save any new entry in the database or not. see CAddonCallbacksPVR::PVRTransferEpgEntry()
     * @return PVR_ERROR_NO_ERROR if the table has been fetched successfully.
     */
    PVR_ERROR GetEPGForChannel(const CPVRChannelPtr &channel, CPVREpg *epg, time_t start = 0, time_t end = 0, bool bSaveInDb = false);

    /*!
     * Tell the client the time frame to use when notifying epg events back to Kodi. The client might push epg events asynchronously
     * to Kodi using the callback function EpgEventStateChange. To be able to only push events that are actually of interest for Kodi,
     * client needs to know about the epg time frame Kodi uses.
     * @param iDays number of days from "now". EPG_TIMEFRAME_UNLIMITED means that Kodi is interested in all epg events, regardless of event times.
     * @return PVR_ERROR_NO_ERROR if new value was successfully set.
     */
    PVR_ERROR SetEPGTimeFrame(int iDays);

    //@}
    /** @name PVR channel group methods */
    //@{

    /*!
     * @brief Get the total amount of channel groups from the backend.
     * @param iGroups The total amount of channel groups on the server or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR GetChannelGroupsAmount(int &iGroups);

    /*!
     * @brief Request the list of all channel groups from the backend.
     * @param groups The groups container to get the groups for.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVR_ERROR GetChannelGroups(CPVRChannelGroups *groups);

    /*!
     * @brief Request the list of all group members from the backend.
     * @param group The group to get the members for.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVR_ERROR GetChannelGroupMembers(CPVRChannelGroup *group);

    //@}
    /** @name PVR channel methods */
    //@{

    /*!
     * @brief Get the total amount of channels from the backend.
     * @param iChannels The total amount of channels on the server or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR GetChannelsAmount(int &iChannels);

    /*!
     * @brief Request the list of all channels from the backend.
     * @param channels The channel group to add the channels to.
     * @param bRadio True to get the radio channels, false to get the TV channels.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVR_ERROR GetChannels(CPVRChannelGroup &channels, bool bRadio);

    //@}
    /** @name PVR recording methods */
    //@{

    /*!
     * @brief Get the total amount of recordings from the backend.
     * @param deleted True to return deleted recordings.
     * @param iRecordings The total amount of recordings on the server or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR GetRecordingsAmount(bool deleted, int &iRecordings);

    /*!
     * @brief Request the list of all recordings from the backend.
     * @param results The container to add the recordings to.
     * @param deleted True to return deleted recordings.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVR_ERROR GetRecordings(CPVRRecordings *results, bool deleted);

    /*!
     * @brief Delete a recording on the backend.
     * @param recording The recording to delete.
     * @return PVR_ERROR_NO_ERROR if the recording has been deleted successfully.
     */
    PVR_ERROR DeleteRecording(const CPVRRecording &recording);

    /*!
     * @brief Undelete a recording on the backend.
     * @param recording The recording to undelete.
     * @return PVR_ERROR_NO_ERROR if the recording has been undeleted successfully.
     */
    PVR_ERROR UndeleteRecording(const CPVRRecording &recording);

    /*!
     * @brief Delete all recordings permanent which in the deleted folder on the backend.
     * @return PVR_ERROR_NO_ERROR if the recordings has been deleted successfully.
     */
    PVR_ERROR DeleteAllRecordingsFromTrash();

    /*!
     * @brief Rename a recording on the backend.
     * @param recording The recording to rename.
     * @return PVR_ERROR_NO_ERROR if the recording has been renamed successfully.
     */
    PVR_ERROR RenameRecording(const CPVRRecording &recording);

    /*!
     * @brief Set the lifetime of a recording on the backend.
     * @param recording The recording to set the lifetime for. recording.m_iLifetime contains the new lifetime value.
     * @return PVR_ERROR_NO_ERROR if the recording's lifetime has been set successfully.
     */
    PVR_ERROR SetRecordingLifetime(const CPVRRecording &recording);

    /*!
     * @brief Set the play count of a recording on the backend.
     * @param recording The recording to set the play count.
     * @param count Play count.
     * @return PVR_ERROR_NO_ERROR if the recording's play count has been set successfully.
     */
    PVR_ERROR SetRecordingPlayCount(const CPVRRecording &recording, int count);

    /*!
    * @brief Set the last watched position of a recording on the backend.
    * @param recording The recording.
    * @param lastplayedposition The last watched position in seconds
    * @return PVR_ERROR_NO_ERROR if the position has been stored successfully.
    */
    PVR_ERROR SetRecordingLastPlayedPosition(const CPVRRecording &recording, int lastplayedposition);

    /*!
    * @brief Retrieve the last watched position of a recording on the backend.
    * @param recording The recording.
    * @param iPosition The last watched position in seconds or -1 on error
    * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
    */
    PVR_ERROR GetRecordingLastPlayedPosition(const CPVRRecording &recording, int &iPosition);

    /*!
    * @brief Retrieve the edit decision list (EDL) from the backend.
    * @param recording The recording.
    * @param edls The edit decision list (empty on error).
    * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
    */
    PVR_ERROR GetRecordingEdl(const CPVRRecording &recording, std::vector<PVR_EDL_ENTRY> &edls);

    /*!
    * @brief Retrieve the edit decision list (EDL) from the backend.
    * @param epgTag The EPG tag.
    * @param edls The edit decision list (empty on error).
    * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
    */
    PVR_ERROR GetEpgTagEdl(const CConstPVREpgInfoTagPtr &epgTag, std::vector<PVR_EDL_ENTRY> &edls);

    //@}
    /** @name PVR timer methods */
    //@{

    /*!
     * @brief Get the total amount of timers from the backend.
     * @param iTimers The total amount of timers on the backend or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR GetTimersAmount(int &iTimers);

    /*!
     * @brief Request the list of all timers from the backend.
     * @param results The container to store the result in.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVR_ERROR GetTimers(CPVRTimersContainer *results);

    /*!
     * @brief Add a timer on the backend.
     * @param timer The timer to add.
     * @return PVR_ERROR_NO_ERROR if the timer has been added successfully.
     */
    PVR_ERROR AddTimer(const CPVRTimerInfoTag &timer);

    /*!
     * @brief Delete a timer on the backend.
     * @param timer The timer to delete.
     * @param bForce Set to true to delete a timer that is currently recording a program.
     * @return PVR_ERROR_NO_ERROR if the timer has been deleted successfully.
     */
    PVR_ERROR DeleteTimer(const CPVRTimerInfoTag &timer, bool bForce = false);

    /*!
     * @brief Update the timer information on the server.
     * @param timer The timer to update.
     * @return PVR_ERROR_NO_ERROR if the timer has been updated successfully.
     */
    PVR_ERROR UpdateTimer(const CPVRTimerInfoTag &timer);

    /*!
     * @brief Get all timer types supported by the backend.
     * @param results The container to store the result in.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVR_ERROR GetTimerTypes(CPVRTimerTypes& results) const;

    //@}
    /** @name PVR live stream methods */
    //@{

    /*!
     * @brief Open a live stream on the server.
     * @param channel The channel to stream.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR OpenLiveStream(const CPVRChannelPtr &channel);

    /*!
     * @brief Close an open live stream.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR CloseLiveStream();

    /*!
     * @brief Read from an open live stream.
     * @param lpBuf The buffer to store the data in.
     * @param uiBufSize The amount of bytes to read.
     * @param iRead The amount of bytes that were actually read from the stream.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR ReadLiveStream(void* lpBuf, int64_t uiBufSize, int &iRead);

    /*!
     * @brief Seek in a live stream on a backend.
     * @param iFilePosition The position to seek to.
     * @param iWhence ?
     * @param iPosition The new position or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR SeekLiveStream(int64_t iFilePosition, int iWhence, int64_t &iPosition);

    /*!
     * @brief Get the lenght of the currently playing live stream, if any.
     * @param iLength The total length of the stream that's currently being read or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR GetLiveStreamLength(int64_t &iLength);

    /*!
     * @brief (Un)Pause a stream.
     * @param bPaused True to pause the stream, false to unpause.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR PauseStream(bool bPaused);

    /*!
     * @brief Get the signal quality of the stream that's currently open.
     * @param qualityinfo The signal quality.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR SignalQuality(PVR_SIGNAL_STATUS &qualityinfo);

    /*!
     * @brief Get the descramble information of the stream that's currently open.
     * @param descrambleinfo The descramble information.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR GetDescrambleInfo(PVR_DESCRAMBLE_INFO &descrambleinfo) const;

    /*!
     * @brief Fill the file item for a channel with the properties required for playback. Values are obtained from the PVR backend.
     * @param fileItem The file item to be filled.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR FillChannelStreamFileItem(CFileItem &fileItem);

    /*!
     * @brief Check whether PVR backend supports pausing the currently playing stream
     * @param bCanPause True if the stream can be paused, false otherwise.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR CanPauseStream(bool &bCanPause) const;

    /*!
     * @brief Check whether PVR backend supports seeking for the currently playing stream
     * @param bCanSeek True if the stream can be seeked, false otherwise.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR CanSeekStream(bool &bCanSeek) const;

    /*!
     * @brief Notify the pvr addon/demuxer that Kodi wishes to seek the stream by time
     * @param time The absolute time since stream start
     * @param backwards True to seek to keyframe BEFORE time, else AFTER
     * @param startpts can be updated to point to where display should start
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     * @remarks Optional, and only used if addon has its own demuxer.
     */
    PVR_ERROR SeekTime(double time, bool backwards, double *startpts);

    /*!
     * @brief Notify the pvr addon/demuxer that Kodi wishes to change playback speed
     * @param speed The requested playback speed
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     * @remarks Optional, and only used if addon has its own demuxer.
     */
    PVR_ERROR SetSpeed(int speed);

    //@}
    /** @name PVR recording stream methods */
    //@{

    /*!
     * @brief Open a recording on the server.
     * @param recording The recording to open.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR OpenRecordedStream(const CPVRRecordingPtr &recording);

    /*!
     * @brief Close an open recording stream.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR CloseRecordedStream();

    /*!
     * @brief Read from an open recording stream.
     * @param lpBuf The buffer to store the data in.
     * @param uiBufSize The amount of bytes to read.
     * @param iRead The amount of bytes that were actually read from the stream.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR ReadRecordedStream(void* lpBuf, int64_t uiBufSize, int &iRead);

    /*!
     * @brief Seek in a recording stream on a backend.
     * @param iFilePosition The position to seek to.
     * @param iWhence ?
     * @param iPosition The new position or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR SeekRecordedStream(int64_t iFilePosition, int iWhence, int64_t &iPosition);

    /*!
     * @brief Get the lenght of the currently playing recording stream, if any.
     * @param iLength The total length of the stream that's currently being read or -1 on error.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR GetRecordedStreamLength(int64_t &iLength);

    /*!
     * @brief Fill the file item for a recording with the properties required for playback. Values are obtained from the PVR backend.
     * @param fileItem The file item to be filled.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR FillRecordingStreamFileItem(CFileItem &fileItem);

    //@}
    /** @name PVR demultiplexer methods */
    //@{

    /*!
     * @brief Reset the demultiplexer in the add-on.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR DemuxReset();

    /*!
     * @brief Abort the demultiplexer thread in the add-on.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR DemuxAbort();

    /*!
     * @brief Flush all data that's currently in the demultiplexer buffer in the add-on.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR DemuxFlush();

    /*!
     * @brief Read a packet from the demultiplexer.
     * @param packet The packet read.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR DemuxRead(DemuxPacket* &packet);

    static const char *ToString(const PVR_ERROR error);

    /*!
     * @brief Check whether the currently playing stream, if any, is a real-time stream.
     * @param bRealTime True if real-time, false otherwise.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR IsRealTimeStream(bool &bRealTime) const;

    /*!
     * @brief Get Stream times for the currently playing stream, if any (will be moved to inputstream).
     * @param times The stream times.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR GetStreamTimes(PVR_STREAM_TIMES *times);

    /*!
     * @brief reads the client's properties.
     * @return True on success, false otherwise.
     */
    bool GetAddonProperties(void);

    /*!
     * @brief Get the client's menu hooks.
     * @return The hooks. Guaranteed never to be null.
     */
    std::shared_ptr<CPVRClientMenuHooks> GetMenuHooks();

    /*!
     * @brief Call one of the menu hooks of the client.
     * @param hook The hook to call.
     * @param item The item associated with the hook to be called.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR CallMenuHook(const CPVRClientMenuHook &hook, const CFileItemPtr &item);

    /*!
     * @brief Propagate power management events to this add-on
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR OnSystemSleep();
    PVR_ERROR OnSystemWake();
    PVR_ERROR OnPowerSavingActivated();
    PVR_ERROR OnPowerSavingDeactivated();

    /*!
     * @brief Get the priority of this client. Larger value means higher priority.
     * @return The priority.
     */
    int GetPriority() const;

    /*!
     * @brief Set a new priority for this client.
     * @param iPriority The new priority.
     */
    void SetPriority(int iPriority);

    /*!
     * @brief Obtain the chunk size to use when reading streams.
     * @param iChunkSize the chunk size in bytes.
     * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
     */
    PVR_ERROR GetStreamReadChunkSize(int &iChunkSize);

    /*!
     * @brief Get the interface table used between addon and Kodi.
     * @todo This function will be removed after old callback library system is removed.
     */
    AddonInstance_PVR* GetInstanceInterface() { return &m_struct; }

  private:
    /*!
     * @brief Resets all class members to their defaults. Called by the constructors.
     */
    void ResetProperties(int iClientId = PVR_INVALID_CLIENT_ID);

    /*!
     * @brief Copy over group info from xbmcGroup to addonGroup.
     * @param xbmcGroup The group on XBMC's side.
     * @param addonGroup The group on the addon's side.
     */
    static void WriteClientGroupInfo(const CPVRChannelGroup &xbmcGroup, PVR_CHANNEL_GROUP &addonGroup);

    /*!
     * @brief Copy over recording info from xbmcRecording to addonRecording.
     * @param xbmcRecording The recording on XBMC's side.
     * @param addonRecording The recording on the addon's side.
     */
    static void WriteClientRecordingInfo(const CPVRRecording &xbmcRecording, PVR_RECORDING &addonRecording);

    /*!
     * @brief Copy over timer info from xbmcTimer to addonTimer.
     * @param xbmcTimer The timer on XBMC's side.
     * @param addonTimer The timer on the addon's side.
     */
    static void WriteClientTimerInfo(const CPVRTimerInfoTag &xbmcTimer, PVR_TIMER &addonTimer);

    /*!
     * @brief Copy over channel info from xbmcChannel to addonClient.
     * @param xbmcChannel The channel on XBMC's side.
     * @param addonChannel The channel on the addon's side.
     */
    static void WriteClientChannelInfo(const CPVRChannelPtr &xbmcChannel, PVR_CHANNEL &addonChannel);

    /*!
     * @brief Write the given addon properties to the properties of the given file item.
     * @param properties Pointer to an array of addon properties.
     * @param iPropertyCount The number of properties contained in the addon properties array.
     * @param fileItem The item the addon properties shall be written to.
     */
    static void WriteFileItemProperties(const PVR_NAMED_VALUE *properties, unsigned int iPropertyCount, CFileItem &fileItem);

    /*!
     * @brief Whether a channel can be played by this add-on
     * @param channel The channel to check.
     * @return True when it can be played, false otherwise.
     */
    bool CanPlayChannel(const CPVRChannelPtr &channel) const;

    /*!
     * @brief Stop this instance, if it is currently running.
     */
    void StopRunningInstance();

    /*!
     * @brief Wraps an addon function call in order to do common pre and post function invocation actions.
     * @param strFunctionName The function name, for logging purposes.
     * @param function The function to wrap. It has to have return type PVR_ERROR and must take one parameter of type const AddonInstance*.
     * @param bIsImplemented If false, this method will return PVR_ERROR_NOT_IMPLEMENTED.
     * @param bCheckReadyToUse If true, this method will check whether this instance is ready for use and return PVR_ERROR_SERVER_ERROR if it is not.
     * @return PVR_ERROR_NO_ERROR on success, any other PVR_ERROR_* value otherwise.
     */
    typedef KodiToAddonFuncTable_PVR AddonInstance;
    PVR_ERROR DoAddonCall(const char* strFunctionName,
                          std::function<PVR_ERROR(const AddonInstance*)> function,
                          bool bIsImplemented = true,
                          bool bCheckReadyToUse = true) const;

    /*!
     * @brief Callback functions from addon to kodi
     */
    //@{

    /*!
     * @brief Transfer a channel group from the add-on to Kodi. The group will be created if it doesn't exist.
     * @param kodiInstance Pointer to Kodi's CPVRClient class
     * @param handle The handle parameter that Kodi used when requesting the channel groups list
     * @param entry The entry to transfer to Kodi
     */
    static void cb_transfer_channel_group(void* kodiInstance, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP* entry);

    /*!
     * @brief Transfer a channel group member entry from the add-on to Kodi. The channel will be added to the group if the group can be found.
     * @param kodiInstance Pointer to Kodi's CPVRClient class
     * @param handle The handle parameter that Kodi used when requesting the channel group members list
     * @param entry The entry to transfer to Kodi
     */
    static void cb_transfer_channel_group_member(void* kodiInstance, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER* entry);

    /*!
     * @brief Transfer an EPG tag from the add-on to Kodi
     * @param kodiInstance Pointer to Kodi's CPVRClient class
     * @param handle The handle parameter that Kodi used when requesting the EPG data
     * @param entry The entry to transfer to Kodi
     */
    static void cb_transfer_epg_entry(void* kodiInstance, const ADDON_HANDLE handle, const EPG_TAG* entry);

    /*!
     * @brief Transfer a channel entry from the add-on to Kodi
     * @param kodiInstance Pointer to Kodi's CPVRClient class
     * @param handle The handle parameter that Kodi used when requesting the channel list
     * @param entry The entry to transfer to Kodi
     */
    static void cb_transfer_channel_entry(void* kodiInstance, const ADDON_HANDLE handle, const PVR_CHANNEL* entry);

    /*!
     * @brief Transfer a timer entry from the add-on to Kodi
     * @param kodiInstance Pointer to Kodi's CPVRClient class
     * @param handle The handle parameter that Kodi used when requesting the timers list
     * @param entry The entry to transfer to Kodi
     */
    static void cb_transfer_timer_entry(void* kodiInstance, const ADDON_HANDLE handle, const PVR_TIMER* entry);

    /*!
     * @brief Transfer a recording entry from the add-on to Kodi
     * @param kodiInstance Pointer to Kodi's CPVRClient class
     * @param handle The handle parameter that Kodi used when requesting the recordings list
     * @param entry The entry to transfer to Kodi
     */
    static void cb_transfer_recording_entry(void* kodiInstance, const ADDON_HANDLE handle, const PVR_RECORDING* entry);

    /*!
     * @brief Add or replace a menu hook for the context menu for this add-on
     * @param kodiInstance Pointer to Kodi's CPVRClient class
     * @param hook The hook to add.
     */
    static void cb_add_menu_hook(void* kodiInstance, PVR_MENUHOOK* hook);

    /*!
     * @brief Display a notification in Kodi that a recording started or stopped on the server
     * @param kodiInstance Pointer to Kodi's CPVRClient class
     * @param strName The name of the recording to display
     * @param strFileName The filename of the recording
     * @param bOnOff True when recording started, false when it stopped
     */
    static void cb_recording(void* kodiInstance, const char* strName, const char* strFileName, bool bOnOff);

    /*!
     * @brief Request Kodi to update it's list of channels
     * @param kodiInstance Pointer to Kodi's CPVRClient class
     */
    static void cb_trigger_channel_update(void* kodiInstance);

    /*!
     * @brief Request Kodi to update it's list of timers
     * @param kodiInstance Pointer to Kodi's CPVRClient class
     */
    static void cb_trigger_timer_update(void* kodiInstance);

    /*!
     * @brief Request Kodi to update it's list of recordings
     * @param kodiInstance Pointer to Kodi's CPVRClient class
     */
    static void cb_trigger_recording_update(void* kodiInstance);

    /*!
     * @brief Request Kodi to update it's list of channel groups
     * @param kodiInstance Pointer to Kodi's CPVRClient class
     */
    static void cb_trigger_channel_groups_update(void* kodiInstance);

    /*!
     * @brief Schedule an EPG update for the given channel channel
     * @param kodiInstance A pointer to the add-on
     * @param iChannelUid The unique id of the channel for this add-on
     */
    static void cb_trigger_epg_update(void* kodiInstance, unsigned int iChannelUid);

    /*!
     * @brief Free a packet that was allocated with AllocateDemuxPacket
     * @param kodiInstance Pointer to Kodi's CPVRClient class
     * @param pPacket The packet to free.
     */
    static void cb_free_demux_packet(void* kodiInstance, DemuxPacket* pPacket);

    /*!
     * @brief Allocate a demux packet. Free with FreeDemuxPacket
     * @param kodiInstance Pointer to Kodi's CPVRClient class.
     * @param iDataSize The size of the data that will go into the packet
     * @return The allocated packet.
     */
    static DemuxPacket* cb_allocate_demux_packet(void* kodiInstance, int iDataSize = 0);

    /*!
     * @brief Notify a state change for a PVR backend connection
     * @param kodiInstance Pointer to Kodi's CPVRClient class
     * @param strConnectionString The connection string reported by the backend that can be displayed in the UI.
     * @param newState The new state.
     * @param strMessage A localized addon-defined string representing the new state, that can be displayed
     *        in the UI or NULL if the Kodi-defined default string for the new state shall be displayed.
     */
    static void cb_connection_state_change(void* kodiInstance, const char* strConnectionString, PVR_CONNECTION_STATE newState, const char *strMessage);

    /*!
     * @brief Notify a state change for an EPG event
     * @param kodiInstance Pointer to Kodi's CPVRClient class
     * @param tag The EPG event.
     * @param newState The new state.
     * @param newState The new state. For EPG_EVENT_CREATED and EPG_EVENT_UPDATED, tag must be filled with all available
     *        event data, not just a delta. For EPG_EVENT_DELETED, it is sufficient to fill EPG_TAG.iUniqueBroadcastId
     */
    static void cb_epg_event_state_change(void* kodiInstance, EPG_TAG* tag, EPG_EVENT_STATE newState);

    /*! @todo remove the use complete from them, or add as generl function?!
     * Returns the ffmpeg codec id from given ffmpeg codec string name
     */
    static xbmc_codec_t cb_get_codec_by_name(const void* kodiInstance, const char* strCodecName);
    //@}

    std::atomic<bool>      m_bReadyToUse;          /*!< true if this add-on is initialised (ADDON_Create returned true), false otherwise */
    std::atomic<bool>      m_bBlockAddonCalls;     /*!< true if no add-on API calls are allowed */
    PVR_CONNECTION_STATE   m_connectionState;      /*!< the backend connection state */
    PVR_CONNECTION_STATE   m_prevConnectionState;  /*!< the previous backend connection state */
    bool                   m_ignoreClient;         /*!< signals to PVRManager to ignore this client until it has been connected */
    CPVRTimerTypes         m_timertypes;           /*!< timer types supported by this backend */
    int                    m_iClientId;            /*!< unique ID of the client */
    mutable int            m_iPriority;            /*!< priority of the client */
    mutable bool           m_bPriorityFetched;

    /* cached data */
    std::string            m_strBackendName;       /*!< the cached backend version */
    std::string            m_strBackendVersion;    /*!< the cached backend version */
    std::string            m_strConnectionString;  /*!< the cached connection string */
    std::string            m_strFriendlyName;      /*!< the cached friendly name */
    std::string            m_strBackendHostname;   /*!< the cached backend hostname */
    CPVRClientCapabilities m_clientCapabilities;   /*!< the cached add-on's capabilities */
    std::shared_ptr<CPVRClientMenuHooks> m_menuhooks; /*!< the menu hooks for this add-on */

    /* stored strings to make sure const char* members in PVR_PROPERTIES stay valid */
    std::string            m_strUserPath;         /*!< @brief translated path to the user profile */
    std::string            m_strClientPath;       /*!< @brief translated path to this add-on */

    mutable CCriticalSection m_critSection;

    AddonInstance_PVR m_struct;
  };
}
