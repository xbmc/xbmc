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
  bool Open(void* addonInstance, INPUTSTREAM& props);

  /*!
   * Close an open stream.
   * @remarks
   */
  void Close(void* addonInstance);

  /*!
   * Get path/url for this addon.
   * @remarks
   */
  const char* GetPathList(void* addonInstance);

  /*!
  * Get Capabilities of this addon.
  * @param pCapabilities The add-on's capabilities.
  * @remarks
  */
  void GetCapabilities(void* addonInstance, INPUTSTREAM_CAPABILITIES *pCapabilities);


  /*!
   * Get IDs of available streams
   * @remarks
   */
  INPUTSTREAM_IDS GetStreamIds(void* addonInstance);

  /*!
   * Get stream properties of a stream.
   * @param streamId unique id of stream
   * @return struc of stream properties
   * @remarks
   */
  INPUTSTREAM_INFO GetStream(void* addonInstance, int streamid);

  /*!
   * Enable or disable a stream.
   * A disabled stream does not send demux packets
   * @param streamId unique id of stream
   * @param enable true for enable, false for disable
   * @remarks
   */
  void EnableStream(void* addonInstance, int streamid, bool enable);

  /*!
   * Reset the demultiplexer in the add-on.
   * @remarks Required if bHandlesDemuxing is set to true.
   */
  void DemuxReset(void* addonInstance);

  /*!
   * Abort the demultiplexer thread in the add-on.
   * @remarks Required if bHandlesDemuxing is set to true.
   */
  void DemuxAbort(void* addonInstance);

  /*!
   * Flush all data that's currently in the demultiplexer buffer in the add-on.
   * @remarks Required if bHandlesDemuxing is set to true.
   */
  void DemuxFlush(void* addonInstance);

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
  DemuxPacket* DemuxRead(void* addonInstance);

  /*!
   * Notify the InputStream addon/demuxer that XBMC wishes to seek the stream by time
   * Demuxer is required to set stream to an IDR frame
   * @param time The absolute time since stream start
   * @param backwards True to seek to keyframe BEFORE time, else AFTER
   * @param startpts can be updated to point to where display should start
   * @return True if the seek operation was possible
   * @remarks Optional, and only used if addon has its own demuxer.
   */
  bool DemuxSeekTime(void* addonInstance, double time, bool backwards, double *startpts);

  /*!
   * Notify the InputStream addon/demuxer that XBMC wishes to change playback speed
   * @param speed The requested playback speed
   * @remarks Optional, and only used if addon has its own demuxer.
   */
  void DemuxSetSpeed(void* addonInstance, int speed);

  /*!
   * Sets desired width / height
   * @param width / hight
   */
  void SetVideoResolution(void* addonInstance, int width, int height);

  /*!
   * Totel time in ms
   * @remarks
   */
  int GetTotalTime(void* addonInstance);

  /*!
   * Playing time in ms
   * @remarks
   */
  int GetTime(void* addonInstance);

  /*!
   * Positions inputstream to playing time given in ms
   * @remarks
   */
  bool PosTime(void* addonInstance, int ms);


  /*!
   * Check if the backend support pausing the currently playing stream
   * This will enable/disable the pause button in XBMC based on the return value
   * @return false if the InputStream addon/backend does not support pausing, true if possible
   */
  bool CanPauseStream(void* addonInstance);

  /*!
   * Check if the backend supports seeking for the currently playing stream
   * This will enable/disable the rewind/forward buttons in XBMC based on the return value
   * @return false if the InputStream addon/backend does not support seeking, true if possible
   */
  bool CanSeekStream(void* addonInstance);

  
  /*!
  * Read from an open stream.
  * @param pBuffer The buffer to store the data in.
  * @param iBufferSize The amount of bytes to read.
  * @return The amount of bytes that were actually read from the stream.
  * @remarks Return -1 if this add-on won't provide this function.
  */
  int ReadStream(void* addonInstance, uint8_t* pBuffer, unsigned int iBufferSize);

  /*!
  * Seek in a stream.
  * @param iPosition The position to seek to.
  * @param iWhence ?
  * @return The new position.
  * @remarks Return -1 if this add-on won't provide this function.
  */
  int64_t SeekStream(void* addonInstance, int64_t iPosition, int iWhence = SEEK_SET);

  /*!
  * @return The position in the stream that's currently being read.
  * @remarks Return -1 if this add-on won't provide this function.
  */
  int64_t PositionStream(void* addonInstance);

  /*!
  * @return The total length of the stream that's currently being read.
  * @remarks Return -1 if this add-on won't provide this function.
  */
  int64_t LengthStream(void* addonInstance);


  /*!
  * @brief Notify the InputStream addon that XBMC (un)paused the currently playing stream
  */
  void PauseStream(void* addonInstance, double time);


  /*!
  *  Check for real-time streaming
  *  @return true if current stream is real-time
  */
  bool IsRealTimeStream(void* addonInstance);

  /*!
  * Called by XBMC to assign the function pointers of this add-on to pClient.
  * @param pClient The struct to assign the function pointers to.
  */
  void __declspec(dllexport) get_addon(void* ptr)
  {
    AddonInstance_InputStream* pClient = static_cast<AddonInstance_InputStream*>(ptr);

    pClient->toAddon.Open = Open;
    pClient->toAddon.Close = Close;
    pClient->toAddon.GetPathList = GetPathList;
    pClient->toAddon.GetCapabilities = GetCapabilities;

    pClient->toAddon.GetStreamIds = GetStreamIds;
    pClient->toAddon.GetStream = GetStream;
    pClient->toAddon.EnableStream = EnableStream;
    pClient->toAddon.DemuxReset = DemuxReset;
    pClient->toAddon.DemuxAbort = DemuxAbort;
    pClient->toAddon.DemuxFlush = DemuxFlush;
    pClient->toAddon.DemuxRead = DemuxRead;
    pClient->toAddon.DemuxSeekTime = DemuxSeekTime;
    pClient->toAddon.DemuxSetSpeed = DemuxSetSpeed;
    pClient->toAddon.SetVideoResolution = SetVideoResolution;

    pClient->toAddon.GetTotalTime = GetTotalTime;
    pClient->toAddon.GetTime = GetTime;

    pClient->toAddon.PosTime = PosTime;

    pClient->toAddon.CanPauseStream = CanPauseStream;
    pClient->toAddon.CanSeekStream = CanSeekStream;

    pClient->toAddon.ReadStream = ReadStream;
    pClient->toAddon.SeekStream = SeekStream;
    pClient->toAddon.PositionStream = PositionStream;
    pClient->toAddon.LengthStream = LengthStream;
    pClient->toAddon.PauseStream = PauseStream;
    pClient->toAddon.IsRealTimeStream = IsRealTimeStream;
  };
};
