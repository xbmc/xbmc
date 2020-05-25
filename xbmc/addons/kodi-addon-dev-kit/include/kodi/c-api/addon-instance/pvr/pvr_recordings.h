/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "pvr_defines.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  typedef enum PVR_RECORDING_FLAG
  {
    PVR_RECORDING_FLAG_UNDEFINED = 0,
    PVR_RECORDING_FLAG_IS_SERIES = (1 << 0),
    PVR_RECORDING_FLAG_IS_NEW = (1 << 1),
    PVR_RECORDING_FLAG_IS_PREMIERE = (1 << 2),
    PVR_RECORDING_FLAG_IS_FINALE = (1 << 3),
    PVR_RECORDING_FLAG_IS_LIVE = (1 << 4),
  } PVR_RECORDING_FLAG;

  #define PVR_RECORDING_INVALID_SERIES_EPISODE EPG_TAG_INVALID_SERIES_EPISODE
  #define PVR_RECORDING_VALUE_NOT_AVAILABLE -1

  typedef enum PVR_RECORDING_CHANNEL_TYPE
  {
    PVR_RECORDING_CHANNEL_TYPE_UNKNOWN = 0,
    PVR_RECORDING_CHANNEL_TYPE_TV = 1,
    PVR_RECORDING_CHANNEL_TYPE_RADIO = 2,
  } PVR_RECORDING_CHANNEL_TYPE;

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
  } PVR_RECORDING;

#ifdef __cplusplus
}
#endif /* __cplusplus */
