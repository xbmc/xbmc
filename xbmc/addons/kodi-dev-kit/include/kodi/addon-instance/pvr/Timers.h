/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr.h"
#include "General.h"

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" Definitions group 6 - PVR timers
#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_Timer_PVRTimer class PVRTimer
/// @ingroup cpp_kodi_addon_pvr_Defs_Timer
/// @brief **PVR add-on timer type**\n
/// Representation of a timer event.
///
/// The related values here are automatically initiated to defaults and need
/// only be set if supported and used.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_Timer_PVRTimer_Help
///
///@{
class PVRTimer : public CStructHdl<PVRTimer, PVR_TIMER>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRTimer()
  {
    m_cStructure->iClientIndex = 0;
    m_cStructure->state = PVR_TIMER_STATE_NEW;
    m_cStructure->iTimerType = PVR_TIMER_TYPE_NONE;
    m_cStructure->iParentClientIndex = 0;
    m_cStructure->iClientChannelUid = PVR_TIMER_VALUE_NOT_AVAILABLE;
    m_cStructure->startTime = 0;
    m_cStructure->endTime = 0;
    m_cStructure->bStartAnyTime = false;
    m_cStructure->bEndAnyTime = false;
    m_cStructure->bFullTextEpgSearch = false;
    m_cStructure->iPriority = PVR_TIMER_VALUE_NOT_AVAILABLE;
    m_cStructure->iLifetime = PVR_TIMER_VALUE_NOT_AVAILABLE;
    m_cStructure->iMaxRecordings = PVR_TIMER_VALUE_NOT_AVAILABLE;
    m_cStructure->iRecordingGroup = 0;
    m_cStructure->firstDay = 0;
    m_cStructure->iWeekdays = PVR_WEEKDAY_NONE;
    m_cStructure->iPreventDuplicateEpisodes = 0;
    m_cStructure->iEpgUid = 0;
    m_cStructure->iMarginStart = 0;
    m_cStructure->iMarginEnd = 0;
    m_cStructure->iGenreType = PVR_TIMER_VALUE_NOT_AVAILABLE;
    m_cStructure->iGenreSubType = PVR_TIMER_VALUE_NOT_AVAILABLE;
  }
  PVRTimer(const PVRTimer& data) : CStructHdl(data) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_Timer_PVRTimer_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_Timer_PVRTimer
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_Timer_PVRTimer :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |------|------|----------|----------|-----------
  /// | **Client index** | `unsigned int` | @ref PVRTimer::SetClientIndex "SetClientIndex" | @ref PVRTimer::GetClientIndex "GetClientIndex" | *required to set*
  /// | **State** | @ref PVR_TIMER_STATE | @ref PVRTimer::SetState "SetState" | @ref PVRTimer::GetState "GetState" | *required to set*
  /// | **Type** | `unsigned int` | @ref PVRTimer::SetTimerType "SetTimerType" | @ref PVRTimer::GetTimerType "GetTimerType" | *required to set*
  /// | **Title** | `std::string` | @ref PVRTimer::SetTitle "SetTitle" | @ref PVRTimer::GetTitle "GetTitle" | *required to set*
  /// | **Parent client index** | `unsigned int` | @ref PVRTimer::SetParentClientIndex "SetParentClientIndex" | @ref PVRTimer::GetParentClientIndex "GetParentClientIndex" | *optional*
  /// | **Client channel unique identifier** | `int` | @ref PVRTimer::SetClientChannelUid "SetClientChannelUid" | @ref PVRTimer::GetClientChannelUid "GetClientChannelUid" | *optional*
  /// | **Start time** | `time_t` | @ref PVRTimer::SetStartTime "SetStartTime" | @ref PVRTimer::GetStartTime "GetStartTime" | *optional*
  /// | **End time** | `time_t` | @ref PVRTimer::SetEndTime "SetEndTime" | @ref PVRTimer::GetEndTime "GetEndTime" | *optional*
  /// | **Start any time** | `bool` | @ref PVRTimer::SetStartAnyTime "SetStartAnyTime" | @ref PVRTimer::GetStartAnyTime "GetStartAnyTime" | *optional*
  /// | **End any time** | `bool` | @ref PVRTimer::SetEndAnyTime "SetEndAnyTime" | @ref PVRTimer::GetEndAnyTime "GetEndAnyTime" | *optional*
  /// | **EPG search string** | `std::string` | @ref PVRTimer::SetEPGSearchString "SetEPGSearchString" | @ref PVRTimer::GetEPGSearchString "GetEPGSearchString" | *optional*
  /// | **Full text EPG search** | `bool` | @ref PVRTimer::SetFullTextEpgSearch "SetFullTextEpgSearch" | @ref PVRTimer::GetFullTextEpgSearch "GetFullTextEpgSearch" | *optional*
  /// | **Recording store directory** | `std::string` | @ref PVRTimer::SetDirectory "SetDirectory" | @ref PVRTimer::GetDirectory "GetDirectory" | *optional*
  /// | **Timer priority** | `int` | @ref PVRTimer::SetPriority "SetPriority" | @ref PVRTimer::GetPriority "GetPriority" | *optional*
  /// | **Timer lifetime** | `int` | @ref PVRTimer::SetLifetime "SetLifetime" | @ref PVRTimer::GetLifetime "GetLifetime" | *optional*
  /// | **Max recordings** | `int` | @ref PVRTimer::SetMaxRecordings "SetMaxRecordings" | @ref PVRTimer::GetMaxRecordings "GetMaxRecordings" | *optional*
  /// | **Recording group** | `unsigned int` | @ref PVRTimer::SetRecordingGroup "SetRecordingGroup" | @ref PVRTimer::GetRecordingGroup "GetRecordingGroup" | *optional*
  /// | **First start day** | `time_t` | @ref PVRTimer::SetFirstDay "SetFirstDay" | @ref PVRTimer::GetFirstDay "GetFirstDay" | *optional*
  /// | **Used timer weekdays** | `unsigned int` | @ref PVRTimer::SetWeekdays "SetWeekdays" | @ref PVRTimer::GetWeekdays "GetWeekdays" | *optional*
  /// | **Prevent duplicate episodes** | `unsigned int` | @ref PVRTimer::SetPreventDuplicateEpisodes "SetPreventDuplicateEpisodes" | @ref PVRTimer::GetPreventDuplicateEpisodes "GetPreventDuplicateEpisodes" | *optional*
  /// | **EPG unique identifier** | `unsigned int` | @ref PVRTimer::SetEPGUid "SetEPGUid" | @ref PVRTimer::GetEPGUid "GetEPGUid" | *optional*
  /// | **Margin start** | `unsigned int` | @ref PVRTimer::SetMarginStart "SetMarginStart" | @ref PVRTimer::GetMarginStart "GetMarginStart" | *optional*
  /// | **Margin end** | `unsigned int` | @ref PVRTimer::SetMarginEnd "SetMarginEnd" | @ref PVRTimer::GetMarginEnd "GetMarginEnd" | *optional*
  /// | **Genre type** | `int` | @ref PVRTimer::SetGenreType "SetGenreType" | @ref PVRTimer::GetGenreType "GetGenreType" | *optional*
  /// | **Genre sub type** | `int` | @ref PVRTimer::SetGenreSubType "SetGenreSubType" | @ref PVRTimer::GetGenreSubType "GetGenreSubType" | *optional*
  /// | **Series link** | `std::string` | @ref PVRTimer::SetSeriesLink "SetSeriesLink" | @ref PVRTimer::GetSeriesLink "GetSeriesLink" | *optional*

  /// @addtogroup cpp_kodi_addon_pvr_Defs_Timer_PVRTimer
  ///@{

  /// @brief **required**\n
  /// The index of this timer given by the client.
  ///
  /// @ref PVR_TIMER_NO_CLIENT_INDEX indicates that the index was not yet set
  /// by the client, for example for new timers created by Kodi and passed the
  /// first time to the client. A valid index must be greater than
  /// @ref PVR_TIMER_NO_CLIENT_INDEX.
  ///
  void SetClientIndex(unsigned int clientIndex) { m_cStructure->iClientIndex = clientIndex; }

  /// @brief To get with @ref SetClientIndex changed values.
  unsigned int GetClientIndex() const { return m_cStructure->iClientIndex; }

  /// @brief **required**\n
  /// The state of this timer.
  ///
  /// @note @ref PVR_TIMER_STATE_NEW is default.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRTimer tag;
  /// tag.SetState(PVR_TIMER_STATE_RECORDING);
  /// ~~~~~~~~~~~~~
  ///
  void SetState(PVR_TIMER_STATE state) { m_cStructure->state = state; }

  /// @brief To get with @ref SetState changed values.
  PVR_TIMER_STATE GetState() const { return m_cStructure->state; }

  /// @brief **required**\n
  /// The type of this timer.
  ///
  /// It is private to the addon and can be freely defined by the addon.
  /// The value must be greater than @ref PVR_TIMER_TYPE_NONE.
  ///
  /// Kodi does not interpret this value (except for checking for @ref PVR_TIMER_TYPE_NONE),
  /// but will pass the right id to the addon with every @ref PVRTimer instance,
  /// thus the addon easily can determine the timer type.
  ///
  /// @note @ref PVR_TIMER_TYPE_NONE is default.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// kodi::addon::PVRTimer tag;
  /// tag.SetTimerType(123);
  /// ~~~~~~~~~~~~~
  ///
  void SetTimerType(unsigned int timerType) { m_cStructure->iTimerType = timerType; }

  /// @brief To get with @ref SetTimerType changed values.
  unsigned int GetTimerType() const { return m_cStructure->iTimerType; }

  /// @brief **required**\n
  /// A title for this timer.
  void SetTitle(const std::string& title)
  {
    strncpy(m_cStructure->strTitle, title.c_str(), sizeof(m_cStructure->strTitle) - 1);
  }

  /// @brief To get with @ref SetTitle changed values.
  std::string GetTitle() const { return m_cStructure->strTitle; }

  /// @brief **optional**\n
  /// For timers scheduled by a repeating timer.
  ///
  /// The index of the repeating timer that scheduled this timer (it's
  /// @ref clientIndex value). Use @ref PVR_TIMER_NO_PARENT to indicate that
  /// this timer was no scheduled by a repeating timer.
  void SetParentClientIndex(unsigned int parentClientIndex)
  {
    m_cStructure->iParentClientIndex = parentClientIndex;
  }

  /// @brief To get with @ref SetParentClientIndex changed values.
  unsigned int GetParentClientIndex() const { return m_cStructure->iParentClientIndex; }

  /// @brief **optional**\n
  /// Unique identifier of the channel to record on.
  ///
  /// @ref PVR_TIMER_ANY_CHANNEL will denote "any channel", not a specific one.
  /// @ref PVR_CHANNEL_INVALID_UID denotes that channel uid is not available.
  void SetClientChannelUid(int clientChannelUid)
  {
    m_cStructure->iClientChannelUid = clientChannelUid;
  }

  /// @brief To get with @ref SetClientChannelUid changed values
  int GetClientChannelUid() const { return m_cStructure->iClientChannelUid; }

  /// @brief **optional**\n
  /// Start time of the recording in UTC.
  ///
  /// Instant timers that are sent to the add-on by Kodi will have this value
  /// set to 0.
  void SetStartTime(time_t startTime) { m_cStructure->startTime = startTime; }

  /// @brief To get with @ref SetStartTime changed values.
  time_t GetStartTime() const { return m_cStructure->startTime; }

  /// @brief **optional**\n
  /// End time of the recording in UTC.
  void SetEndTime(time_t endTime) { m_cStructure->endTime = endTime; }

  /// @brief To get with @ref SetEndTime changed values.
  time_t GetEndTime() const { return m_cStructure->endTime; }

  /// @brief **optional**\n
  /// For EPG based (not Manual) timers indicates startTime does not apply.
  ///
  /// Default = false.
  void SetStartAnyTime(bool startAnyTime) { m_cStructure->bStartAnyTime = startAnyTime; }

  /// @brief To get with @ref SetStartAnyTime changed values.
  bool GetStartAnyTime() const { return m_cStructure->bStartAnyTime; }

  /// @brief **optional**\n
  /// For EPG based (not Manual) timers indicates endTime does not apply.
  ///
  /// Default = false
  void SetEndAnyTime(bool endAnyTime) { m_cStructure->bEndAnyTime = endAnyTime; }

  /// @brief To get with @ref SetEndAnyTime changed values.
  bool GetEndAnyTime() const { return m_cStructure->bEndAnyTime; }

  /// @brief **optional**\n
  /// A string used to search epg data for repeating epg-based timers.
  ///
  /// Format is backend-dependent, for example regexp.
  void SetEPGSearchString(const std::string& epgSearchString)
  {
    strncpy(m_cStructure->strEpgSearchString, epgSearchString.c_str(),
            sizeof(m_cStructure->strEpgSearchString) - 1);
  }

  /// @brief To get with @ref SetEPGSearchString changed values
  std::string GetEPGSearchString() const { return m_cStructure->strEpgSearchString; }

  /// @brief **optional**\n
  /// Indicates, whether @ref SetEPGSearchString() is to match against the epg
  /// episode title only or also against "other" epg data (backend-dependent).
  void SetFullTextEpgSearch(bool fullTextEpgSearch)
  {
    m_cStructure->bFullTextEpgSearch = fullTextEpgSearch;
  }

  /// @brief To get with @ref SetFullTextEpgSearch changed values.
  bool GetFullTextEpgSearch() const { return m_cStructure->bFullTextEpgSearch; }

  /// @brief **optional**\n
  /// The (relative) directory where the recording will be stored in.
  void SetDirectory(const std::string& directory)
  {
    strncpy(m_cStructure->strDirectory, directory.c_str(), sizeof(m_cStructure->strDirectory) - 1);
  }

  /// @brief To get with @ref SetDirectory changed values.
  std::string GetDirectory() const { return m_cStructure->strDirectory; }

  /// @brief **optional**\n
  /// The summary for this timer.
  void SetSummary(const std::string& summary)
  {
    strncpy(m_cStructure->strSummary, summary.c_str(), sizeof(m_cStructure->strSummary) - 1);
  }

  /// @brief To get with @ref SetDirectory changed values.
  std::string GetSummary() const { return m_cStructure->strSummary; }

  /// @brief **optional**\n
  /// The priority of this timer.
  void SetPriority(int priority) { m_cStructure->iPriority = priority; }

  /// @brief To get with @ref SetPriority changed values.
  int GetPriority() const { return m_cStructure->iPriority; }

  /// @brief **optional**\n
  /// Lifetime of recordings created by this timer.
  ///
  /// Value > 0 days after which recordings will be deleted by the backend, < 0
  /// addon defined integer list reference, == 0 disabled.
  void SetLifetime(int priority) { m_cStructure->iLifetime = priority; }

  /// @brief To get with @ref SetLifetime changed values.
  int GetLifetime() const { return m_cStructure->iLifetime; }

  /// @brief **optional**\n
  /// Maximum number of recordings this timer shall create.
  ///
  /// Value > 0 number of recordings, < 0 addon defined integer list reference, == 0 disabled.
  void SetMaxRecordings(int maxRecordings) { m_cStructure->iMaxRecordings = maxRecordings; }

  /// @brief To get with @ref SetMaxRecordings changed values.
  int GetMaxRecordings() const { return m_cStructure->iMaxRecordings; }

  /// @brief **optional**\n
  /// Integer ref to addon/backend defined list of recording groups.
  void SetRecordingGroup(unsigned int recordingGroup)
  {
    m_cStructure->iRecordingGroup = recordingGroup;
  }

  /// @brief To get with @ref SetRecordingGroup changed values.
  unsigned int GetRecordingGroup() const { return m_cStructure->iRecordingGroup; }

  /// @brief **optional**\n
  /// The first day this timer is active, for repeating timers.
  void SetFirstDay(time_t firstDay) { m_cStructure->firstDay = firstDay; }

  /// @brief To get with @ref SetFirstDay changed values.
  time_t GetFirstDay() const { return m_cStructure->firstDay; }

  /// @brief **optional**\n
  /// Week days, for repeating timers (see
  /// @ref cpp_kodi_addon_pvr_Defs_Timer_PVR_WEEKDAY "PVR_WEEKDAY_*" constant values)
  ///
  /// @note @ref PVR_WEEKDAY_NONE is default.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  /// kodi::addon::PVRTimer tag;
  /// tag.SetWeekdays(PVR_WEEKDAY_MONDAY | PVR_WEEKDAY_SATURDAY);
  /// ...
  /// ~~~~~~~~~~~~~
  void SetWeekdays(unsigned int weekdays) { m_cStructure->iWeekdays = weekdays; }

  /// @brief To get with @ref SetFirstDay changed values.
  unsigned int GetWeekdays() const { return m_cStructure->iWeekdays; }

  /// @brief **optional**\n
  /// Prevent duplicate episodes.
  ///
  /// Should 1 if backend should only record new episodes in case of a repeating
  /// epg-based timer, 0 if all episodes shall be recorded (no duplicate detection).
  ///
  /// Actual algorithm for duplicate detection is defined by the backend.
  /// Addons may define own values for different duplicate detection
  /// algorithms, thus this is not just a bool.
  void SetPreventDuplicateEpisodes(unsigned int preventDuplicateEpisodes)
  {
    m_cStructure->iPreventDuplicateEpisodes = preventDuplicateEpisodes;
  }

  /// @brief To get with @ref SetPreventDuplicateEpisodes changed values.
  unsigned int GetPreventDuplicateEpisodes() const
  {
    return m_cStructure->iPreventDuplicateEpisodes;
  }

  /// @brief **optional**\n
  /// EPG event id associated with this timer. Event ids must be unique for a
  /// channel.
  ///
  /// Valid ids must be greater than @ref EPG_TAG_INVALID_UID.
  void SetEPGUid(unsigned int epgUid) { m_cStructure->iEpgUid = epgUid; }

  /// @brief To get with @ref SetEPGUid changed values.
  unsigned int GetEPGUid() const { return m_cStructure->iEpgUid; }

  /// @brief **optional**\n
  /// If set, the backend starts the recording selected minutes before
  /// @ref SetStartTime.
  void SetMarginStart(unsigned int marginStart) { m_cStructure->iMarginStart = marginStart; }

  /// @brief To get with @ref SetMarginStart changed values.
  unsigned int GetMarginStart() const { return m_cStructure->iMarginStart; }

  /// @brief **optional**\n
  /// If set, the backend ends the recording selected minutes after
  /// @ref SetEndTime.
  void SetMarginEnd(unsigned int marginEnd) { m_cStructure->iMarginEnd = marginEnd; }

  /// @brief To get with @ref SetMarginEnd changed values.
  unsigned int GetMarginEnd() const { return m_cStructure->iMarginEnd; }

  /// @brief **optional**\n
  /// Genre type.
  ///
  /// @copydetails EPG_EVENT_CONTENTMASK
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  /// kodi::addon::PVRTimer tag;
  /// tag.SetGenreType(EPG_EVENT_CONTENTMASK_MOVIEDRAMA);
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  /// @note If confirmed that backend brings the types in [ETSI EN 300 468](https://www.etsi.org/deliver/etsi_en/300400_300499/300468/01.14.01_60/en_300468v011401p.pdf)
  /// conform values, can be @ref EPG_EVENT_CONTENTMASK ignored and to set here
  /// with backend value.
  ///
  void SetGenreType(int genreType) { m_cStructure->iGenreType = genreType; }

  /// @brief To get with @ref SetGenreType changed values.
  int GetGenreType() const { return m_cStructure->iGenreType; }

  /// @brief **optional**\n
  /// Genre sub type.
  ///
  /// @copydetails EPG_EVENT_CONTENTMASK
  ///
  /// Subtypes groups related to set by @ref SetGenreType:
  /// | Main genre type | List with available sub genre types
  /// |-----------------|-----------------------------------------
  /// | @ref EPG_EVENT_CONTENTMASK_UNDEFINED | Nothing, should be 0
  /// | @ref EPG_EVENT_CONTENTMASK_MOVIEDRAMA | @ref EPG_EVENT_CONTENTSUBMASK_MOVIEDRAMA
  /// | @ref EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS | @ref EPG_EVENT_CONTENTSUBMASK_NEWSCURRENTAFFAIRS
  /// | @ref EPG_EVENT_CONTENTMASK_SHOW | @ref EPG_EVENT_CONTENTSUBMASK_SHOW
  /// | @ref EPG_EVENT_CONTENTMASK_SPORTS | @ref EPG_EVENT_CONTENTSUBMASK_SPORTS
  /// | @ref EPG_EVENT_CONTENTMASK_CHILDRENYOUTH | @ref EPG_EVENT_CONTENTSUBMASK_CHILDRENYOUTH
  /// | @ref EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE | @ref EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE
  /// | @ref EPG_EVENT_CONTENTMASK_ARTSCULTURE | @ref EPG_EVENT_CONTENTSUBMASK_ARTSCULTURE
  /// | @ref EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS | @ref EPG_EVENT_CONTENTSUBMASK_SOCIALPOLITICALECONOMICS
  /// | @ref EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE | @ref EPG_EVENT_CONTENTSUBMASK_EDUCATIONALSCIENCE
  /// | @ref EPG_EVENT_CONTENTMASK_LEISUREHOBBIES | @ref EPG_EVENT_CONTENTSUBMASK_LEISUREHOBBIES
  /// | @ref EPG_EVENT_CONTENTMASK_SPECIAL | @ref EPG_EVENT_CONTENTSUBMASK_SPECIAL
  /// | @ref EPG_EVENT_CONTENTMASK_USERDEFINED | Can be defined by you
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  /// kodi::addon::PVRTimer tag;
  /// tag.SetGenreType(EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE);
  /// tag.SetGenreSubType(EPG_EVENT_CONTENTSUBMASK_MUSICBALLETDANCE_JAZZ);
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  void SetGenreSubType(int genreSubType) { m_cStructure->iGenreSubType = genreSubType; }

  /// @brief To get with @ref SetGenreType changed values.
  int GetGenreSubType() const { return m_cStructure->iGenreSubType; }

  /// @brief **optional**\n
  /// Series link for this timer.
  ///
  /// If set for an epg-based timer rule, matching events will be found by
  /// checking with here, instead of @ref SetTitle() (and @ref SetFullTextEpgSearch()).
  void SetSeriesLink(const std::string& seriesLink)
  {
    strncpy(m_cStructure->strSeriesLink, seriesLink.c_str(),
            sizeof(m_cStructure->strSeriesLink) - 1);
  }

  /// @brief To get with @ref SetSeriesLink changed values.
  std::string GetSeriesLink() const { return m_cStructure->strSeriesLink; }
  ///@}

