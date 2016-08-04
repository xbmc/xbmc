#pragma once

/*
*      Copyright (C) 2005-2016 Team Kodi
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

#include "kodi_inputstream_types.h"
#include "xbmc_addon_dll.h"

/*!
* Functions that the InputStream client add-on must implement, but some can be empty.
*
* The 'remarks' field indicates which methods should be implemented, and which ones are optional.
*/

extern "C"
{
  /*!
   * Open a stream.
   * @param props
   * @return True if the stream has been opened successfully, false otherwise.
   * @remarks
   */
  bool Open(INPUTSTREAM& props);

  /*!
   * Close an open stream.
   * @remarks
   */
  void Close(void);

  /*!
   * Get path/url for this addon.
   * @remarks
   */
  const char* GetPathList(void);

  /*!
  * Get Capabilities of this addon.
  * @remarks
  */
  struct INPUTSTREAM_CAPABILITIES GetCapabilities();


  /*!
   * Get IDs of available streams
   * @remarks
   */
  INPUTSTREAM_IDS GetStreamIds();

  /*!
   * Get stream properties of a stream.
   * @param streamId unique id of stream
   * @return struc of stream properties
   * @remarks
   */
  INPUTSTREAM_INFO GetStream(int streamid);

  /*!
   * Enable or disable a stream.
   * A disabled stream does not send demux packets
   * @param streamId unique id of stream
   * @param enable true for enable, false for disable
   * @remarks
   */
  void EnableStream(int streamid, bool enable);

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
   * @remarks Return NULL if this add-on won't provide this function.
   */
  DemuxPacket* DemuxRead(void);

  /*!
   * Notify the InputStream addon/demuxer that XBMC wishes to seek the stream by time
   * Demuxer is required to set stream to an IDR frame
   * @param time The absolute time since stream start
   * @param backwards True to seek to keyframe BEFORE time, else AFTER
   * @param startpts can be updated to point to where display should start
   * @return True if the seek operation was possible
   * @remarks Optional, and only used if addon has its own demuxer.
   */
  bool DemuxSeekTime(int time, bool backwards, double *startpts);

  /*!
   * Notify the InputStream addon/demuxer that XBMC wishes to change playback speed
   * @param speed The requested playback speed
   * @remarks Optional, and only used if addon has its own demuxer.
   */
  void DemuxSetSpeed(int speed);

  /*!
   * Sets desired width / height
   * @param width / hight
   */
  void SetVideoResolution(int width, int height);

  /*!
   * Totel time in ms
   * @remarks
   */
  int GetTotalTime();

  /*!
   * Playing time in ms
   * @remarks
   */
  int GetTime();

  /*!
   * Positions inputstream to playing time given in ms
   * @remarks
   */
  bool PosTime(int ms);


  /*!
   * Check if the backend support pausing the currently playing stream
   * This will enable/disable the pause button in XBMC based on the return value
   * @return false if the InputStream addon/backend does not support pausing, true if possible
   */
  bool CanPauseStream();

  /*!
   * Check if the backend supports seeking for the currently playing stream
   * This will enable/disable the rewind/forward buttons in XBMC based on the return value
   * @return false if the InputStream addon/backend does not support seeking, true if possible
   */
  bool CanSeekStream();

  
  /*!
  * Read from an open stream.
  * @param pBuffer The buffer to store the data in.
  * @param iBufferSize The amount of bytes to read.
  * @return The amount of bytes that were actually read from the stream.
  * @remarks Return -1 if this add-on won't provide this function.
  */
  int ReadStream(uint8_t* pBuffer, unsigned int iBufferSize);

  /*!
  * Seek in a stream.
  * @param iPosition The position to seek to.
  * @param iWhence ?
  * @return The new position.
  * @remarks Return -1 if this add-on won't provide this function.
  */
  int64_t SeekStream(int64_t iPosition, int iWhence = SEEK_SET);

  /*!
  * @return The position in the stream that's currently being read.
  * @remarks Return -1 if this add-on won't provide this function.
  */
  int64_t PositionStream(void);

  /*!
  * @return The total length of the stream that's currently being read.
  * @remarks Return -1 if this add-on won't provide this function.
  */
  int64_t LengthStream(void);


  /*!
  * @brief Notify the InputStream addon that XBMC (un)paused the currently playing stream
  */
  void PauseStream(double time);


  /*!
  *  Check for real-time streaming
  *  @return true if current stream is real-time
  */
  bool IsRealTimeStream();

  const char* GetApiVersion()
  {
    static const char *ApiVersion = INPUTSTREAM_API_VERSION;
    return ApiVersion;
  }

  /*!
  * Called by XBMC to assign the function pointers of this add-on to pClient.
  * @param pClient The struct to assign the function pointers to.
  */
  void __declspec(dllexport) get_addon(struct InputStreamAddonFunctions* pClient)
  {
    pClient->Open = Open;
    pClient->Close = Close;
    pClient->GetPathList = GetPathList;
    pClient->GetCapabilities = GetCapabilities;
    pClient->GetApiVersion = GetApiVersion;

    pClient->GetStreamIds = GetStreamIds;
    pClient->GetStream = GetStream;
    pClient->EnableStream = EnableStream;
    pClient->DemuxReset = DemuxReset;
    pClient->DemuxAbort = DemuxAbort;
    pClient->DemuxFlush = DemuxFlush;
    pClient->DemuxRead = DemuxRead;
    pClient->DemuxSeekTime = DemuxSeekTime;
    pClient->DemuxSetSpeed = DemuxSetSpeed;
    pClient->SetVideoResolution = SetVideoResolution;

    pClient->GetTotalTime = GetTotalTime;
    pClient->GetTime = GetTime;

    pClient->PosTime = PosTime;

    pClient->CanPauseStream = CanPauseStream;
    pClient->CanSeekStream = CanSeekStream;

    pClient->ReadStream = ReadStream;
    pClient->SeekStream = SeekStream;
    pClient->PositionStream = PositionStream;
    pClient->LengthStream = LengthStream;
    pClient->PauseStream = PauseStream;
    pClient->IsRealTimeStream = IsRealTimeStream;
  };
};
