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
    /// @brief YUV 4:2:2 planar 8-bit
    VIDEOCODEC_FORMAT_YUV422P,
    VIDEOCODEC_FORMAT_YUV422P9,
    VIDEOCODEC_FORMAT_YUV422P10,
    VIDEOCODEC_FORMAT_YUV422P12,
    /// @brief YUV 4:4:4 planar 8-bit
    VIDEOCODEC_FORMAT_YUV444P,
    VIDEOCODEC_FORMAT_YUV444P9,
    VIDEOCODEC_FORMAT_YUV444P10,
    VIDEOCODEC_FORMAT_YUV444P12,

    /// @brief NV12 4:2:0 semi-planar (Y + interleaved UV)
    VIDEOCODEC_FORMAT_NV12,
    /// @brief P010 4:2:0 10-bit semi-planar (Y + interleaved UV, 16-bit samples)
    VIDEOCODEC_FORMAT_P010,
    /// @brief YUYV 4:2:2 packed (Y0 U Y1 V)
    VIDEOCODEC_FORMAT_YUYV422,
    /// @brief UYVY 4:2:2 packed (U Y0 V Y1)
    VIDEOCODEC_FORMAT_UYVY422,

    /// @brief Packed RGB 8:8:8:8, 32 bpp, X (padding) in the high byte.
    /// In memory (little-endian): B, G, R, X. Matches DRM_FORMAT_XRGB8888.
    VIDEOCODEC_FORMAT_XRGB8888,
    /// @brief Packed RGB 10:10:10, 32 bpp, 2-bit X (padding) in the top bits.
    /// In memory: little-endian dword 0xXXRRRGGGBBB. Matches DRM_FORMAT_XRGB2101010.
    VIDEOCODEC_FORMAT_XRGB2101010,
    /// @brief Packed RGB 16:16:16, 64 bpp, X in the high 16 bits.
    /// In memory (little-endian): B, G, R, X each 2 bytes. Matches DRM_FORMAT_XRGB16161616.
    /// Used for 12-bit content stored in 16-bit containers; lower bits are zero.
    VIDEOCODEC_FORMAT_XRGB16161616,
    /// @brief Packed RGB 16:16:16, 64 bpp half-float (IEEE 754 binary16),
    /// X in the high 16 bits. Matches DRM_FORMAT_XRGB16161616F. HDR float path.
    VIDEOCODEC_FORMAT_XRGB16161616F,

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

    /// @brief HEVC/H.265
    VIDEOCODEC_HEVC,

    /// @brief Raw uncompressed video
    VIDEOCODEC_RAWVIDEO,
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
  /// @ingroup cpp_kodi_addon_videocodec_Defs_VIDEOCODEC_PICTURE
  /// @brief HDR type reported per decoded picture.
  ///
  /// Values mirror the Kodi-core enum `StreamHdrType` defined in
  /// `xbmc/cores/VideoPlayer/Interface/StreamInfo.h` one-to-one. Keep the two
  /// enums in sync if either is extended. Compile-time verification lives in
  /// `xbmc/cores/VideoPlayer/DVDCodecs/Video/AddonVideoCodec.cpp`.
  ///
  enum VIDEOCODEC_HDR_TYPE
  {
    /// @brief No HDR (SDR content)
    VIDEOCODEC_HDR_TYPE_NONE = 0,

    /// @brief HDR10 (SMPTE ST 2084 / PQ transfer, static metadata)
    VIDEOCODEC_HDR_TYPE_HDR10,

    /// @brief Dolby Vision
    VIDEOCODEC_HDR_TYPE_DOLBYVISION,

    /// @brief Hybrid Log-Gamma (ARIB STD-B67)
    VIDEOCODEC_HDR_TYPE_HLG,

    /// @brief HDR10+ (SMPTE ST 2094-40 dynamic metadata)
    VIDEOCODEC_HDR_TYPE_HDR10PLUS,

    /// @brief Sentinel, keep last. Must mirror StreamHdrType::HDR_TYPE_COUNT in StreamInfo.h
    VIDEOCODEC_HDR_TYPE_COUNT,
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

    /// @brief HDR type of this picture.
    ///
    /// Mirrors `VideoPicture::hdrType` in Kodi core. When left at
    /// @ref VIDEOCODEC_HDR_TYPE_NONE, Kodi will seed the picture's HDR type
    /// from the stream hints (matching the behavior of the ffmpeg video
    /// decoder at `DVDVideoCodecFFmpeg.cpp`). Addons that detect per-frame
    /// HDR signaling (e.g. HDR10+ dynamic metadata) should set this field
    /// to override the stream-level value.
    enum VIDEOCODEC_HDR_TYPE hdrType;

    /// @brief Alternate HDR type carried alongside @ref hdrType.
    ///
    /// Mirrors `VideoPicture::hdrTypeAlt`. Used by the ffmpeg video decoder
    /// to signal HDR10+ dynamic metadata present in a Dolby Vision stream.
    /// Zero-initialized (@ref VIDEOCODEC_HDR_TYPE_NONE) when not applicable.
    enum VIDEOCODEC_HDR_TYPE hdrTypeAlt;

    /// @brief Dolby Vision enhancement layer type.
    ///
    /// Mirrors `VideoPicture::strDVELType`. Typically "MEL" (Minimal
    /// Enhancement Layer) or "FEL" (Full Enhancement Layer). Empty string
    /// when not a Dolby Vision stream or when the enhancement layer type is
    /// unknown. The addon is responsible for the NUL-terminated string.
    char strDVELType[8];
  };
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_videocodec_Defs
  /// @brief Platform-native buffer type, used to identify the meaning of the
  /// `handle` field in @ref VIDEOCODEC_PLATFORM_BUFFER.
  ///
  /// Returned by
  /// @ref kodi::addon::CInstanceVideoCodec::GetFrameBufferPlatformHandle so
  /// addons can render directly into the underlying GPU/hardware buffer
  /// without going through CPU memory.
  ///
  enum VIDEOCODEC_PLATFORM_BUFFER_TYPE
  {
    /// @brief No native handle available, addon must use the CPU-mapped pointer
    /// in @ref VIDEOCODEC_PICTURE::decodedData.
    VIDEOCODEC_PLATFORM_BUFFER_NONE = 0,
    /// @brief Linux DMA-BUF. `handle` points at a @ref KODI_DRM_FRAME_DESCRIPTOR
    /// which carries DMA-BUF fd(s), DRM fourcc, modifier, and per-plane
    /// offsets/pitches. All fields are native DRM types (from
    /// `<drm_fourcc.h>`); no ffmpeg/libavutil dependency required.
    VIDEOCODEC_PLATFORM_BUFFER_DRM_PRIME,
    /// @brief Windows D3D11. `handle` is `ID3D11Texture2D*`. Reserved -- not
    /// implemented yet.
    VIDEOCODEC_PLATFORM_BUFFER_D3D11_TEXTURE,
    /// @brief macOS/iOS. `handle` is `IOSurfaceRef`. Reserved -- not
    /// implemented yet.
    VIDEOCODEC_PLATFORM_BUFFER_IOSURFACE,
    /// @brief Android. `handle` is `AHardwareBuffer*`. Reserved -- not
    /// implemented yet.
    VIDEOCODEC_PLATFORM_BUFFER_AHARDWAREBUFFER,
  };
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_videocodec_Defs_KODI_DRM_FRAME_DESCRIPTOR DRM PRIME frame descriptor
  /// @ingroup cpp_kodi_addon_videocodec_Defs
  /// @brief DRM-native multi-planar DMA-BUF descriptor.
  ///
  /// Pointed at by @ref VIDEOCODEC_PLATFORM_BUFFER::handle when
  /// @ref VIDEOCODEC_PLATFORM_BUFFER::type is
  /// @ref VIDEOCODEC_PLATFORM_BUFFER_DRM_PRIME. Layout mirrors the information
  /// DRM itself exposes via `drmModeAddFB2` + DMA-BUF fds; no ffmpeg types are
  /// used so addons need only `<drm_fourcc.h>` from libdrm.
  ///
  ///@{
