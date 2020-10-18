/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifndef C_API_ADDONINSTANCE_INPUTSTREAM_H
#define C_API_ADDONINSTANCE_INPUTSTREAM_H

#include "../addon_base.h"
#include "inputstream/demux_packet.h"
#include "inputstream/stream_codec.h"
#include "inputstream/stream_constants.h"
#include "inputstream/stream_crypto.h"
#include "inputstream/timing_constants.h"

#include <time.h>

// Increment this level always if you add features which can lead to compile failures in the addon
#define INPUTSTREAM_VERSION_LEVEL 2

#define INPUTSTREAM_MAX_INFO_COUNT 8
#define INPUTSTREAM_MAX_STREAM_COUNT 256
#define INPUTSTREAM_MAX_STRING_NAME_SIZE 256
#define INPUTSTREAM_MAX_STRING_CODEC_SIZE 32
#define INPUTSTREAM_MAX_STRING_LANGUAGE_SIZE 64

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  enum INPUTSTREAM_MASKTYPE
  {
    /// supports interface IDemux
    INPUTSTREAM_SUPPORTS_IDEMUX = (1 << 0),

    /// supports interface IPosTime
    INPUTSTREAM_SUPPORTS_IPOSTIME = (1 << 1),

    /// supports interface IDisplayTime
    INPUTSTREAM_SUPPORTS_IDISPLAYTIME = (1 << 2),

    /// supports seek
    INPUTSTREAM_SUPPORTS_SEEK = (1 << 3),

    /// supports pause
    INPUTSTREAM_SUPPORTS_PAUSE = (1 << 4),

    /// supports interface ITime
    INPUTSTREAM_SUPPORTS_ITIME = (1 << 5),

    /// supports interface IChapter
    INPUTSTREAM_SUPPORTS_ICHAPTER = (1 << 6),
  };

  /*!
   * @brief InputStream add-on capabilities. All capabilities are set to "false" as default.
   */
  struct INPUTSTREAM_CAPABILITIES
  {
    /// set of supported capabilities
    uint32_t m_mask;
  };

  /*!
   * @brief structure of key/value pairs passed to addon on Open()
   */
  struct INPUTSTREAM_PROPERTY
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
    unsigned int m_streamCount;
    unsigned int m_streamIds[INPUTSTREAM_MAX_STREAM_COUNT];
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

  enum INPUTSTREAM_TYPE
  {
    INPUTSTREAM_TYPE_NONE = 0,
    INPUTSTREAM_TYPE_VIDEO,
    INPUTSTREAM_TYPE_AUDIO,
    INPUTSTREAM_TYPE_SUBTITLE,
    INPUTSTREAM_TYPE_TELETEXT,
    INPUTSTREAM_TYPE_RDS,
  };

  enum INPUTSTREAM_CODEC_FEATURES
  {
    INPUTSTREAM_FEATURE_NONE = 0,
    INPUTSTREAM_FEATURE_DECODE = (1 << 0)
  };

  enum INPUTSTREAM_FLAGS
  {
    INPUTSTREAM_FLAG_NONE = (1 << 0),
    INPUTSTREAM_FLAG_DEFAULT = (1 << 1),
    INPUTSTREAM_FLAG_DUB = (1 << 2),
    INPUTSTREAM_FLAG_ORIGINAL = (1 << 3),
    INPUTSTREAM_FLAG_COMMENT = (1 << 4),
    INPUTSTREAM_FLAG_LYRICS = (1 << 5),
    INPUTSTREAM_FLAG_KARAOKE = (1 << 6),
    INPUTSTREAM_FLAG_FORCED = (1 << 7),
    INPUTSTREAM_FLAG_HEARING_IMPAIRED = (1 << 8),
    INPUTSTREAM_FLAG_VISUAL_IMPAIRED = (1 << 9),
  };

  // Keep in sync with AVColorSpace
  enum INPUTSTREAM_COLORSPACE
  {
    INPUTSTREAM_COLORSPACE_RGB = 0,
    INPUTSTREAM_COLORSPACE_BT709 = 1,
    INPUTSTREAM_COLORSPACE_UNSPECIFIED = 2,
    INPUTSTREAM_COLORSPACE_UNKNOWN = INPUTSTREAM_COLORSPACE_UNSPECIFIED, // compatibility
    INPUTSTREAM_COLORSPACE_RESERVED = 3,
    INPUTSTREAM_COLORSPACE_FCC = 4,
    INPUTSTREAM_COLORSPACE_BT470BG = 5,
    INPUTSTREAM_COLORSPACE_SMPTE170M = 6,
    INPUTSTREAM_COLORSPACE_SMPTE240M = 7,
    INPUTSTREAM_COLORSPACE_YCGCO = 8,
    INPUTSTREAM_COLORSPACE_YCOCG = INPUTSTREAM_COLORSPACE_YCGCO,
    INPUTSTREAM_COLORSPACE_BT2020_NCL = 9,
    INPUTSTREAM_COLORSPACE_BT2020_CL = 10,
    INPUTSTREAM_COLORSPACE_SMPTE2085 = 11,
    INPUTSTREAM_COLORSPACE_CHROMA_DERIVED_NCL = 12,
    INPUTSTREAM_COLORSPACE_CHROMA_DERIVED_CL = 13,
    INPUTSTREAM_COLORSPACE_ICTCP = 14,
    INPUTSTREAM_COLORSPACE_MAX
  };

  // Keep in sync with AVColorPrimaries
  enum INPUTSTREAM_COLORPRIMARIES
  {
    INPUTSTREAM_COLORPRIMARY_RESERVED0 = 0,
    INPUTSTREAM_COLORPRIMARY_BT709 = 1,
    INPUTSTREAM_COLORPRIMARY_UNSPECIFIED = 2,
    INPUTSTREAM_COLORPRIMARY_RESERVED = 3,
    INPUTSTREAM_COLORPRIMARY_BT470M = 4,
    INPUTSTREAM_COLORPRIMARY_BT470BG = 5,
    INPUTSTREAM_COLORPRIMARY_SMPTE170M = 6,
    INPUTSTREAM_COLORPRIMARY_SMPTE240M = 7,
    INPUTSTREAM_COLORPRIMARY_FILM = 8,
    INPUTSTREAM_COLORPRIMARY_BT2020 = 9,
    INPUTSTREAM_COLORPRIMARY_SMPTE428 = 10,
    INPUTSTREAM_COLORPRIMARY_SMPTEST428_1 = INPUTSTREAM_COLORPRIMARY_SMPTE428,
    INPUTSTREAM_COLORPRIMARY_SMPTE431 = 11,
    INPUTSTREAM_COLORPRIMARY_SMPTE432 = 12,
    INPUTSTREAM_COLORPRIMARY_JEDEC_P22 = 22,
    INPUTSTREAM_COLORPRIMARY_MAX
  };

  // Keep in sync with AVColorRange
  enum INPUTSTREAM_COLORRANGE
  {
    INPUTSTREAM_COLORRANGE_UNKNOWN = 0,
    INPUTSTREAM_COLORRANGE_LIMITED,
    INPUTSTREAM_COLORRANGE_FULLRANGE,
    INPUTSTREAM_COLORRANGE_MAX
  };

  // keep in sync with AVColorTransferCharacteristic
  enum INPUTSTREAM_COLORTRC
  {
    INPUTSTREAM_COLORTRC_RESERVED0 = 0,
    INPUTSTREAM_COLORTRC_BT709 = 1,
    INPUTSTREAM_COLORTRC_UNSPECIFIED = 2,
    INPUTSTREAM_COLORTRC_RESERVED = 3,
    INPUTSTREAM_COLORTRC_GAMMA22 = 4,
    INPUTSTREAM_COLORTRC_GAMMA28 = 5,
    INPUTSTREAM_COLORTRC_SMPTE170M = 6,
    INPUTSTREAM_COLORTRC_SMPTE240M = 7,
    INPUTSTREAM_COLORTRC_LINEAR = 8,
    INPUTSTREAM_COLORTRC_LOG = 9,
    INPUTSTREAM_COLORTRC_LOG_SQRT = 10,
    INPUTSTREAM_COLORTRC_IEC61966_2_4 = 11,
    INPUTSTREAM_COLORTRC_BT1361_ECG = 12,
    INPUTSTREAM_COLORTRC_IEC61966_2_1 = 13,
    INPUTSTREAM_COLORTRC_BT2020_10 = 14,
    INPUTSTREAM_COLORTRC_BT2020_12 = 15,
    INPUTSTREAM_COLORTRC_SMPTE2084 = 16,
    INPUTSTREAM_COLORTRC_SMPTEST2084 = INPUTSTREAM_COLORTRC_SMPTE2084,
    INPUTSTREAM_COLORTRC_SMPTE428 = 17,
    INPUTSTREAM_COLORTRC_SMPTEST428_1 = INPUTSTREAM_COLORTRC_SMPTE428,
    INPUTSTREAM_COLORTRC_ARIB_STD_B67 = 18,
    INPUTSTREAM_COLORTRC_MAX
  };

  /*!
   * @brief stream properties
   */
  struct INPUTSTREAM_INFO
  {
    enum INPUTSTREAM_TYPE m_streamType;
    uint32_t m_features;
    uint32_t m_flags;

    //! @brief (optional) name of the stream, \0 for default handling
    char m_name[INPUTSTREAM_MAX_STRING_NAME_SIZE];

    //! @brief (required) name of codec according to ffmpeg
    char m_codecName[INPUTSTREAM_MAX_STRING_CODEC_SIZE];

    //! @brief (optional) internal name of codec (selectionstream info)
    char m_codecInternalName[INPUTSTREAM_MAX_STRING_CODEC_SIZE];

    //! @brief (optional) the profile of the codec
    enum STREAMCODEC_PROFILE m_codecProfile;

    //! @brief (required) physical index
    unsigned int m_pID;

    const uint8_t* m_ExtraData;
    unsigned int m_ExtraSize;

    //! @brief RFC 5646 language code (empty string if undefined)
    char m_language[INPUTSTREAM_MAX_STRING_LANGUAGE_SIZE];

    //! Video stream related data
    //@{

    //! @brief Scale of 1000 and a rate of 29970 will result in 29.97 fps
    unsigned int m_FpsScale;

    unsigned int m_FpsRate;

    //! @brief height of the stream reported by the demuxer
    unsigned int m_Height;

    //! @brief width of the stream reported by the demuxer
    unsigned int m_Width;

    //! @brief display aspect of stream
    float m_Aspect;

    //@}

    //! Audio stream related data
    //@{

    //! @brief (required) amount of channels
    unsigned int m_Channels;

    //! @brief (required) sample rate
    unsigned int m_SampleRate;

    //! @brief (required) bit rate
    unsigned int m_BitRate;

    //! @brief (required) bits per sample
    unsigned int m_BitsPerSample;

    unsigned int m_BlockAlign;

    //@}

    struct STREAM_CRYPTO_SESSION m_cryptoSession;

    // new in API version 2.0.8
    //@{
    //! @brief Codec If available, the fourcc code codec
    unsigned int m_codecFourCC;

    //! @brief definition of colorspace
    enum INPUTSTREAM_COLORSPACE m_colorSpace;

    //! @brief color range if available
    enum INPUTSTREAM_COLORRANGE m_colorRange;
    //@}

    //new in API 2.0.9 / INPUTSTREAM_VERSION_LEVEL 1
    //@{
    enum INPUTSTREAM_COLORPRIMARIES m_colorPrimaries;
    enum INPUTSTREAM_COLORTRC m_colorTransferCharacteristic;
    //@}

    //! @brief mastering static Metadata
    struct INPUTSTREAM_MASTERING_METADATA* m_masteringMetadata;

    //! @brief content light static Metadata
    struct INPUTSTREAM_CONTENTLIGHT_METADATA* m_contentLightMetadata;
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
    struct DEMUX_PACKET* (*allocate_demux_packet)(void* kodiInstance, int data_size);
    struct DEMUX_PACKET* (*allocate_encrypted_demux_packet)(void* kodiInstance,
                                                            unsigned int data_size,
                                                            unsigned int encrypted_subsample_count);
    void (*free_demux_packet)(void* kodiInstance, struct DEMUX_PACKET* packet);
  } AddonToKodiFuncTable_InputStream;

  struct AddonInstance_InputStream;
  typedef struct KodiToAddonFuncTable_InputStream /* internal */
  {
    KODI_HANDLE addonInstance;

    bool(__cdecl* open)(const struct AddonInstance_InputStream* instance,
                        struct INPUTSTREAM_PROPERTY* props);
    void(__cdecl* close)(const struct AddonInstance_InputStream* instance);
    const char*(__cdecl* get_path_list)(const struct AddonInstance_InputStream* instance);
    void(__cdecl* get_capabilities)(const struct AddonInstance_InputStream* instance,
                                    struct INPUTSTREAM_CAPABILITIES* capabilities);

    // IDemux
    bool(__cdecl* get_stream_ids)(const struct AddonInstance_InputStream* instance,
                                  struct INPUTSTREAM_IDS* ids);
    bool(__cdecl* get_stream)(const struct AddonInstance_InputStream* instance,
                              int streamid,
                              struct INPUTSTREAM_INFO* info,
                              KODI_HANDLE* demuxStream,
                              KODI_HANDLE (*transfer_stream)(KODI_HANDLE handle,
                                                             int streamId,
                                                             struct INPUTSTREAM_INFO* stream));
    void(__cdecl* enable_stream)(const struct AddonInstance_InputStream* instance,
                                 int streamid,
                                 bool enable);
    bool(__cdecl* open_stream)(const struct AddonInstance_InputStream* instance, int streamid);
    void(__cdecl* demux_reset)(const struct AddonInstance_InputStream* instance);
    void(__cdecl* demux_abort)(const struct AddonInstance_InputStream* instance);
    void(__cdecl* demux_flush)(const struct AddonInstance_InputStream* instance);
    struct DEMUX_PACKET*(__cdecl* demux_read)(const struct AddonInstance_InputStream* instance);
    bool(__cdecl* demux_seek_time)(const struct AddonInstance_InputStream* instance,
                                   double time,
                                   bool backwards,
                                   double* startpts);
    void(__cdecl* demux_set_speed)(const struct AddonInstance_InputStream* instance, int speed);
    void(__cdecl* set_video_resolution)(const struct AddonInstance_InputStream* instance,
                                        int width,
                                        int height);

    // IDisplayTime
    int(__cdecl* get_total_time)(const struct AddonInstance_InputStream* instance);
    int(__cdecl* get_time)(const struct AddonInstance_InputStream* instance);

    // ITime
    bool(__cdecl* get_times)(const struct AddonInstance_InputStream* instance,
                             struct INPUTSTREAM_TIMES* times);

    // IPosTime
    bool(__cdecl* pos_time)(const struct AddonInstance_InputStream* instance, int ms);

    int(__cdecl* read_stream)(const struct AddonInstance_InputStream* instance,
                              uint8_t* buffer,
                              unsigned int bufferSize);
    int64_t(__cdecl* seek_stream)(const struct AddonInstance_InputStream* instance,
                                  int64_t position,
                                  int whence);
    int64_t(__cdecl* position_stream)(const struct AddonInstance_InputStream* instance);
    int64_t(__cdecl* length_stream)(const struct AddonInstance_InputStream* instance);
    bool(__cdecl* is_real_time_stream)(const struct AddonInstance_InputStream* instance);

    // IChapter
    int(__cdecl* get_chapter)(const struct AddonInstance_InputStream* instance);
    int(__cdecl* get_chapter_count)(const struct AddonInstance_InputStream* instance);
    const char*(__cdecl* get_chapter_name)(const struct AddonInstance_InputStream* instance,
                                           int ch);
    int64_t(__cdecl* get_chapter_pos)(const struct AddonInstance_InputStream* instance, int ch);
    bool(__cdecl* seek_chapter)(const struct AddonInstance_InputStream* instance, int ch);

    int(__cdecl* block_size_stream)(const struct AddonInstance_InputStream* instance);
  } KodiToAddonFuncTable_InputStream;

  typedef struct AddonInstance_InputStream /* internal */
  {
    struct AddonProps_InputStream* props;
    struct AddonToKodiFuncTable_InputStream* toKodi;
    struct KodiToAddonFuncTable_InputStream* toAddon;
  } AddonInstance_InputStream;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_INPUTSTREAM_H */
