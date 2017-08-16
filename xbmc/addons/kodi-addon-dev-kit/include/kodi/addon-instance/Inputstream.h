#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
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

/*
 * Parts with a comment named "internal" are only used inside header and not
 * used or accessed direct during add-on development!
 */

#include "../AddonBase.h"
#include "../StreamCrypto.h"
#include "../StreamCodec.h"

#ifdef BUILD_KODI_ADDON
#include "../DVDDemuxPacket.h"
#else
#include "cores/VideoPlayer/DVDDemuxers/DVDDemuxPacket.h"
#endif

namespace kodi { namespace addon { class CInstanceInputStream; }}

extern "C" {

  /*!
   * @brief InputStream add-on capabilities. All capabilities are set to "false" as default.
   */
  typedef struct INPUTSTREAM_CAPABILITIES
  {
    enum MASKTYPE: uint32_t
    {
      /// supports interface IDemux
      SUPPORTS_IDEMUX = (1 << 0),

      /// supports interface IPosTime
      SUPPORTS_IPOSTIME = (1 << 1),

      /// supports interface IDisplayTime
      SUPPORTS_IDISPLAYTIME = (1 << 2),

      /// supports seek
      SUPPORTS_SEEK = (1 << 3),

      /// supports pause
      SUPPORTS_PAUSE = (1 << 4)
    };

    /// set of supported capabilities
    uint32_t m_mask;
  } INPUTSTREAM_CAPABILITIES;

  /*!
   * @brief structure of key/value pairs passed to addon on Open()
   */
  typedef struct INPUTSTREAM
  {
    static const unsigned int MAX_INFO_COUNT = 8;

    const char *m_strURL;

    unsigned int m_nCountInfoValues;
    struct LISTITEMPROPERTY
    {
      const char *m_strKey;
      const char *m_strValue;
    } m_ListItemProperties[MAX_INFO_COUNT];

    const char *m_libFolder;
    const char *m_profileFolder;
  } INPUTSTREAM;

  /*!
   * @brief Array of stream IDs
   */
  typedef struct INPUTSTREAM_IDS
  {
    static const unsigned int MAX_STREAM_COUNT = 32;
    unsigned int m_streamCount;
    unsigned int m_streamIds[MAX_STREAM_COUNT];
  } INPUTSTREAM_IDS;

  /*!
   * @brief stream properties
   */
  typedef struct INPUTSTREAM_INFO
  {
    enum STREAM_TYPE
    {
      TYPE_NONE,
      TYPE_VIDEO,
      TYPE_AUDIO,
      TYPE_SUBTITLE,
      TYPE_TELETEXT
    } m_streamType;

    enum Codec_FEATURES
    {
      FEATURE_DECODE = 1
    };
    unsigned int m_features;

    char m_codecName[32];                /*!< @brief (required) name of codec according to ffmpeg */
    char m_codecInternalName[32];        /*!< @brief (optional) internal name of codec (selectionstream info) */
    STREAMCODEC_PROFILE m_codecProfile;  /*!< @brief (optional) the profile of the codec */
    unsigned int m_pID;                  /*!< @brief (required) physical index */

    const uint8_t *m_ExtraData;
    unsigned int m_ExtraSize;

    char m_language[4];                  /*!< @brief ISO 639 3-letter language code (empty string if undefined) */

    unsigned int m_FpsScale;             /*!< @brief Scale of 1000 and a rate of 29970 will result in 29.97 fps */
    unsigned int m_FpsRate;
    unsigned int m_Height;               /*!< @brief height of the stream reported by the demuxer */
    unsigned int m_Width;                /*!< @brief width of the stream reported by the demuxer */
    float m_Aspect;                      /*!< @brief display aspect of stream */

    unsigned int m_Channels;             /*!< @brief (required) amount of channels */
    unsigned int m_SampleRate;           /*!< @brief (required) sample rate */
    unsigned int m_BitRate;              /*!< @brief (required) bit rate */
    unsigned int m_BitsPerSample;        /*!< @brief (required) bits per sample */
    unsigned int m_BlockAlign;

    CRYPTO_INFO m_cryptoInfo;
  } INPUTSTREAM_INFO;

  /*!
   * @brief Structure to transfer the methods from xbmc_inputstream_dll.h to XBMC
   */

  // this are properties given to the addon on create
  // at this time we have no parameters for the addon
  typedef struct AddonProps_InputStream /* internal */
  {
    int dummy;
  } AddonProps_InputStream;

  typedef struct AddonToKodiFuncTable_InputStream /* internal */
  {
    KODI_HANDLE kodiInstance;
    DemuxPacket* (*allocate_demux_packet)(void* kodiInstance, int data_size);
    DemuxPacket* (*allocate_encrypted_demux_packet)(void* kodiInstance, unsigned int data_size, unsigned int encrypted_subsample_count);
    void (*free_demux_packet)(void* kodiInstance, DemuxPacket* packet);
  } AddonToKodiFuncTable_InputStream;

  struct AddonInstance_InputStream;
  typedef struct KodiToAddonFuncTable_InputStream /* internal */
  {
    kodi::addon::CInstanceInputStream* addonInstance;

    bool (__cdecl* open)(const AddonInstance_InputStream* instance, INPUTSTREAM* props);
    void (__cdecl* close)(const AddonInstance_InputStream* instance);
    const char* (__cdecl* get_path_list)(const AddonInstance_InputStream* instance);
    void (__cdecl* get_capabilities)(const AddonInstance_InputStream* instance, INPUTSTREAM_CAPABILITIES* capabilities);

    // IDemux
    struct INPUTSTREAM_IDS (__cdecl* get_stream_ids)(const AddonInstance_InputStream* instance);
    struct INPUTSTREAM_INFO (__cdecl* get_stream)(const AddonInstance_InputStream* instance, int streamid);
    void (__cdecl* enable_stream)(const AddonInstance_InputStream* instance, int streamid, bool enable);
    bool(__cdecl* open_stream)(const AddonInstance_InputStream* instance, int streamid);
    void (__cdecl* demux_reset)(const AddonInstance_InputStream* instance);
    void (__cdecl* demux_abort)(const AddonInstance_InputStream* instance);
    void (__cdecl* demux_flush)(const AddonInstance_InputStream* instance);
    DemuxPacket* (__cdecl* demux_read)(const AddonInstance_InputStream* instance);
    bool (__cdecl* demux_seek_time)(const AddonInstance_InputStream* instance, double time, bool backwards, double* startpts);
    void (__cdecl* demux_set_speed)(const AddonInstance_InputStream* instance, int speed);
    void (__cdecl* set_video_resolution)(const AddonInstance_InputStream* instance, int width, int height);

    // IDisplayTime
    int (__cdecl* get_total_time)(const AddonInstance_InputStream* instance);
    int (__cdecl* get_time)(const AddonInstance_InputStream* instance);

    // IPosTime
    bool (__cdecl* pos_time)(const AddonInstance_InputStream* instance, int ms);

    // Seekable (mandatory)
    bool (__cdecl* can_pause_stream)(const AddonInstance_InputStream* instance);
    bool (__cdecl* can_seek_stream)(const AddonInstance_InputStream* instance);

    int (__cdecl* read_stream)(const AddonInstance_InputStream* instance, uint8_t* buffer, unsigned int bufferSize);
    int64_t(__cdecl* seek_stream)(const AddonInstance_InputStream* instance, int64_t position, int whence);
    int64_t (__cdecl* position_stream)(const AddonInstance_InputStream* instance);
    int64_t (__cdecl* length_stream)(const AddonInstance_InputStream* instance);
    void (__cdecl* pause_stream)(const AddonInstance_InputStream* instance, double time);
    bool (__cdecl* is_real_time_stream)(const AddonInstance_InputStream* instance);
  } KodiToAddonFuncTable_InputStream;

  typedef struct AddonInstance_InputStream /* internal */
  {
    AddonProps_InputStream props;
    AddonToKodiFuncTable_InputStream toKodi;
    KodiToAddonFuncTable_InputStream toAddon;
  } AddonInstance_InputStream;

} /* extern "C" */

