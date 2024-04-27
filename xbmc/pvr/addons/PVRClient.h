/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/binary-addons/AddonInstanceHandler.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr.h"
#include "pvr/addons/PVRClientCapabilities.h"
#include "threads/Event.h"

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

struct DemuxPacket;

namespace PVR
{
class CPVRChannel;
class CPVRChannelGroup;
class CPVRChannelGroupMember;
class CPVRChannelGroups;
class CPVRProvider;
class CPVRProvidersContainer;
class CPVRClientMenuHook;
class CPVRClientMenuHooks;
class CPVREpg;
class CPVREpgInfoTag;
class CPVRRecording;
class CPVRRecordings;
class CPVRStreamProperties;
class CPVRTimerInfoTag;
class CPVRTimerType;
class CPVRTimersContainer;

#define PVR_INVALID_CLIENT_ID (-2)

/*!
 * Interface from Kodi to a PVR add-on.
 *
 * Also translates Kodi's C++ structures to the add-on's C structures.
 */
class CPVRClient : public ADDON::IAddonInstanceHandler
{
public:
  CPVRClient(const ADDON::AddonInfoPtr& addonInfo, ADDON::AddonInstanceId instanceId, int clientId);
  ~CPVRClient() override;

  void OnPreInstall() override;
  void OnPreUnInstall() override;

  /** @name PVR add-on methods */
  //@{

  /*!
   * @brief Initialise the instance of this add-on.
   */
  ADDON_STATUS Create();

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
  void Destroy();

  /*!
   * @brief Destroy and recreate this add-on.
   */
  void ReCreate();

  /*!
   * @return True if this instance is initialised (ADDON_Create returned true), false otherwise.
   */
  bool ReadyToUse() const;

  /*!
   * @brief Gets the backend connection state.
   * @return the backend connection state.
   */
  PVR_CONNECTION_STATE GetConnectionState() const;

  /*!
   * @brief Sets the backend connection state.
   * @param state the new backend connection state.
   */
  void SetConnectionState(PVR_CONNECTION_STATE state);

  /*!
   * @brief Gets the backend's previous connection state.
   * @return the backend's previous connection state.
   */
  PVR_CONNECTION_STATE GetPreviousConnectionState() const;

  /*!
   * @brief Check whether this client should be ignored.
   * @return True if this client should be ignored, false otherwise.
   */
  bool IgnoreClient() const;

  /*!
   * @brief Check whether this client is enabled, according to its instance/add-on configuration.
   * @return True if this client is enabled, false otherwise.
   */
  bool IsEnabled() const;

  /*!
   * @return The ID of this instance.
   */
  int GetID() const;

  //@}
  /** @name PVR server methods */
  //@{

  /*!
   * @brief Query this add-on's capabilities.
   * @return The add-on's capabilities.
   */
  const CPVRClientCapabilities& GetClientCapabilities() const { return m_clientCapabilities; }

  /*!
   * @brief Get the stream properties of the stream that's currently being read.
   * @param pProperties The properties.
   * @return PVR_ERROR_NO_ERROR if the properties have been fetched successfully.
   */
  PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties) const;

  /*!
   * @return The name reported by the backend.
   */
  const std::string& GetBackendName() const;

  /*!
   * @return The version string reported by the backend.
   */
  const std::string& GetBackendVersion() const;

  /*!
   * @brief the ip address or alias of the pvr backend server
   */
  const std::string& GetBackendHostname() const;

  /*!
   * @return The connection string reported by the backend.
   */
  const std::string& GetConnectionString() const;

  /*!
   * @brief A friendly name used to uniquely identify the addon instance
   * @return string that can be used in log messages and the GUI.
   */
  const std::string GetFriendlyName() const;

  /*!
   * @brief The name used by the PVR client addon instance
   * @return string that can be used in log messages and the GUI.
   */
  std::string GetInstanceName() const;

