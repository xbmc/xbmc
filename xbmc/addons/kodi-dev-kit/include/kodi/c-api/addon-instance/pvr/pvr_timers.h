/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_PVR_TIMERS_H
#define C_API_ADDONINSTANCE_PVR_TIMERS_H

#include "pvr_defines.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C" Definitions group 6 - PVR timers
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_Timer_PVR_TIMER_ definition PVR_TIMER (various)
  /// @ingroup cpp_kodi_addon_pvr_Defs_Timer
  /// @brief **PVR timer various different definitions**\n
  /// This mostly used on @ref cpp_kodi_addon_pvr_Defs_Timer_PVRTimer "kodi::addon::PVRTimer"
  /// to define default or not available.
  ///
  ///@{

  //============================================================================
  /// @brief Numeric PVR timer type definitions (@ref kodi::addon::PVRTimer::SetTimerType()
  /// values).
  ///
  /// "Null" value for a numeric timer type.
#define PVR_TIMER_TYPE_NONE 0
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Special @ref kodi::addon::PVRTimer::SetClientIndex() value to indicate
  /// that a timer has not (yet) a valid client index.
  ///
  /// Timer has not (yet) a valid client index.
#define PVR_TIMER_NO_CLIENT_INDEX 0
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Special @ref kodi::addon::PVRTimer::SetParentClientIndex() value to
  /// indicate that a timer has no parent.
  ///
  /// Timer has no parent; it was not scheduled by a repeating timer.
#define PVR_TIMER_NO_PARENT PVR_TIMER_NO_CLIENT_INDEX
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Special @ref kodi::addon::PVRTimer::SetEPGUid() value to indicate
  /// that a timer has no EPG event uid.
  ///
  /// Timer has no EPG event unique identifier.
#define PVR_TIMER_NO_EPG_UID EPG_TAG_INVALID_UID
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Special @ref kodi::addon::PVRTimer::SetClientChannelUid() value to
  /// indicate "any channel". Useful for some repeating timer types.
  ///
  /// denotes "any channel", not a specific one.
  ///
#define PVR_TIMER_ANY_CHANNEL -1
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Value where set in background to inform that related part not used.
  ///
  /// Normally this related parts need not to set by this as it is default.
