/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../StreamCrypto.h"
#include "../c-api/addon-instance/inputstream.h"

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

class ATTRIBUTE_HIDDEN CInstanceInputStream : public IAddonInstance
{
public:
  explicit CInstanceInputStream(KODI_HANDLE instance, const std::string& kodiVersion = "")
    : IAddonInstance(ADDON_INSTANCE_INPUTSTREAM,
                     !kodiVersion.empty() ? kodiVersion
                                          : GetKodiTypeVersion(ADDON_INSTANCE_INPUTSTREAM))
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceInputStream: Creation of multiple together "
                             "with single instance way is not allowed!");

    SetAddonStruct(instance, m_kodiVersion);
  }

  ~CInstanceInputStream() override = default;

  /*!
     * Open a stream.
     * @param props
     * @return True if the stream has been opened successfully, false otherwise.
     * @remarks
     */
  virtual bool Open(INPUTSTREAM_PROPERTY& props) = 0;

  /*!
     * Close an open stream.
     * @remarks
     */
  virtual void Close() = 0;

  /*!
     * Get Capabilities of this addon.
     * @param capabilities The add-on's capabilities.
     * @remarks
     */
  virtual void GetCapabilities(INPUTSTREAM_CAPABILITIES& capabilities) = 0;

  /*!
     * Get IDs of available streams
     * @remarks
     */
  virtual bool GetStreamIds(std::vector<unsigned int>& ids) = 0;

  /*!
     * Get stream properties of a stream.
     * @param streamid unique id of stream
     * @return struc of stream properties
     * @remarks
     */
  virtual bool GetStream(int streamid,
                         INPUTSTREAM_INFO* info,
                         KODI_HANDLE* demuxStream,
                         KODI_HANDLE (*transfer_stream)(KODI_HANDLE handle,
                                                        int streamId,
                                                        struct INPUTSTREAM_INFO* stream)) = 0;

  /*!
     * Enable or disable a stream.
     * A disabled stream does not send demux packets
     * @param streamid unique id of stream
     * @param enable true for enable, false for disable
     * @remarks
     */
  virtual void EnableStream(int streamid, bool enable) = 0;

  /*!
    * Opens a stream for playback.
    * @param streamid unique id of stream
    * @remarks
    */
  virtual bool OpenStream(int streamid) = 0;

  /*!
     * Reset the demultiplexer in the add-on.
     * @remarks Required if bHandlesDemuxing is set to true.
     */
  virtual void DemuxReset() {}

  /*!
     * Abort the demultiplexer thread in the add-on.
     * @remarks Required if bHandlesDemuxing is set to true.
     */
  virtual void DemuxAbort() {}

  /*!
     * Flush all data that's currently in the demultiplexer buffer in the add-on.
     * @remarks Required if bHandlesDemuxing is set to true.
     */
  virtual void DemuxFlush() {}

  /*!
     * Read the next packet from the demultiplexer, if there is one.
     * @return The next packet.
     *         If there is no next packet, then the add-on should return the
     *         packet created by calling AllocateDemuxPacket(0) on the callback.
     *         If the stream changed and Kodi's player needs to be reinitialised,
     *         then, the add-on should call AllocateDemuxPacket(0) on the
     *         callback, and set the streamid to DMX_SPECIALID_STREAMCHANGE and
     *         return the value.
     *         The add-on should return NULL if an error occurred.
     * @remarks Return NULL if this add-on won't provide this function.
     */
  virtual DemuxPacket* DemuxRead() { return nullptr; }

  /*!
     * Notify the InputStream addon/demuxer that Kodi wishes to seek the stream by time
     * Demuxer is required to set stream to an IDR frame
     * @param time The absolute time since stream start
     * @param backwards True to seek to keyframe BEFORE time, else AFTER
     * @param startpts can be updated to point to where display should start
     * @return True if the seek operation was possible
     * @remarks Optional, and only used if addon has its own demuxer.
     */
  virtual bool DemuxSeekTime(double time, bool backwards, double& startpts) { return false; }

  /*!
     * Notify the InputStream addon/demuxer that Kodi wishes to change playback speed
     * @param speed The requested playback speed
     * @remarks Optional, and only used if addon has its own demuxer.
     */
  virtual void DemuxSetSpeed(int speed) {}

  /*!
     * Sets desired width / height
     * @param width / hight
     */
  virtual void SetVideoResolution(int width, int height) {}

  /*!
     * Totel time in ms
     * @remarks
     */
  virtual int GetTotalTime() { return -1; }

  /*!
     * Playing time in ms
     * @remarks
     */
  virtual int GetTime() { return -1; }

  /*!
    * Get current timing values in PTS scale
    * @remarks
    */
  virtual bool GetTimes(INPUTSTREAM_TIMES& times) { return false; }

  /*!
     * Positions inputstream to playing time given in ms
     * @remarks
     */
  virtual bool PosTime(int ms) { return false; }

  /*!
  * Return currently selected chapter
  * @remarks
  */
  virtual int GetChapter() { return -1; };

  /*!
  * Return number of available chapters
  * @remarks
  */
  virtual int GetChapterCount() { return 0; };

  /*!
  * Return name of chapter # ch
  * @remarks
  */
  virtual const char* GetChapterName(int ch) { return nullptr; };

  /*!
  * Return position if chapter # ch in milliseconds
  * @remarks
  */
  virtual int64_t GetChapterPos(int ch) { return 0; };

  /*!
  * Seek to the beginning of chapter # ch
  * @remarks
  */
  virtual bool SeekChapter(int ch) { return false; };

  /*!
     * Read from an open stream.
     * @param buffer The buffer to store the data in.
     * @param bufferSize The amount of bytes to read.
     * @return The amount of bytes that were actually read from the stream.
     * @remarks Return -1 if this add-on won't provide this function.
     */
  virtual int ReadStream(uint8_t* buffer, unsigned int bufferSize) { return -1; }

  /*!
     * Seek in a stream.
     * @param position The position to seek to.
     * @param whence ?
     * @return The new position.
     * @remarks Return -1 if this add-on won't provide this function.
     */
  virtual int64_t SeekStream(int64_t position, int whence = SEEK_SET) { return -1; }

  /*!
     * @return The position in the stream that's currently being read.
     * @remarks Return -1 if this add-on won't provide this function.
     */
  virtual int64_t PositionStream() { return -1; }

  /*!
     * @return The total length of the stream that's currently being read.
     * @remarks Return -1 if this add-on won't provide this function.
     */
  virtual int64_t LengthStream() { return -1; }

  /*!
     * @return Obtain the chunk size to use when reading streams.
     * @remarks Return 0 if this add-on won't provide this function.
     */
  virtual int GetBlockSize() { return 0; }

  /*!
     *  Check for real-time streaming
     *  @return true if current stream is real-time
     */
  virtual bool IsRealTimeStream() { return true; }

  /*!
     * @brief Allocate a demux packet. Free with FreeDemuxPacket
     * @param dataSize The size of the data that will go into the packet
     * @return The allocated packet
     */
  DemuxPacket* AllocateDemuxPacket(int dataSize)
  {
    return m_instanceData->toKodi->allocate_demux_packet(m_instanceData->toKodi->kodiInstance,
                                                         dataSize);
  }

  /*!
     * @brief Allocate a demux packet. Free with FreeDemuxPacket
     * @param dataSize The size of the data that will go into the packet
     * @return The allocated packet
     */
  DemuxPacket* AllocateEncryptedDemuxPacket(int dataSize, unsigned int encryptedSubsampleCount)
  {
    return m_instanceData->toKodi->allocate_encrypted_demux_packet(
        m_instanceData->toKodi->kodiInstance, dataSize, encryptedSubsampleCount);
  }

  /*!
     * @brief Free a packet that was allocated with AllocateDemuxPacket
     * @param packet The packet to free
     */
  void FreeDemuxPacket(DemuxPacket* packet)
  {
    return m_instanceData->toKodi->free_demux_packet(m_instanceData->toKodi->kodiInstance, packet);
  }