namespace kodi
{
namespace addon
{

  class CInstanceInputStream : public IAddonInstance
  {
  public:
    explicit CInstanceInputStream(KODI_HANDLE instance)
      : IAddonInstance(ADDON_INSTANCE_INPUTSTREAM)
    {
      if (CAddonBase::m_interface->globalSingleInstance != nullptr)
        throw std::logic_error("kodi::addon::CInstanceInputStream: Creation of multiple together with single instance way is not allowed!");

      SetAddonStruct(instance);
    }

    ~CInstanceInputStream() override = default;

    /*!
     * Open a stream.
     * @param props
     * @return True if the stream has been opened successfully, false otherwise.
     * @remarks
     */
    virtual bool Open(INPUTSTREAM& props) = 0;

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
    virtual INPUTSTREAM_IDS GetStreamIds() = 0;

    /*!
     * Get stream properties of a stream.
     * @param streamid unique id of stream
     * @return struc of stream properties
     * @remarks
     */
    virtual INPUTSTREAM_INFO GetStream(int streamid) = 0;

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
    virtual void DemuxReset() { }

    /*!
     * Abort the demultiplexer thread in the add-on.
     * @remarks Required if bHandlesDemuxing is set to true.
     */
    virtual void DemuxAbort() { }

    /*!
     * Flush all data that's currently in the demultiplexer buffer in the add-on.
     * @remarks Required if bHandlesDemuxing is set to true.
     */
    virtual void DemuxFlush() { }

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
    virtual bool DemuxSeekTime(double time, bool backwards, double &startpts) { return false; }

