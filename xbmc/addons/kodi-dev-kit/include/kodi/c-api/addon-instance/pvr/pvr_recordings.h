/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_PVR_RECORDINGS_H
#define C_API_ADDONINSTANCE_PVR_RECORDINGS_H

#include "pvr_defines.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C" Definitions group 5 - PVR recordings
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_Recording_PVR_RECORDING_FLAG enum PVR_RECORDING_FLAG
  /// @ingroup cpp_kodi_addon_pvr_Defs_Recording
  /// @brief **Bit field of independent flags associated with the EPG entry.**\n
  /// Values used by @ref kodi::addon::PVRRecording::SetFlags().
  ///
  /// Here's example about the use of this:
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRRecording tag;
  /// tag.SetFlags(PVR_RECORDING_FLAG_IS_SERIES | PVR_RECORDING_FLAG_IS_PREMIERE);
  /// ~~~~~~~~~~~~~
  ///
  ///@{
  typedef enum PVR_RECORDING_FLAG
  {
    /// @brief __0000 0000__ : Nothing special to say about this recording.
    PVR_RECORDING_FLAG_UNDEFINED = 0,

    /// @brief __0000 0001__ : This recording is part of a series.
    PVR_RECORDING_FLAG_IS_SERIES = (1 << 0),

    /// @brief __0000 0010__ : This recording will be flagged as new.
    PVR_RECORDING_FLAG_IS_NEW = (1 << 1),

    /// @brief __0000 0100__ : This recording will be flagged as a premiere.
    PVR_RECORDING_FLAG_IS_PREMIERE = (1 << 2),

    /// @brief __0000 1000__ : This recording will be flagged as a finale.
    PVR_RECORDING_FLAG_IS_FINALE = (1 << 3),

    /// @brief __0001 0000__ : This recording will be flagged as live.
    PVR_RECORDING_FLAG_IS_LIVE = (1 << 4),
  } PVR_RECORDING_FLAG;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_Recording_PVRRecording
  /// @brief Special @ref kodi::addon::PVRRecording::SetSeriesNumber() and
  /// @ref kodi::addon::PVRRecording::SetEpisodeNumber() value to indicate it is
  /// not to be used.
  ///
  /// Used if recording has no valid season and/or episode info.
  ///
#define PVR_RECORDING_INVALID_SERIES_EPISODE EPG_TAG_INVALID_SERIES_EPISODE
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_Recording_PVRRecording
  /// @brief Value where set in background to inform that related part not used.
  ///
  /// Normally this related parts need not to set by this as it is default.
#define PVR_RECORDING_VALUE_NOT_AVAILABLE -1
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_Recording_PVR_RECORDING_CHANNEL_TYPE enum PVR_RECORDING_CHANNEL_TYPE
  /// @ingroup cpp_kodi_addon_pvr_Defs_Recording
  /// @brief **PVR recording channel types**\n
  /// Used on @ref kodi::addon::PVRRecording::SetChannelType() value to set related
  /// type.
  ///
  ///@{
  typedef enum PVR_RECORDING_CHANNEL_TYPE
  {
    /// @brief __0__ : Unknown type.
    PVR_RECORDING_CHANNEL_TYPE_UNKNOWN = 0,

    /// @brief __1__ : TV channel.
    PVR_RECORDING_CHANNEL_TYPE_TV = 1,

    /// @brief __2__ : Radio channel.
    PVR_RECORDING_CHANNEL_TYPE_RADIO = 2,
  } PVR_RECORDING_CHANNEL_TYPE;
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief "C" PVR add-on recording.
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref kodi::addon::PVRRecording for description of values.
   */
  typedef struct PVR_RECORDING
  {
    char strRecordingId[PVR_ADDON_NAME_STRING_LENGTH];
    char strTitle[PVR_ADDON_NAME_STRING_LENGTH];
    char strEpisodeName[PVR_ADDON_NAME_STRING_LENGTH];
    int iSeriesNumber;
    int iEpisodeNumber;
    int iYear;
    char strDirectory[PVR_ADDON_URL_STRING_LENGTH];
    char strPlotOutline[PVR_ADDON_DESC_STRING_LENGTH];
    char strPlot[PVR_ADDON_DESC_STRING_LENGTH];
    char strGenreDescription[PVR_ADDON_DESC_STRING_LENGTH];
    char strChannelName[PVR_ADDON_NAME_STRING_LENGTH];
    char strIconPath[PVR_ADDON_URL_STRING_LENGTH];
    char strThumbnailPath[PVR_ADDON_URL_STRING_LENGTH];
    char strFanartPath[PVR_ADDON_URL_STRING_LENGTH];
    time_t recordingTime;
    int iDuration;
    int iPriority;
    int iLifetime;
    int iGenreType;
    int iGenreSubType;
    int iPlayCount;
    int iLastPlayedPosition;
    bool bIsDeleted;
    unsigned int iEpgEventId;
    int iChannelUid;
    enum PVR_RECORDING_CHANNEL_TYPE channelType;
    char strFirstAired[PVR_ADDON_DATE_STRING_LENGTH];
    unsigned int iFlags;
    int64_t sizeInBytes;
    int iClientProviderUid;
    char strProviderName[PVR_ADDON_NAME_STRING_LENGTH];
  } PVR_RECORDING;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_PVR_RECORDINGS_H */