  /*!
   * @brief Get the disk space reported by the server.
   * @param iTotal The total disk space.
   * @param iUsed The used disk space.
   * @return PVR_ERROR_NO_ERROR if the drive space has been fetched successfully.
   */
  PVR_ERROR GetDriveSpace(uint64_t& iTotal, uint64_t& iUsed) const;

  /*!
   * @brief Start a channel scan on the server.
   * @return PVR_ERROR_NO_ERROR if the channel scan has been started successfully.
   */
  PVR_ERROR StartChannelScan();

  /*!
   * @brief Request the client to open dialog about given channel to add
   * @param channel The channel to add
   * @return PVR_ERROR_NO_ERROR if the add has been fetched successfully.
   */
  PVR_ERROR OpenDialogChannelAdd(const std::shared_ptr<const CPVRChannel>& channel);

  /*!
   * @brief Request the client to open dialog about given channel settings
   * @param channel The channel to edit
   * @return PVR_ERROR_NO_ERROR if the edit has been fetched successfully.
   */
  PVR_ERROR OpenDialogChannelSettings(const std::shared_ptr<const CPVRChannel>& channel);

  /*!
   * @brief Request the client to delete given channel
   * @param channel The channel to delete
   * @return PVR_ERROR_NO_ERROR if the delete has been fetched successfully.
   */
  PVR_ERROR DeleteChannel(const std::shared_ptr<const CPVRChannel>& channel);

  /*!
   * @brief Request the client to rename given channel
   * @param channel The channel to rename
   * @return PVR_ERROR_NO_ERROR if the rename has been fetched successfully.
   */
  PVR_ERROR RenameChannel(const std::shared_ptr<const CPVRChannel>& channel);

  /*
   * @brief Check if an epg tag can be recorded
   * @param tag The epg tag
   * @param bIsRecordable Set to true if the tag can be recorded
   * @return PVR_ERROR_NO_ERROR if bIsRecordable has been set successfully.
   */
  PVR_ERROR IsRecordable(const std::shared_ptr<const CPVREpgInfoTag>& tag,
                         bool& bIsRecordable) const;

  /*
   * @brief Check if an epg tag can be played
   * @param tag The epg tag
   * @param bIsPlayable Set to true if the tag can be played
   * @return PVR_ERROR_NO_ERROR if bIsPlayable has been set successfully.
   */
  PVR_ERROR IsPlayable(const std::shared_ptr<const CPVREpgInfoTag>& tag, bool& bIsPlayable) const;

  /*!
   * @brief Fill the given container with the properties required for playback
   * of the given EPG tag. Values are obtained from the PVR backend.
   *
   * @param tag The EPG tag.
   * @param props The container to be filled with the stream properties.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetEpgTagStreamProperties(const std::shared_ptr<const CPVREpgInfoTag>& tag,
                                      CPVRStreamProperties& props) const;

  //@}
  /** @name PVR EPG methods */
  //@{

  /*!
   * @brief Request an EPG table for a channel from the client.
   * @param iChannelUid The UID of the channel to get the EPG table for.
   * @param epg The table to write the data to.
   * @param start The start time to use.
   * @param end The end time to use.
   * @return PVR_ERROR_NO_ERROR if the table has been fetched successfully.
   */
  PVR_ERROR GetEPGForChannel(int iChannelUid, CPVREpg* epg, time_t start, time_t end) const;

  /*!
   * @brief Tell the client the past time frame to use when notifying epg events back
   * to Kodi.
   *
   * The client might push epg events asynchronously to Kodi using the callback
   * function EpgEventStateChange. To be able to only push events that are
   * actually of interest for Kodi, client needs to know about the past epg time
   * frame Kodi uses.
   *
   * @param[in] iPastDays number of days before "now".
                @ref EPG_TIMEFRAME_UNLIMITED means that Kodi is interested in all epg events,
                regardless of event times.
   * @return PVR_ERROR_NO_ERROR if new value was successfully set.
   */
  PVR_ERROR SetEPGMaxPastDays(int iPastDays);