private:
  PVRTimer(const PVR_TIMER* data) : CStructHdl(data) {}
  PVRTimer(PVR_TIMER* data) : CStructHdl(data) {}
};

///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_Timer_PVRTimersResultSet class PVRTimersResultSet
/// @ingroup cpp_kodi_addon_pvr_Defs_Timer_PVRTimer
/// @brief **PVR add-on timer transfer class**\n
/// To transfer the content of @ref kodi::addon::CInstancePVRClient::GetTimers().
///
/// @note This becomes only be used on addon call above, not usable outside on
/// addon itself.
///@{
class PVRTimersResultSet
{
public:
  /*! \cond PRIVATE */
  PVRTimersResultSet() = delete;
  PVRTimersResultSet(const AddonInstance_PVR* instance, PVR_HANDLE handle)
    : m_instance(instance), m_handle(handle)
  {
  }
  /*! \endcond */

  /// @addtogroup cpp_kodi_addon_pvr_Defs_Timer_PVRTimersResultSet
  ///@{

  /// @brief To add and give content from addon to Kodi on related call.
  ///
  /// @param[in] tag The to transferred data.
  void Add(const kodi::addon::PVRTimer& tag)
  {
    m_instance->toKodi->TransferTimerEntry(m_instance->toKodi->kodiInstance, m_handle, tag);
  }

