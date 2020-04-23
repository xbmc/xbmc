/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*
 * Parts with a comment named "internal" are only used inside header and not
 * used or accessed direct during add-on development!
 */

#include "../AddonBase.h"
#include "../StreamCodec.h"
#include "../StreamCrypto.h"

#ifdef BUILD_KODI_ADDON
#include "../DemuxPacket.h"
#include "../InputStreamConstants.h"
#else
#include "cores/VideoPlayer/Interface/Addon/DemuxPacket.h"
#include "cores/VideoPlayer/Interface/Addon/InputStreamConstants.h"
#endif

//Increment this level always if you add features which can lead to compile failures in the addon
#define INPUTSTREAM_VERSION_LEVEL 2

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  /*!
   * @brief InputStream add-on capabilities. All capabilities are set to "false" as default.
   */
  struct INPUTSTREAM_CAPABILITIES
  {
    enum MASKTYPE : uint32_t
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
      SUPPORTS_PAUSE = (1 << 4),

      /// supports interface ITime
      SUPPORTS_ITIME = (1 << 5),

      /// supports interface IChapter
      SUPPORTS_ICHAPTER = (1 << 6),
    };

    /// set of supported capabilities
    uint32_t m_mask;
  };

  /*!
   * @brief structure of key/value pairs passed to addon on Open()
   */
  struct INPUTSTREAM
  {
    const char* m_strURL;
    const char* m_mimeType;

    unsigned int m_nCountInfoValues;
    struct LISTITEMPROPERTY
    {
      const char* m_strKey;
      const char* m_strValue;
    } m_ListItemProperties[STREAM_MAX_PROPERTY_COUNT];

    const char* m_libFolder;
    const char* m_profileFolder;
  };

  /*!
   * @brief Array of stream IDs
   */
  struct INPUTSTREAM_IDS
  {
    static const unsigned int MAX_STREAM_COUNT = 256;
    unsigned int m_streamCount;
    unsigned int m_streamIds[MAX_STREAM_COUNT];
  };

  /*!
   * @brief MASTERING Metadata
   */
  struct INPUTSTREAM_MASTERING_METADATA
  {
    double primary_r_chromaticity_x;
    double primary_r_chromaticity_y;
    double primary_g_chromaticity_x;
    double primary_g_chromaticity_y;
    double primary_b_chromaticity_x;
    double primary_b_chromaticity_y;
    double white_point_chromaticity_x;
    double white_point_chromaticity_y;
    double luminance_max;
    double luminance_min;
  };

  /*!
  * @brief CONTENTLIGHT Metadata
  */
  struct INPUTSTREAM_CONTENTLIGHT_METADATA
  {
    uint64_t max_cll;
    uint64_t max_fall;
  };

  /*!
   * @brief stream properties
   */
  struct INPUTSTREAM_INFO
  {
    enum STREAM_TYPE
    {
      TYPE_NONE = 0,
      TYPE_VIDEO,
      TYPE_AUDIO,
      TYPE_SUBTITLE,
      TYPE_TELETEXT,
      TYPE_RDS,
    } m_streamType;

    enum Codec_FEATURES : uint32_t
    {
      FEATURE_DECODE = 1
    };
    uint32_t m_features;

    enum STREAM_FLAGS : uint32_t
    {
      FLAG_NONE = 0x0000,
      FLAG_DEFAULT = 0x0001,
      FLAG_DUB = 0x0002,
      FLAG_ORIGINAL = 0x0004,
      FLAG_COMMENT = 0x0008,
      FLAG_LYRICS = 0x0010,
      FLAG_KARAOKE = 0x0020,
      FLAG_FORCED = 0x0040,
      FLAG_HEARING_IMPAIRED = 0x0080,
      FLAG_VISUAL_IMPAIRED = 0x0100,
    };

    // Keep in sync with AVColorSpace
    enum COLORSPACE
    {
      COLORSPACE_RGB = 0,
      COLORSPACE_BT709 = 1,
      COLORSPACE_UNSPECIFIED = 2,
      COLORSPACE_UNKNOWN = COLORSPACE_UNSPECIFIED, // compatibility
      COLORSPACE_RESERVED = 3,
      COLORSPACE_FCC = 4,
      COLORSPACE_BT470BG = 5,
      COLORSPACE_SMPTE170M = 6,
      COLORSPACE_SMPTE240M = 7,
      COLORSPACE_YCGCO = 8,
      COLORSPACE_YCOCG = COLORSPACE_YCGCO,
      COLORSPACE_BT2020_NCL = 9,
      COLORSPACE_BT2020_CL = 10,
      COLORSPACE_SMPTE2085 = 11,
      COLORSPACE_CHROMA_DERIVED_NCL = 12,
      COLORSPACE_CHROMA_DERIVED_CL = 13,
      COLORSPACE_ICTCP = 14,
      COLORSPACE_MAX
    };

    // Keep in sync with AVColorPrimaries
    enum COLORPRIMARIES : int32_t
    {
      COLORPRIMARY_RESERVED0 = 0,
      COLORPRIMARY_BT709 = 1,
      COLORPRIMARY_UNSPECIFIED = 2,
      COLORPRIMARY_RESERVED = 3,
      COLORPRIMARY_BT470M = 4,
      COLORPRIMARY_BT470BG = 5,
      COLORPRIMARY_SMPTE170M = 6,
      COLORPRIMARY_SMPTE240M = 7,
      COLORPRIMARY_FILM = 8,
      COLORPRIMARY_BT2020 = 9,
      COLORPRIMARY_SMPTE428 = 10,
      COLORPRIMARY_SMPTEST428_1 = COLORPRIMARY_SMPTE428,
      COLORPRIMARY_SMPTE431 = 11,
      COLORPRIMARY_SMPTE432 = 12,
      COLORPRIMARY_JEDEC_P22 = 22,
      COLORPRIMARY_MAX
    };

    // Keep in sync with AVColorRange
    enum COLORRANGE
    {
      COLORRANGE_UNKNOWN = 0,
      COLORRANGE_LIMITED,
      COLORRANGE_FULLRANGE,
      COLORRANGE_MAX
    };

    // keep in sync with AVColorTransferCharacteristic
    enum COLORTRC : int32_t
    {
      COLORTRC_RESERVED0 = 0,
      COLORTRC_BT709 = 1,
      COLORTRC_UNSPECIFIED = 2,
      COLORTRC_RESERVED = 3,
      COLORTRC_GAMMA22 = 4,
      COLORTRC_GAMMA28 = 5,
      COLORTRC_SMPTE170M = 6,
      COLORTRC_SMPTE240M = 7,
      COLORTRC_LINEAR = 8,
      COLORTRC_LOG = 9,
      COLORTRC_LOG_SQRT = 10,
      COLORTRC_IEC61966_2_4 = 11,
      COLORTRC_BT1361_ECG = 12,
      COLORTRC_IEC61966_2_1 = 13,
      COLORTRC_BT2020_10 = 14,
      COLORTRC_BT2020_12 = 15,
      COLORTRC_SMPTE2084 = 16,
      COLORTRC_SMPTEST2084 = COLORTRC_SMPTE2084,
      COLORTRC_SMPTE428 = 17,
      COLORTRC_SMPTEST428_1 = COLORTRC_SMPTE428,
      COLORTRC_ARIB_STD_B67 = 18,
      COLORTRC_MAX
    };

    uint32_t m_flags;

    char m_name[256]; /*!< @brief (optinal) name of the stream, \0 for default handling */
    char m_codecName[32]; /*!< @brief (required) name of codec according to ffmpeg */
    char m_codecInternalName
        [32]; /*!< @brief (optional) internal name of codec (selectionstream info) */
    STREAMCODEC_PROFILE m_codecProfile; /*!< @brief (optional) the profile of the codec */
    unsigned int m_pID; /*!< @brief (required) physical index */

    const uint8_t* m_ExtraData;
    unsigned int m_ExtraSize;

    char m_language[64]; /*!< @brief RFC 5646 language code (empty string if undefined) */

    unsigned int
        m_FpsScale; /*!< @brief Scale of 1000 and a rate of 29970 will result in 29.97 fps */
    unsigned int m_FpsRate;
    unsigned int m_Height; /*!< @brief height of the stream reported by the demuxer */
    unsigned int m_Width; /*!< @brief width of the stream reported by the demuxer */
    float m_Aspect; /*!< @brief display aspect of stream */


    unsigned int m_Channels; /*!< @brief (required) amount of channels */
    unsigned int m_SampleRate; /*!< @brief (required) sample rate */
    unsigned int m_BitRate; /*!< @brief (required) bit rate */
    unsigned int m_BitsPerSample; /*!< @brief (required) bits per sample */
    unsigned int m_BlockAlign;

    CRYPTO_INFO m_cryptoInfo;

    // new in API version 2.0.8
    unsigned int m_codecFourCC; /*!< @brief Codec If available, the fourcc code codec */
    COLORSPACE m_colorSpace; /*!< @brief definition of colorspace */
    COLORRANGE m_colorRange; /*!< @brief color range if available */

    //new in API 2.0.9 / INPUTSTREAM_VERSION_LEVEL 1
    COLORPRIMARIES m_colorPrimaries;
    COLORTRC m_colorTransferCharacteristic;
    INPUTSTREAM_MASTERING_METADATA* m_masteringMetadata; /*!< @brief mastering static Metadata */
    INPUTSTREAM_CONTENTLIGHT_METADATA*
        m_contentLightMetadata; /*!< @brief content light static Metadata */
  };

  struct INPUTSTREAM_TIMES
  {
    time_t startTime;
    double ptsStart;
    double ptsBegin;
    double ptsEnd;
  };

  /*!
   * @brief "C" ABI Structures to transfer the methods from this to Kodi
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
    DemuxPacket* (*allocate_encrypted_demux_packet)(void* kodiInstance,
                                                    unsigned int data_size,
                                                    unsigned int encrypted_subsample_count);
    void (*free_demux_packet)(void* kodiInstance, DemuxPacket* packet);
  } AddonToKodiFuncTable_InputStream;

  struct AddonInstance_InputStream;
  typedef struct KodiToAddonFuncTable_InputStream /* internal */
  {
    KODI_HANDLE addonInstance;

    bool(__cdecl* open)(const AddonInstance_InputStream* instance, INPUTSTREAM* props);
    void(__cdecl* close)(const AddonInstance_InputStream* instance);
    const char*(__cdecl* get_path_list)(const AddonInstance_InputStream* instance);
    void(__cdecl* get_capabilities)(const AddonInstance_InputStream* instance,
                                    INPUTSTREAM_CAPABILITIES* capabilities);

    // IDemux
    struct INPUTSTREAM_IDS(__cdecl* get_stream_ids)(const AddonInstance_InputStream* instance);
    struct INPUTSTREAM_INFO(__cdecl* get_stream)(const AddonInstance_InputStream* instance,
                                                 int streamid);
    void(__cdecl* enable_stream)(const AddonInstance_InputStream* instance,
                                 int streamid,
                                 bool enable);
    bool(__cdecl* open_stream)(const AddonInstance_InputStream* instance, int streamid);
    void(__cdecl* demux_reset)(const AddonInstance_InputStream* instance);
    void(__cdecl* demux_abort)(const AddonInstance_InputStream* instance);
    void(__cdecl* demux_flush)(const AddonInstance_InputStream* instance);
    DemuxPacket*(__cdecl* demux_read)(const AddonInstance_InputStream* instance);
    bool(__cdecl* demux_seek_time)(const AddonInstance_InputStream* instance,
                                   double time,
                                   bool backwards,
                                   double* startpts);
    void(__cdecl* demux_set_speed)(const AddonInstance_InputStream* instance, int speed);
    void(__cdecl* set_video_resolution)(const AddonInstance_InputStream* instance,
                                        int width,
                                        int height);

    // IDisplayTime
    int(__cdecl* get_total_time)(const AddonInstance_InputStream* instance);
    int(__cdecl* get_time)(const AddonInstance_InputStream* instance);

    // ITime
    bool(__cdecl* get_times)(const AddonInstance_InputStream* instance, INPUTSTREAM_TIMES* times);

    // IPosTime
    bool(__cdecl* pos_time)(const AddonInstance_InputStream* instance, int ms);

    int(__cdecl* read_stream)(const AddonInstance_InputStream* instance,
                              uint8_t* buffer,
                              unsigned int bufferSize);
    int64_t(__cdecl* seek_stream)(const AddonInstance_InputStream* instance,
                                  int64_t position,
                                  int whence);
    int64_t(__cdecl* position_stream)(const AddonInstance_InputStream* instance);
    int64_t(__cdecl* length_stream)(const AddonInstance_InputStream* instance);
    bool(__cdecl* is_real_time_stream)(const AddonInstance_InputStream* instance);

    // IChapter
    int(__cdecl* get_chapter)(const AddonInstance_InputStream* instance);
    int(__cdecl* get_chapter_count)(const AddonInstance_InputStream* instance);
    const char*(__cdecl* get_chapter_name)(const AddonInstance_InputStream* instance, int ch);
    int64_t(__cdecl* get_chapter_pos)(const AddonInstance_InputStream* instance, int ch);
    bool(__cdecl* seek_chapter)(const AddonInstance_InputStream* instance, int ch);

    int(__cdecl* block_size_stream)(const AddonInstance_InputStream* instance);
  } KodiToAddonFuncTable_InputStream;

  typedef struct AddonInstance_InputStream /* internal */
  {
    AddonProps_InputStream props;
    AddonToKodiFuncTable_InputStream toKodi;
    KodiToAddonFuncTable_InputStream toAddon;
  } AddonInstance_InputStream;