  /*!
   * @brief Tell the client the future time frame to use when notifying epg events back
   * to Kodi.
   *
   * The client might push epg events asynchronously to Kodi using the callback
   * function EpgEventStateChange. To be able to only push events that are
   * actually of interest for Kodi, client needs to know about the future epg time
   * frame Kodi uses.
   *
   * @param[in] iFutureDays number of days after "now".
                @ref EPG_TIMEFRAME_UNLIMITED means that Kodi is interested in all epg events,
                regardless of event times.
   * @return PVR_ERROR_NO_ERROR if new value was successfully set.
   */
  PVR_ERROR SetEPGMaxFutureDays(int iFutureDays);

  //@}
  /** @name PVR channel group methods */
  //@{

  /*!
   * @brief Get the total amount of channel groups from the backend.
   * @param iGroups The total amount of channel groups on the server or -1 on error.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetChannelGroupsAmount(int& iGroups) const;

  /*!
   * @brief Request the list of all channel groups from the backend.
   * @param groups The groups container to get the groups for.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   */
  PVR_ERROR GetChannelGroups(CPVRChannelGroups* groups) const;

  /*!
   * @brief Request the list of all group members from the backend.
   * @param group The group to get the members for.
   * @param groupMembers The container for the group members.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   */
  PVR_ERROR GetChannelGroupMembers(
      CPVRChannelGroup* group,
      std::vector<std::shared_ptr<CPVRChannelGroupMember>>& groupMembers) const;

  //@}
  /** @name PVR channel methods */
  //@{

  /*!
   * @brief Get the total amount of channels from the backend.
   * @param iChannels The total amount of channels on the server or -1 on error.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetChannelsAmount(int& iChannels) const;

  /*!
   * @brief Request the list of all channels from the backend.
   * @param bRadio True to get the radio channels, false to get the TV channels.
   * @param channels The container for the channels.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   */
  PVR_ERROR GetChannels(bool bRadio, std::vector<std::shared_ptr<CPVRChannel>>& channels) const;

  /*!
   * @brief Get the total amount of providers from the backend.
   * @param iChannels The total amount of channels on the server or -1 on error.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetProvidersAmount(int& iProviders) const;

  /*!
   * @brief Request the list of all providers from the backend.
   * @param providers The providers list to add the providers to.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   */
  PVR_ERROR GetProviders(CPVRProvidersContainer& providers) const;

  //@}
  /** @name PVR recording methods */
  //@{

  /*!
   * @brief Get the total amount of recordings from the backend.
   * @param deleted True to return deleted recordings.
   * @param iRecordings The total amount of recordings on the server or -1 on error.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetRecordingsAmount(bool deleted, int& iRecordings) const;

  /*!
   * @brief Request the list of all recordings from the backend.
   * @param results The container to add the recordings to.
   * @param deleted True to return deleted recordings.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   */
  PVR_ERROR GetRecordings(CPVRRecordings* results, bool deleted) const;

  /*!
   * @brief Delete a recording on the backend.
   * @param recording The recording to delete.
   * @return PVR_ERROR_NO_ERROR if the recording has been deleted successfully.
   */
  PVR_ERROR DeleteRecording(const CPVRRecording& recording);

  /*!
   * @brief Undelete a recording on the backend.
   * @param recording The recording to undelete.
   * @return PVR_ERROR_NO_ERROR if the recording has been undeleted successfully.
   */
  PVR_ERROR UndeleteRecording(const CPVRRecording& recording);

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
  PVR_ERROR RenameRecording(const CPVRRecording& recording);

