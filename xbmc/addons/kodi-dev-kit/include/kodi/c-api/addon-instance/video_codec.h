/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_VIDEOCODEC_H
#define C_API_ADDONINSTANCE_VIDEOCODEC_H

#include "../addon_base.h"
#include "inputstream/demux_packet.h"
#include "inputstream/stream_codec.h"
#include "inputstream/stream_crypto.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @ingroup cpp_kodi_addon_videocodec_Defs
  /// @brief Return values used by video decoder interface
  enum VIDEOCODEC_RETVAL
  {
    /// @brief Noop
    VC_NONE = 0,

    /// @brief An error occurred, no other messages will be returned
    VC_ERROR,

    /// @brief The decoder needs more data
    VC_BUFFER,

    /// @brief The decoder got a picture
    VC_PICTURE,

    /// @brief The decoder signals EOF
    VC_EOF,
  };
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_videocodec_Defs_VideoCodecInitdata
  /// @brief The video stream representations requested by Kodi
  ///
  enum VIDEOCODEC_FORMAT
  {
    /// @brief Unknown types, this is used to declare the end of a list of
    /// requested types
    VIDEOCODEC_FORMAT_UNKNOWN = 0,

    /// @brief YV12 4:2:0 YCrCb planar format
    VIDEOCODEC_FORMAT_YV12,

    /// @brief These formats are identical to YV12 except that the U and V plane
    /// order is reversed.
    VIDEOCODEC_FORMAT_I420,

    VIDEOCODEC_FORMAT_YUV420P9,
    VIDEOCODEC_FORMAT_YUV420P10,
    VIDEOCODEC_FORMAT_YUV420P12,
    VIDEOCODEC_FORMAT_YUV422P9,
    VIDEOCODEC_FORMAT_YUV422P10,
    VIDEOCODEC_FORMAT_YUV422P12,
    VIDEOCODEC_FORMAT_YUV444P9,
    VIDEOCODEC_FORMAT_YUV444P10,
    VIDEOCODEC_FORMAT_YUV444P12,

    /// @brief The maximum value to use in a list.
    VIDEOCODEC_FORMAT_MAXFORMATS
  };
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_videocodec_Defs_VideoCodecInitdata
  /// @brief Video codec types that can be requested from Kodi
  ///
  enum VIDEOCODEC_TYPE
  {
    /// @brief Unknown or other type requested
    ///
    VIDEOCODEC_UNKNOWN = 0,

    /// @brief [VP8](https://en.wikipedia.org/wiki/VP8) video coding format
    ///
    VIDEOCODEC_VP8,

    /// @brief [Advanced Video Coding (AVC)](https://en.wikipedia.org/wiki/Advanced_Video_Coding),
    /// also referred to as H.264 or [MPEG-4](https://en.wikipedia.org/wiki/MPEG-4)
    /// Part 10, Advanced Video Coding (MPEG-4 AVC).
    VIDEOCODEC_H264,

    /// @brief [VP9](https://en.wikipedia.org/wiki/VP9) video coding format\n
    /// \n
    /// VP9 is the successor to VP8 and competes mainly with MPEG's
    /// [High Efficiency Video Coding](https://en.wikipedia.org/wiki/High_Efficiency_Video_Coding)
    /// (HEVC/H.265).
    VIDEOCODEC_VP9,

    /// @brief [AV1](https://en.wikipedia.org/wiki/AV1) video coding format\n
    /// \n
    /// AV1 is the successor to VP9 and competes mainly with MPEG's
    /// [High Efficiency Video Coding](https://en.wikipedia.org/wiki/High_Efficiency_Video_Coding)
    /// (HEVC/H.265).
    VIDEOCODEC_AV1,
  };
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_videocodec_Defs_VIDEOCODEC_PICTURE
  /// @brief YUV Plane identification pointers
  ///
  /// YUV is a color encoding system typically used as part of a color image pipeline.
  ///
  /// These are used to access stored data in @ref VIDEOCODEC_PICTURE::planeOffsets
  /// and @ref VIDEOCODEC_PICTURE::stride.
  ///
  enum VIDEOCODEC_PLANE
  {
    /// @brief "luminance" component Y (equivalent to grey scale)
    VIDEOCODEC_PICTURE_Y_PLANE = 0,

    /// @brief "chrominance" component U (blue projection)
    VIDEOCODEC_PICTURE_U_PLANE,

    /// @brief "chrominance" component V (red projection)
    VIDEOCODEC_PICTURE_V_PLANE,

    /// @brief The maximum value to use in a list.
    VIDEOCODEC_PICTURE_MAXPLANES = 3,
  };
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_videocodec_Defs_VIDEOCODEC_PICTURE
  /// @brief Video coded process flags, used to perform special operations in
  /// stream calls.
  ///
  /// These are used to access stored data in @ref VIDEOCODEC_PICTURE::flags.
  ///
  /// @note These variables are bit flags which are created using "|" can be used together.
  ///
  enum VIDEOCODEC_PICTURE_FLAG
  {
    /// @brief Empty and nothing defined
    VIDEOCODEC_PICTURE_FLAG_NONE = 0,

    /// @brief Drop in decoder
    VIDEOCODEC_PICTURE_FLAG_DROP = (1 << 0),

    /// @brief Squeeze out pictured without feeding new packets
    VIDEOCODEC_PICTURE_FLAG_DRAIN = (1 << 1),
  };
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_videocodec_Defs_VIDEOCODEC_PICTURE struct VIDEOCODEC_PICTURE
  /// @ingroup cpp_kodi_addon_videocodec_Defs
  /// @brief Data structure which is given to the addon when a decoding call is made.
  ///
  ///@{
  struct VIDEOCODEC_PICTURE
  {
    /// @brief The video format declared with @ref VIDEOCODEC_FORMAT and to be
    /// used on the addon.
    enum VIDEOCODEC_FORMAT videoFormat;

    /// @brief Video coded process flags, used to perform special operations in
    /// stream calls.
    ///
    /// Possible flags are declared here @ref VIDEOCODEC_PICTURE_FLAGS.
    uint32_t flags;

    /// @brief Picture width.
    uint32_t width;

    /// @brief Picture height.
    uint32_t height;

    /// @brief Data to be decoded in the addon.
    uint8_t* decodedData;

    /// @brief Size of the data given with @ref decodedData
    size_t decodedDataSize;

    /// @brief YUV color plane calculation array.
    ///
    /// This includes the three values of the YUV and can be identified using
    /// @ref VIDEOCODEC_PLANE.
    uint32_t planeOffsets[VIDEOCODEC_PICTURE_MAXPLANES];

    /// @brief YUV color stride calculation array
    ///
    /// This includes the three values of the YUV and can be identified using
    /// @ref VIDEOCODEC_PLANE.
    uint32_t stride[VIDEOCODEC_PICTURE_MAXPLANES];

    /// @brief Picture presentation time stamp (PTS).
    int64_t pts;

    /// @brief This is used to save the related handle from Kodi.
    ///
    /// To handle the input stream buffer, this is given by Kodi using
    /// @ref kodi::addon::CInstanceVideoCodec::GetFrameBuffer and must be
    /// released again using @ref kodi::addon::CInstanceVideoCodec::ReleaseFrameBuffer.
    KODI_HANDLE videoBufferHandle;
  };
  ///@}
  //----------------------------------------------------------------------------

  struct VIDEOCODEC_INITDATA
  {
    enum VIDEOCODEC_TYPE codec;
    enum STREAMCODEC_PROFILE codecProfile;
    enum VIDEOCODEC_FORMAT* videoFormats;
    uint32_t width;
    uint32_t height;
    const uint8_t* extraData;
    unsigned int extraDataSize;
    struct STREAM_CRYPTO_SESSION cryptoSession;
  };

  // this are properties given to the addon on create
  // at this time we have no parameters for the addon
  typedef struct AddonProps_VideoCodec
  {
    int dummy;
  } AddonProps_VideoCodec;

  struct AddonInstance_VideoCodec;
  typedef struct KodiToAddonFuncTable_VideoCodec
  {
    KODI_HANDLE addonInstance;

    //! @brief Opens a codec
    bool(__cdecl* open)(const struct AddonInstance_VideoCodec* instance,
                        struct VIDEOCODEC_INITDATA* initData);

    //! @brief Reconfigures a codec
    bool(__cdecl* reconfigure)(const struct AddonInstance_VideoCodec* instance,
                               struct VIDEOCODEC_INITDATA* initData);

    //! @brief Feed codec if requested from GetPicture() (return VC_BUFFER)
    bool(__cdecl* add_data)(const struct AddonInstance_VideoCodec* instance,
                            const struct DEMUX_PACKET* packet);

    //! @brief Get a decoded picture / request new data
    enum VIDEOCODEC_RETVAL(__cdecl* get_picture)(const struct AddonInstance_VideoCodec* instance,
                                                 struct VIDEOCODEC_PICTURE* picture);

    //! @brief Get the name of this video decoder
    const char*(__cdecl* get_name)(const struct AddonInstance_VideoCodec* instance);

    //! @brief Reset the codec
    void(__cdecl* reset)(const struct AddonInstance_VideoCodec* instance);
  } KodiToAddonFuncTable_VideoCodec;

  typedef struct AddonToKodiFuncTable_VideoCodec
  {
    KODI_HANDLE kodiInstance;
    bool (*get_frame_buffer)(void* kodiInstance, struct VIDEOCODEC_PICTURE* picture);
    void (*release_frame_buffer)(void* kodiInstance, void* buffer);
  } AddonToKodiFuncTable_VideoCodec;

  typedef struct AddonInstance_VideoCodec
  {
    struct AddonProps_VideoCodec* props;
    struct AddonToKodiFuncTable_VideoCodec* toKodi;
    struct KodiToAddonFuncTable_VideoCodec* toAddon;
  } AddonInstance_VideoCodec;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_VIDEOCODEC_H */
