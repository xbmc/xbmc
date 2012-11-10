#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "addons/Addon.h"
#include "addons/AddonDll.h"
#include "addons/DllPVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/recordings/PVRRecordings.h"

namespace EPG
{
  class CEpg;
}

namespace PVR
{
  class CPVRChannelGroup;
  class CPVRChannelGroupInternal;
  class CPVRChannelGroups;
  class CPVRTimers;
  class CPVRTimerInfoTag;
  class CPVRRecordings;
  class CPVREpgContainer;
  class CPVRClient;

  typedef std::vector<PVR_MENUHOOK> PVR_MENUHOOKS;
  typedef boost::shared_ptr<CPVRClient> PVR_CLIENT;
  #define PVR_INVALID_CLIENT_ID (-2)
  #define PVR_VIRTUAL_CLIENT_ID (-1)

  /*!
   * Interface from XBMC to a PVR add-on.
   *
   * Also translates XBMC's C++ structures to the addon's C structures.
   */
  class CPVRClient : public ADDON::CAddonDll<DllPVRClient, PVRClient, PVR_PROPERTIES>
  {
  public:
    CPVRClient(const ADDON::AddonProps& props);
    CPVRClient(const cp_extension_t *ext);
    ~CPVRClient(void);

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
     * @brief Destroy the instance of this add-on.
     */
    void Destroy(void);

    /*!
     * @brief Destroy and recreate this add-on.
     */
    void ReCreate(void);

    /*!
     * @return True if this instance is initialised, false otherwise.
     */
    bool ReadyToUse(void) const;

    /*!
     * @return The ID of this instance.
     */
    int GetID(void) const;

    //@}
    /** @name PVR server methods */
    //@{

    /*!
     * @brief Query this add-on's capabilities.
     * @return pCapabilities The add-on's capabilities.
     */
    PVR_ADDON_CAPABILITIES GetAddonCapabilities(void) const;

    /*!
     * @brief Get the stream properties of the stream that's currently being read.
     * @param pProperties The properties.
     * @return PVR_ERROR_NO_ERROR if the properties have been fetched successfully.
     */
    PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES *pProperties);

    /*!
     * @return The name reported by the backend.
     */
    CStdString GetBackendName(void) const;

    /*!
     * @return The version string reported by the backend.
     */
    CStdString GetBackendVersion(void) const;

    /*!
     * @return The connection string reported by the backend.
     */
    CStdString GetConnectionString(void) const;

    /*!
     * @return A friendly name for this add-on that can be used in log messages.
     */
    CStdString GetFriendlyName(void) const;