#ifdef __cplusplus
} /* extern "C" */

namespace kodi
{
namespace addon
{

class CInstanceInputStream : public IAddonInstance
{
public:
  explicit CInstanceInputStream(KODI_HANDLE instance, const std::string& kodiVersion = "0.0.0")
    : IAddonInstance(ADDON_INSTANCE_INPUTSTREAM)
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceInputStream: Creation of multiple together "
                             "with single instance way is not allowed!");

    SetAddonStruct(instance, kodiVersion);
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
    return m_instanceData->toKodi.allocate_demux_packet(m_instanceData->toKodi.kodiInstance,
                                                        dataSize);
  }

  /*!
     * @brief Allocate a demux packet. Free with FreeDemuxPacket
     * @param dataSize The size of the data that will go into the packet
     * @return The allocated packet
     */
  DemuxPacket* AllocateEncryptedDemuxPacket(int dataSize, unsigned int encryptedSubsampleCount)
  {
    return m_instanceData->toKodi.allocate_encrypted_demux_packet(
        m_instanceData->toKodi.kodiInstance, dataSize, encryptedSubsampleCount);
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

    m_instanceData->toAddon.get_times = ADDON_GetTimes;
    m_instanceData->toAddon.pos_time = ADDON_PosTime;

    m_instanceData->toAddon.read_stream = ADDON_ReadStream;
    m_instanceData->toAddon.seek_stream = ADDON_SeekStream;
    m_instanceData->toAddon.position_stream = ADDON_PositionStream;
    m_instanceData->toAddon.length_stream = ADDON_LengthStream;
    m_instanceData->toAddon.is_real_time_stream = ADDON_IsRealTimeStream;

    int minChapterVersion[3] = { 2, 0, 10 };
    if (compareVersion(api, minChapterVersion) >= 0)
    {
      m_instanceData->toAddon.get_chapter = ADDON_GetChapter;
      m_instanceData->toAddon.get_chapter_count = ADDON_GetChapterCount;
      m_instanceData->toAddon.get_chapter_name = ADDON_GetChapterName;
      m_instanceData->toAddon.get_chapter_pos = ADDON_GetChapterPos;
      m_instanceData->toAddon.seek_chapter = ADDON_SeekChapter;
    }

    int minBlockSizeVersion[3] = {2, 0, 12};
    if (compareVersion(api, minBlockSizeVersion) >= 0)
    {
      m_instanceData->toAddon.block_size_stream = ADDON_GetBlockSize;
    }
  }