  /*!
   * @brief Set the lifetime of a recording on the backend.
   * @param recording The recording to set the lifetime for. recording.m_iLifetime contains the new lifetime value.
   * @return PVR_ERROR_NO_ERROR if the recording's lifetime has been set successfully.
   */
  PVR_ERROR SetRecordingLifetime(const CPVRRecording& recording);

  /*!
   * @brief Set the play count of a recording on the backend.
   * @param recording The recording to set the play count.
   * @param count Play count.
   * @return PVR_ERROR_NO_ERROR if the recording's play count has been set successfully.
   */
  PVR_ERROR SetRecordingPlayCount(const CPVRRecording& recording, int count);

  /*!
   * @brief Set the last watched position of a recording on the backend.
   * @param recording The recording.
   * @param lastplayedposition The last watched position in seconds
   * @return PVR_ERROR_NO_ERROR if the position has been stored successfully.
   */
  PVR_ERROR SetRecordingLastPlayedPosition(const CPVRRecording& recording, int lastplayedposition);

  /*!
   * @brief Retrieve the last watched position of a recording on the backend.
   * @param recording The recording.
   * @param iPosition The last watched position in seconds or -1 on error
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetRecordingLastPlayedPosition(const CPVRRecording& recording, int& iPosition) const;

  /*!
   * @brief Retrieve the edit decision list (EDL) from the backend.
   * @param recording The recording.
   * @param edls The edit decision list (empty on error).
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetRecordingEdl(const CPVRRecording& recording, std::vector<PVR_EDL_ENTRY>& edls) const;

  /*!
   * @brief Retrieve the size of a recording on the backend.
   * @param recording The recording.
   * @param sizeInBytes The size in bytes
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetRecordingSize(const CPVRRecording& recording, int64_t& sizeInBytes) const;

  /*!
   * @brief Retrieve the edit decision list (EDL) from the backend.
   * @param epgTag The EPG tag.
   * @param edls The edit decision list (empty on error).
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetEpgTagEdl(const std::shared_ptr<const CPVREpgInfoTag>& epgTag,
                         std::vector<PVR_EDL_ENTRY>& edls) const;

  //@}
  /** @name PVR timer methods */
  //@{

  /*!
   * @brief Get the total amount of timers from the backend.
   * @param iTimers The total amount of timers on the backend or -1 on error.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetTimersAmount(int& iTimers) const;

  /*!
   * @brief Request the list of all timers from the backend.
   * @param results The container to store the result in.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   */
  PVR_ERROR GetTimers(CPVRTimersContainer* results) const;

  /*!
   * @brief Add a timer on the backend.
   * @param timer The timer to add.
   * @return PVR_ERROR_NO_ERROR if the timer has been added successfully.
   */
  PVR_ERROR AddTimer(const CPVRTimerInfoTag& timer);

  /*!
   * @brief Delete a timer on the backend.
   * @param timer The timer to delete.
   * @param bForce Set to true to delete a timer that is currently recording a program.
   * @return PVR_ERROR_NO_ERROR if the timer has been deleted successfully.
   */
  PVR_ERROR DeleteTimer(const CPVRTimerInfoTag& timer, bool bForce = false);

  /*!
   * @brief Update the timer information on the server.
   * @param timer The timer to update.
   * @return PVR_ERROR_NO_ERROR if the timer has been updated successfully.
   */
  PVR_ERROR UpdateTimer(const CPVRTimerInfoTag& timer);

  /*!
   * @brief Update all timer types supported by the backend.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   */
  PVR_ERROR UpdateTimerTypes();

  /*!
   * @brief Get the timer types supported by the backend, without updating them from the backend.
   * @return the types.
   */
  const std::vector<std::shared_ptr<CPVRTimerType>>& GetTimerTypes() const;

  //@}
  /** @name PVR live stream methods */
  //@{

