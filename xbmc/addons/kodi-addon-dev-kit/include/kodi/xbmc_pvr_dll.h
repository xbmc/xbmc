/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
   * Get the list of features that this add-on provides.
   * Called by Kodi to query the add-on's capabilities.
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
   *  @remarks Only used by Kodi if bSupportsEPG is set to true.
   */
  //@{
  /*!
   * Request the EPG for a channel from the backend.
   * EPG entries are added to Kodi by calling TransferEpgEntry() on the callback.
   * @param handle Handle to pass to the callback method.
   * @param iChannelUid The UID of the channel to get the EPG table for.
   * @param iStart Get events after this time (UTC).
   * @param iEnd Get events before this time (UTC).
   * @return PVR_ERROR_NO_ERROR if the table has been fetched successfully.
   * @remarks Required if bSupportsEPG is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetEPGForChannel(ADDON_HANDLE handle, int iChannelUid, time_t iStart, time_t iEnd);

  /*
   * Check if the given EPG tag can be recorded.
   * @param tag the epg tag to check.
   * @param [out] bIsRecordable Set to true if the tag can be recorded.
   * @return PVR_ERROR_NO_ERROR if bIsRecordable has been set successfully.
   * @remarks Optional, return PVR_ERROR_NOT_IMPLEMENTED to let Kodi decide.
   */
  PVR_ERROR IsEPGTagRecordable(const EPG_TAG* tag, bool* bIsRecordable);

  /*
   * Check if the given EPG tag can be played.
   * @param tag the epg tag to check.
   * @param [out] bIsPlayable Set to true if the tag can be played.
   * @return PVR_ERROR_NO_ERROR if bIsPlayable has been set successfully.
   * @remarks Required if add-on supports playing epg tags.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR IsEPGTagPlayable(const EPG_TAG* tag, bool* bIsPlayable);

  /*!
   * Retrieve the edit decision list (EDL) of an EPG tag on the backend.
   * @param epgTag The EPG tag.
   * @param edl out: The function has to write the EDL into this array.
   * @param size in: The maximum size of the EDL, out: the actual size of the EDL.
   * @return PVR_ERROR_NO_ERROR if the EDL was successfully read or no EDL exists.
   * @remarks Required if bSupportsEpgEdl is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetEPGTagEdl(const EPG_TAG* epgTag, PVR_EDL_ENTRY edl[], int *size);

  /*!
   * Get the stream properties for an epg tag from the backend.
   * @param[in] tag The epg tag to get the stream properties for.
   * @param[inout] properties in: an array for the properties to return, out: the properties required to play the stream.
   * @param[inout] iPropertiesCount in: the size of the properties array, out: the number of properties returned.
   * @return PVR_ERROR_NO_ERROR if the stream is available.
   * @remarks Required if add-on supports playing epg tags.
   *          In this case your implementation must fill the property PVR_STREAM_PROPERTY_STREAMURL with the URL Kodi should resolve to playback the epg tag.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetEPGTagStreamProperties(const EPG_TAG* tag, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount);

  //@}

  /*! @name PVR channel group methods
   *  @remarks Only used by Kodi if bSupportsChannelGroups is set to true.
   *           If a group or one of the group members changes after the initial import, or if a new one was added, then the add-on
   *           should call TriggerChannelGroupsUpdate()
   */
  //@{
  /*!
   * Get the total amount of channel groups on the backend if it supports channel groups.
   * @return The amount of channels, or -1 on error.
   * @remarks Required if bSupportsChannelGroups is set to true.
   *          Return -1 if this add-on won't provide this function.
   */
  int GetChannelGroupsAmount(void);

  /*!
   * Request the list of all channel groups from the backend if it supports channel groups.
   * Channel group entries are added to Kodi by calling TransferChannelGroup() on the callback.
   * @param handle Handle to pass to the callback method.
   * @param bRadio True to get the radio channel groups, false to get the TV channel groups.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   * @remarks Required if bSupportsChannelGroups is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetChannelGroups(ADDON_HANDLE handle, bool bRadio);

  /*!
   * Request the list of all group members of a group from the backend if it supports channel groups.
   * Member entries are added to Kodi by calling TransferChannelGroupMember() on the callback.
   * @param handle Handle to pass to the callback method.
   * @param group The group to get the members for.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   * @remarks Required if bSupportsChannelGroups is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
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
   * @remarks Required if bSupportsChannelScan is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
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
   * Channel entries are added to Kodi by calling TransferChannelEntry() on the callback.
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
   * @remarks Optional, and only used if bSupportsChannelSettings is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR RenameChannel(const PVR_CHANNEL& channel);

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
   *  @remarks Only used by Kodi if bSupportsRecordings is set to true.
   *           If a recording changes after the initial import, or if a new one was added,
   *           then the add-on should call TriggerRecordingUpdate()
   */
  //@{
  /*!
   * @return The total amount of recordings on the backend or -1 on error.
   * @param deleted if set return deleted recording (called if bSupportsRecordingsUndelete set to true)
   * @remarks Required if bSupportsRecordings is set to true. Return -1 if this add-on won't provide this function.
   */
  int GetRecordingsAmount(bool deleted);

  /*!
   * Request the list of all recordings from the backend, if supported.
   * Recording entries are added to Kodi by calling TransferRecordingEntry() on the callback.
   * @param handle Handle to pass to the callback method.
   * @param deleted if set return deleted recording (called if bSupportsRecordingsUndelete set to true)
   * @return PVR_ERROR_NO_ERROR if the recordings have been fetched successfully.
   * @remarks Required if bSupportsRecordings is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetRecordings(ADDON_HANDLE handle, bool deleted);

  /*!
   * Delete a recording on the backend.
   * @param recording The recording to delete.
   * @return PVR_ERROR_NO_ERROR if the recording has been deleted successfully.
   * @remarks Optional, and only used if bSupportsRecordings is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR DeleteRecording(const PVR_RECORDING& recording);

  /*!
   * Undelete a recording on the backend.
   * @param recording The recording to undelete.
   * @return PVR_ERROR_NO_ERROR if the recording has been undeleted successfully.
   * @remarks Optional, and only used if bSupportsRecordingsUndelete is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
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
   * @remarks Optional, and only used if bSupportsRecordings is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR RenameRecording(const PVR_RECORDING& recording);

  /*!
   * Set the lifetime of a recording on the backend.
   * @param recording The recording to change the lifetime for. recording.iLifetime contains the new lieftime value.
   * @return PVR_ERROR_NO_ERROR if the recording's lifetime has been set successfully.
   * @remarks Required if bSupportsRecordingsLifetimeChange is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR SetRecordingLifetime(const PVR_RECORDING* recording);

  /*!
   * Set the play count of a recording on the backend.
   * @param recording The recording to change the play count.
   * @param count Play count.
   * @return PVR_ERROR_NO_ERROR if the recording's play count has been set successfully.
   * @remarks Required if bSupportsRecordingPlayCount is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR SetRecordingPlayCount(const PVR_RECORDING& recording, int count);

  /*!
   * Set the last watched position of a recording on the backend.
   * @param recording The recording.
   * @param lastplayedposition The last watched position in seconds
   * @return PVR_ERROR_NO_ERROR if the position has been stored successfully.
   * @remarks Required if bSupportsLastPlayedPosition is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR SetRecordingLastPlayedPosition(const PVR_RECORDING& recording, int lastplayedposition);

  /*!
   * Retrieve the last watched position of a recording on the backend.
   * @param recording The recording.
   * @return The last watched position in seconds or -1 on error
   * @remarks Required if bSupportsRecordingPlayCount is set to true.
   *          Return -1 if this add-on won't provide this function.
   */
  int GetRecordingLastPlayedPosition(const PVR_RECORDING& recording);

  /*!
   * Retrieve the edit decision list (EDL) of a recording on the backend.
   * @param recording The recording.
   * @param edl out: The function has to write the EDL into this array.
   * @param size in: The maximum size of the EDL, out: the actual size of the EDL.
   * @return PVR_ERROR_NO_ERROR if the EDL was successfully read or no EDL exists.
   * @remarks Required if bSupportsRecordingEdl is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetRecordingEdl(const PVR_RECORDING& recording, PVR_EDL_ENTRY edl[], int *size);

  /*!
   * Retrieve the timer types supported by the backend.
   * @param types out: The function has to write the definition of the supported timer types into this array.
   * @param typesCount in: The maximum size of the list, out: the actual size of the list. default: PVR_ADDON_TIMERTYPE_ARRAY_SIZE
   * @return PVR_ERROR_NO_ERROR if the types were successfully written to the array.
   * @remarks Required if bSupportsTimers is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetTimerTypes(PVR_TIMER_TYPE types[], int *typesCount);

  //@}
  /** @name PVR timer methods
   *  @remarks Only used by Kodi if bSupportsTimers is set to true.
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
   * Timer entries are added to Kodi by calling TransferTimerEntry() on the callback.
   * @param handle Handle to pass to the callback method.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   * @remarks Required if bSupportsTimers is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetTimers(ADDON_HANDLE handle);

  /*!
   * Add a timer on the backend.
   * @param timer The timer to add.
   * @return PVR_ERROR_NO_ERROR if the timer has been added successfully.
   * @remarks Required if bSupportsTimers is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR AddTimer(const PVR_TIMER& timer);

  /*!
   * Delete a timer on the backend.
   * @param timer The timer to delete.
   * @param bForceDelete Set to true to delete a timer that is currently recording a program.
   * @return PVR_ERROR_NO_ERROR if the timer has been deleted successfully.
   * @remarks Required if bSupportsTimers is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR DeleteTimer(const PVR_TIMER& timer, bool bForceDelete);

  /*!
   * Update the timer information on the backend.
   * @param timer The timer to update.
   * @return PVR_ERROR_NO_ERROR if the timer has been updated successfully.
   * @remarks Required if bSupportsTimers is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR UpdateTimer(const PVR_TIMER& timer);

  //@}

  /** @name PVR live stream methods, used to open and close a stream to a channel, and optionally perform read operations on the stream */
  //@{
  /*!
   * Open a live stream on the backend.
   * @param channel The channel to stream.
   * @return True if the stream has been opened successfully, false otherwise.
   * @remarks Required if bHandlesInputStream or bHandlesDemuxing is set to true.
   *          CloseLiveStream() will always be called by Kodi prior to calling this function.
   *          Return false if this add-on won't provide this function.
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
   * @remarks Required if bHandlesInputStream is set to true.
   *          Return -1 if this add-on won't provide this function.
   */
  int ReadLiveStream(unsigned char* pBuffer, unsigned int iBufferSize);

  /*!
   * Seek in a live stream on a backend that supports timeshifting.
   * @param iPosition The position to seek to.
   * @param iWhence ?
   * @return The new position.
   * @remarks Optional, and only used if bHandlesInputStream is set to true.
   *          Return -1 if this add-on won't provide this function.
   */
  long long SeekLiveStream(long long iPosition, int iWhence = SEEK_SET);

  /*!
   * Obtain the length of a live stream.
   * @return The total length of the stream that's currently being read.
   * @remarks Optional, and only used if bHandlesInputStream is set to true.
   *          Return -1 if this add-on won't provide this function.
   */
  long long LengthLiveStream(void);

  /*!
   * Get the signal status of the stream that's currently open.
   * @param signalStatus The signal status.
   * @return PVR_ERROR_NO_ERROR if the signal status has been read successfully, false otherwise.
   * @remarks Optional, and only used if PVR_ADDON_CAPABILITIES::bHandlesInputStream or PVR_ADDON_CAPABILITIES::bHandlesDemuxing is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS& signalStatus);

  /*!
   * Get the descramble information of the stream that's currently open.
   * @param [out] descrambleInfo The descramble information.
   * @return PVR_ERROR_NO_ERROR if the descramble information has been read successfully, false otherwise.
   * @remarks Optional, and only used if PVR_ADDON_CAPABILITIES::bSupportsDescrambleInfo is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetDescrambleInfo(PVR_DESCRAMBLE_INFO* descrambleInfo);

  /*!
   * Get the stream properties for a channel from the backend.
   * @param[in] channel The channel to get the stream properties for.
   * @param[inout] properties in: an array for the properties to return, out: the properties required to play the stream.
   * @param[inout] iPropertiesCount in: the size of the properties array, out: the number of properties returned.
   * @return PVR_ERROR_NO_ERROR if the stream is available.
   * @remarks Required if PVR_ADDON_CAPABILITIES::bSupportsTV or PVR_ADDON_CAPABILITIES::bSupportsRadio are set to true and PVR_ADDON_CAPABILITIES::bHandlesInputStream is set to false.
   *          In this case the implementation must fill the property PVR_STREAM_PROPERTY_STREAMURL with the URL Kodi should resolve to playback the channel.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetChannelStreamProperties(const PVR_CHANNEL* channel, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount);

  /*!
   * Get the stream properties for a recording from the backend.
   * @param[in] recording The recording to get the stream properties for.
   * @param[inout] properties in: an array for the properties to return, out: the properties required to play the stream.
   * @param[inout] iPropertiesCount in: the size of the properties array, out: the number of properties returned.
   * @return PVR_ERROR_NO_ERROR if the stream is available.
   * @remarks Required if PVR_ADDON_CAPABILITIES::bSupportsRecordings is set to true and the add-on does not implement recording stream functions (OpenRecordedStream, ...).
   *          In this case your implementation must fill the property PVR_STREAM_PROPERTY_STREAMURL with the URL Kodi should resolve to playback the recording.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetRecordingStreamProperties(const PVR_RECORDING* recording, PVR_NAMED_VALUE* properties, unsigned int* iPropertiesCount);

  /*!
   * Get the stream properties of the stream that's currently being read.
   * @param pProperties The properties of the currently playing stream.
   * @return PVR_ERROR_NO_ERROR if the properties have been fetched successfully.
   * @remarks Required if bHandlesDemuxing is set to true.
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES* pProperties);
  //@}

  /** @name PVR recording stream methods, used to open and close a stream to a recording, and perform read operations on the stream.
   *  @remarks This will only be used if the backend doesn't provide a direct URL in the recording tag.
   */
  //@{
  /*!
   * Obtain the chunk size to use when reading streams.
   * @param chunksize must be filled with the chunk size in bytes.
   * @return PVR_ERROR_NO_ERROR if the chunk size has been fetched successfully.
   * @remarks Optional, and only used if not reading from demuxer (=> DemuxRead) and
   *          PVR_ADDON_CAPABILITIES::bSupportsRecordings is true (=> ReadRecordedStream) or
   *          PVR_ADDON_CAPABILITIES::bHandlesInputStream is true (=> ReadLiveStream).
   *          Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function. In this case Kodi will decide on the chunk size to use.
   */
  PVR_ERROR GetStreamReadChunkSize(int* chunksize);

  /*!
   * Open a stream to a recording on the backend.
   * @param recording The recording to open.
   * @return True if the stream has been opened successfully, false otherwise.
   * @remarks Optional, and only used if bSupportsRecordings is set to true.
   *          CloseRecordedStream() will always be called by Kodi prior to calling this function.
   *          Return false if this add-on won't provide this function.
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
   * @remarks Optional, and only used if bSupportsRecordings is set to true, but required if OpenRecordedStream() is implemented.
   *          Return -1 if this add-on won't provide this function.
   */
  int ReadRecordedStream(unsigned char* pBuffer, unsigned int iBufferSize);

  /*!
   * Seek in a recorded stream.
   * @param iPosition The position to seek to.
   * @param iWhence ?
   * @return The new position.
   * @remarks Optional, and only used if bSupportsRecordings is set to true.
   *          Return -1 if this add-on won't provide this function.
   */
  long long SeekRecordedStream(long long iPosition, int iWhence = SEEK_SET);

  /*!
   * Obtain the length of a recorded stream.
   * @return The total length of the stream that's currently being read.
   * @remarks Optional, and only used if bSupportsRecordings is set to true.
   *          Return -1 if this add-on won't provide this function.
   */
  long long LengthRecordedStream(void);

  //@}

  /** @name PVR demultiplexer methods
   *  @remarks Only used by Kodi if bHandlesDemuxing is set to true.
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
   *         If the stream changed and Kodi's player needs to be reinitialised,
   *         then, the add-on should call AllocateDemuxPacket(0) on the
   *         callback, and set the streamid to DMX_SPECIALID_STREAMCHANGE and
   *         return the value.
   *         The add-on should return NULL if an error occured.
   * @remarks Required if bHandlesDemuxing is set to true.
   *          Return NULL if this add-on won't provide this function.
   */
  DemuxPacket* DemuxRead(void);
  //@}

  /*!
   * Check if the backend support pausing the currently playing stream
   * This will enable/disable the pause button in Kodi based on the return value
   * @return false if the PVR addon/backend does not support pausing, true if possible
   */
  bool CanPauseStream();

  /*!
   * Check if the backend supports seeking for the currently playing stream
   * This will enable/disable the rewind/forward buttons in Kodi based on the return value
   * @return false if the PVR addon/backend does not support seeking, true if possible
   */
  bool CanSeekStream();

  /*!
   * @brief Notify the pvr addon that Kodi (un)paused the currently playing stream
   */
  void PauseStream(bool bPaused);

  /*!
   * Notify the pvr addon/demuxer that Kodi wishes to seek the stream by time
   * @param time The absolute time since stream start
   * @param backwards True to seek to keyframe BEFORE time, else AFTER
   * @param startpts can be updated to point to where display should start
   * @return True if the seek operation was possible
   * @remarks Optional, and only used if addon has its own demuxer.
   *          Return False if this add-on won't provide this function.
   */
  bool SeekTime(double time, bool backwards, double *startpts);

  /*!
   * Notify the pvr addon/demuxer that Kodi wishes to change playback speed
   * @param speed The requested playback speed
   * @remarks Optional, and only used if addon has its own demuxer.
   */
  void SetSpeed(int speed);

  /*!
   * Notify the pvr addon/demuxer that Kodi wishes to fill demux queue
   * @param mode The requested filling mode
   * @remarks Optional, and only used if addon has its own demuxer.
   */
  void FillBuffer(bool mode);

  /*!
   *  Get the hostname of the pvr backend server
   *  @return hostname as ip address or alias. If backend does not utilize a server, return empty string.
   */
  const char* GetBackendHostname();

  /*!
   *  Check for real-time streaming
   *  @return true if current stream is real-time
   */
  bool IsRealTimeStream();

  /*!
   * Tell the client the time frame to use when notifying epg events back to Kodi. The client might push epg events asynchronously
   * to Kodi using the callback function EpgEventStateChange. To be able to only push events that are actually of interest for Kodi,
   * client needs to know about the epg time frame Kodi uses. Kodi supplies the current epg time frame value in PVR_PROPERTIES.iEpgMaxDays
   * when creating the addon and calls SetEPGTimeFrame later whenever Kodi's epg time frame value changes.
   * @param iDays number of days from "now". EPG_TIMEFRAME_UNLIMITED means that Kodi is interested in all epg events, regardless of event times.
   * @return PVR_ERROR_NO_ERROR if new value was successfully set.
   * @remarks Required if bSupportsEPG is set to true. Return PVR_ERROR_NOT_IMPLEMENTED if this add-on won't provide this function.
   */
  PVR_ERROR SetEPGTimeFrame(int iDays);

  /*!
   * Notify the pvr addon for power management events
   */
  void OnSystemSleep();
  void OnSystemWake();
  void OnPowerSavingActivated();
  void OnPowerSavingDeactivated();

  /*!
   * Get stream times.
   * @param times A pointer to the data to be filled by the implementation.
   * @return PVR_ERROR_NO_ERROR on success.
   */
  PVR_ERROR GetStreamTimes(PVR_STREAM_TIMES *times);

  /*!
   * Called by Kodi to assign the function pointers of this add-on to pClient.
   * @param ptr The struct to assign the function pointers to.
   */
  void __declspec(dllexport) get_addon(void* ptr)
  {
    AddonInstance_PVR* pClient = static_cast<AddonInstance_PVR*>(ptr);

    pClient->toAddon.addonInstance = nullptr; // used in future

    pClient->toAddon.GetAddonCapabilities           = GetAddonCapabilities;
    pClient->toAddon.GetStreamProperties            = GetStreamProperties;
    pClient->toAddon.GetConnectionString            = GetConnectionString;
    pClient->toAddon.GetBackendName                 = GetBackendName;
    pClient->toAddon.GetBackendVersion              = GetBackendVersion;
    pClient->toAddon.GetDriveSpace                  = GetDriveSpace;
    pClient->toAddon.OpenDialogChannelScan          = OpenDialogChannelScan;
    pClient->toAddon.MenuHook                       = CallMenuHook;

    pClient->toAddon.GetEPGForChannel               = GetEPGForChannel;
    pClient->toAddon.IsEPGTagRecordable             = IsEPGTagRecordable;
    pClient->toAddon.IsEPGTagPlayable               = IsEPGTagPlayable;
    pClient->toAddon.GetEPGTagEdl                   = GetEPGTagEdl;
    pClient->toAddon.GetEPGTagStreamProperties      = GetEPGTagStreamProperties;

    pClient->toAddon.GetChannelGroupsAmount         = GetChannelGroupsAmount;
    pClient->toAddon.GetChannelGroups               = GetChannelGroups;
    pClient->toAddon.GetChannelGroupMembers         = GetChannelGroupMembers;

    pClient->toAddon.GetChannelsAmount              = GetChannelsAmount;
    pClient->toAddon.GetChannels                    = GetChannels;
    pClient->toAddon.DeleteChannel                  = DeleteChannel;
    pClient->toAddon.RenameChannel                  = RenameChannel;
    pClient->toAddon.OpenDialogChannelSettings      = OpenDialogChannelSettings;
    pClient->toAddon.OpenDialogChannelAdd           = OpenDialogChannelAdd;

    pClient->toAddon.GetRecordingsAmount            = GetRecordingsAmount;
    pClient->toAddon.GetRecordings                  = GetRecordings;
    pClient->toAddon.DeleteRecording                = DeleteRecording;
    pClient->toAddon.UndeleteRecording              = UndeleteRecording;
    pClient->toAddon.DeleteAllRecordingsFromTrash   = DeleteAllRecordingsFromTrash;
    pClient->toAddon.RenameRecording                = RenameRecording;
    pClient->toAddon.SetRecordingLifetime           = SetRecordingLifetime;
    pClient->toAddon.SetRecordingPlayCount          = SetRecordingPlayCount;
    pClient->toAddon.SetRecordingLastPlayedPosition = SetRecordingLastPlayedPosition;
    pClient->toAddon.GetRecordingLastPlayedPosition = GetRecordingLastPlayedPosition;
    pClient->toAddon.GetRecordingEdl                = GetRecordingEdl;

    pClient->toAddon.GetTimerTypes                  = GetTimerTypes;
    pClient->toAddon.GetTimersAmount                = GetTimersAmount;
    pClient->toAddon.GetTimers                      = GetTimers;
    pClient->toAddon.AddTimer                       = AddTimer;
    pClient->toAddon.DeleteTimer                    = DeleteTimer;
    pClient->toAddon.UpdateTimer                    = UpdateTimer;

    pClient->toAddon.OpenLiveStream                 = OpenLiveStream;
    pClient->toAddon.CloseLiveStream                = CloseLiveStream;
    pClient->toAddon.ReadLiveStream                 = ReadLiveStream;
    pClient->toAddon.SeekLiveStream                 = SeekLiveStream;
    pClient->toAddon.LengthLiveStream               = LengthLiveStream;
    pClient->toAddon.SignalStatus                   = SignalStatus;
    pClient->toAddon.GetDescrambleInfo              = GetDescrambleInfo;
    pClient->toAddon.GetChannelStreamProperties     = GetChannelStreamProperties;
    pClient->toAddon.GetRecordingStreamProperties   = GetRecordingStreamProperties;
    pClient->toAddon.CanPauseStream                 = CanPauseStream;
    pClient->toAddon.PauseStream                    = PauseStream;
    pClient->toAddon.CanSeekStream                  = CanSeekStream;
    pClient->toAddon.SeekTime                       = SeekTime;
    pClient->toAddon.SetSpeed                       = SetSpeed;
    pClient->toAddon.FillBuffer                     = FillBuffer;

    pClient->toAddon.OpenRecordedStream             = OpenRecordedStream;
    pClient->toAddon.CloseRecordedStream            = CloseRecordedStream;
    pClient->toAddon.ReadRecordedStream             = ReadRecordedStream;
    pClient->toAddon.SeekRecordedStream             = SeekRecordedStream;
    pClient->toAddon.LengthRecordedStream           = LengthRecordedStream;

    pClient->toAddon.DemuxReset                     = DemuxReset;
    pClient->toAddon.DemuxAbort                     = DemuxAbort;
    pClient->toAddon.DemuxFlush                     = DemuxFlush;
    pClient->toAddon.DemuxRead                      = DemuxRead;

    pClient->toAddon.GetBackendHostname             = GetBackendHostname;

    pClient->toAddon.IsRealTimeStream               = IsRealTimeStream;

    pClient->toAddon.SetEPGTimeFrame                = SetEPGTimeFrame;

    pClient->toAddon.OnSystemSleep                  = OnSystemSleep;
    pClient->toAddon.OnSystemWake                   = OnSystemWake;
    pClient->toAddon.OnPowerSavingActivated         = OnPowerSavingActivated;
    pClient->toAddon.OnPowerSavingDeactivated       = OnPowerSavingDeactivated;
    pClient->toAddon.GetStreamTimes                 = GetStreamTimes;

    pClient->toAddon.GetStreamReadChunkSize         = GetStreamReadChunkSize;
  };
};
