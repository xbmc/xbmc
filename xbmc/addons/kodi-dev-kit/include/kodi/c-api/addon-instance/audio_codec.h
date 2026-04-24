/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_AUDIOCODEC_H
#define C_API_ADDONINSTANCE_AUDIOCODEC_H

#include "../addon_base.h"
#include "inputstream/demux_packet.h"
#include "inputstream/stream_codec.h"
#include "inputstream/stream_crypto.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @ingroup cpp_kodi_addon_audio_codec_Defs
  /// @brief Return values used by audio decoder interface
  enum AUDIOCODEC_RETVAL
  {
    /// @brief Noop
    AC_NONE = 0,

    /// @brief An error occurred, no other messages will be returned
    AC_ERROR,

    /// @brief The decoder needs more data
    AC_BUFFER,

    /// @brief The decoder got a frame
    AC_FRAME,

    /// @brief The decoder signals EOF
    AC_EOF,
  };
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audiocodec_Defs_AudioCodecInitdata
  /// @brief Audio codec types that can be requested from Kodi
  ///
  enum AUDIOCODEC_TYPE
  {
    /// @brief Unknown or other type requested
    AUDIOCODEC_UNKNOWN = 0,
    AUDIOCODEC_MP1,
    AUDIOCODEC_MP2,
    AUDIOCODEC_MP3, /// @brief preferred ID for decoding MPEG audio layer 1, 2 or 3
    AUDIOCODEC_AAC,
    AUDIOCODEC_AAC_LATM,
    AUDIOCODEC_AC3,
    AUDIOCODEC_EAC3,
    AUDIOCODEC_DTS,
    AUDIOCODEC_TRUEHD,
    AUDIOCODEC_VORBIS,
    AUDIOCODEC_OPUS,
    AUDIOCODEC_AC4,
  };
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audiocodec_Defs
  /// @brief Audio formats that can be requested from Kodi
  ///
  /// Minimal set, addon and Kodi can extend this later if needed.
  enum AUDIOCODEC_FORMAT
  {
    AUDIOCODEC_FMT_UNKNOWN = 0,

    AUDIOCODEC_FMT_U8,

    AUDIOCODEC_FMT_S16BE,
    AUDIOCODEC_FMT_S16LE,
    AUDIOCODEC_FMT_S16NE,

    AUDIOCODEC_FMT_S32BE,
    AUDIOCODEC_FMT_S32LE,
    AUDIOCODEC_FMT_S32NE,

    AUDIOCODEC_FMT_S24BE4,
    AUDIOCODEC_FMT_S24LE4,
    AUDIOCODEC_FMT_S24NE4, // 24 bits in lower 3 bytes
    AUDIOCODEC_FMT_S24NE4MSB, // S32 with bits_per_sample < 32

    AUDIOCODEC_FMT_S24BE3,
    AUDIOCODEC_FMT_S24LE3,
    AUDIOCODEC_FMT_S24NE3, // S24 in 3 bytes

    AUDIOCODEC_FMT_DOUBLE,
    AUDIOCODEC_FMT_FLOAT,

    // Bitstream
    AUDIOCODEC_FMT_RAW,

    // Planar formats
    AUDIOCODEC_FMT_U8P,
    AUDIOCODEC_FMT_S16NEP,
    AUDIOCODEC_FMT_S32NEP,
    AUDIOCODEC_FMT_S24NE4P,
    AUDIOCODEC_FMT_S24NE4MSBP,
    AUDIOCODEC_FMT_S24NE3P,
    AUDIOCODEC_FMT_DOUBLEP,
    AUDIOCODEC_FMT_FLOATP,

    AUDIOCODEC_FMT_MAX
  };
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audiocodec_Defs_AUDIOCODEC_FRAME
  /// @brief Audio coded process flags, used to perform special operations in
  /// stream calls.
  ///
  /// These are used to access stored data in @ref AUDIOCODEC_FRAME::flags.
  ///
  /// @note These variables are bit flags which are created using "|" can be used together.
  ///
  enum AUDIOCODEC_FRAME_FLAGS
  {
    /// @brief Empty and nothing defined
    AUDIOCODEC_FRAME_FLAG_NONE = 0,

    /// @brief Drop in decoder
    AUDIOCODEC_FRAME_FLAG_DROP = (1 << 0),

    /// @brief Squeeze out framed without feeding new packets
    AUDIOCODEC_FRAME_FLAG_DRAIN = (1 << 1),
  };
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_audiocodec_Defs_AUDIOCODEC_FRAME struct AUDIOCODEC_FRAME
  /// @ingroup cpp_kodi_addon_audiocodec_Defs
  /// @brief Data structure which is given to the addon when a decoding call is made.
  ///
  ///@{
  struct AUDIOCODEC_FRAME
  {
    /// @brief The audio format declared with @ref AUDIOCODEC_FORMAT and to be
    /// used on the addon.
    enum AUDIOCODEC_FORMAT audioFormat;

    /// @brief Audio coded process flags, used to perform special operations in
    /// stream calls.
    ///
    /// Possible flags are declared here @ref AUDIOCODEC_FRAME_FLAGS.
    uint32_t flags;

    /// @brief Data to be decoded in the addon.
    uint8_t* decodedData;

    /// @brief Size of the data given with @ref decodedData
    size_t decodedDataSize;

    /// @brief Picture presentation time stamp (PTS).
    int64_t pts;

    /// @brief This is used to save the related handle from Kodi.
    ///
    /// To handle the input stream buffer, this is given by Kodi using
    /// @ref kodi::addon::CInstanceAudioCodec::GetFrameBuffer and must be
    /// released again using @ref kodi::addon::CInstanceAudioCodec::ReleaseFrameBuffer.
    KODI_HANDLE audioBufferHandle;
  };
  ///@}
  //----------------------------------------------------------------------------

  struct AUDIOCODEC_INITDATA
  {
    enum AUDIOCODEC_TYPE codec;
    enum STREAMCODEC_PROFILE codecProfile;
    const uint8_t* extraData;
    unsigned int extraDataSize;
    struct STREAM_CRYPTO_SESSION cryptoSession;
    int32_t sampleRate;
    int32_t channels;
    int32_t bitsPerSample;
  };

  // this are properties given to the addon on create
  // at this time we have no parameters for the addon
  typedef struct AddonProps_AudioCodec
  {
    int dummy;
  } AddonProps_AudioCodec;

  struct AddonInstance_AudioCodec;
  typedef struct KodiToAddonFuncTable_AudioCodec
  {
    KODI_HANDLE addonInstance;

    //! @brief Opens a codec
    bool(__cdecl* open)(const struct AddonInstance_AudioCodec* instance,
                        struct AUDIOCODEC_INITDATA* initData);

    //! @brief Feed codec if requested from GetFrame() (return VC_BUFFER)
    bool(__cdecl* add_data)(const struct AddonInstance_AudioCodec* instance,
                            const struct DEMUX_PACKET* packet);

    //! @brief Get a decoded frame / request new data
    enum AUDIOCODEC_RETVAL(__cdecl* get_frame)(const struct AddonInstance_AudioCodec* instance,
                                               struct AUDIOCODEC_FRAME* frame);

    //! @brief Get the name of this audio decoder
    const char*(__cdecl* get_name)(const struct AddonInstance_AudioCodec* instance);

    //! @brief Reset the codec
    void(__cdecl* reset)(const struct AddonInstance_AudioCodec* instance);
  } KodiToAddonFuncTable_AudioCodec;

  typedef struct AddonToKodiFuncTable_AudioCodec
  {
    KODI_HANDLE kodiInstance;
    bool (*get_frame_buffer)(void* kodiInstance, struct AUDIOCODEC_FRAME* frame);
    void (*release_frame_buffer)(void* kodiInstance, void* buffer);
  } AddonToKodiFuncTable_AudioCodec;

  typedef struct AddonInstance_AudioCodec
  {
    struct AddonProps_AudioCodec* props;
    struct AddonToKodiFuncTable_AudioCodec* toKodi;
    struct KodiToAddonFuncTable_AudioCodec* toAddon;
  } AddonInstance_AudioCodec;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_AUDIOCODEC_H */