  ///@}

private:
  const AddonInstance_PVR* m_instance = nullptr;
  const PVR_HANDLE m_handle;
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_Timer_PVRTimerType class PVRTimerType
/// @ingroup cpp_kodi_addon_pvr_Defs_Timer
/// @brief **PVR add-on timer type**\n
/// To define the content of @ref kodi::addon::CInstancePVRClient::GetTimerTypes()
/// given groups.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_Timer_PVRTimerType_Help
///
///@{
class PVRTimerType : public CStructHdl<PVRTimerType, PVR_TIMER_TYPE>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRTimerType()
  {
    memset(m_cStructure, 0, sizeof(PVR_TIMER_TYPE));
    m_cStructure->iPrioritiesDefault = -1;
    m_cStructure->iLifetimesDefault = -1;
    m_cStructure->iPreventDuplicateEpisodesDefault = -1;
    m_cStructure->iRecordingGroupDefault = -1;
    m_cStructure->iMaxRecordingsDefault = -1;
  }
  PVRTimerType(const PVRTimerType& type) : CStructHdl(type) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_Timer_PVRTimerType_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_Timer_PVRTimerType
  /// ----------------------------------------------------------------------------
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_Timer_PVRTimerType :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |------|------|----------|----------|-----------
  /// | **Identifier** | `unsigned int` | @ref PVRTimerType::SetId "SetId" | @ref PVRTimerType::GetId "GetId" | *required to set*
  /// | **Attributes** | `unsigned int` | @ref PVRTimerType::SetAttributes "SetAttributes" | @ref PVRTimerType::GetAttributes "GetAttributes" | *required to set*
  /// | **Description** | `std::string` | @ref PVRTimerType::SetDescription "SetDescription" | @ref PVRTimerType::GetDescription "GetDescription" | *optional*
  /// | | | | | |
  /// | **Priority selection** |  @ref cpp_kodi_addon_pvr_Defs_PVRTypeIntValue "PVRTypeIntValue" | @ref PVRTimerType::SetPriorities "SetPriorities" | @ref PVRTimerType::GetPriorities "GetPriorities" | *optional*
  /// | **Priority default selection** | `int`| @ref PVRTimerType::SetPrioritiesDefault "SetPrioritiesDefault" | @ref PVRTimerType::GetPrioritiesDefault "GetPrioritiesDefault" | *optional*
  /// | | | | | |
  /// | **Lifetime selection** |  @ref cpp_kodi_addon_pvr_Defs_PVRTypeIntValue "PVRTypeIntValue" | @ref PVRTimerType::SetLifetimes "SetLifetimes" | @ref PVRTimerType::GetLifetimes "GetLifetimes" | *optional*
  /// | **Lifetime default selection** | `int`| @ref PVRTimerType::SetLifetimesDefault "SetLifetimesDefault" | @ref PVRTimerType::GetLifetimesDefault "GetLifetimesDefault" | *optional*
  /// | | | | | |
  /// | **Prevent duplicate episodes selection** |  @ref cpp_kodi_addon_pvr_Defs_PVRTypeIntValue "PVRTypeIntValue" | @ref PVRTimerType::SetPreventDuplicateEpisodes "SetPreventDuplicateEpisodes" | @ref PVRTimerType::GetPreventDuplicateEpisodes "GetPreventDuplicateEpisodes" | *optional*
  /// | **Prevent duplicate episodes default** | `int`| @ref PVRTimerType::SetPreventDuplicateEpisodesDefault "SetPreventDuplicateEpisodesDefault" | @ref PVRTimerType::GetPreventDuplicateEpisodesDefault "GetPreventDuplicateEpisodesDefault" | *optional*
  /// | | | | | |
  /// | **Recording group selection**|  @ref cpp_kodi_addon_pvr_Defs_PVRTypeIntValue "PVRTypeIntValue" | @ref PVRTimerType::SetRecordingGroups "SetRecordingGroups" | @ref PVRTimerType::GetRecordingGroups "GetRecordingGroups" | *optional*
  /// | **Recording group default** | `int`| @ref PVRTimerType::SetRecordingGroupDefault "SetRecordingGroupDefault" | @ref PVRTimerType::GetRecordingGroupDefault "GetRecordingGroupDefault" | *optional*
  /// | | | | | |
  /// | **Max recordings selection** | @ref cpp_kodi_addon_pvr_Defs_PVRTypeIntValue "PVRTypeIntValue" | @ref PVRTimerType::SetMaxRecordings "SetMaxRecordings" | @ref PVRTimerType::GetMaxRecordings "GetMaxRecordings" | *optional*
  /// | **Max recordings default** | `int`| @ref PVRTimerType::SetMaxRecordingsDefault "SetMaxRecordingsDefault" | @ref PVRTimerType::GetMaxRecordingsDefault "GetMaxRecordingsDefault" | *optional*
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_Timer_PVRTimerType
  ///@{

