#pragma once

/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "xbmc_addon_dll.h"
#include "xbmc_pvr_types.h"

/*!
 * Functions that the PVR client add-on must implement, but some can be empty.
 *
 * The 'remarks' field indicates which methods should be implemented, and which ones are optional.
 */

extern "C"
{
  /*! @name PVR add-on methods */
  //@{
  /*!
   * Get the XBMC_PVR_API_VERSION that was used to compile this add-on.
   * Used to check if this add-on is compatible with XBMC.
   * @return The XBMC_PVR_API_VERSION that was used to compile this add-on.
   * @remarks Valid implementation required.
   */
  const char* GetPVRAPIVersion(void);

  /*!
   * Get the XBMC_PVR_MIN_API_VERSION that was used to compile this add-on.
   * Used to check if this add-on is compatible with XBMC.
   * @return The XBMC_PVR_MIN_API_VERSION that was used to compile this add-on.
   * @remarks Valid implementation required.
   */
  const char* GetMininumPVRAPIVersion(void);

  /*!
   * Get the XBMC_GUI_API_VERSION that was used to compile this add-on.
   * Used to check if this add-on is compatible with XBMC.
   * @return The XBMC_GUI_API_VERSION that was used to compile this add-on or empty string if this add-on does not depend on Kodi GUI API.
   * @remarks Valid implementation required.
   * @note see libKODI_guilib.h about related parts
   */
  const char* GetGUIAPIVersion(void);

  /*!
   * Get the XBMC_GUI_MIN_API_VERSION that was used to compile this add-on.
   * Used to check if this add-on is compatible with XBMC.
   * @return The XBMC_GUI_MIN_API_VERSION that was used to compile this add-on or empty string if this add-on does not depend on Kodi GUI API.
   * @remarks Valid implementation required.
   * @note see libKODI_guilib.h about related parts
   */
  const char* GetMininumGUIAPIVersion(void);

  /*!
   * Get the list of features that this add-on provides.
   * Called by XBMC to query the add-on's capabilities.
   * Used to check which options should be presented in the UI, which methods to call, etc.
   * All capabilities that the add-on supports should be set to true.
   * @param pCapabilities The add-on's capabilities.
   * @return PVR_ERROR_NO_ERROR if the properties were fetched successfully.
   * @remarks Valid implementation required.
   */
  PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES *pCapabilities);

  /*!
   * @return The name reported by the backend that will be displayed in the UI.
   * @remarks Valid implementation required.
   */
  const char* GetBackendName(void);

  /*!
   * @return The version string reported by the backend that will be displayed in the UI.
   * @remarks Valid implementation required.
   */
  const char* GetBackendVersion(void);

  /*!
   * @return The connection string reported by the backend that will be displayed in the UI.
   * @remarks Valid implementation required.
   */
  const char* GetConnectionString(void);

  /*!
   * Get the disk space reported by the backend (if supported).
   * @param iTotal The total disk space in bytes.
   * @param iUsed The used disk space in bytes.
   * @return PVR_ERROR_NO_ERROR if the drive space has been fetched successfully.
   * @remarks Optional. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetDriveSpace(long long* iTotal, long long* iUsed);

  /*!
   * Call one of the menu hooks (if supported).
   * Supported PVR_MENUHOOK instances have to be added in ADDON_Create(), by calling AddMenuHook() on the callback.
   * @param menuhook The hook to call.
   * @param item The selected item for which the hook was called.
   * @return PVR_ERROR_NO_ERROR if the hook was called successfully.
   * @remarks Optional. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR CallMenuHook(const PVR_MENUHOOK& menuhook, const PVR_MENUHOOK_DATA &item);
  //@}

  /*! @name PVR EPG methods
   *  @remarks Only used by XBMC if bSupportsEPG is set to true.
   */
  //@{
  /*!
   * Request the EPG for a channel from the backend.
   * EPG entries are added to XBMC by calling TransferEpgEntry() on the callback.
   * @param handle Handle to pass to the callback method.
   * @param channel The channel to get the EPG table for.
   * @param iStart Get events after this time (UTC).
   * @param iEnd Get events before this time (UTC).
   * @return PVR_ERROR_NO_ERROR if the table has been fetched successfully.
   * @remarks Required if bSupportsEPG is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, const PVR_CHANNEL& channel, time_t iStart, time_t iEnd);
  //@}

  /*! @name PVR channel group methods
   *  @remarks Only used by XBMC is bSupportsChannelGroups is set to true.
   *           If a group or one of the group members changes after the initial import, or if a new one was added, then the add-on
   *           should call TriggerChannelGroupsUpdate()
   */
  //@{
  /*!
   * Get the total amount of channel groups on the backend if it supports channel groups.
   * @return The amount of channels, or -1 on error.
   * @remarks Required if bSupportsChannelGroups is set to true. Return -1 if this add-on won't provide this function.
   */
  int GetChannelGroupsAmount(void);

  /*!
   * Request the list of all channel groups from the backend if it supports channel groups.
   * Channel group entries are added to XBMC by calling TransferChannelGroup() on the callback.
   * @param handle Handle to pass to the callback method.
   * @param bRadio True to get the radio channel groups, false to get the TV channel groups.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   * @remarks Required if bSupportsChannelGroups is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);

  /*!
   * Request the list of all group members of a group from the backend if it supports channel groups.
   * Member entries are added to XBMC by calling TransferChannelGroupMember() on the callback.
   * @param handle Handle to pass to the callback method.
   * @param group The group to get the members for.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   * @remarks Required if bSupportsChannelGroups is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetChannelGroupMembers(ADDON_HANDLE handle, const PVR_CHANNEL_GROUP& group);
  //@}

  /** @name PVR channel methods
   *  @remarks Either bSupportsTV or bSupportsRadio is required to be set to true.
   *           If a channel changes after the initial import, or if a new one was added, then the add-on
   *           should call TriggerChannelUpdate()
   */
  //@{
  /*!
   * Show the channel scan dialog if this backend supports it.
   * @return PVR_ERROR_NO_ERROR if the dialog was displayed successfully.
   * @remarks Required if bSupportsChannelScan is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   * @note see libKODI_guilib.h about related parts
   */
  PVR_ERROR OpenDialogChannelScan(void);

  /*!
    * @return The total amount of channels on the backend, or -1 on error.
    * @remarks Valid implementation required.
    */
  int GetChannelsAmount(void);

  /*!
   * Request the list of all channels from the backend.
   * Channel entries are added to XBMC by calling TransferChannelEntry() on the callback.
   * @param handle Handle to pass to the callback method.
   * @param bRadio True to get the radio channels, false to get the TV channels.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   * @remarks If bSupportsTV is set to true, a valid result set needs to be provided for bRadio = false.
   *          If bSupportsRadio is set to true, a valid result set needs to be provided for bRadio = true.
   *          At least one of these two must provide a valid result set.
   */
  PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);

  /*!
   * Delete a channel from the backend.
   * @param channel The channel to delete.
   * @return PVR_ERROR_NO_ERROR if the channel has been deleted successfully.
   * @remarks Required if bSupportsChannelSettings is set to true.
   */
  PVR_ERROR DeleteChannel(const PVR_CHANNEL& channel);

  /*!
   * Rename a channel on the backend.
   * @param channel The channel to rename, containing the new channel name.
   * @return PVR_ERROR_NO_ERROR if the channel has been renamed successfully.
   * @remarks Optional, and only used if bSupportsChannelSettings is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR RenameChannel(const PVR_CHANNEL& channel);

  /*!
   * Move a channel to another channel number on the backend.
   * @param channel The channel to move, containing the new channel number.
   * @return PVR_ERROR_NO_ERROR if the channel has been moved successfully.
   * @remarks Optional, and only used if bSupportsChannelSettings is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR MoveChannel(const PVR_CHANNEL& channel);

  /*!
   * Show the channel settings dialog, if supported by the backend.
   * @param channel The channel to show the dialog for.
   * @return PVR_ERROR_NO_ERROR if the dialog has been displayed successfully.
   * @remarks Required if bSupportsChannelSettings is set to true.
   * @note see libKODI_guilib.h about related parts
   */
  PVR_ERROR OpenDialogChannelSettings(const PVR_CHANNEL& channel);

  /*!
   * Show the dialog to add a channel on the backend, if supported by the backend.
   * @param channel The channel to add.
   * @return PVR_ERROR_NO_ERROR if the channel has been added successfully.
   * @remarks Required if bSupportsChannelSettings is set to true.
   * @note see libKODI_guilib.h about related parts
   */
  PVR_ERROR OpenDialogChannelAdd(const PVR_CHANNEL& channel);
  //@}

  /** @name PVR recording methods
   *  @remarks Only used by XBMC is bSupportsRecordings is set to true.
   *           If a recording changes after the initial import, or if a new one was added,
   *           then the add-on should call TriggerRecordingUpdate()
   */
  //@{
  /*!
   * @return The total amount of recordings on the backend or -1 on error.
   * @param deleted if set return deleted recording (called if bSupportsRecordingsUndelete set to true)
   * @remarks Required if bSupportsRecordings is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  int GetRecordingsAmount(bool deleted);

  /*!
   * Request the list of all recordings from the backend, if supported.
   * Recording entries are added to XBMC by calling TransferRecordingEntry() on the callback.
   * @param handle Handle to pass to the callback method.
   * @param deleted if set return deleted recording (called if bSupportsRecordingsUndelete set to true)
   * @return PVR_ERROR_NO_ERROR if the recordings have been fetched successfully.
   * @remarks Required if bSupportsRecordings is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted);

  /*!
   * Delete a recording on the backend.
   * @param recording The recording to delete.
   * @return PVR_ERROR_NO_ERROR if the recording has been deleted successfully.
   * @remarks Optional, and only used if bSupportsRecordings is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR DeleteRecording(const PVR_RECORDING& recording);

  /*!
   * Undelete a recording on the backend.
   * @param recording The recording to undelete.
   * @return PVR_ERROR_NO_ERROR if the recording has been undeleted successfully.
   * @remarks Optional, and only used if bSupportsRecordingsUndelete is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR UndeleteRecording(const PVR_RECORDING& recording);

  /*!
   * @brief Delete all recordings permanent which in the deleted folder on the backend.
   * @return PVR_ERROR_NO_ERROR if the recordings has been deleted successfully.
   */
  PVR_ERROR DeleteAllRecordingsFromTrash();

  /*!
   * Rename a recording on the backend.
   * @param recording The recording to rename, containing the new name.
   * @return PVR_ERROR_NO_ERROR if the recording has been renamed successfully.
   * @remarks Optional, and only used if bSupportsRecordings is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR RenameRecording(const PVR_RECORDING& recording);

  /*!
   * Set the play count of a recording on the backend.
   * @param recording The recording to change the play count.
   * @param count Play count.
   * @return PVR_ERROR_NO_ERROR if the recording's play count has been set successfully.
   * @remarks Required if bSupportsRecordingPlayCount is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING& recording, int count);

  /*!
  * Set the last watched position of a recording on the backend.
  * @param recording The recording.
  * @param position The last watched position in seconds
  * @return PVR_ERROR_NO_ERROR if the position has been stored successfully.
  * @remarks Required if bSupportsLastPlayedPosition is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
  */
  PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING& recording, int lastplayedposition);

  /*!
  * Retrieve the last watched position of a recording on the backend.
  * @param recording The recording.
  * @return The last watched position in seconds or -1 on error
  * @remarks Required if bSupportsRecordingPlayCount is set to true. Return -1 if this add-on won't provide this function.
  */
  int GetRecordingLastPlayedPosition(const PVR_RECORDING& recording);

  /*!
  * Retrieve the edit decision list (EDL) of a recording on the backend.
  * @param recording The recording.
  * @param edl out: The function has to write the EDL list into this array.
  * @param size in: The maximum size of the EDL, out: the actual size of the EDL.
  * @return PVR_ERROR_NO_ERROR if the EDL was successfully read.
  * @remarks Required if bSupportsRecordingEdl is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
  */
  PVR_ERROR GetRecordingEdl(const PVR_RECORDING&, PVR_EDL_ENTRY edl[], int *size);

  /*!
  * Retrieve the timer types supported by the backend.
  * @param types out: The function has to write the definition of the supported timer types into this array.
  * @param typesCount in: The maximum size of the list, out: the actual size of the list. default: PVR_ADDON_TIMERTYPE_ARRAY_SIZE
  * @return PVR_ERROR_NO_ERROR if the types were successfully written to the array.
  * @remarks Required if bSupportsTimers is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
  */
  PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE types[], int *typesCount);

  //@}
  /** @name PVR timer methods
   *  @remarks Only used by XBMC is bSupportsTimers is set to true.
   *           If a timer changes after the initial import, or if a new one was added,
   *           then the add-on should call TriggerTimerUpdate()
   */
  //@{
  /*!
   * @return The total amount of timers on the backend or -1 on error.
   * @remarks Required if bSupportsTimers is set to true. Return -1 if this add-on won't provide this function.
   */
  int GetTimersAmount(void);

  /*!
   * Request the list of all timers from the backend if supported.
   * Timer entries are added to XBMC by calling TransferTimerEntry() on the callback.
   * @param handle Handle to pass to the callback method.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   * @remarks Required if bSupportsTimers is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetTimers(ADDON_HANDLE handle);

  /*!
   * Add a timer on the backend.
   * @param timer The timer to add.
   * @return PVR_ERROR_NO_ERROR if the timer has been added successfully.
   * @remarks Required if bSupportsTimers is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR AddTimer(const PVR_TIMER& timer);

  /*!
   * Delete a timer on the backend.
   * @param timer The timer to delete.
   * @param bForceDelete Set to true to delete a timer that is currently recording a program.
   * @return PVR_ERROR_NO_ERROR if the timer has been deleted successfully.
   * @remarks Required if bSupportsTimers is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR DeleteTimer(const PVR_TIMER& timer, bool bForceDelete);

  /*!
   * Update the timer information on the backend.
   * @param timer The timer to update.
   * @return PVR_ERROR_NO_ERROR if the timer has been updated successfully.
   * @remarks Required if bSupportsTimers is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR UpdateTimer(const PVR_TIMER& timer);

  //@}

  /** @name PVR live stream methods, used to open and close a stream to a channel, and optionally perform read operations on the stream */
  //@{
  /*!
   * Open a live stream on the backend.
   * @param channel The channel to stream.
   * @return True if the stream has been opened successfully, false otherwise.
   * @remarks Required if bHandlesInputStream or bHandlesDemuxing is set to true. Return false if this add-on won't provide this function.
   */
  bool OpenLiveStream(const PVR_CHANNEL& channel);

  /*!
   * Close an open live stream.
   * @remarks Required if bHandlesInputStream or bHandlesDemuxing is set to true.
   */
  void CloseLiveStream(void);

  /*!
   * Read from an open live stream.
   * @param pBuffer The buffer to store the data in.
   * @param iBufferSize The amount of bytes to read.
   * @return The amount of bytes that were actually read from the stream.
   * @remarks Required if bHandlesInputStream is set to true. Return -1 if this add-on won't provide this function.
   */
  int ReadLiveStream(unsigned char* pBuffer, unsigned int iBufferSize);

  /*!
   * Seek in a live stream on a backend that supports timeshifting.
   * @param iPosition The position to seek to.
   * @param iWhence ?
   * @return The new position.
   * @remarks Optional, and only used if bHandlesInputStream is set to true. Return -1 if this add-on won't provide this function.
   */
  long long SeekLiveStream(long long iPosition, int iWhence = SEEK_SET);

  /*!
   * @return The position in the stream that's currently being read.
   * @remarks Optional, and only used if bHandlesInputStream is set to true. Return -1 if this add-on won't provide this function.
   */
  long long PositionLiveStream(void);

  /*!
   * @return The total length of the stream that's currently being read.
   * @remarks Optional, and only used if bHandlesInputStream is set to true. Return -1 if this add-on won't provide this function.
   */
  long long LengthLiveStream(void);

  /*!
   * Switch to another channel. Only to be called when a live stream has already been opened.
   * @param channel The channel to switch to.
   * @return True if the switch was successful, false otherwise.
   * @remarks Required if bHandlesInputStream or bHandlesDemuxing is set to true. Return false if this add-on won't provide this function.
   */
  bool SwitchChannel(const PVR_CHANNEL& channel);

  /*!
   * Get the signal status of the stream that's currently open.
   * @param signalStatus The signal status.
   * @return True if the signal status has been read successfully, false otherwise.
   * @remarks Optional, and only used if bHandlesInputStream or bHandlesDemuxing is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS& signalStatus);

  /*!
   * Get the stream URL for a channel from the backend. Used by the MediaPortal add-on.
   * @param channel The channel to get the stream URL for.
   * @return The requested URL.
   * @remarks Optional, and only used if bHandlesInputStream is set to true. Return NULL if this add-on won't provide this function.
   */
  const char* GetLiveStreamURL(const PVR_CHANNEL& channel);

  /*!
   * Get the stream properties of the stream that's currently being read.
   * @param pProperties The properties of the currently playing stream.
   * @return PVR_ERROR_NO_ERROR if the properties have been fetched successfully.
   * @remarks Required if bHandlesInputStream or bHandlesDemuxing is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties);
  //@}

  /** @name PVR recording stream methods, used to open and close a stream to a recording, and perform read operations on the stream.
   *  @remarks This will only be used if the backend doesn't provide a direct URL in the recording tag.
   */
  //@{
  /*!
   * Open a stream to a recording on the backend.
   * @param recording The recording to open.
   * @return True if the stream has been opened successfully, false otherwise.
   * @remarks Optional, and only used if bSupportsRecordings is set to true. Return false if this add-on won't provide this function.
   */
  bool OpenRecordedStream(const PVR_RECORDING& recording);

  /*!
   * Close an open stream from a recording.
   * @remarks Optional, and only used if bSupportsRecordings is set to true.
   */
  void CloseRecordedStream(void);

  /*!
   * Read from a recording.
   * @param pBuffer The buffer to store the data in.
   * @param iBufferSize The amount of bytes to read.
   * @return The amount of bytes that were actually read from the stream.
   * @remarks Optional, and only used if bSupportsRecordings is set to true, but required if OpenRecordedStream() is implemented. Return -1 if this add-on won't provide this function.
   */
  int ReadRecordedStream(unsigned char* pBuffer, unsigned int iBufferSize);

  /*!
   * Seek in a recorded stream.
   * @param iPosition The position to seek to.
   * @param iWhence ?
   * @return The new position.
   * @remarks Optional, and only used if bSupportsRecordings is set to true. Return -1 if this add-on won't provide this function.
   */
  long long SeekRecordedStream(long long iPosition, int iWhence = SEEK_SET);

  /*!
   * @return The position in the stream that's currently being read.
   * @remarks Optional, and only used if bSupportsRecordings is set to true. Return -1 if this add-on won't provide this function.
   */
  long long PositionRecordedStream(void);

  /*!
   * @return The total length of the stream that's currently being read.
   * @remarks Optional, and only used if bSupportsRecordings is set to true. Return -1 if this add-on won't provide this function.
   */
  long long LengthRecordedStream(void);
  //@}

  /** @name PVR demultiplexer methods
   *  @remarks Only used by XBMC is bHandlesDemuxing is set to true.
   */
  //@{
  /*!
   * Reset the demultiplexer in the add-on.
   * @remarks Required if bHandlesDemuxing is set to true.
   */
  void DemuxReset(void);

  /*!
   * Abort the demultiplexer thread in the add-on.
   * @remarks Required if bHandlesDemuxing is set to true.
   */
  void DemuxAbort(void);

  /*!
   * Flush all data that's currently in the demultiplexer buffer in the add-on.
   * @remarks Required if bHandlesDemuxing is set to true.
   */
  void DemuxFlush(void);

  /*!
   * Read the next packet from the demultiplexer, if there is one.
   * @return The next packet.
   *         If there is no next packet, then the add-on should return the
   *         packet created by calling AllocateDemuxPacket(0) on the callback.
   *         If the stream changed and XBMC's player needs to be reinitialised,
   *         then, the add-on should call AllocateDemuxPacket(0) on the
   *         callback, and set the streamid to DMX_SPECIALID_STREAMCHANGE and
   *         return the value.
   *         The add-on should return NULL if an error occured.
   * @remarks Required if bHandlesDemuxing is set to true. Return NULL if this add-on won't provide this function.
   */
  DemuxPacket* DemuxRead(void);
  //@}

  /*!
   * Delay to use when using switching channels for add-ons not providing an input stream.
   * If the add-on does provide an input stream, then this method will not be called.
   * Those add-ons can do that in OpenLiveStream() if needed.
   * @return The delay in milliseconds.
   */
  unsigned int GetChannelSwitchDelay(void);

  /*!
   * Check if the backend support pausing the currently playing stream
   * This will enable/disable the pause button in XBMC based on the return value
   * @return false if the PVR addon/backend does not support pausing, true if possible
   */
  bool CanPauseStream();

  /*!
   * Check if the backend supports seeking for the currently playing stream
   * This will enable/disable the rewind/forward buttons in XBMC based on the return value
   * @return false if the PVR addon/backend does not support seeking, true if possible
   */
  bool CanSeekStream();

  /*!
   * @brief Notify the pvr addon that XBMC (un)paused the currently playing stream
   */
  void PauseStream(bool bPaused);

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

  /*!
   *  Get actual playing time from addon. With timeshift enabled this is
   *  different to live.
   *  @return time as UTC
   */
  time_t GetPlayingTime();

  /*!
   *  Get time of oldest packet in timeshift buffer
   *  @return time as UTC
   */
  time_t GetBufferTimeStart();

  /*!
   *  Get time of latest packet in timeshift buffer
   *  @return time as UTC
   */
  time_t GetBufferTimeEnd();

  /*!
   *  Get the hostname of the pvr backend server
   *  @return hostname as ip address or alias. If backend does not
   *          utilize a server, return empty string.
   */
  const char* GetBackendHostname();

  /*!
   *  Check if timeshift is active
   *  @return true if timeshift is active
   */
  bool IsTimeshifting();

  /*!
   *  Check for real-time streaming
   *  @return true if current stream is real-time
   */
  bool IsRealTimeStream();

  /*!
   * Tell the client the time frame to use when notifying epg events back to Kodi. The client might push epg events asynchronously
   * to Kodi using the callback function EpgEventStateChange. To be able to only push events that are actually of interest for Kodi,
   * client needs to know about the epg time frame Kodi uses. Kodi calls this function once after the client add-on has been sucessfully
   * initialized and then everytime the time frame value changes.
   * @param iDays number of days from "now". EPG_TIMEFRAME_UNLIMITED means that Kodi is interested in all epg events, regardless of event times.
   * @return PVR_ERROR_NO_ERROR if new value was successfully set.
   * @remarks Required if bSupportsEPG is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR SetEPGTimeFrame(int iDays);

  /*!
   * Called by XBMC to assign the function pointers of this add-on to pClient.
   * @param pClient The struct to assign the function pointers to.
   */
  void __declspec(dllexport) get_addon(struct PVRClient* pClient)
  {
    pClient->GetPVRAPIVersion               = GetPVRAPIVersion;
    pClient->GetMininumPVRAPIVersion        = GetMininumPVRAPIVersion;
    pClient->GetGUIAPIVersion               = GetGUIAPIVersion;
    pClient->GetMininumGUIAPIVersion        = GetMininumGUIAPIVersion;
    pClient->GetAddonCapabilities           = GetAddonCapabilities;
    pClient->GetStreamProperties            = GetStreamProperties;
    pClient->GetConnectionString            = GetConnectionString;
    pClient->GetBackendName                 = GetBackendName;
    pClient->GetBackendVersion              = GetBackendVersion;
    pClient->GetDriveSpace                  = GetDriveSpace;
    pClient->OpenDialogChannelScan          = OpenDialogChannelScan;
    pClient->MenuHook                       = CallMenuHook;

    pClient->GetEpg                         = GetEPGForChannel;

    pClient->GetChannelGroupsAmount         = GetChannelGroupsAmount;
    pClient->GetChannelGroups               = GetChannelGroups;
    pClient->GetChannelGroupMembers         = GetChannelGroupMembers;

    pClient->GetChannelsAmount              = GetChannelsAmount;
    pClient->GetChannels                    = GetChannels;
    pClient->DeleteChannel                  = DeleteChannel;
    pClient->RenameChannel                  = RenameChannel;
    pClient->MoveChannel                    = MoveChannel;
    pClient->OpenDialogChannelSettings      = OpenDialogChannelSettings;
    pClient->OpenDialogChannelAdd           = OpenDialogChannelAdd;

    pClient->GetRecordingsAmount            = GetRecordingsAmount;
    pClient->GetRecordings                  = GetRecordings;
    pClient->DeleteRecording                = DeleteRecording;
    pClient->UndeleteRecording              = UndeleteRecording;
    pClient->DeleteAllRecordingsFromTrash   = DeleteAllRecordingsFromTrash;
    pClient->RenameRecording                = RenameRecording;
    pClient->SetRecordingPlayCount          = SetRecordingPlayCount;
    pClient->SetRecordingLastPlayedPosition = SetRecordingLastPlayedPosition;
    pClient->GetRecordingLastPlayedPosition = GetRecordingLastPlayedPosition;
    pClient->GetRecordingEdl                = GetRecordingEdl;

    pClient->GetTimerTypes                  = GetTimerTypes;
    pClient->GetTimersAmount                = GetTimersAmount;
    pClient->GetTimers                      = GetTimers;
    pClient->AddTimer                       = AddTimer;
    pClient->DeleteTimer                    = DeleteTimer;
    pClient->UpdateTimer                    = UpdateTimer;

    pClient->OpenLiveStream                 = OpenLiveStream;
    pClient->CloseLiveStream                = CloseLiveStream;
    pClient->ReadLiveStream                 = ReadLiveStream;
    pClient->SeekLiveStream                 = SeekLiveStream;
    pClient->PositionLiveStream             = PositionLiveStream;
    pClient->LengthLiveStream               = LengthLiveStream;
    pClient->SwitchChannel                  = SwitchChannel;
    pClient->SignalStatus                   = SignalStatus;
    pClient->GetLiveStreamURL               = GetLiveStreamURL;
    pClient->GetChannelSwitchDelay          = GetChannelSwitchDelay;
    pClient->CanPauseStream                 = CanPauseStream;
    pClient->PauseStream                    = PauseStream;
    pClient->CanSeekStream                  = CanSeekStream;
    pClient->SeekTime                       = SeekTime;
    pClient->SetSpeed                       = SetSpeed;

    pClient->OpenRecordedStream             = OpenRecordedStream;
    pClient->CloseRecordedStream            = CloseRecordedStream;
    pClient->ReadRecordedStream             = ReadRecordedStream;
    pClient->SeekRecordedStream             = SeekRecordedStream;
    pClient->PositionRecordedStream         = PositionRecordedStream;
    pClient->LengthRecordedStream           = LengthRecordedStream;

    pClient->DemuxReset                     = DemuxReset;
    pClient->DemuxAbort                     = DemuxAbort;
    pClient->DemuxFlush                     = DemuxFlush;
    pClient->DemuxRead                      = DemuxRead;

    pClient->GetPlayingTime                 = GetPlayingTime;
    pClient->GetBufferTimeStart             = GetBufferTimeStart;
    pClient->GetBufferTimeEnd               = GetBufferTimeEnd;

    pClient->GetBackendHostname             = GetBackendHostname;

    pClient->IsTimeshifting                 = IsTimeshifting;
    pClient->IsRealTimeStream               = IsRealTimeStream;

    pClient->SetEPGTimeFrame                = SetEPGTimeFrame;
  };
};