    /*!
     * @brief Get the disk space reported by the server.
     * @param iTotal The total disk space.
     * @param iUsed The used disk space.
     * @return PVR_ERROR_NO_ERROR if the drive space has been fetched successfully.
     */
    PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed);

    /*!
     * @brief Start a channel scan on the server.
     * @return PVR_ERROR_NO_ERROR if the channel scan has been started successfully.
     */
    PVR_ERROR StartChannelScan(void);

    /*!
     * @return True if this add-on has menu hooks, false otherwise.
     */
    bool HaveMenuHooks(PVR_MENUHOOK_CAT cat) const;

    /*!
     * @return The menu hooks for this add-on.
     */
    PVR_MENUHOOKS *GetMenuHooks(void);

    /*!
     * @brief Call one of the menu hooks of this client.
     * @param hook The hook to call.
     */
    void CallMenuHook(const PVR_MENUHOOK &hook);

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
    PVR_ERROR GetEPGForChannel(const CPVRChannel &channel, EPG::CEpg *epg, time_t start = 0, time_t end = 0, bool bSaveInDb = false);

    //@}
    /** @name PVR channel group methods */
    //@{

    /*!
      * @return The total amount of channel groups on the server or -1 on error.
      */
    int GetChannelGroupsAmount(void);

    /*!
     * @brief Request the list of all channel groups from the backend.
     * @param groups The groups container to get the groups for.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVR_ERROR GetChannelGroups(CPVRChannelGroups *groups);

    /*!
     * @brief Request the list of all group members from the backend.
     * @param groups The group to get the members for.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVR_ERROR GetChannelGroupMembers(CPVRChannelGroup *group);

    //@}
    /** @name PVR channel methods */
    //@{

    /*!
     * @return The total amount of channels on the server or -1 on error.
     */
    int GetChannelsAmount(void);

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
     * @return The total amount of channels on the server or -1 on error.
     */
    int GetRecordingsAmount(void);

    /*!
     * @brief Request the list of all recordings from the backend.
     * @param results The container to add the recordings to.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVR_ERROR GetRecordings(CPVRRecordings *results);

    /*!
     * @brief Delete a recording on the backend.
     * @param recording The recording to delete.
     * @return PVR_ERROR_NO_ERROR if the recording has been deleted successfully.
     */
    PVR_ERROR DeleteRecording(const CPVRRecording &recording);

    /*!
     * @brief Rename a recording on the backend.
     * @param recording The recording to rename.
     * @return PVR_ERROR_NO_ERROR if the recording has been renamed successfully.
     */
    PVR_ERROR RenameRecording(const CPVRRecording &recording);

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
    * @param position The last watched position in seconds
    * @return PVR_ERROR_NO_ERROR if the position has been stored successfully.
    */
    PVR_ERROR SetRecordingLastPlayedPosition(const CPVRRecording &recording, int lastplayedposition);

    /*!
    * @brief Retrieve the last watched position of a recording on the backend.
    * @param recording The recording.
    * @return The last watched position in seconds or -1 on error
    */
    int GetRecordingLastPlayedPosition(const CPVRRecording &recording);

    //@}
    /** @name PVR timer methods */
    //@{

    /*!
     * @return The total amount of timers on the backend or -1 on error.
     */
    int GetTimersAmount(void);

    /*!
     * @brief Request the list of all timers from the backend.
     * @param results The container to store the result in.
     * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
     */
    PVR_ERROR GetTimers(CPVRTimers *results);

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
     * @brief Rename a timer on the server.
     * @param timer The timer to rename.
     * @param strNewName The new name of the timer.
     * @return PVR_ERROR_NO_ERROR if the timer has been renamed successfully.
     */
    PVR_ERROR RenameTimer(const CPVRTimerInfoTag &timer, const CStdString &strNewName);

    /*!
     * @brief Update the timer information on the server.
     * @param timer The timer to update.
     * @return PVR_ERROR_NO_ERROR if the timer has been updated successfully.
     */
    PVR_ERROR UpdateTimer(const CPVRTimerInfoTag &timer);

    //@}
    /** @name PVR live stream methods */
    //@{

    /*!
     * @brief Open a live stream on the server.
     * @param channel The channel to stream.
     * @param bIsSwitchingChannel True when switching channels, false otherwise.
     * @return True if the stream opened successfully, false otherwise.
     */
    bool OpenStream(const CPVRChannel &channel, bool bIsSwitchingChannel);

    /*!
     * @brief Close an open live stream.
     */
    void CloseStream(void);

    /*!
     * @brief Read from an open live stream.
     * @param lpBuf The buffer to store the data in.
     * @param uiBufSize The amount of bytes to read.
     * @return The amount of bytes that were actually read from the stream.
     */
    int ReadStream(void* lpBuf, int64_t uiBufSize);

    /*!
     * @brief Seek in a live stream on a backend that supports timeshifting.
     * @param iFilePosition The position to seek to.
     * @param iWhence ?
     * @return The new position.
     */
    int64_t SeekStream(int64_t iFilePosition, int iWhence = SEEK_SET);

    /*!
     * @return The position in the stream that's currently being read.
     */
    int64_t GetStreamPosition(void);

    /*!
     * @return The total length of the stream that's currently being read.
     */
    int64_t GetStreamLength(void);

    /*!
     * @brief (Un)Pause a stream
     */
    void PauseStream(bool bPaused);

    /*!
     * @return The channel number on the server of the live stream that's currently being read.
     */
    int GetCurrentClientChannel(void);

    /*!
     * @brief Switch to another channel. Only to be called when a live stream has already been opened.
     * @param channel The channel to switch to.
     * @return True if the switch was successful, false otherwise.
     */
    bool SwitchChannel(const CPVRChannel &channel);

    /*!
     * @brief Get the signal quality of the stream that's currently open.
     * @param qualityinfo The signal quality.
     * @return True if the signal quality has been read successfully, false otherwise.
     */
    bool SignalQuality(PVR_SIGNAL_STATUS &qualityinfo);

    /*!
     * @brief Get the stream URL for a channel from the server. Used by the MediaPortal add-on.
     * @param channel The channel to get the stream URL for.
     * @return The requested URL.
     */
    CStdString GetLiveStreamURL(const CPVRChannel &channel);

    /*!
     * @brief Check whether PVR backend supports pausing the currently playing stream
     */
    bool CanPauseStream(void) const;

    /*!
     * @brief Check whether PVR backend supports seeking for the currently playing stream
     */
    bool CanSeekStream(void) const;

    /*!
     * Notify the pvr addon/demuxer that XBMC wishes to seek the stream by time
     * @param time The absolute time since stream start
     * @param backwards True to seek to keyframe BEFORE time, else AFTER
     * @param startpts can be updated to point to where display should start
     * @return True if the seek operation was possible
     * @remarks Optional, and only used if addon has its own demuxer. Return False if this add-on won't provide this function.
     */
    bool SeekTime(int time, bool backwards, double *startpts);

    /*!
     * Notify the pvr addon/demuxer that XBMC wishes to change playback speed
     * @param speed The requested playback speed
     * @remarks Optional, and only used if addon has its own demuxer.
     */
    void SetSpeed(int speed);

    //@}
    /** @name PVR recording stream methods */
    //@{

    /*!
     * @brief Open a recording on the server.
     * @param recording The recording to open.
     * @return True if the stream has been opened succesfully, false otherwise.
     */
    bool OpenStream(const CPVRRecording &recording);

    //@}
    /** @name PVR demultiplexer methods */
    //@{

    /*!
     * @brief Reset the demultiplexer in the add-on.
     */
    void DemuxReset(void);

    /*!
     * @brief Abort the demultiplexer thread in the add-on.
     */
    void DemuxAbort(void);

    /*!
     * @brief Flush all data that's currently in the demultiplexer buffer in the add-on.
     */
    void DemuxFlush(void);

    /*!
     * @brief Read a packet from the demultiplexer.
     * @return The packet.
     */
    DemuxPacket *DemuxRead(void);

    //@}

    bool SupportsChannelGroups(void) const;
    bool SupportsChannelScan(void) const;
    bool SupportsEPG(void) const;
    bool SupportsLastPlayedPosition(void) const;
    bool SupportsRadio(void) const;
    bool SupportsRecordings(void) const;
    bool SupportsRecordingFolders(void) const;
    bool SupportsRecordingPlayCount(void) const;
    bool SupportsTimers(void) const;
    bool SupportsTV(void) const;
    bool HandlesDemuxing(void) const;
    bool HandlesInputStream(void) const;

    bool IsPlayingLiveStream(void) const;
    bool IsPlayingLiveTV(void) const;
    bool IsPlayingLiveRadio(void) const;
    bool IsPlayingEncryptedChannel(void) const;
    bool IsPlayingRecording(void) const;
    bool IsPlaying(void) const;
    bool GetPlayingChannel(CPVRChannelPtr &channel) const;
    bool GetPlayingRecording(CPVRRecording &recording) const;

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

    /*!
     * @brief Update the signal status for the tv stream that's currently being read.
     */
    void UpdateCharInfoSignalStatus(void);

    //@}

    static const char *ToString(const PVR_ERROR error);

  private:
    /*!
     * @brief Checks whether the provided API version is compatible with XBMC
     * @param minVersion The add-on's XBMC_PVR_MIN_API_VERSION version
     * @param version The add-on's XBMC_PVR_API_VERSION version
     * @return True when compatible, false otherwise
     */
    static bool IsCompatibleAPIVersion(const ADDON::AddonVersion &minVersion, const ADDON::AddonVersion &version);

    /*!
     * @brief Reset the signal quality data to the initial values.
     */
    void ResetQualityData(PVR_SIGNAL_STATUS &qualityInfo);

    /*!
     * @brief Resets all class members to their defaults. Called by the constructors.
     */
    void ResetProperties(int iClientId = PVR_INVALID_CLIENT_ID);

    bool GetAddonProperties(void);

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
    static void WriteClientChannelInfo(const CPVRChannel &xbmcChannel, PVR_CHANNEL &addonChannel);

    /*!
     * @brief Whether a channel can be played by this add-on
     * @param channel The channel to check.
     * @return True when it can be played, false otherwise.
     */
    bool CanPlayChannel(const CPVRChannel &channel) const;

    bool LogError(const PVR_ERROR error, const char *strMethod) const;
    void LogException(const std::exception &e, const char *strFunctionName) const;

    bool                   m_bReadyToUse;          /*!< true if this add-on is connected to the backend, false otherwise */
    CStdString             m_strHostName;          /*!< the host name */
    PVR_MENUHOOKS          m_menuhooks;            /*!< the menu hooks for this add-on */
    int                    m_iClientId;            /*!< database ID of the client */

    /* cached data */
    CStdString             m_strBackendName;       /*!< the cached backend version */
    bool                   m_bGotBackendName;      /*!< true if the backend name has already been fetched */
    CStdString             m_strBackendVersion;    /*!< the cached backend version */
    bool                   m_bGotBackendVersion;   /*!< true if the backend version has already been fetched */
    CStdString             m_strConnectionString;  /*!< the cached connection string */
    bool                   m_bGotConnectionString; /*!< true if the connection string has already been fetched */
    CStdString             m_strFriendlyName;      /*!< the cached friendly name */
    bool                   m_bGotFriendlyName;     /*!< true if the friendly name has already been fetched */
    PVR_ADDON_CAPABILITIES m_addonCapabilities;     /*!< the cached add-on capabilities */
    bool                   m_bGotAddonCapabilities; /*!< true if the add-on capabilities have already been fetched */
    PVR_SIGNAL_STATUS      m_qualityInfo;           /*!< stream quality information */

    /* stored strings to make sure const char* members in PVR_PROPERTIES stay valid */
    std::string m_strUserPath;    /*!< @brief translated path to the user profile */
    std::string m_strClientPath;  /*!< @brief translated path to this add-on */

    CCriticalSection m_critSection;

    bool           m_bIsPlayingTV;
    CPVRChannelPtr m_playingChannel;
    bool           m_bIsPlayingRecording;
    CPVRRecording  m_playingRecording;
    ADDON::AddonVersion m_apiVersion;
    bool           m_bCanPauseStream;
    bool           m_bCanSeekStream;
  };
}