  /*!
   * @brief Open a live stream on the server.
   * @param channel The channel to stream.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR OpenLiveStream(const std::shared_ptr<const CPVRChannel>& channel);

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
  PVR_ERROR ReadLiveStream(void* lpBuf, int64_t uiBufSize, int& iRead);

  /*!
   * @brief Seek in a live stream on a backend.
   * @param iFilePosition The position to seek to.
   * @param iWhence ?
   * @param iPosition The new position or -1 on error.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR SeekLiveStream(int64_t iFilePosition, int iWhence, int64_t& iPosition);

  /*!
   * @brief Get the length of the currently playing live stream, if any.
   * @param iLength The total length of the stream that's currently being read or -1 on error.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetLiveStreamLength(int64_t& iLength) const;

  /*!
   * @brief (Un)Pause a stream.
   * @param bPaused True to pause the stream, false to unpause.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR PauseStream(bool bPaused);

  /*!
   * @brief Get the signal quality of the stream that's currently open.
   * @param channelUid Channel unique identifier
   * @param qualityinfo The signal quality.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR SignalQuality(int channelUid, PVR_SIGNAL_STATUS& qualityinfo) const;

  /*!
   * @brief Get the descramble information of the stream that's currently open.
   * @param channelUid Channel unique identifier
   * @param descrambleinfo The descramble information.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetDescrambleInfo(int channelUid, PVR_DESCRAMBLE_INFO& descrambleinfo) const;

  /*!
   * @brief Fill the given container with the properties required for playback of the given channel. Values are obtained from the PVR backend.
   * @param channel The channel.
   * @param props The container to be filled with the stream properties.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetChannelStreamProperties(const std::shared_ptr<const CPVRChannel>& channel,
                                       CPVRStreamProperties& props) const;

  /*!
   * @brief Check whether PVR backend supports pausing the currently playing stream
   * @param bCanPause True if the stream can be paused, false otherwise.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR CanPauseStream(bool& bCanPause) const;

  /*!
   * @brief Check whether PVR backend supports seeking for the currently playing stream
   * @param bCanSeek True if the stream can be seeked, false otherwise.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR CanSeekStream(bool& bCanSeek) const;

  /*!
   * @brief Notify the pvr addon/demuxer that Kodi wishes to seek the stream by time
   * @param time The absolute time since stream start
   * @param backwards True to seek to keyframe BEFORE time, else AFTER
   * @param startpts can be updated to point to where display should start
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   * @remarks Optional, and only used if addon has its own demuxer.
   */
  PVR_ERROR SeekTime(double time, bool backwards, double* startpts);

  /*!
   * @brief Notify the pvr addon/demuxer that Kodi wishes to change playback speed
   * @param speed The requested playback speed
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   * @remarks Optional, and only used if addon has its own demuxer.
   */
  PVR_ERROR SetSpeed(int speed);

  /*!
   * @brief Notify the pvr addon/demuxer that Kodi wishes to fill demux queue
   * @param mode for setting on/off
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   * @remarks Optional, and only used if addon has its own demuxer.
   */
  PVR_ERROR FillBuffer(bool mode);

  //@}
  /** @name PVR recording stream methods */
  //@{

