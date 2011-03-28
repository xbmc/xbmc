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

#ifndef __XBMC_PVR_H__
#define __XBMC_PVR_H__

#include "xbmc_addon_dll.h"
#include "xbmc_pvr_types.h"

extern "C"
{
  // Functions that your PVR client must implement

  /** @name PVR server methods */
  //@{

  /*!
   * @brief Query this add-on's capabilities.
   * @param pCapabilities The add-on properties.
   * @return PVR_ERROR_NO_ERROR if the properties were fetched successfully.
   */
  PVR_ERROR GetAddonCapabilities(PVR_ADDON_CAPABILITIES *pCapabilities);

  /*!
   * @brief Get the stream properties of the stream that's currently being read.
   * @param pProperties The properties.
   * @return PVR_ERROR_NO_ERROR if the properties have been fetched successfully.
   */
  PVR_ERROR GetStreamProperties(PVR_STREAM_PROPERTIES *pProperties);

  /*!
   * @return The name reported by the backend.
   */
  const char *GetBackendName(void);

  /*!
   * @return The version string reported by the backend.
   */
  const char *GetBackendVersion(void);

  /*!
   * @return The connection string reported by the backend.
   */
  const char *GetConnectionString(void);

  /*!
   * @brief Get the disk space reported by the server.
   * @param iTotal The total disk space.
   * @param iUsed The used disk space.
   * @return PVR_ERROR_NO_ERROR if the drive space has been fetched successfully.
   */
  PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed);

  /*!
   * @brief Show the channel scan dialog.
   * @return PVR_ERROR_NO_ERROR if the dialog was displayed successfully.
   */
  PVR_ERROR DialogChannelScan(void);

  /*!
   * @brief Call one of the menu hooks.
   * @param menuhook The hook to call.
   * @return PVR_ERROR_NO_ERROR if the hook was called successfully.
   */
  PVR_ERROR CallMenuHook(const PVR_MENUHOOK &menuhook);

  //@}
  /** @name PVR EPG methods */
  //@{

  /*!
   * @brief Request an EPG table for a channel from the client.
   * @param handle Callback handle
   * @param channel The channel to get the EPG table for.
   * @param iStart The start time to use.
   * @param iEnd The end time to use.
   * @return PVR_ERROR_NO_ERROR if the table has been fetched successfully.
   */
  PVR_ERROR GetEPGForChannel(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);

  //@}
  /** @name PVR channel group methods */
  //@{

  /*!
    * @return The total amount of channel groups on the server or -1 on error.
    */
  int GetChannelGroupsAmount(void);

  /*!
   * @brief Request the list of all channel groups from the backend.
   * @param bRadio True to get the radio channel groups, false to get the TV channel groups.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   */
  PVR_ERROR GetChannelGroups(PVR_HANDLE handle, bool bRadio);

  /*!
   * @brief Request the list of all group members of a group.
   * @param handle Callback.
   * @param group The group to get the members for.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   */
  PVR_ERROR GetChannelGroupMembers(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group);

  //@}
  /** @name PVR channel methods */
  //@{

  /*!
    * @return The total amount of channels on the server or -1 on error.
    */
  int GetChannelsAmount(void);

  /*!
   * @brief Request the list of all channels from the backend.
   * @param bRadio True to get the radio channels, false to get the TV channels.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   */
  PVR_ERROR GetChannels(PVR_HANDLE handle, bool bRadio);

  /*!
   * @brief Delete a channel.
   * @param channel The channel to delete.
   * @return PVR_ERROR_NO_ERROR if the channel has been deleted successfully.
   */
  PVR_ERROR DeleteChannel(const PVR_CHANNEL &channel);

  /*!
   * @brief Rename a channel.
   * @param channel The channel to rename, containing the new channel name.
   * @return PVR_ERROR_NO_ERROR if the channel has been renamed successfully.
   */
  PVR_ERROR RenameChannel(const PVR_CHANNEL &channel);

  /*!
   * @brief Move a channel to another channel number.
   * @param channel The channel to move, containing the new channel number.
   * @return PVR_ERROR_NO_ERROR if the channel has been moved successfully.
   */
  PVR_ERROR MoveChannel(const PVR_CHANNEL &channel);

  /*!
   * @brief Show the channel settings dialog.
   * @param channel The channel to show the dialog for.
   * @return PVR_ERROR_NO_ERROR if the dialog has been displayed successfully.
   */
  PVR_ERROR DialogChannelSettings(const PVR_CHANNEL &channel);

  /*!
   * @brief Show the dialog to add a channel on the backend.
   * @param channel The channel to add.
   * @return PVR_ERROR_NO_ERROR if the channel has been added successfully.
   */
  PVR_ERROR DialogAddChannel(const PVR_CHANNEL &channel);

  //@}
  /** @name PVR recording methods */
  //@{

  /*!
   * @return The total amount of channels on the server or -1 on error.
   */
  int GetRecordingsAmount(void);

  /*!
   * @brief Request the list of all recordings from the backend.
   * @param handle The callback handle.
   * @return PVR_ERROR_NO_ERROR if the recordings have been fetched successfully.
   */
  PVR_ERROR GetRecordings(PVR_HANDLE handle);

  /*!
   * @brief Delete a recording on the backend.
   * @param recording The recording to delete.
   * @return PVR_ERROR_NO_ERROR if the recording has been deleted successfully.
   */
  PVR_ERROR DeleteRecording(const PVR_RECORDING &recording);

  /*!
   * @brief Rename a recording on the backend.
   * @param recording The recording to rename, containing the new name.
   * @return PVR_ERROR_NO_ERROR if the recording has been renamed successfully.
   */
  PVR_ERROR RenameRecording(const PVR_RECORDING &recording);

  //@}
  /** @name PVR timer methods */
  //@{

  /*!
   * @return The total amount of timers on the backend or -1 on error.
   */
  int GetTimersAmount(void);

  /*!
   * @brief Request the list of all timers from the backend.
   * @param handle The callback handle.
   * @return PVR_ERROR_NO_ERROR if the list has been fetched successfully.
   */
  PVR_ERROR GetTimers(PVR_HANDLE handle);

  /*!
   * @brief Add a timer on the backend.
   * @param timer The timer to add.
   * @return PVR_ERROR_NO_ERROR if the timer has been added successfully.
   */
  PVR_ERROR AddTimer(const PVR_TIMER &timer);

  /*!
   * @brief Delete a timer on the backend.
   * @param timer The timer to delete.
   * @param bForceDelete Set to true to delete a timer that is currently recording a program.
   * @return PVR_ERROR_NO_ERROR if the timer has been deleted successfully.
   */
  PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete);

  /*!
   * @brief Update the timer information on the server.
   * @param timer The timer to update.
   * @return PVR_ERROR_NO_ERROR if the timer has been updated successfully.
   */
  PVR_ERROR UpdateTimer(const PVR_TIMER &timer);

  //@}
  /** @name PVR live stream methods */
  //@{

  /*!
   * @brief Open a live stream on the server.
   * @param channel The channel to stream.
   * @return True if the stream opened successfully, false otherwise.
   */
  bool OpenLiveStream(const PVR_CHANNEL &channel);

  /*!
   * @brief Close an open live stream.
   */
  void CloseLiveStream(void);

  /*!
   * @brief Read from an open live stream.
   * @param pBuffer The buffer to store the data in.
   * @param iBufferSize The amount of bytes to read.
   * @return The amount of bytes that were actually read from the stream.
   */
  int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize);

  /*!
   * @brief Seek in a live stream on a backend that supports timeshifting.
   * @param iPosition The position to seek to.
   * @param iWhence ?
   * @return The new position.
   */
  long long SeekLiveStream(long long iPosition, int iWhence = SEEK_SET);

  /*!
   * @return The position in the stream that's currently being read.
   */
  long long PositionLiveStream(void);

  /*!
   * @return The total length of the stream that's currently being read.
   */
  long long LengthLiveStream(void);

  /*!
   * @return The channel number on the server of the live stream that's currently being read.
   */
  int GetCurrentClientChannel(void);

  /*!
   * @brief Switch to another channel. Only to be called when a live stream has already been opened.
   * @param channel The channel to switch to.
   * @return True if the switch was successful, false otherwise.
   */
  bool SwitchChannel(const PVR_CHANNEL &channel);

  /*!
   * @brief Get the signal status of the stream that's currently open.
   * @param signalStatus The signal status.
   * @return True if the signal status has been read successfully, false otherwise.
   */
  PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus);

  /*!
   * @brief Get the stream URL for a channel from the server. Used by the MediaPortal add-on.
   * @param channel The channel to get the stream URL for.
   * @return The requested URL.
   */
  const char *GetLiveStreamURL(const PVR_CHANNEL &channel);

  //@}
  /** @name PVR recording stream methods */
  //@{

  /*!
   * @brief Open a recording on the server.
   * @param recording The recording to open.
   * @return True if the stream has been opened succesfully, false otherwise.
   */
  bool OpenRecordedStream(const PVR_RECORDING &recording);

  /*!
   * @brief Close an open stream from a recording.
   */
  void CloseRecordedStream(void);

  /*!
   * @brief Read from a recording.
   * @param pBuffer The buffer to store the data in.
   * @param iBufferSize The amount of bytes to read.
   * @return The amount of bytes that were actually read from the stream.
   */
  int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize);

  /*!
   * @brief Seek in a recorded stream.
   * @param iPosition The position to seek to.
   * @param iWhence ?
   * @return The new position.
   */
  long long SeekRecordedStream(long long iPosition, int iWhence = SEEK_SET);

  /*!
   * @return The position in the stream that's currently being read.
   */
  long long PositionRecordedStream(void);

  /*!
   * @return The total length of the stream that's currently being read.
   */
  long long LengthRecordedStream(void);

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

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_addon(struct PVRClient *pClient)
  {
    pClient->GetAddonCapabilities    = GetAddonCapabilities;
    pClient->GetStreamProperties     = GetStreamProperties;
    pClient->GetConnectionString     = GetConnectionString;
    pClient->GetBackendName          = GetBackendName;
    pClient->GetBackendVersion       = GetBackendVersion;
    pClient->GetDriveSpace           = GetDriveSpace;
    pClient->DialogChannelScan       = DialogChannelScan;
    pClient->MenuHook                = CallMenuHook;

    pClient->GetEpg                  = GetEPGForChannel;

    pClient->GetChannelGroupsAmount  = GetChannelGroupsAmount;
    pClient->GetChannelGroups        = GetChannelGroups;
    pClient->GetChannelGroupMembers  = GetChannelGroupMembers;

    pClient->GetChannelsAmount       = GetChannelsAmount;
    pClient->GetChannels             = GetChannels;
    pClient->DeleteChannel           = DeleteChannel;
    pClient->RenameChannel           = RenameChannel;
    pClient->MoveChannel             = MoveChannel;
    pClient->DialogChannelSettings   = DialogChannelSettings;
    pClient->DialogAddChannel        = DialogAddChannel;

    pClient->GetRecordingsAmount     = GetRecordingsAmount;
    pClient->GetRecordings           = GetRecordings;
    pClient->DeleteRecording         = DeleteRecording;
    pClient->RenameRecording         = RenameRecording;

    pClient->GetTimersAmount         = GetTimersAmount;
    pClient->GetTimers               = GetTimers;
    pClient->AddTimer                = AddTimer;
    pClient->DeleteTimer             = DeleteTimer;
    pClient->UpdateTimer             = UpdateTimer;

    pClient->OpenLiveStream          = OpenLiveStream;
    pClient->CloseLiveStream         = CloseLiveStream;
    pClient->ReadLiveStream          = ReadLiveStream;
    pClient->SeekLiveStream          = SeekLiveStream;
    pClient->PositionLiveStream      = PositionLiveStream;
    pClient->LengthLiveStream        = LengthLiveStream;
    pClient->GetCurrentClientChannel = GetCurrentClientChannel;
    pClient->SwitchChannel           = SwitchChannel;
    pClient->SignalStatus            = SignalStatus;
    pClient->GetLiveStreamURL        = GetLiveStreamURL;

    pClient->OpenRecordedStream      = OpenRecordedStream;
    pClient->CloseRecordedStream     = CloseRecordedStream;
    pClient->ReadRecordedStream      = ReadRecordedStream;
    pClient->SeekRecordedStream      = SeekRecordedStream;
    pClient->PositionRecordedStream  = PositionRecordedStream;
    pClient->LengthRecordedStream    = LengthRecordedStream;

    pClient->DemuxReset              = DemuxReset;
    pClient->DemuxAbort              = DemuxAbort;
    pClient->DemuxFlush              = DemuxFlush;
    pClient->DemuxRead               = DemuxRead;
  };
};

#endif