  /// @brief **required**\n
  /// This type's identifier. Ids must be > @ref PVR_TIMER_TYPE_NONE.
  void SetId(unsigned int id) { m_cStructure->iId = id; }

  /// @brief To get with @ref SetAttributes changed values.
  unsigned int GetId() const { return m_cStructure->iId; }

  /// @brief **required**\n
  /// Defines the attributes for this type (@ref cpp_kodi_addon_pvr_Defs_Timer_PVR_TIMER_TYPE "PVR_TIMER_TYPE_*" constants).
  ///
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
  void SetAttributes(uint64_t attributes) { m_cStructure->iAttributes = attributes; }

  /// @brief To get with @ref SetAttributes changed values.
  uint64_t GetAttributes() const { return m_cStructure->iAttributes; }

  /// @brief **optional**\n
  /// A short localized string describing the purpose of the type. (e.g.
  /// "Any time at this channel if title matches").
  ///
  /// If left blank, Kodi will generate a description based on the attributes
  /// REPEATING and MANUAL. (e.g. "Repeating EPG-based.")
  void SetDescription(const std::string& description)
  {
    strncpy(m_cStructure->strDescription, description.c_str(),
            sizeof(m_cStructure->strDescription) - 1);
  }

  /// @brief To get with @ref SetDescription changed values.
  std::string GetDescription() const { return m_cStructure->strDescription; }