private:
  static int compareVersion(const int v1[3], const int v2[3])
  {
    for (unsigned i(0); i < 3; ++i)
      if (v1[i] != v2[i])
        return v1[i] - v2[i];
    return 0;
  }

  void SetAddonStruct(KODI_HANDLE instance, const std::string& kodiVersion)
  {
    if (instance == nullptr)
      throw std::logic_error("kodi::addon::CInstanceInputStream: Creation with empty addon "
                             "structure not allowed, table must be given from Kodi!");
    int api[3] = { 0, 0, 0 };
    sscanf(kodiVersion.c_str(), "%d.%d.%d", &api[0], &api[1], &api[2]);

    m_instanceData = static_cast<AddonInstance_InputStream*>(instance);
    m_instanceData->toAddon->addonInstance = this;
    m_instanceData->toAddon->open = ADDON_Open;
    m_instanceData->toAddon->close = ADDON_Close;
    m_instanceData->toAddon->get_capabilities = ADDON_GetCapabilities;

    m_instanceData->toAddon->get_stream_ids = ADDON_GetStreamIds;
    m_instanceData->toAddon->get_stream = ADDON_GetStream;
    m_instanceData->toAddon->enable_stream = ADDON_EnableStream;
    m_instanceData->toAddon->open_stream = ADDON_OpenStream;
    m_instanceData->toAddon->demux_reset = ADDON_DemuxReset;
    m_instanceData->toAddon->demux_abort = ADDON_DemuxAbort;
    m_instanceData->toAddon->demux_flush = ADDON_DemuxFlush;
    m_instanceData->toAddon->demux_read = ADDON_DemuxRead;
    m_instanceData->toAddon->demux_seek_time = ADDON_DemuxSeekTime;
    m_instanceData->toAddon->demux_set_speed = ADDON_DemuxSetSpeed;
    m_instanceData->toAddon->set_video_resolution = ADDON_SetVideoResolution;

    m_instanceData->toAddon->get_total_time = ADDON_GetTotalTime;
    m_instanceData->toAddon->get_time = ADDON_GetTime;

    m_instanceData->toAddon->get_times = ADDON_GetTimes;
    m_instanceData->toAddon->pos_time = ADDON_PosTime;

    m_instanceData->toAddon->read_stream = ADDON_ReadStream;
    m_instanceData->toAddon->seek_stream = ADDON_SeekStream;
    m_instanceData->toAddon->position_stream = ADDON_PositionStream;
    m_instanceData->toAddon->length_stream = ADDON_LengthStream;
    m_instanceData->toAddon->is_real_time_stream = ADDON_IsRealTimeStream;

    // Added on 2.0.10
    m_instanceData->toAddon->get_chapter = ADDON_GetChapter;
    m_instanceData->toAddon->get_chapter_count = ADDON_GetChapterCount;
    m_instanceData->toAddon->get_chapter_name = ADDON_GetChapterName;
    m_instanceData->toAddon->get_chapter_pos = ADDON_GetChapterPos;
    m_instanceData->toAddon->seek_chapter = ADDON_SeekChapter;

    // Added on 2.0.12
    m_instanceData->toAddon->block_size_stream = ADDON_GetBlockSize;

    /*
    // Way to include part on new API version
    int minPartVersion[3] = { 3, 0, 0 };
    if (compareVersion(api, minPartVersion) >= 0)
    {

    }
    */
  }

  inline static bool ADDON_Open(const AddonInstance_InputStream* instance,
                                INPUTSTREAM_PROPERTY* props)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->Open(*props);
  }

  inline static void ADDON_Close(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->Close();
  }

  inline static void ADDON_GetCapabilities(const AddonInstance_InputStream* instance,
                                           INPUTSTREAM_CAPABILITIES* capabilities)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->GetCapabilities(*capabilities);
  }


  // IDemux
  inline static bool ADDON_GetStreamIds(const AddonInstance_InputStream* instance,
                                        struct INPUTSTREAM_IDS* ids)
  {
    std::vector<unsigned int> idList;
    bool ret =
        static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetStreamIds(idList);
    if (ret)
    {
      for (size_t i = 0; i < idList.size() && i < INPUTSTREAM_MAX_STREAM_COUNT; ++i)
      {
        ids->m_streamCount++;
        ids->m_streamIds[i] = idList[i];
      }
    }
    return ret;
  }

  inline static bool ADDON_GetStream(
      const AddonInstance_InputStream* instance,
      int streamid,
      struct INPUTSTREAM_INFO* info,
      KODI_HANDLE* demuxStream,
      KODI_HANDLE (*transfer_stream)(KODI_HANDLE handle,
                                     int streamId,
                                     struct INPUTSTREAM_INFO* stream))
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->GetStream(streamid, info, demuxStream, transfer_stream);
  }

  inline static void ADDON_EnableStream(const AddonInstance_InputStream* instance,
                                        int streamid,
                                        bool enable)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->EnableStream(streamid, enable);
  }

  inline static bool ADDON_OpenStream(const AddonInstance_InputStream* instance, int streamid)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->OpenStream(streamid);
  }

  inline static void ADDON_DemuxReset(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->DemuxReset();
  }

  inline static void ADDON_DemuxAbort(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->DemuxAbort();
  }

  inline static void ADDON_DemuxFlush(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->DemuxFlush();
  }

  inline static DemuxPacket* ADDON_DemuxRead(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->DemuxRead();
  }

  inline static bool ADDON_DemuxSeekTime(const AddonInstance_InputStream* instance,
                                         double time,
                                         bool backwards,
                                         double* startpts)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->DemuxSeekTime(time, backwards, *startpts);
  }

  inline static void ADDON_DemuxSetSpeed(const AddonInstance_InputStream* instance, int speed)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->DemuxSetSpeed(speed);
  }

  inline static void ADDON_SetVideoResolution(const AddonInstance_InputStream* instance,
                                              int width,
                                              int height)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->SetVideoResolution(width, height);
  }


  // IDisplayTime
  inline static int ADDON_GetTotalTime(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetTotalTime();
  }

  inline static int ADDON_GetTime(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetTime();
  }

  // ITime
  inline static bool ADDON_GetTimes(const AddonInstance_InputStream* instance,
                                    INPUTSTREAM_TIMES* times)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetTimes(*times);
  }

  // IPosTime
  inline static bool ADDON_PosTime(const AddonInstance_InputStream* instance, int ms)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->PosTime(ms);
  }

  inline static int ADDON_GetChapter(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetChapter();
  }

  inline static int ADDON_GetChapterCount(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetChapterCount();
  }

  inline static const char* ADDON_GetChapterName(const AddonInstance_InputStream* instance, int ch)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetChapterName(ch);
  }

  inline static int64_t ADDON_GetChapterPos(const AddonInstance_InputStream* instance, int ch)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetChapterPos(ch);
  }

  inline static bool ADDON_SeekChapter(const AddonInstance_InputStream* instance, int ch)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->SeekChapter(ch);
  }

  inline static int ADDON_ReadStream(const AddonInstance_InputStream* instance,
                                     uint8_t* buffer,
                                     unsigned int bufferSize)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->ReadStream(buffer, bufferSize);
  }

  inline static int64_t ADDON_SeekStream(const AddonInstance_InputStream* instance,
                                         int64_t position,
                                         int whence)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->SeekStream(position, whence);
  }

  inline static int64_t ADDON_PositionStream(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->PositionStream();
  }

  inline static int64_t ADDON_LengthStream(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->LengthStream();
  }

  inline static int ADDON_GetBlockSize(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetBlockSize();
  }

  inline static bool ADDON_IsRealTimeStream(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->IsRealTimeStream();
  }

public: // temporary to have commit usable on addon
  AddonInstance_InputStream* m_instanceData;
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