  /*!
   * @brief Open a recording on the server.
   * @param recording The recording to open.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR OpenRecordedStream(const std::shared_ptr<const CPVRRecording>& recording);

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
  PVR_ERROR ReadRecordedStream(void* lpBuf, int64_t uiBufSize, int& iRead);

  /*!
   * @brief Seek in a recording stream on a backend.
   * @param iFilePosition The position to seek to.
   * @param iWhence ?
   * @param iPosition The new position or -1 on error.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR SeekRecordedStream(int64_t iFilePosition, int iWhence, int64_t& iPosition);

  /*!
   * @brief Get the length of the currently playing recording stream, if any.
   * @param iLength The total length of the stream that's currently being read or -1 on error.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetRecordedStreamLength(int64_t& iLength) const;

  /*!
   * @brief Fill the given container with the properties required for playback of the given recording. Values are obtained from the PVR backend.
   * @param recording The recording.
   * @param props The container to be filled with the stream properties.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetRecordingStreamProperties(const std::shared_ptr<const CPVRRecording>& recording,
                                         CPVRStreamProperties& props) const;

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
  PVR_ERROR DemuxRead(DemuxPacket*& packet);

  static const char* ToString(const PVR_ERROR error);

  /*!
   * @brief Check whether the currently playing stream, if any, is a real-time stream.
   * @param bRealTime True if real-time, false otherwise.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR IsRealTimeStream(bool& bRealTime) const;

  /*!
   * @brief Get Stream times for the currently playing stream, if any (will be moved to inputstream).
   * @param times The stream times.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR GetStreamTimes(PVR_STREAM_TIMES* times) const;

  /*!
   * @brief Get the client's menu hooks.
   * @return The hooks. Guaranteed never to be nullptr.
   */
  std::shared_ptr<CPVRClientMenuHooks> GetMenuHooks() const;

  /*!
   * @brief Call one of the EPG tag menu hooks of the client.
   * @param hook The hook to call.
   * @param tag The EPG tag associated with the hook to be called.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR CallEpgTagMenuHook(const CPVRClientMenuHook& hook,
                               const std::shared_ptr<const CPVREpgInfoTag>& tag);

  /*!
   * @brief Call one of the channel menu hooks of the client.
   * @param hook The hook to call.
   * @param tag The channel associated with the hook to be called.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR CallChannelMenuHook(const CPVRClientMenuHook& hook,
                                const std::shared_ptr<const CPVRChannel>& channel);

  /*!
   * @brief Call one of the recording menu hooks of the client.
   * @param hook The hook to call.
   * @param tag The recording associated with the hook to be called.
   * @param bDeleted True, if the recording is deleted (trashed), false otherwise
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR CallRecordingMenuHook(const CPVRClientMenuHook& hook,
                                  const std::shared_ptr<const CPVRRecording>& recording,
                                  bool bDeleted);

  /*!
   * @brief Call one of the timer menu hooks of the client.
   * @param hook The hook to call.
   * @param tag The timer associated with the hook to be called.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR CallTimerMenuHook(const CPVRClientMenuHook& hook,
                              const std::shared_ptr<const CPVRTimerInfoTag>& timer);

  /*!
   * @brief Call one of the settings menu hooks of the client.
   * @param hook The hook to call.
   * @return PVR_ERROR_NO_ERROR on success, respective error code otherwise.
   */
  PVR_ERROR CallSettingsMenuHook(const CPVRClientMenuHook& hook);

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
  PVR_ERROR GetStreamReadChunkSize(int& iChunkSize) const;

  /*!
   * @brief Get the interface table used between addon and Kodi.
   * @todo This function will be removed after old callback library system is removed.
   */
  AddonInstance_PVR* GetInstanceInterface() { return m_ifc.pvr; }

private:
  /*!
   * @brief Resets all class members to their defaults, accept the client id.
   */
  void ResetProperties();

  /*!
   * @brief reads the client's properties.
   * @return True on success, false otherwise.
   */
  bool GetAddonProperties();

  /*!
   * @brief reads the client's name string properties
   * @return True on success, false otherwise.
   */
  bool GetAddonNameStringProperties();

  /*!
   * @brief Write the given addon properties to the given properties container.
   * @param properties Pointer to an array of addon properties.
   * @param iPropertyCount The number of properties contained in the addon properties array.
   * @param props The container the addon properties shall be written to.
   */
  static void WriteStreamProperties(const PVR_NAMED_VALUE* properties,
                                    unsigned int iPropertyCount,
                                    CPVRStreamProperties& props);