  //----------------------------------------------------------------------------

  /// @brief **optional**\n
  /// Priority value definitions.
  ///
  /// Array containing the possible values for @ref PVRTimer::SetPriority().
  ///
  /// @param[in] priorities List of priority values
  /// @param[in] prioritiesDefault [opt] The default value in list, can also be
  ///                              set by @ref SetPrioritiesDefault()
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_PVRTypeIntValue_Help
  void SetPriorities(const std::vector<PVRTypeIntValue>& priorities, int prioritiesDefault = -1)
  {
    m_cStructure->iPrioritiesSize = static_cast<unsigned int>(priorities.size());
    for (unsigned int i = 0;
         i < m_cStructure->iPrioritiesSize && i < sizeof(m_cStructure->priorities); ++i)
    {
      m_cStructure->priorities[i].iValue = priorities[i].GetCStructure()->iValue;
      strncpy(m_cStructure->priorities[i].strDescription,
              priorities[i].GetCStructure()->strDescription,
              sizeof(m_cStructure->priorities[i].strDescription) - 1);
    }
    if (prioritiesDefault != -1)
      m_cStructure->iPrioritiesDefault = prioritiesDefault;
  }

  /// @brief To get with @ref SetPriorities changed values.
  std::vector<PVRTypeIntValue> GetPriorities() const
  {
    std::vector<PVRTypeIntValue> ret;
    for (unsigned int i = 0; i < m_cStructure->iPrioritiesSize; ++i)
      ret.emplace_back(m_cStructure->priorities[i].iValue,
                       m_cStructure->priorities[i].strDescription);
    return ret;
  }

