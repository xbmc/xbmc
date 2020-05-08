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
#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  #define PVR_TIMER_TYPE_NONE 0
  #define PVR_TIMER_NO_CLIENT_INDEX 0
  #define PVR_TIMER_NO_PARENT PVR_TIMER_NO_CLIENT_INDEX
  #define PVR_TIMER_NO_EPG_UID EPG_TAG_INVALID_UID
  #define PVR_TIMER_ANY_CHANNEL -1

  typedef enum PVR_TIMER_TYPES
  {
    PVR_TIMER_TYPE_ATTRIBUTE_NONE = 0,
    PVR_TIMER_TYPE_IS_MANUAL = (1 << 0),
    PVR_TIMER_TYPE_IS_REPEATING = (1 << 1),
    PVR_TIMER_TYPE_IS_READONLY = (1 << 2),
    PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES = (1 << 3),
    PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE = (1 << 4),
    PVR_TIMER_TYPE_SUPPORTS_CHANNELS = (1 << 5),
    PVR_TIMER_TYPE_SUPPORTS_START_TIME = (1 << 6),
    PVR_TIMER_TYPE_SUPPORTS_TITLE_EPG_MATCH = (1 << 7),
    PVR_TIMER_TYPE_SUPPORTS_FULLTEXT_EPG_MATCH = (1 << 8),
    PVR_TIMER_TYPE_SUPPORTS_FIRST_DAY = (1 << 9),
    PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS = (1 << 10),
    PVR_TIMER_TYPE_SUPPORTS_RECORD_ONLY_NEW_EPISODES = (1 << 11),
    PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN = (1 << 12),
    PVR_TIMER_TYPE_SUPPORTS_PRIORITY = (1 << 13),
    PVR_TIMER_TYPE_SUPPORTS_LIFETIME = (1 << 14),
    PVR_TIMER_TYPE_SUPPORTS_RECORDING_FOLDERS = (1 << 15),
    PVR_TIMER_TYPE_SUPPORTS_RECORDING_GROUP = (1 << 16),
    PVR_TIMER_TYPE_SUPPORTS_END_TIME = (1 << 17),
    PVR_TIMER_TYPE_SUPPORTS_START_ANYTIME = (1 << 18),
    PVR_TIMER_TYPE_SUPPORTS_END_ANYTIME = (1 << 19),
    PVR_TIMER_TYPE_SUPPORTS_MAX_RECORDINGS = (1 << 20),
    PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE = (1 << 21),
    PVR_TIMER_TYPE_FORBIDS_EPG_TAG_ON_CREATE = (1 << 22),
    PVR_TIMER_TYPE_REQUIRES_EPG_SERIES_ON_CREATE = (1 << 23),
    PVR_TIMER_TYPE_SUPPORTS_ANY_CHANNEL = (1 << 24),
    PVR_TIMER_TYPE_REQUIRES_EPG_SERIESLINK_ON_CREATE = (1 << 25),
    PVR_TIMER_TYPE_SUPPORTS_READONLY_DELETE = (1 << 26),
    PVR_TIMER_TYPE_IS_REMINDER = (1 << 27),
    PVR_TIMER_TYPE_SUPPORTS_START_MARGIN = (1 << 28),
    PVR_TIMER_TYPE_SUPPORTS_END_MARGIN = (1 << 29),
  } PVR_TIMER_TYPES;

  typedef enum PVR_WEEKDAYS
  {
    PVR_WEEKDAY_NONE = 0,
    PVR_WEEKDAY_MONDAY = (1 << 0),
    PVR_WEEKDAY_TUESDAY = (1 << 1),
    PVR_WEEKDAY_WEDNESDAY = (1 << 2),
    PVR_WEEKDAY_THURSDAY = (1 << 3),
    PVR_WEEKDAY_FRIDAY = (1 << 4),
    PVR_WEEKDAY_SATURDAY = (1 << 5),
    PVR_WEEKDAY_SUNDAY = (1 << 6),
    PVR_WEEKDAY_ALLDAYS = PVR_WEEKDAY_MONDAY | PVR_WEEKDAY_TUESDAY | PVR_WEEKDAY_WEDNESDAY |
                          PVR_WEEKDAY_THURSDAY | PVR_WEEKDAY_FRIDAY | PVR_WEEKDAY_SATURDAY |
                          PVR_WEEKDAY_SUNDAY
  } PVR_WEEKDAY;

  typedef enum PVR_TIMER_STATE
  {
    PVR_TIMER_STATE_NEW = 0,
    PVR_TIMER_STATE_SCHEDULED = 1,
    PVR_TIMER_STATE_RECORDING = 2,
    PVR_TIMER_STATE_COMPLETED = 3,
    PVR_TIMER_STATE_ABORTED = 4,
    PVR_TIMER_STATE_CANCELLED = 5,
    PVR_TIMER_STATE_CONFLICT_OK = 6,
    PVR_TIMER_STATE_CONFLICT_NOK = 7,
    PVR_TIMER_STATE_ERROR = 8,
    PVR_TIMER_STATE_DISABLED = 9,
  } PVR_TIMER_STATE;

  typedef struct PVR_TIMER
  {
    unsigned int iClientIndex;
    unsigned int iParentClientIndex;
    int iClientChannelUid;
    time_t startTime;
    time_t endTime;
    bool bStartAnyTime;
    bool bEndAnyTime;
    enum PVR_TIMER_STATE state;
    unsigned int iTimerType;
    char strTitle[PVR_ADDON_NAME_STRING_LENGTH];
    char strEpgSearchString[PVR_ADDON_NAME_STRING_LENGTH];
    bool bFullTextEpgSearch;
    char strDirectory[PVR_ADDON_URL_STRING_LENGTH];
    char strSummary[PVR_ADDON_DESC_STRING_LENGTH];
    int iPriority;
    int iLifetime;
    int iMaxRecordings;
    unsigned int iRecordingGroup;
    time_t firstDay;
    unsigned int iWeekdays;
    unsigned int iPreventDuplicateEpisodes;
    unsigned int iEpgUid;
    unsigned int iMarginStart;
    unsigned int iMarginEnd;
    int iGenreType;
    int iGenreSubType;
    char strSeriesLink[PVR_ADDON_URL_STRING_LENGTH];
  } PVR_TIMER;

  typedef struct PVR_TIMER_TYPE
  {
    unsigned int iId;
    unsigned int iAttributes;
    char strDescription[PVR_ADDON_TIMERTYPE_STRING_LENGTH];

    unsigned int iPrioritiesSize;
    struct PVR_ATTRIBUTE_INT_VALUE priorities[PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE];
    int iPrioritiesDefault;

    unsigned int iLifetimesSize;
    struct PVR_ATTRIBUTE_INT_VALUE lifetimes[PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE];
    int iLifetimesDefault;

    unsigned int iPreventDuplicateEpisodesSize;
    struct PVR_ATTRIBUTE_INT_VALUE preventDuplicateEpisodes[PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE];
    unsigned int iPreventDuplicateEpisodesDefault;

    unsigned int iRecordingGroupSize;
    struct PVR_ATTRIBUTE_INT_VALUE recordingGroup[PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE];
    unsigned int iRecordingGroupDefault;

    unsigned int iMaxRecordingsSize;
    struct PVR_ATTRIBUTE_INT_VALUE maxRecordings[PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE_SMALL];
    int iMaxRecordingsDefault;
  } PVR_TIMER_TYPE;

#ifdef __cplusplus
}
#endif /* __cplusplus */