  /*!
   * @brief Whether a channel can be played by this add-on
   * @param channel The channel to check.
   * @return True when it can be played, false otherwise.
   */
  bool CanPlayChannel(const std::shared_ptr<const CPVRChannel>& channel) const;

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
  typedef AddonInstance_PVR AddonInstance;
  PVR_ERROR DoAddonCall(const char* strFunctionName,
                        const std::function<PVR_ERROR(const AddonInstance*)>& function,
                        bool bIsImplemented = true,
                        bool bCheckReadyToUse = true) const;

  /*!
   * @brief Wraps an addon callback function call in order to do common pre and post function invocation actions.
   * @param strFunctionName The function name, for logging purposes.
   * @param kodiInstance The addon instance pointer.
   * @param function The function to wrap. It must take one parameter of type CPVRClient*.
   * @param bForceCall If true, make the call, ignoring client's state.
   */
  static void HandleAddonCallback(const char* strFunctionName,
                                  void* kodiInstance,
                                  const std::function<void(CPVRClient* client)>& function,
                                  bool bForceCall = false);

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
  static void cb_transfer_channel_group(void* kodiInstance,
                                        const PVR_HANDLE handle,
                                        const PVR_CHANNEL_GROUP* entry);

  /*!
   * @brief Transfer a channel group member entry from the add-on to Kodi. The channel will be added to the group if the group can be found.
   * @param kodiInstance Pointer to Kodi's CPVRClient class
   * @param handle The handle parameter that Kodi used when requesting the channel group members list
   * @param entry The entry to transfer to Kodi
   */
  static void cb_transfer_channel_group_member(void* kodiInstance,
                                               const PVR_HANDLE handle,
                                               const PVR_CHANNEL_GROUP_MEMBER* entry);

  /*!
   * @brief Transfer an EPG tag from the add-on to Kodi
   * @param kodiInstance Pointer to Kodi's CPVRClient class
   * @param handle The handle parameter that Kodi used when requesting the EPG data
   * @param entry The entry to transfer to Kodi
   */
  static void cb_transfer_epg_entry(void* kodiInstance,
                                    const PVR_HANDLE handle,
                                    const EPG_TAG* entry);

  /*!
   * @brief Transfer a channel entry from the add-on to Kodi
   * @param kodiInstance Pointer to Kodi's CPVRClient class
   * @param handle The handle parameter that Kodi used when requesting the channel list
   * @param entry The entry to transfer to Kodi
   */
  static void cb_transfer_channel_entry(void* kodiInstance,
                                        const PVR_HANDLE handle,
                                        const PVR_CHANNEL* entry);

  /*!
   * @brief Transfer a provider entry from the add-on to Kodi
   * @param kodiInstance Pointer to Kodi's CPVRClient class
   * @param handle The handle parameter that Kodi used when requesting the channel list
   * @param entry The entry to transfer to Kodi
   */
  static void cb_transfer_provider_entry(void* kodiInstance,
                                         const PVR_HANDLE handle,
                                         const PVR_PROVIDER* entry);

  /*!
   * @brief Transfer a timer entry from the add-on to Kodi
   * @param kodiInstance Pointer to Kodi's CPVRClient class
   * @param handle The handle parameter that Kodi used when requesting the timers list
   * @param entry The entry to transfer to Kodi
   */
  static void cb_transfer_timer_entry(void* kodiInstance,
                                      const PVR_HANDLE handle,
                                      const PVR_TIMER* entry);

  /*!
   * @brief Transfer a recording entry from the add-on to Kodi
   * @param kodiInstance Pointer to Kodi's CPVRClient class
   * @param handle The handle parameter that Kodi used when requesting the recordings list
   * @param entry The entry to transfer to Kodi
   */
  static void cb_transfer_recording_entry(void* kodiInstance,
                                          const PVR_HANDLE handle,
                                          const PVR_RECORDING* entry);

  /*!
   * @brief Add or replace a menu hook for the context menu for this add-on
   * @param kodiInstance Pointer to Kodi's CPVRClient class
   * @param hook The hook to add.
   */
  static void cb_add_menu_hook(void* kodiInstance, const PVR_MENUHOOK* hook);