  /// @brief **optional**\n
  /// The default value for @ref PVRTimer::SetPriority().
  ///
  /// @note Must be filled if @ref SetPriorities contain values and not
  /// defined there on second function value.
  void SetPrioritiesDefault(int prioritiesDefault)
  {
    m_cStructure->iPrioritiesDefault = prioritiesDefault;
  }

  /// @brief To get with @ref SetPrioritiesDefault changed values.
  int GetPrioritiesDefault() const { return m_cStructure->iPrioritiesDefault; }

  //----------------------------------------------------------------------------

  /// @brief **optional**\n
  /// Lifetime value definitions.
  ///
  /// Array containing the possible values for @ref PVRTimer::SetLifetime().
  ///
  /// @param[in] lifetimes List of lifetimes values
  /// @param[in] lifetimesDefault [opt] The default value in list, can also be
  ///                             set by @ref SetLifetimesDefault()
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_PVRTypeIntValue_Help
  void SetLifetimes(const std::vector<PVRTypeIntValue>& lifetimes, int lifetimesDefault = -1)
  {
    m_cStructure->iLifetimesSize = static_cast<unsigned int>(lifetimes.size());
    for (unsigned int i = 0;
         i < m_cStructure->iLifetimesSize && i < sizeof(m_cStructure->lifetimes); ++i)
    {
      m_cStructure->lifetimes[i].iValue = lifetimes[i].GetCStructure()->iValue;
      strncpy(m_cStructure->lifetimes[i].strDescription,
              lifetimes[i].GetCStructure()->strDescription,
              sizeof(m_cStructure->lifetimes[i].strDescription) - 1);
    }
    if (lifetimesDefault != -1)
      m_cStructure->iLifetimesDefault = lifetimesDefault;
  }