#define KODI_DRM_MAX_PLANES 4

  /// @brief One plane of a DRM layer: which object holds its data, and where.
  struct KODI_DRM_PLANE
  {
    /// @brief Index into @ref KODI_DRM_FRAME_DESCRIPTOR::objects.
    uint32_t object_index;
    /// @brief Byte offset within that object where this plane begins.
    uint32_t offset;
    /// @brief Row stride in bytes.
    uint32_t pitch;
  };

  /// @brief One backing object (DMA-BUF) underlying one or more layers/planes.
  struct KODI_DRM_OBJECT
  {
    /// @brief DMA-BUF file descriptor. Owned by Kodi; do not close.
    int32_t fd;
    /// @brief Total size of the DMA-BUF in bytes.
    uint32_t size;
    /// @brief DRM format modifier (`DRM_FORMAT_MOD_*` from `<drm_fourcc.h>`);
    /// `DRM_FORMAT_MOD_INVALID` when unknown.
    uint64_t format_modifier;
  };

  /// @brief One image layer. Most buffers have a single layer; multi-layer
  /// (e.g. planar YUV with separately allocated Y and UV) is also supported.
  struct KODI_DRM_LAYER
  {
    /// @brief DRM fourcc (`DRM_FORMAT_*` from `<drm_fourcc.h>`).
    uint32_t format;
    /// @brief Number of planes used (up to @ref KODI_DRM_MAX_PLANES).
    uint32_t nb_planes;
    struct KODI_DRM_PLANE planes[KODI_DRM_MAX_PLANES];
  };

  /// @brief Full descriptor: which objects back which layers, and how.
  struct KODI_DRM_FRAME_DESCRIPTOR
  {
    uint32_t nb_objects;
    struct KODI_DRM_OBJECT objects[KODI_DRM_MAX_PLANES];
    uint32_t nb_layers;
    struct KODI_DRM_LAYER layers[KODI_DRM_MAX_PLANES];
  };
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_videocodec_Defs_VIDEOCODEC_PLATFORM_BUFFER struct VIDEOCODEC_PLATFORM_BUFFER
  /// @ingroup cpp_kodi_addon_videocodec_Defs
  /// @brief Native buffer descriptor returned by
  /// @ref kodi::addon::CInstanceVideoCodec::GetFrameBufferPlatformHandle.
  ///
  /// The meaning of `handle` depends on `type` -- see
  /// @ref VIDEOCODEC_PLATFORM_BUFFER_TYPE for per-type interpretation.
  ///
  ///@{
  struct VIDEOCODEC_PLATFORM_BUFFER
  {
    /// @brief Type of the native handle. Determines how to interpret `handle`.
    enum VIDEOCODEC_PLATFORM_BUFFER_TYPE type;
    /// @brief Opaque pointer to the platform-native buffer object. The addon
    /// must cast based on `type`. Lifetime is bounded by the underlying
    /// `videoBufferHandle` (released via @ref ReleaseFrameBuffer).
    void* handle;
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
    bool (*get_frame_buffer_platform_handle)(void* kodiInstance,
                                             KODI_HANDLE videoBufferHandle,
                                             struct VIDEOCODEC_PLATFORM_BUFFER* platformBuffer);
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