    /*!
     * Notify the InputStream addon/demuxer that Kodi wishes to change playback speed
     * @param speed The requested playback speed
     * @remarks Optional, and only used if addon has its own demuxer.
     */
    virtual void DemuxSetSpeed(int speed) { }

    /*!
     * Sets desired width / height
     * @param width / hight
     */
    virtual void SetVideoResolution(int width, int height) { }

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
     * Positions inputstream to playing time given in ms
     * @remarks
     */
    virtual bool PosTime(int ms) { return false; }


    /*!
     * Check if the backend support pausing the currently playing stream
     * This will enable/disable the pause button in Kodi based on the return value
     * @return false if the InputStream addon/backend does not support pausing, true if possible
     */
    virtual bool CanPauseStream() { return false; }

    /*!
     * Check if the backend supports seeking for the currently playing stream
     * This will enable/disable the rewind/forward buttons in Kodi based on the return value
     * @return false if the InputStream addon/backend does not support seeking, true if possible
     */
    virtual bool CanSeekStream() { return false; }

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
     * @brief Notify the InputStream addon that Kodi (un)paused the currently playing stream
     */
    virtual void PauseStream(double time) { }


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
      return m_instanceData->toKodi.allocate_demux_packet(m_instanceData->toKodi.kodiInstance, dataSize);
    }

    /*!
     * @brief Allocate a demux packet. Free with FreeDemuxPacket
     * @param dataSize The size of the data that will go into the packet
     * @return The allocated packet
     */
    DemuxPacket* AllocateEncryptedDemuxPacket(int dataSize, unsigned int encryptedSubsampleCount)
    {
      return m_instanceData->toKodi.allocate_encrypted_demux_packet(m_instanceData->toKodi.kodiInstance, dataSize, encryptedSubsampleCount);
    }

    /*!
     * @brief Free a packet that was allocated with AllocateDemuxPacket
     * @param packet The packet to free
     */
    void FreeDemuxPacket(DemuxPacket* packet)
    {
      return m_instanceData->toKodi.free_demux_packet(m_instanceData->toKodi.kodiInstance, packet);
    }

  private:
    void SetAddonStruct(KODI_HANDLE instance)
    {
      if (instance == nullptr)
        throw std::logic_error("kodi::addon::CInstanceInputStream: Creation with empty addon structure not allowed, table must be given from Kodi!");

      m_instanceData = static_cast<AddonInstance_InputStream*>(instance);
      m_instanceData->toAddon.addonInstance = this;
      m_instanceData->toAddon.open = ADDON_Open;
      m_instanceData->toAddon.close = ADDON_Close;
      m_instanceData->toAddon.get_capabilities = ADDON_GetCapabilities;

      m_instanceData->toAddon.get_stream_ids = ADDON_GetStreamIds;
      m_instanceData->toAddon.get_stream = ADDON_GetStream;
      m_instanceData->toAddon.enable_stream = ADDON_EnableStream;
      m_instanceData->toAddon.open_stream = ADDON_OpenStream;
      m_instanceData->toAddon.demux_reset = ADDON_DemuxReset;
      m_instanceData->toAddon.demux_abort = ADDON_DemuxAbort;
      m_instanceData->toAddon.demux_flush = ADDON_DemuxFlush;
      m_instanceData->toAddon.demux_read = ADDON_DemuxRead;
      m_instanceData->toAddon.demux_seek_time = ADDON_DemuxSeekTime;
      m_instanceData->toAddon.demux_set_speed = ADDON_DemuxSetSpeed;
      m_instanceData->toAddon.set_video_resolution = ADDON_SetVideoResolution;

      m_instanceData->toAddon.get_total_time = ADDON_GetTotalTime;
      m_instanceData->toAddon.get_time = ADDON_GetTime;

      m_instanceData->toAddon.pos_time = ADDON_PosTime;

      m_instanceData->toAddon.can_pause_stream = ADDON_CanPauseStream;
      m_instanceData->toAddon.can_seek_stream = ADDON_CanSeekStream;

      m_instanceData->toAddon.read_stream = ADDON_ReadStream;
      m_instanceData->toAddon.seek_stream = ADDON_SeekStream;
      m_instanceData->toAddon.position_stream = ADDON_PositionStream;
      m_instanceData->toAddon.length_stream = ADDON_LengthStream;
      m_instanceData->toAddon.pause_stream = ADDON_PauseStream;
      m_instanceData->toAddon.is_real_time_stream = ADDON_IsRealTimeStream;
    }