  /// @brief To get with @ref SetLifetimes changed values.
  std::vector<PVRTypeIntValue> GetLifetimes() const
  {
    std::vector<PVRTypeIntValue> ret;
    for (unsigned int i = 0; i < m_cStructure->iLifetimesSize; ++i)
      ret.emplace_back(m_cStructure->lifetimes[i].iValue,
                       m_cStructure->lifetimes[i].strDescription);
    return ret;
  }

  /// @brief **optional**\n
  /// The default value for @ref SetLifetimes().
  ///
  /// @note Must be filled if @ref SetLifetimes contain values and not
  /// defined there on second function value.
  void SetLifetimesDefault(int lifetimesDefault)
  {
    m_cStructure->iLifetimesDefault = lifetimesDefault;
  }

  /// @brief To get with @ref SetLifetimesDefault changed values.
  int GetLifetimesDefault() const { return m_cStructure->iLifetimesDefault; }

  //----------------------------------------------------------------------------

  /// @brief **optional**\n
  /// Prevent duplicate episodes value definitions.
  ///
  /// Array containing the possible values for @ref PVRTimer::SetPreventDuplicateEpisodes().
  ///
  /// @note Must be filled if @ref PVRTimer::SetPreventDuplicateEpisodes() is not empty.
  ///
  /// @param[in] preventDuplicateEpisodes List of duplicate episodes values
  /// @param[in] preventDuplicateEpisodesDefault [opt] The default value in list, can also be
  ///                                            set by @ref SetPreventDuplicateEpisodesDefault()
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_PVRTypeIntValue_Help
  void SetPreventDuplicateEpisodes(const std::vector<PVRTypeIntValue>& preventDuplicateEpisodes,
                                   int preventDuplicateEpisodesDefault = -1)
  {
    m_cStructure->iPreventDuplicateEpisodesSize =
        static_cast<unsigned int>(preventDuplicateEpisodes.size());
    for (unsigned int i = 0; i < m_cStructure->iPreventDuplicateEpisodesSize &&
                             i < sizeof(m_cStructure->preventDuplicateEpisodes);
         ++i)
    {
      m_cStructure->preventDuplicateEpisodes[i].iValue =
          preventDuplicateEpisodes[i].GetCStructure()->iValue;
      strncpy(m_cStructure->preventDuplicateEpisodes[i].strDescription,
              preventDuplicateEpisodes[i].GetCStructure()->strDescription,
              sizeof(m_cStructure->preventDuplicateEpisodes[i].strDescription) - 1);
    }
    if (preventDuplicateEpisodesDefault != -1)
      m_cStructure->iPreventDuplicateEpisodesDefault = preventDuplicateEpisodesDefault;
  }

  /// @brief To get with @ref SetPreventDuplicateEpisodes changed values.
  std::vector<PVRTypeIntValue> GetPreventDuplicateEpisodes() const
  {
    std::vector<PVRTypeIntValue> ret;
    for (unsigned int i = 0; i < m_cStructure->iPreventDuplicateEpisodesSize; ++i)
      ret.emplace_back(m_cStructure->preventDuplicateEpisodes[i].iValue,
                       m_cStructure->preventDuplicateEpisodes[i].strDescription);
    return ret;
  }

