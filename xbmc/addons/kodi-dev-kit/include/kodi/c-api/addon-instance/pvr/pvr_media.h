/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_PVR_MEDIA_H
#define C_API_ADDONINSTANCE_PVR_MEDIA_H

#include "pvr_defines.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C" Definitions group 7 - PVR media
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_MediaTag_PVR_MEDIA_TAG_FLAG enum PVR_MEDIA_TAG_FLAG
  /// @ingroup cpp_kodi_addon_pvr_Defs_MediaTag
  /// @brief **Bit field of independent flags associated with the EPG entry.**\n
  /// Values used by @ref kodi::addon::PVRMediaTag::SetFlags().
  ///
  /// Here's example about the use of this:
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRMediaTag tag;
  /// tag.SetFlags(PVR_MEDIA_TAG_FLAG_IS_SERIES | PVR_MEDIA_TAG_FLAG_IS_PREMIERE);
  /// ~~~~~~~~~~~~~
  ///
  ///@{
  typedef enum PVR_MEDIA_TAG_FLAG
  {
    /// @brief __0000 0000__ : Nothing special to say about this mediaTag.
    PVR_MEDIA_TAG_FLAG_UNDEFINED = 0,

    /// @brief __0000 0001__ : This mediaTag is part of a series.
    PVR_MEDIA_TAG_FLAG_IS_SERIES = (1 << 0),

    /// @brief __0000 0010__ : This mediaTag will be flagged as new.
    PVR_MEDIA_TAG_FLAG_IS_NEW = (1 << 1),

    /// @brief __0000 0100__ : This mediaTag will be flagged as a premiere.
    PVR_MEDIA_TAG_FLAG_IS_PREMIERE = (1 << 2),

    /// @brief __0000 1000__ : This mediaTag will be flagged as a finale.
    PVR_MEDIA_TAG_FLAG_IS_FINALE = (1 << 3),

    /// @brief __0001 0000__ : This mediaTag will be flagged as live.
    PVR_MEDIA_TAG_FLAG_IS_LIVE = (1 << 4),
  } PVR_MEDIA_TAG_FLAG;
///@}
//----------------------------------------------------------------------------

//============================================================================
/// @ingroup cpp_kodi_addon_pvr_Defs_MediaTag_PVRMediaTag
/// @brief Special @ref kodi::addon::PVRMediaTag::SetSeriesNumber() and
/// @ref kodi::addon::PVRMediaTag::SetEpisodeNumber() value to indicate it is
/// not to be used.
///
/// Used if mediaTag has no valid season and/or episode info.
///
#define PVR_MEDIA_TAG_INVALID_SERIES_EPISODE -1
//----------------------------------------------------------------------------

//============================================================================
/// @ingroup cpp_kodi_addon_pvr_Defs_MediaTag_PVRMediaTag
/// @brief Value where set in background to inform that related part not used.
///
/// Normally this related parts need not to set by this as it is default.
#define PVR_MEDIA_TAG_VALUE_NOT_AVAILABLE -1
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_MediaTag_PVR_MEDIA_TAG_SECTION_TYPE enum PVR_MEDIA_TAG_SECTION_TYPE
  /// @ingroup cpp_kodi_addon_pvr_Defs_MediaTag
  /// @brief **PVR media tag section types**\n
  /// Used on @ref kodi::addon::PVRMediaTag::SetSectionType() value to set related
  /// type.
  ///
  ///@{
  typedef enum PVR_MEDIA_TAG_SECTION_TYPE
  {
    /// @brief __0__ : Unknown type.
    PVR_MEDIA_TAG_SECTION_TYPE_UNKNOWN = 0,

    /// @brief __1__ : TV section.
    PVR_MEDIA_TAG_SECTION_TYPE_TV = 1,

    /// @brief __2__ : Radio section.
    PVR_MEDIA_TAG_SECTION_TYPE_RADIO = 2,
  } PVR_MEDIA_TAG_SECTION_TYPE;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_MediaTag_PVR_MEDIA_TAG_TYPE enum PVR_MEDIA_TAG_TYPE
  /// @ingroup cpp_kodi_addon_pvr_Defs_MediaTag
  /// @brief **PVR mediaTag types**\n
  /// Used on @ref kodi::addon::PVRMediaTag::SetMediaType() value to set related
  /// type.
  ///
  ///@{
  typedef enum PVR_MEDIA_TAG_TYPE
  {
    /// @brief __0__ : Unknown type - Unknown class.
    PVR_MEDIA_TAG_TYPE_UNKNOWN = 0,

    /// @brief __1__ : TV show - Video class.
    PVR_MEDIA_TAG_TYPE_TV_SHOW = 1,

    /// @brief __2__ : Movie - Video class.
    PVR_MEDIA_TAG_TYPE_MOVIE = 2,

    /// @brief __3__ : Music video - Video class.
    PVR_MEDIA_TAG_TYPE_MUSIC_VIDEO = 3,

    /// @brief __4__ : Music - Audio class.
    PVR_MEDIA_TAG_TYPE_MUSIC = 4,

    /// @brief __5__ : Radio show - Audio class.
    PVR_MEDIA_TAG_TYPE_RADIO_SHOW = 5,

    /// @brief __6__ : Podcast - Audio class.
    PVR_MEDIA_TAG_TYPE_PODCAST = 6,
  } PVR_MEDIA_TAG_TYPE;
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief "C" PVR add-on mediaTag.
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref kodi::addon::PVRMediaTag for description of values.
   */
  typedef struct PVR_MEDIA_TAG
  {
    const char* strMediaTagId;
    const char* strTitle;
    const char* strEpisodeName;
    int iSeriesNumber;
    int iEpisodeNumber;
    int iEpisodePartNumber;
    int iYear;
    const char* strDirectory;
    const char* strPlotOutline;
    const char* strPlot;
    const char* strOriginalTitle;
    const char* strCast;
    const char* strDirector;
    const char* strWriter;
    const char* strGenreDescription;
    const char* strIMDBNumber;
    const char* strIconPath;
    const char* strThumbnailPath;
    const char* strFanartPath;
    time_t mediaTagTime;
    int iDuration;
    int iPriority;
    int iGenreType;
    int iGenreSubType;
    int iPlayCount;
    int iLastPlayedPosition;
    enum PVR_MEDIA_TAG_SECTION_TYPE sectionType;
    enum PVR_MEDIA_TAG_TYPE mediaType;
    const char* strFirstAired;
    unsigned int iFlags;
    int64_t sizeInBytes;
    int iClientProviderUid;
    const char* strProviderName;
    unsigned int iParentalRating;
    const char* strParentalRatingCode;
    const char* strParentalRatingIcon;
    const char* strParentalRatingSource;
  } PVR_MEDIA_TAG;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_PVR_MEDIA_H */
