/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_AUDIO_ENGINE_H
#define C_API_AUDIO_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  // "C" Definitions, structures and enumerators of audio engine
  //{{{

  //============================================================================
  /// @defgroup cpp_kodi_audioengine_Defs_AudioEngineStreamOptions enum AudioEngineStreamOptions
  /// @ingroup cpp_kodi_audioengine_Defs
  /// @brief **Bit options to pass to CAEStream**\n
  /// A bit field of stream options.
  ///
  ///
  /// ------------------------------------------------------------------------
  ///
  /// **Usage example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// // Here only as minimal, "format" must be set to wanted types
  /// kodi::audioengine::AudioEngineFormat format;
  /// m_audioengine = new kodi::audioengine::CAEStream(format, AUDIO_STREAM_FORCE_RESAMPLE | AUDIO_STREAM_AUTOSTART);
  /// ~~~~~~~~~~~~~
  ///
  ///@{
  typedef enum AudioEngineStreamOptions
  {
    /// force resample even if rates match
    AUDIO_STREAM_FORCE_RESAMPLE = 1 << 0,
    /// create the stream paused
    AUDIO_STREAM_PAUSED = 1 << 1,
    /// autostart the stream when enough data is buffered
    AUDIO_STREAM_AUTOSTART = 1 << 2,
  } AudioEngineStreamOptions;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_audioengine_Defs_AudioEngineChannel enum AudioEngineChannel
  /// @ingroup cpp_kodi_audioengine_Defs
  /// @brief **The possible channels**\n
  /// Used to set available or used channels on stream.
  ///
  ///
  /// ------------------------------------------------------------------------
  ///
  /// **Usage example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::audioengine::AudioEngineFormat format;
  /// format.SetChannelLayout(std::vector<AudioEngineChannel>(AUDIOENGINE_CH_FL, AUDIOENGINE_CH_FR));
  /// ~~~~~~~~~~~~~
  ///
  ///@{
  enum AudioEngineChannel
  {
    /// Used inside to indicate the end of a list and not for addon use directly.
    AUDIOENGINE_CH_NULL = -1,
    /// RAW Audio format
    AUDIOENGINE_CH_RAW,
    /// Front left
    AUDIOENGINE_CH_FL,
    /// Front right
    AUDIOENGINE_CH_FR,
    /// Front center
    AUDIOENGINE_CH_FC,
    /// LFE / Subwoofer
    AUDIOENGINE_CH_LFE,
    /// Back left
    AUDIOENGINE_CH_BL,
    /// Back right
    AUDIOENGINE_CH_BR,
    /// Front left over center
    AUDIOENGINE_CH_FLOC,
    /// Front right over center
    AUDIOENGINE_CH_FROC,
    /// Back center
    AUDIOENGINE_CH_BC,
    /// Side left
    AUDIOENGINE_CH_SL,
    /// Side right
    AUDIOENGINE_CH_SR,
    /// Top front left
    AUDIOENGINE_CH_TFL,
    /// Top front right
    AUDIOENGINE_CH_TFR,
    /// Top front center
    AUDIOENGINE_CH_TFC,
    /// Top center
    AUDIOENGINE_CH_TC,
    /// Top back left
    AUDIOENGINE_CH_TBL,
    /// Top back right
    AUDIOENGINE_CH_TBR,
    /// Top back center
    AUDIOENGINE_CH_TBC,
    /// Back left over center
    AUDIOENGINE_CH_BLOC,
    /// Back right over center
    AUDIOENGINE_CH_BROC,
    /// Maximum possible value, to use e.g. as size inside list
    AUDIOENGINE_CH_MAX
  };
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_audioengine_Defs_AudioEngineDataFormat enum AudioEngineDataFormat
  /// @ingroup cpp_kodi_audioengine_Defs
  /// @brief **Audio sample formats**\n
  /// The bit layout of the audio data.
  ///
  /// LE = Little Endian, BE = Big Endian, NE = Native Endian
  ///
  /// For planar sample formats, each audio channel is in a separate data plane,
  /// and linesize is the buffer size, in bytes, for a single plane. All data
  /// planes must be the same size. For packed sample formats, only the first
  /// data plane is used, and samples for each channel are interleaved. In this
  /// case, linesize is the buffer size, in bytes, for the 1 plane.
  ///
  /// @note This is ordered from the worst to best preferred formats
  ///
  ///
  /// ------------------------------------------------------------------------
  ///
  /// **Usage example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::audioengine::AudioEngineFormat format;
  /// format.SetDataFormat(AUDIOENGINE_FMT_FLOATP);
  /// ~~~~~~~~~~~~~
  ///
  ///@{
  enum AudioEngineDataFormat
  {
    /// To define format as invalid
    AUDIOENGINE_FMT_INVALID = -1,

    /// Unsigned integer 8 bit
    AUDIOENGINE_FMT_U8,

    /// Big Endian signed integer 16 bit
    AUDIOENGINE_FMT_S16BE,
    /// Little Endian signed integer 16 bit
    AUDIOENGINE_FMT_S16LE,
    /// Native Endian signed integer 16 bit
    AUDIOENGINE_FMT_S16NE,

    /// Big Endian signed integer 32 bit
    AUDIOENGINE_FMT_S32BE,
    /// Little Endian signed integer 32 bit
    AUDIOENGINE_FMT_S32LE,
    /// Native Endian signed integer 32 bit
    AUDIOENGINE_FMT_S32NE,

    /// Big Endian signed integer 24 bit (in 4 bytes)
    AUDIOENGINE_FMT_S24BE4,
    /// Little Endian signed integer 24 bit (in 4 bytes)
    AUDIOENGINE_FMT_S24LE4,
    /// Native Endian signed integer 24 bit (in 4 bytes)
    AUDIOENGINE_FMT_S24NE4,
    /// S32 with bits_per_sample < 32
    AUDIOENGINE_FMT_S24NE4MSB,

    /// Big Endian signed integer 24 bit (3 bytes)
    AUDIOENGINE_FMT_S24BE3,
    /// Little Endian signed integer 24 bit (3 bytes)
    AUDIOENGINE_FMT_S24LE3,
    /// Native Endian signed integer 24 bit (3 bytes)
    AUDIOENGINE_FMT_S24NE3,

    /// Double floating point
    AUDIOENGINE_FMT_DOUBLE,
    /// Floating point
    AUDIOENGINE_FMT_FLOAT,

    /// **Bitstream**\n
    /// RAW Audio format
    AUDIOENGINE_FMT_RAW,

    /// **Planar format**\n
    /// Unsigned byte
    AUDIOENGINE_FMT_U8P,
    /// **Planar format**\n
    /// Native Endian signed 16 bit
    AUDIOENGINE_FMT_S16NEP,
    /// **Planar format**\n
    /// Native Endian signed 32 bit
    AUDIOENGINE_FMT_S32NEP,
    /// **Planar format**\n
    /// Native Endian signed integer 24 bit (in 4 bytes)
    AUDIOENGINE_FMT_S24NE4P,
    /// **Planar format**\n
    /// S32 with bits_per_sample < 32
    AUDIOENGINE_FMT_S24NE4MSBP,
    /// **Planar format**\n
    /// Native Endian signed integer 24 bit (in 3 bytes)
    AUDIOENGINE_FMT_S24NE3P,
    /// **Planar format**\n
    /// Double floating point
    AUDIOENGINE_FMT_DOUBLEP,
    /// **Planar format**\n
    /// Floating point
    AUDIOENGINE_FMT_FLOATP,

    /// Amount of sample formats.
    AUDIOENGINE_FMT_MAX
  };
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief Internal API structure which are used for data exchange between
   * Kodi and addon.
   */
  struct AUDIO_ENGINE_FORMAT
  {
    /*! The stream's data format (eg, AUDIOENGINE_FMT_S16LE) */
    enum AudioEngineDataFormat m_dataFormat;

    /*! The stream's sample rate (eg, 48000) */
    unsigned int m_sampleRate;

    /*! The encoded streams sample rate if a bitstream, otherwise undefined */
    unsigned int m_encodedRate;

    /*! The amount of used speaker channels */
    unsigned int m_channelCount;

    /*! The stream's channel layout */
    enum AudioEngineChannel m_channels[AUDIOENGINE_CH_MAX];

    /*! The number of frames per period */
    unsigned int m_frames;

    /*! The size of one frame in bytes */
    unsigned int m_frameSize;
  };

  /* A stream handle pointer, which is only used internally by the addon stream handle */
  typedef void AEStreamHandle;

  //}}}

  //¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  // "C" Internal interface tables for intercommunications between addon and kodi
  //{{{

  /*
   * Function address structure, not need to visible on dev kit doxygen
   * documentation
   */
  typedef struct AddonToKodiFuncTable_kodi_audioengine
  {
    AEStreamHandle* (*make_stream)(void* kodiBase,
                                   struct AUDIO_ENGINE_FORMAT* format,
                                   unsigned int options);
    void (*free_stream)(void* kodiBase, AEStreamHandle* stream);
    bool (*get_current_sink_format)(void* kodiBase, struct AUDIO_ENGINE_FORMAT* sink_format);

    // Audio Engine Stream definitions
    unsigned int (*aestream_get_space)(void* kodiBase, AEStreamHandle* handle);
    unsigned int (*aestream_add_data)(void* kodiBase,
                                      AEStreamHandle* handle,
                                      uint8_t* const* data,
                                      unsigned int offset,
                                      unsigned int frames,
                                      double pts,
                                      bool hasDownmix,
                                      double centerMixLevel);
    double (*aestream_get_delay)(void* kodiBase, AEStreamHandle* handle);
    bool (*aestream_is_buffering)(void* kodiBase, AEStreamHandle* handle);
    double (*aestream_get_cache_time)(void* kodiBase, AEStreamHandle* handle);
    double (*aestream_get_cache_total)(void* kodiBase, AEStreamHandle* handle);
    void (*aestream_pause)(void* kodiBase, AEStreamHandle* handle);
    void (*aestream_resume)(void* kodiBase, AEStreamHandle* handle);
    void (*aestream_drain)(void* kodiBase, AEStreamHandle* handle, bool wait);
    bool (*aestream_is_draining)(void* kodiBase, AEStreamHandle* handle);
    bool (*aestream_is_drained)(void* kodiBase, AEStreamHandle* handle);
    void (*aestream_flush)(void* kodiBase, AEStreamHandle* handle);
    float (*aestream_get_volume)(void* kodiBase, AEStreamHandle* handle);
    void (*aestream_set_volume)(void* kodiBase, AEStreamHandle* handle, float volume);
    float (*aestream_get_amplification)(void* kodiBase, AEStreamHandle* handle);
    void (*aestream_set_amplification)(void* kodiBase, AEStreamHandle* handle, float amplify);
    unsigned int (*aestream_get_frame_size)(void* kodiBase, AEStreamHandle* handle);
    unsigned int (*aestream_get_channel_count)(void* kodiBase, AEStreamHandle* handle);
    unsigned int (*aestream_get_sample_rate)(void* kodiBase, AEStreamHandle* handle);
    enum AudioEngineDataFormat (*aestream_get_data_format)(void* kodiBase, AEStreamHandle* handle);
    double (*aestream_get_resample_ratio)(void* kodiBase, AEStreamHandle* handle);
    void (*aestream_set_resample_ratio)(void* kodiBase, AEStreamHandle* handle, double ratio);
  } AddonToKodiFuncTable_kodi_audioengine;

  //}}}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_AUDIO_ENGINE_H */
