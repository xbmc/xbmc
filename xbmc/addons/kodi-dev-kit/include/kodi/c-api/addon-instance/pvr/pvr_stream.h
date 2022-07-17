/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_PVR_STREAM_H
#define C_API_ADDONINSTANCE_PVR_STREAM_H

#include "../inputstream/demux_packet.h"
#include "pvr_defines.h"

#include <stdint.h>
#include <time.h>

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C" Definitions group 9 - PVR stream definitions (NOTE: Becomes replaced
// in future by inputstream addon instance way)
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_Stream
  /// @brief Maximum of allowed streams
  ///
#define PVR_STREAM_MAX_STREAMS 20
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_Stream
  /// @brief Invalid codec identifier
  ///
#define PVR_INVALID_CODEC_ID 0
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_Stream
  /// @brief Invalid codec
  ///
#define PVR_INVALID_CODEC \
  { \
    PVR_CODEC_TYPE_UNKNOWN, PVR_INVALID_CODEC_ID \
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_Stream_PVR_CODEC_TYPE enum PVR_CODEC_TYPE
  /// @ingroup cpp_kodi_addon_pvr_Defs_Stream
  /// @brief **Inputstream types**\n
  /// To identify type on stream.
  ///
  /// Used on @ref kodi::addon::PVRStreamProperties::SetCodecType and @ref kodi::addon::PVRStreamProperties::SetCodecType.
  ///
  ///@{
  typedef enum PVR_CODEC_TYPE
  {
    /// @brief To set nothing defined.
    PVR_CODEC_TYPE_UNKNOWN = -1,

    /// @brief To identify @ref cpp_kodi_addon_pvr_Defs_Stream_PVRStreamProperties as Video.
    PVR_CODEC_TYPE_VIDEO,

    /// @brief To identify @ref cpp_kodi_addon_pvr_Defs_Stream_PVRStreamProperties as Audio.
    PVR_CODEC_TYPE_AUDIO,

    /// @brief To identify @ref cpp_kodi_addon_pvr_Defs_Stream_PVRStreamProperties as Data.
    ///
    /// With codec id related source identified.
    PVR_CODEC_TYPE_DATA,

    /// @brief To identify @ref cpp_kodi_addon_pvr_Defs_Stream_PVRStreamProperties as Subtitle.
    PVR_CODEC_TYPE_SUBTITLE,

    /// @brief To identify @ref cpp_kodi_addon_pvr_Defs_Stream_PVRStreamProperties as Radio RDS.
    PVR_CODEC_TYPE_RDS,

    /// @brief To identify @ref cpp_kodi_addon_pvr_Defs_Stream_PVRStreamProperties as Audio ID3 tags.
    PVR_CODEC_TYPE_ID3,

    PVR_CODEC_TYPE_NB
  } PVR_CODEC_TYPE;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_Stream_PVR_CODEC struct PVR_CODEC
  /// @ingroup cpp_kodi_addon_pvr_Defs_Stream
  /// @brief **Codec identification structure**\n
  /// Identifier about stream between Kodi and addon.
  ///
  ///@{
  typedef struct PVR_CODEC
  {
    /// @brief Used codec type for stream.
    enum PVR_CODEC_TYPE codec_type;

    /// @brief Related codec identifier, normally match the ffmpeg id's.
    unsigned int codec_id;
  } PVR_CODEC;
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief "C" Stream properties
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref cpp_kodi_addon_pvr_Defs_Stream_PVRStreamProperties for description of values.
   */
  typedef struct PVR_STREAM_PROPERTIES
  {
    unsigned int iStreamCount;
    struct PVR_STREAM
    {
      unsigned int iPID;
      enum PVR_CODEC_TYPE iCodecType;
      unsigned int iCodecId;
      char strLanguage[4];
      int iSubtitleInfo;
      int iFPSScale;
      int iFPSRate;
      int iHeight;
      int iWidth;
      float fAspect;
      int iChannels;
      int iSampleRate;
      int iBlockAlign;
      int iBitRate;
      int iBitsPerSample;
    } stream[PVR_STREAM_MAX_STREAMS];
  } PVR_STREAM_PROPERTIES;

  /*!
   * @brief "C" Times of playing stream (Live TV and recordings)
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref cpp_kodi_addon_pvr_Defs_Stream_PVRStreamTimes for description of values.
   */
  typedef struct PVR_STREAM_TIMES
  {
    time_t startTime;
    int64_t ptsStart;
    int64_t ptsBegin;
    int64_t ptsEnd;
  } PVR_STREAM_TIMES;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_PVR_STREAM_H */