  /*!
   * @brief Display a notification in Kodi that a recording started or stopped on the server
   * @param kodiInstance Pointer to Kodi's CPVRClient class
   * @param strName The name of the recording to display
   * @param strFileName The filename of the recording
   * @param bOnOff True when recording started, false when it stopped
   */
  static void cb_recording_notification(void* kodiInstance,
                                        const char* strName,
                                        const char* strFileName,
                                        bool bOnOff);

  /*!
   * @brief Request Kodi to update it's list of channels
   * @param kodiInstance Pointer to Kodi's CPVRClient class
   */
  static void cb_trigger_channel_update(void* kodiInstance);

  /*!
   * @brief Request Kodi to update it's list of providers
   * @param kodiInstance Pointer to Kodi's CPVRClient class
   */
  static void cb_trigger_provider_update(void* kodiInstance);

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
  static void cb_free_demux_packet(void* kodiInstance, DEMUX_PACKET* pPacket);

  /*!
   * @brief Allocate a demux packet. Free with FreeDemuxPacket
   * @param kodiInstance Pointer to Kodi's CPVRClient class.
   * @param iDataSize The size of the data that will go into the packet
   * @return The allocated packet.
   */
  static DEMUX_PACKET* cb_allocate_demux_packet(void* kodiInstance, int iDataSize = 0);

  /*!
   * @brief Notify a state change for a PVR backend connection
   * @param kodiInstance Pointer to Kodi's CPVRClient class
   * @param strConnectionString The connection string reported by the backend that can be displayed in the UI.
   * @param newState The new state.
   * @param strMessage A localized addon-defined string representing the new state, that can be displayed
   *        in the UI or NULL if the Kodi-defined default string for the new state shall be displayed.
   */
  static void cb_connection_state_change(void* kodiInstance,
                                         const char* strConnectionString,
                                         PVR_CONNECTION_STATE newState,
                                         const char* strMessage);

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
  static PVR_CODEC cb_get_codec_by_name(const void* kodiInstance, const char* strCodecName);
  //@}

  const int m_iClientId; /*!< unique ID of the client */
  std::atomic<bool>
      m_bReadyToUse; /*!< true if this add-on is initialised (ADDON_Create returned true), false otherwise */
  std::atomic<bool> m_bBlockAddonCalls; /*!< true if no add-on API calls are allowed */
  mutable std::atomic<int> m_iAddonCalls; /*!< number of in-progress addon calls */
  mutable CEvent m_allAddonCallsFinished; /*!< fires after last in-progress addon call finished */
  PVR_CONNECTION_STATE m_connectionState; /*!< the backend connection state */
  PVR_CONNECTION_STATE m_prevConnectionState; /*!< the previous backend connection state */
  bool
      m_ignoreClient; /*!< signals to PVRManager to ignore this client until it has been connected */
  std::vector<std::shared_ptr<CPVRTimerType>>
      m_timertypes; /*!< timer types supported by this backend */
  mutable std::optional<int> m_priority; /*!< priority of the client */

  /* cached data */
  std::string m_strBackendName; /*!< the cached backend version */
  std::string m_strBackendVersion; /*!< the cached backend version */
  std::string m_strConnectionString; /*!< the cached connection string */
  std::string m_strBackendHostname; /*!< the cached backend hostname */
  CPVRClientCapabilities m_clientCapabilities; /*!< the cached add-on's capabilities */
  mutable std::shared_ptr<CPVRClientMenuHooks> m_menuhooks; /*!< the menu hooks for this add-on */

  /* stored strings to make sure const char* members in AddonProperties_PVR stay valid */
  std::string m_strUserPath; /*!< @brief translated path to the user profile */
  std::string m_strClientPath; /*!< @brief translated path to this add-on */

  mutable CCriticalSection m_critSection;
};
} // namespace PVR