    inline static bool ADDON_Open(const AddonInstance_InputStream* instance, INPUTSTREAM* props)
    {
      return instance->toAddon.addonInstance->Open(*props);
    }

    inline static void ADDON_Close(const AddonInstance_InputStream* instance)
    {
      instance->toAddon.addonInstance->Close();
    }

    inline static void ADDON_GetCapabilities(const AddonInstance_InputStream* instance, INPUTSTREAM_CAPABILITIES* capabilities)
    {
      instance->toAddon.addonInstance->GetCapabilities(*capabilities);
    }


    // IDemux
    inline static struct INPUTSTREAM_IDS ADDON_GetStreamIds(const AddonInstance_InputStream* instance)
    {
      return instance->toAddon.addonInstance->GetStreamIds();
    }

    inline static struct INPUTSTREAM_INFO ADDON_GetStream(const AddonInstance_InputStream* instance, int streamid)
    {
      return instance->toAddon.addonInstance->GetStream(streamid);
    }

    inline static void ADDON_EnableStream(const AddonInstance_InputStream* instance, int streamid, bool enable)
    {
      instance->toAddon.addonInstance->EnableStream(streamid, enable);
    }

    inline static bool ADDON_OpenStream(const AddonInstance_InputStream* instance, int streamid)
    {
      return instance->toAddon.addonInstance->OpenStream(streamid);
    }

    inline static void ADDON_DemuxReset(const AddonInstance_InputStream* instance)
    {
      instance->toAddon.addonInstance->DemuxReset();
    }

    inline static void ADDON_DemuxAbort(const AddonInstance_InputStream* instance)
    {
      instance->toAddon.addonInstance->DemuxAbort();
    }

    inline static void ADDON_DemuxFlush(const AddonInstance_InputStream* instance)
    {
      instance->toAddon.addonInstance->DemuxFlush();
    }

    inline static DemuxPacket* ADDON_DemuxRead(const AddonInstance_InputStream* instance)
    {
      return instance->toAddon.addonInstance->DemuxRead();
    }

    inline static bool ADDON_DemuxSeekTime(const AddonInstance_InputStream* instance, double time, bool backwards, double *startpts)
    {
      return instance->toAddon.addonInstance->DemuxSeekTime(time, backwards, *startpts);
    }

    inline static void ADDON_DemuxSetSpeed(const AddonInstance_InputStream* instance, int speed)
    {
      instance->toAddon.addonInstance->DemuxSetSpeed(speed);
    }

    inline static void ADDON_SetVideoResolution(const AddonInstance_InputStream* instance, int width, int height)
    {
      instance->toAddon.addonInstance->SetVideoResolution(width, height);
    }


    // IDisplayTime
    inline static int ADDON_GetTotalTime(const AddonInstance_InputStream* instance)
    {
      return instance->toAddon.addonInstance->GetTotalTime();
    }

    inline static int ADDON_GetTime(const AddonInstance_InputStream* instance)
    {
      return instance->toAddon.addonInstance->GetTime();
    }


    // IPosTime
    inline static bool ADDON_PosTime(const AddonInstance_InputStream* instance, int ms)
    {
      return instance->toAddon.addonInstance->PosTime(ms);
    }

    // Seekable (mandatory)
    inline static bool ADDON_CanPauseStream(const AddonInstance_InputStream* instance)
    {
      return instance->toAddon.addonInstance->CanPauseStream();
    }

    inline static bool ADDON_CanSeekStream(const AddonInstance_InputStream* instance)
    {
      return instance->toAddon.addonInstance->CanSeekStream();
    }


    inline static int ADDON_ReadStream(const AddonInstance_InputStream* instance, uint8_t* buffer, unsigned int bufferSize)
    {
      return instance->toAddon.addonInstance->ReadStream(buffer, bufferSize);
    }

    inline static int64_t ADDON_SeekStream(const AddonInstance_InputStream* instance, int64_t position, int whence)
    {
      return instance->toAddon.addonInstance->SeekStream(position, whence);
    }

    inline static int64_t ADDON_PositionStream(const AddonInstance_InputStream* instance)
    {
      return instance->toAddon.addonInstance->PositionStream();
    }

    inline static int64_t ADDON_LengthStream(const AddonInstance_InputStream* instance)
    {
      return instance->toAddon.addonInstance->LengthStream();
    }

    inline static void ADDON_PauseStream(const AddonInstance_InputStream* instance, double time)
    {
      instance->toAddon.addonInstance->PauseStream(time);
    }

    inline static bool ADDON_IsRealTimeStream(const AddonInstance_InputStream* instance)
    {
      return instance->toAddon.addonInstance->IsRealTimeStream();
    }

    AddonInstance_InputStream* m_instanceData;
  };

} /* namespace addon */
} /* namespace kodi */