  /// @brief **optional**\n
  /// The default value for @ref PVRTimer::SetPreventDuplicateEpisodes().
  ///
  /// @note Must be filled if @ref SetPreventDuplicateEpisodes contain values and not
  /// defined there on second function value.
  void SetPreventDuplicateEpisodesDefault(int preventDuplicateEpisodesDefault)
  {
    m_cStructure->iPreventDuplicateEpisodesDefault = preventDuplicateEpisodesDefault;
  }

  /// @brief To get with @ref SetPreventDuplicateEpisodesDefault changed values.
  int GetPreventDuplicateEpisodesDefault() const
  {
    return m_cStructure->iPreventDuplicateEpisodesDefault;
  }

  //----------------------------------------------------------------------------

  /// @brief **optional**\n
  /// Array containing the possible values of @ref PVRTimer::SetRecordingGroup()
  ///
  /// @param[in] recordingGroup List of recording group values
  /// @param[in] recordingGroupDefault [opt] The default value in list, can also be
  ///                                  set by @ref SetRecordingGroupDefault()
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_PVRTypeIntValue_Help
  void SetRecordingGroups(const std::vector<PVRTypeIntValue>& recordingGroup,
                          int recordingGroupDefault = -1)
  {
    m_cStructure->iRecordingGroupSize = static_cast<unsigned int>(recordingGroup.size());
    for (unsigned int i = 0;
         i < m_cStructure->iRecordingGroupSize && i < sizeof(m_cStructure->recordingGroup); ++i)
    {
      m_cStructure->recordingGroup[i].iValue = recordingGroup[i].GetCStructure()->iValue;
      strncpy(m_cStructure->recordingGroup[i].strDescription,
              recordingGroup[i].GetCStructure()->strDescription,
              sizeof(m_cStructure->recordingGroup[i].strDescription) - 1);
    }
    if (recordingGroupDefault != -1)
      m_cStructure->iRecordingGroupDefault = recordingGroupDefault;
  }

  /// @brief To get with @ref SetRecordingGroups changed values
  std::vector<PVRTypeIntValue> GetRecordingGroups() const
  {
    std::vector<PVRTypeIntValue> ret;
    for (unsigned int i = 0; i < m_cStructure->iRecordingGroupSize; ++i)
      ret.emplace_back(m_cStructure->recordingGroup[i].iValue,
                       m_cStructure->recordingGroup[i].strDescription);
    return ret;
  }

  /// @brief **optional**\n
  /// The default value for @ref PVRTimer::SetRecordingGroup().
  ///
  /// @note Must be filled if @ref SetRecordingGroups contain values and not
  /// defined there on second function value.
  void SetRecordingGroupDefault(int recordingGroupDefault)
  {
    m_cStructure->iRecordingGroupDefault = recordingGroupDefault;
  }

  /// @brief To get with @ref SetRecordingGroupDefault changed values
  int GetRecordingGroupDefault() const { return m_cStructure->iRecordingGroupDefault; }

  //----------------------------------------------------------------------------

  /// @brief **optional**\n
  /// Array containing the possible values of @ref PVRTimer::SetMaxRecordings().
  ///
  /// @param[in] maxRecordings List of lifetimes values
  /// @param[in] maxRecordingsDefault [opt] The default value in list, can also be
  ///                                 set by @ref SetMaxRecordingsDefault()
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_PVRTypeIntValue_Help
  void SetMaxRecordings(const std::vector<PVRTypeIntValue>& maxRecordings,
                        int maxRecordingsDefault = -1)
  {
    m_cStructure->iMaxRecordingsSize = static_cast<unsigned int>(maxRecordings.size());
    for (unsigned int i = 0;
         i < m_cStructure->iMaxRecordingsSize && i < sizeof(m_cStructure->maxRecordings); ++i)
    {
      m_cStructure->maxRecordings[i].iValue = maxRecordings[i].GetCStructure()->iValue;
      strncpy(m_cStructure->maxRecordings[i].strDescription,
              maxRecordings[i].GetCStructure()->strDescription,
              sizeof(m_cStructure->maxRecordings[i].strDescription) - 1);
    }
    if (maxRecordingsDefault != -1)
      m_cStructure->iMaxRecordingsDefault = maxRecordingsDefault;
  }

  /// @brief To get with @ref SetMaxRecordings changed values
  std::vector<PVRTypeIntValue> GetMaxRecordings() const
  {
    std::vector<PVRTypeIntValue> ret;
    for (unsigned int i = 0; i < m_cStructure->iMaxRecordingsSize; ++i)
      ret.emplace_back(m_cStructure->maxRecordings[i].iValue,
                       m_cStructure->maxRecordings[i].strDescription);
    return ret;
  }

  /// @brief **optional**\n
  /// The default value for @ref SetMaxRecordings().
  ///
  /// Can be set with here if on @ref SetMaxRecordings not given as second value.
  void SetMaxRecordingsDefault(int maxRecordingsDefault)
  {
    m_cStructure->iMaxRecordingsDefault = maxRecordingsDefault;
  }

  /// @brief To get with @ref SetMaxRecordingsDefault changed values
  int GetMaxRecordingsDefault() const { return m_cStructure->iMaxRecordingsDefault; }
  ///@}

private:
  PVRTimerType(const PVR_TIMER_TYPE* type) : CStructHdl(type) {}
  PVRTimerType(PVR_TIMER_TYPE* type) : CStructHdl(type) {}
};
///@}
//------------------------------------------------------------------------------

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