  inline static bool ADDON_Open(const AddonInstance_InputStream* instance, INPUTSTREAM* props)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->Open(*props);
  }

  inline static void ADDON_Close(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->Close();
  }

  inline static void ADDON_GetCapabilities(const AddonInstance_InputStream* instance,
                                           INPUTSTREAM_CAPABILITIES* capabilities)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)
        ->GetCapabilities(*capabilities);
  }


  // IDemux
  inline static struct INPUTSTREAM_IDS ADDON_GetStreamIds(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->GetStreamIds();
  }

  inline static struct INPUTSTREAM_INFO ADDON_GetStream(const AddonInstance_InputStream* instance,
                                                        int streamid)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->GetStream(streamid);
  }

  inline static void ADDON_EnableStream(const AddonInstance_InputStream* instance,
                                        int streamid,
                                        bool enable)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)
        ->EnableStream(streamid, enable);
  }

  inline static bool ADDON_OpenStream(const AddonInstance_InputStream* instance, int streamid)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)
        ->OpenStream(streamid);
  }

  inline static void ADDON_DemuxReset(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->DemuxReset();
  }

  inline static void ADDON_DemuxAbort(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->DemuxAbort();
  }

  inline static void ADDON_DemuxFlush(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->DemuxFlush();
  }

  inline static DemuxPacket* ADDON_DemuxRead(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->DemuxRead();
  }

  inline static bool ADDON_DemuxSeekTime(const AddonInstance_InputStream* instance,
                                         double time,
                                         bool backwards,
                                         double* startpts)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)
        ->DemuxSeekTime(time, backwards, *startpts);
  }

  inline static void ADDON_DemuxSetSpeed(const AddonInstance_InputStream* instance, int speed)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->DemuxSetSpeed(speed);
  }

  inline static void ADDON_SetVideoResolution(const AddonInstance_InputStream* instance,
                                              int width,
                                              int height)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)
        ->SetVideoResolution(width, height);
  }


  // IDisplayTime
  inline static int ADDON_GetTotalTime(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->GetTotalTime();
  }

  inline static int ADDON_GetTime(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->GetTime();
  }

  // ITime
  inline static bool ADDON_GetTimes(const AddonInstance_InputStream* instance,
                                    INPUTSTREAM_TIMES* times)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->GetTimes(*times);
  }

  // IPosTime
  inline static bool ADDON_PosTime(const AddonInstance_InputStream* instance, int ms)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->PosTime(ms);
  }

  inline static int ADDON_GetChapter(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->GetChapter();
  }

  inline static int ADDON_GetChapterCount(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->GetChapterCount();
  }

  inline static const char* ADDON_GetChapterName(const AddonInstance_InputStream* instance, int ch)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->GetChapterName(ch);
  }

  inline static int64_t ADDON_GetChapterPos(const AddonInstance_InputStream* instance, int ch)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->GetChapterPos(ch);
  }

  inline static bool ADDON_SeekChapter(const AddonInstance_InputStream* instance, int ch)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->SeekChapter(ch);
  }

  inline static int ADDON_ReadStream(const AddonInstance_InputStream* instance,
                                     uint8_t* buffer,
                                     unsigned int bufferSize)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)
        ->ReadStream(buffer, bufferSize);
  }

  inline static int64_t ADDON_SeekStream(const AddonInstance_InputStream* instance,
                                         int64_t position,
                                         int whence)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)
        ->SeekStream(position, whence);
  }

  inline static int64_t ADDON_PositionStream(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->PositionStream();
  }

  inline static int64_t ADDON_LengthStream(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->LengthStream();
  }

  inline static int ADDON_GetBlockSize(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->GetBlockSize();
  }

  inline static bool ADDON_IsRealTimeStream(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon.addonInstance)->IsRealTimeStream();
  }

  AddonInstance_InputStream* m_instanceData;
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