#define PVR_TIMER_VALUE_NOT_AVAILABLE -1
  //----------------------------------------------------------------------------

  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_Timer_PVR_TIMER_TYPES enum PVR_TIMER_TYPES
  /// @ingroup cpp_kodi_addon_pvr_Defs_Timer
  /// @brief **PVR timer type attributes (@ref kodi::addon::PVRTimerType::SetAttributes() values).**\n
  /// To defines the attributes for a type. These values are bit fields that can be
  /// used together.
  ///
  ///--------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRTimerType tag;
  /// tag.SetAttributes(PVR_TIMER_TYPE_IS_MANUAL | PVR_TIMER_TYPE_IS_REPEATING);
  /// ~~~~~~~~~~~~~
  ///
  ///@{
  typedef enum PVR_TIMER_TYPES
  {
    /// @brief __0000 0000 0000 0000 0000 0000 0000 0000__ :\n Empty attribute value.
    PVR_TIMER_TYPE_ATTRIBUTE_NONE = 0,

    /// @brief __0000 0000 0000 0000 0000 0000 0000 0001__ :\n Defines whether this is a type for
    /// manual (time-based) or epg-based timers.
    PVR_TIMER_TYPE_IS_MANUAL = (1 << 0),

    /// @brief __0000 0000 0000 0000 0000 0000 0000 0010__ :\n Defines whether this is a type for
    /// repeating or one-shot timers.
    PVR_TIMER_TYPE_IS_REPEATING = (1 << 1),

    /// @brief __0000 0000 0000 0000 0000 0000 0000 0100__ :\n Timers of this type must not be edited
    /// by Kodi.
    PVR_TIMER_TYPE_IS_READONLY = (1 << 2),

    /// @brief __0000 0000 0000 0000 0000 0000 0000 1000__ :\n Timers of this type must not be created
    /// by Kodi. All other operations are allowed, though.
    PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES = (1 << 3),

    /// @brief __0000 0000 0000 0000 0000 0000 0001 0000__ :\n This type supports enabling/disabling
    /// of the timer (@ref kodi::addon::PVRTimer::SetState() with
    /// @ref PVR_TIMER_STATE_SCHEDULED | @ref PVR_TIMER_STATE_DISABLED).
    PVR_TIMER_TYPE_SUPPORTS_ENABLE_DISABLE = (1 << 4),

    /// @brief __0000 0000 0000 0000 0000 0000 0010 0000__ :\n This type supports channels
    /// (@ref kodi::addon::PVRTimer::SetClientChannelUid()).
    PVR_TIMER_TYPE_SUPPORTS_CHANNELS = (1 << 5),

    /// @brief __0000 0000 0000 0000 0000 0000 0100 0000__ :\n This type supports a recording start
    /// time (@ref kodi::addon::PVRTimer::SetStartTime()).
    PVR_TIMER_TYPE_SUPPORTS_START_TIME = (1 << 6),

    /// @brief __0000 0000 0000 0000 0000 0000 1000 0000__ :\n This type supports matching epg episode
    ///  title using@ref kodi::addon::PVRTimer::SetEPGSearchString().
    PVR_TIMER_TYPE_SUPPORTS_TITLE_EPG_MATCH = (1 << 7),

    /// @brief __0000 0000 0000 0000 0000 0001 0000 0000__ :\n This type supports matching "more" epg
    /// data (not just episode title) using @ref kodi::addon::PVRTimer::SetEPGSearchString().
    /// Setting @ref PVR_TIMER_TYPE_SUPPORTS_FULLTEXT_EPG_MATCH implies
    /// @ref PVR_TIMER_TYPE_SUPPORTS_TITLE_EPG_MATCH.
    PVR_TIMER_TYPE_SUPPORTS_FULLTEXT_EPG_MATCH = (1 << 8),

    /// @brief __0000 0000 0000 0000 0000 0010 0000 0000__ :\n This type supports a first day the
    /// timer gets active (@ref kodi::addon::PVRTimer::SetFirstDay()).
    PVR_TIMER_TYPE_SUPPORTS_FIRST_DAY = (1 << 9),

    /// @brief __0000 0000 0000 0000 0000 0100 0000 0000__ :\n This type supports weekdays for
    /// defining the recording schedule (@ref kodi::addon::PVRTimer::SetWeekdays()).
    PVR_TIMER_TYPE_SUPPORTS_WEEKDAYS = (1 << 10),

    /// @brief __0000 0000 0000 0000 0000 1000 0000 0000__ :\n This type supports the <b>"record only new episodes"</b> feature
    /// (@ref kodi::addon::PVRTimer::SetPreventDuplicateEpisodes()).
    PVR_TIMER_TYPE_SUPPORTS_RECORD_ONLY_NEW_EPISODES = (1 << 11),

    /// @brief __0000 0000 0000 0000 0001 0000 0000 0000__ :\n This type supports pre and post record time (@ref kodi::addon::PVRTimer::SetMarginStart(),
    /// @ref kodi::addon::PVRTimer::SetMarginEnd()).
    PVR_TIMER_TYPE_SUPPORTS_START_END_MARGIN = (1 << 12),

    /// @brief __0000 0000 0000 0000 0010 0000 0000 0000__ :\n This type supports recording priority (@ref kodi::addon::PVRTimer::SetPriority()).
    PVR_TIMER_TYPE_SUPPORTS_PRIORITY = (1 << 13),

    /// @brief __0000 0000 0000 0000 0100 0000 0000 0000__ :\n This type supports recording lifetime (@ref kodi::addon::PVRTimer::SetLifetime()).
    PVR_TIMER_TYPE_SUPPORTS_LIFETIME = (1 << 14),

    /// @brief __0000 0000 0000 0000 1000 0000 0000 0000__ :\n This type supports placing recordings in user defined folders
    /// (@ref kodi::addon::PVRTimer::SetDirectory()).
    PVR_TIMER_TYPE_SUPPORTS_RECORDING_FOLDERS = (1 << 15),

    /// @brief __0000 0000 0000 0001 0000 0000 0000 0000__ :\n This type supports a list of recording groups
    /// (@ref kodi::addon::PVRTimer::SetRecordingGroup()).
    PVR_TIMER_TYPE_SUPPORTS_RECORDING_GROUP = (1 << 16),

    /// @brief __0000 0000 0000 0010 0000 0000 0000 0000__ :\n This type supports a recording end time (@ref kodi::addon::PVRTimer::SetEndTime()).
    PVR_TIMER_TYPE_SUPPORTS_END_TIME = (1 << 17),

    /// @brief __0000 0000 0000 0100 0000 0000 0000 0000__ :\n Enables an 'Any Time' over-ride option for start time
    /// (using @ref kodi::addon::PVRTimer::SetStartAnyTime()).
    PVR_TIMER_TYPE_SUPPORTS_START_ANYTIME = (1 << 18),

    /// @brief __0000 0000 0000 1000 0000 0000 0000 0000__ :\n Enables a separate <b>'Any Time'</b> over-ride for end time
    /// (using @ref kodi::addon::PVRTimer::SetEndAnyTime()).
    PVR_TIMER_TYPE_SUPPORTS_END_ANYTIME = (1 << 19),

    /// @brief __0000 0000 0001 0000 0000 0000 0000 0000__ :\n This type supports specifying a maximum recordings setting'
    /// (@ref kodi::addon::PVRTimer::SetMaxRecordings()).
    PVR_TIMER_TYPE_SUPPORTS_MAX_RECORDINGS = (1 << 20),

    /// @brief __0000 0000 0010 0000 0000 0000 0000 0000__ :\n This type should not appear on any create menus which don't
    /// provide an associated @ref cpp_kodi_addon_pvr_Defs_epg_PVREPGTag "EPG tag".
    PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE = (1 << 21),

    /// @brief __0000 0000 0100 0000 0000 0000 0000 0000__ :\n This type should not appear on any create menus which provide an
    /// associated @ref cpp_kodi_addon_pvr_Defs_epg_PVREPGTag "EPG tag".
    PVR_TIMER_TYPE_FORBIDS_EPG_TAG_ON_CREATE = (1 << 22),

    /// @brief __0000 0000 1000 0000 0000 0000 0000 0000__ :\n This type should not appear on any create menus unless associated
    /// with an @ref cpp_kodi_addon_pvr_Defs_epg_PVREPGTag "EPG tag" with
    /// 'series' attributes.
    ///
    /// Following conditions allow this:
    /// - @ref kodi::addon::PVREPGTag::SetFlags() have flag @ref EPG_TAG_FLAG_IS_SERIES
    /// - @ref kodi::addon::PVREPGTag::SetSeriesNumber() > 0
    /// - @ref kodi::addon::PVREPGTag::SetEpisodeNumber() > 0
    /// - @ref kodi::addon::PVREPGTag::SetEpisodePartNumber() > 0
    ///
    /// Implies @ref PVR_TIMER_TYPE_REQUIRES_EPG_TAG_ON_CREATE.
    PVR_TIMER_TYPE_REQUIRES_EPG_SERIES_ON_CREATE = (1 << 23),

    /// @brief __0000 0001 0000 0000 0000 0000 0000 0000__ :\n This type supports 'any channel', for example when defining a timer
    /// rule that should match any channel instead of a particular channel.
    PVR_TIMER_TYPE_SUPPORTS_ANY_CHANNEL = (1 << 24),

    /// @brief __0000 0010 0000 0000 0000 0000 0000 0000__ :\n This type should not appear on any create menus which don't provide
    /// an associated @ref cpp_kodi_addon_pvr_Defs_epg_PVREPGTag "EPG tag" with
    /// a series link.
    PVR_TIMER_TYPE_REQUIRES_EPG_SERIESLINK_ON_CREATE = (1 << 25),

    /// @brief __0000 0100 0000 0000 0000 0000 0000 0000__ :\n This type allows deletion of an otherwise read-only timer.
    PVR_TIMER_TYPE_SUPPORTS_READONLY_DELETE = (1 << 26),

    /// @brief __0000 1000 0000 0000 0000 0000 0000 0000__ :\n Timers of this type do trigger a reminder if time is up.
    PVR_TIMER_TYPE_IS_REMINDER = (1 << 27),

    /// @brief __0001 0000 0000 0000 0000 0000 0000 0000__ :\n This type supports pre record time (@ref kodi::addon::PVRTimer::SetMarginStart()).
    PVR_TIMER_TYPE_SUPPORTS_START_MARGIN = (1 << 28),

    /// @brief __0010 0000 0000 0000 0000 0000 0000 0000__ :\n This type supports post record time (@ref kodi::addon::PVRTimer::SetMarginEnd()).
    PVR_TIMER_TYPE_SUPPORTS_END_MARGIN = (1 << 29),
  } PVR_TIMER_TYPES;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_Timer_PVR_WEEKDAY enum PVR_WEEKDAY
  /// @ingroup cpp_kodi_addon_pvr_Defs_Timer
  /// @brief **PVR timer weekdays** (@ref kodi::addon::PVRTimer::SetWeekdays() **values**)\n
  /// Used to select the days of a week you want.
  ///
  /// It can be also used to select several days e.g.:
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  /// unsigned int day = PVR_WEEKDAY_MONDAY | PVR_WEEKDAY_SATURDAY;
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  ///@{
  typedef enum PVR_WEEKDAYS
  {
    /// @brief __0000 0000__ : Nothing selected.
    PVR_WEEKDAY_NONE = 0,

    /// @brief __0000 0001__ : To select Monday.
    PVR_WEEKDAY_MONDAY = (1 << 0),

    /// @brief __0000 0010__ : To select Tuesday.
    PVR_WEEKDAY_TUESDAY = (1 << 1),

    /// @brief __0000 0100__ : To select Wednesday.
    PVR_WEEKDAY_WEDNESDAY = (1 << 2),

    /// @brief __0000 1000__ : To select Thursday.
    PVR_WEEKDAY_THURSDAY = (1 << 3),

    /// @brief __0001 0000__ : To select Friday.
    PVR_WEEKDAY_FRIDAY = (1 << 4),

    /// @brief __0010 0000__ : To select Saturday.
    PVR_WEEKDAY_SATURDAY = (1 << 5),

    /// @brief __0100 0000__ : To select Sunday.
    PVR_WEEKDAY_SUNDAY = (1 << 6),

    /// @brief __0111 1111__ : To select all days of week.
    PVR_WEEKDAY_ALLDAYS = PVR_WEEKDAY_MONDAY | PVR_WEEKDAY_TUESDAY | PVR_WEEKDAY_WEDNESDAY |
                          PVR_WEEKDAY_THURSDAY | PVR_WEEKDAY_FRIDAY | PVR_WEEKDAY_SATURDAY |
                          PVR_WEEKDAY_SUNDAY
  } PVR_WEEKDAY;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Defs_Timer_PVR_TIMER_STATE enum PVR_TIMER_STATE
  /// @ingroup cpp_kodi_addon_pvr_Defs_Timer
  /// @brief **PVR timer states**\n
  /// To set within @ref cpp_kodi_addon_pvr_Defs_Timer_PVRTimer "kodi::addon::PVRTimer"
  /// the needed state about.
  ///
  ///@{
  typedef enum PVR_TIMER_STATE
  {
    /// @brief __0__ : The timer was just created on the backend and is not yet active.
    ///
    /// This state must not be used for timers just created on the client side.
    PVR_TIMER_STATE_NEW = 0,

    /// @brief __1__ : The timer is scheduled for recording.
    PVR_TIMER_STATE_SCHEDULED = 1,

    /// @brief __2__ : The timer is currently recordings.
    PVR_TIMER_STATE_RECORDING = 2,

    /// @brief __3__ : The recording completed successfully.
    PVR_TIMER_STATE_COMPLETED = 3,

    /// @brief __4__ : Recording started, but was aborted.
    PVR_TIMER_STATE_ABORTED = 4,

    /// @brief __5__ : The timer was scheduled, but was canceled.
    PVR_TIMER_STATE_CANCELLED = 5,

    /// @brief __6__ : The scheduled timer conflicts with another one, but will be
    /// recorded.
    PVR_TIMER_STATE_CONFLICT_OK = 6,

    /// @brief __7__ : The scheduled timer conflicts with another one and won't be
    /// recorded.
    PVR_TIMER_STATE_CONFLICT_NOK = 7,

    /// @brief __8__ : The timer is scheduled, but can't be recorded for some reason.
    PVR_TIMER_STATE_ERROR = 8,

    /// @brief __9__ : The timer was disabled by the user, can be enabled via setting
    /// the state to @ref PVR_TIMER_STATE_SCHEDULED.
    PVR_TIMER_STATE_DISABLED = 9,
  } PVR_TIMER_STATE;
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief "C" PVR add-on timer event.
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref cpp_kodi_addon_pvr_Defs_Timer_PVRTimer "kodi::addon::PVRTimer" for
   * description of values.
   */
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

  /*!
   * @brief "C" PVR add-on timer event type.
   *
   * Structure used to interface in "C" between Kodi and Addon.
   *
   * See @ref cpp_kodi_addon_pvr_Defs_Timer_PVRTimerType "kodi::addon::PVRTimerType" for
   * description of values.
   */
  typedef struct PVR_TIMER_TYPE
  {
    unsigned int iId;
    uint64_t iAttributes;
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

#endif /* !C_API_ADDONINSTANCE_PVR_TIMERS_H */
