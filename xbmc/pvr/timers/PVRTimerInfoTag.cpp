/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PVRTimerInfoTag.h"

#include "ServiceBroker.h"
#include "addons/PVRClient.h"
#include "guilib/LocalizeStrings.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgContainer.h"
#include "pvr/timers/PVRTimers.h"

using namespace PVR;

CPVRTimerInfoTag::CPVRTimerInfoTag(bool bRadio /* = false */) :
  m_strTitle(g_localizeStrings.Get(19056)), // New Timer
  m_iClientId(CServiceBroker::GetPVRManager().Clients()->GetFirstCreatedClientID()),
  m_iClientIndex(PVR_TIMER_NO_CLIENT_INDEX),
  m_iParentClientIndex(PVR_TIMER_NO_PARENT),
  m_iClientChannelUid(PVR_CHANNEL_INVALID_UID),
  m_iPriority(DEFAULT_RECORDING_PRIORITY),
  m_iLifetime(DEFAULT_RECORDING_LIFETIME),
  m_iPreventDupEpisodes(DEFAULT_RECORDING_DUPLICATEHANDLING),
  m_bIsRadio(bRadio),
  m_iMarginStart(CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRRECORD_MARGINSTART)),
  m_iMarginEnd(CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRRECORD_MARGINEND)),
  m_StartTime(CDateTime::GetUTCDateTime()),
  m_StopTime(m_StartTime),
  m_iEpgUid(EPG_TAG_INVALID_UID)
{
  m_FirstDay.SetValid(false);

  const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsTimers())
  {
    // default to manual one-shot timer for given client
    CPVRTimerTypePtr type(CPVRTimerType::CreateFromAttributes(
      PVR_TIMER_TYPE_IS_MANUAL, PVR_TIMER_TYPE_IS_REPEATING | PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES, m_iClientId));

    if (!type)
    {
      // last resort. default to first available type from any client.
      type = CPVRTimerType::GetFirstAvailableType();
    }

    if (type)
      SetTimerType(type);
    else
      CLog::LogF(LOGERROR, "No timer type, although timers are supported by client %d!", m_iClientId);
  }

  m_iWeekdays = (m_timerType && m_timerType->IsTimerRule()) ? PVR_WEEKDAY_ALLDAYS : PVR_WEEKDAY_NONE;

  ResetChildState();
  UpdateSummary();
}

CPVRTimerInfoTag::CPVRTimerInfoTag(const PVR_TIMER &timer, const CPVRChannelPtr &channel, unsigned int iClientId) :
  m_strTitle(timer.strTitle),
  m_strEpgSearchString(timer.strEpgSearchString),
  m_bFullTextEpgSearch(timer.bFullTextEpgSearch),
  m_strDirectory(timer.strDirectory),
  m_state(timer.state),
  m_iClientId(iClientId),
  m_iClientIndex(timer.iClientIndex),
  m_iParentClientIndex(timer.iParentClientIndex),
  m_iClientChannelUid(channel ? channel->UniqueID() : (timer.iClientChannelUid > 0) ? timer.iClientChannelUid : PVR_CHANNEL_INVALID_UID),
  m_bStartAnyTime(timer.bStartAnyTime),
  m_bEndAnyTime(timer.bEndAnyTime),
  m_iPriority(timer.iPriority),
  m_iLifetime(timer.iLifetime),
  m_iMaxRecordings(timer.iMaxRecordings),
  m_iWeekdays(timer.iWeekdays),
  m_iPreventDupEpisodes(timer.iPreventDuplicateEpisodes),
  m_iRecordingGroup(timer.iRecordingGroup),
  m_strFileNameAndPath(StringUtils::Format("pvr://client%i/timers/%i", m_iClientId, m_iClientIndex)),
  m_bIsRadio(channel && channel->IsRadio()),
  m_iMarginStart(timer.iMarginStart),
  m_iMarginEnd(timer.iMarginEnd),
  m_StartTime(timer.startTime + g_advancedSettings.m_iPVRTimeCorrection),
  m_StopTime(timer.endTime + g_advancedSettings.m_iPVRTimeCorrection),
  m_FirstDay(timer.firstDay + g_advancedSettings.m_iPVRTimeCorrection),
  m_strSeriesLink(timer.strSeriesLink),
  m_iEpgUid(timer.iEpgUid),
  m_channel(channel)
{
  if (m_iClientIndex == PVR_TIMER_NO_CLIENT_INDEX)
    CLog::LogF(LOGERROR, "Invalid client index supplied by client %d (must be > 0)!", m_iClientId);

  const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client && client->GetClientCapabilities().SupportsTimers())
  {
    // begin compat section

    // Create timer type according to certain timer values already available in Isengard.
    // This is for migration only and does not make changes to the addons obsolete. Addons should
    // work and benefit from some UI changes (e.g. some of the timer settings dialog enhancements),
    // but all old problems/bugs due to static attributes and values will remain the same as in
    // Isengard. Also, new features (like epg search) are not available to addons automatically.
    // This code can be removed once all addons actually support the respective PVR Addon API version.
    if (timer.iTimerType == PVR_TIMER_TYPE_NONE)
    {
      // Create type according to certain timer values.
      unsigned int iMustHave    = PVR_TIMER_TYPE_ATTRIBUTE_NONE;
      unsigned int iMustNotHave = PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES;

      if (timer.iEpgUid == PVR_TIMER_NO_EPG_UID && timer.iWeekdays != PVR_WEEKDAY_NONE)
        iMustHave |= PVR_TIMER_TYPE_IS_REPEATING;
      else
        iMustNotHave |= PVR_TIMER_TYPE_IS_REPEATING;

      if (timer.iEpgUid == PVR_TIMER_NO_EPG_UID)
        iMustHave |= PVR_TIMER_TYPE_IS_MANUAL;
      else
        iMustNotHave |= PVR_TIMER_TYPE_IS_MANUAL;

      CPVRTimerTypePtr type(CPVRTimerType::CreateFromAttributes(iMustHave, iMustNotHave, m_iClientId));

      if (type)
        SetTimerType(type);
    }
    // end compat section
    else
    {
      SetTimerType(CPVRTimerType::CreateFromIds(timer.iTimerType, m_iClientId));
    }

    if (!m_timerType)
      CLog::LogF(LOGERROR, "No timer type, although timers are supported by client %d!", m_iClientId);
    else if (m_iEpgUid == EPG_TAG_INVALID_UID && m_timerType->IsEpgBasedOnetime())
      CLog::LogF(LOGERROR, "No epg tag given for epg based timer type (%d)!", m_timerType->GetTypeId());
  }

  ResetChildState();
  UpdateSummary();
  UpdateEpgInfoTag();
}

bool CPVRTimerInfoTag::operator ==(const CPVRTimerInfoTag& right) const
{
  bool bChannelsMatch = true;
  if (m_channel && right.m_channel)
    bChannelsMatch = *m_channel == *right.m_channel;
  else if (m_channel != right.m_channel)
    bChannelsMatch = false;

  return (bChannelsMatch &&
          m_iClientIndex        == right.m_iClientIndex &&
          m_iParentClientIndex  == right.m_iParentClientIndex &&
          m_strSummary          == right.m_strSummary &&
          m_iClientChannelUid   == right.m_iClientChannelUid &&
          m_bIsRadio            == right.m_bIsRadio &&
          m_iPreventDupEpisodes == right.m_iPreventDupEpisodes &&
          m_iRecordingGroup     == right.m_iRecordingGroup &&
          m_StartTime           == right.m_StartTime &&
          m_StopTime            == right.m_StopTime &&
          m_bStartAnyTime       == right.m_bStartAnyTime &&
          m_bEndAnyTime         == right.m_bEndAnyTime &&
          m_FirstDay            == right.m_FirstDay &&
          m_iWeekdays           == right.m_iWeekdays &&
          m_iPriority           == right.m_iPriority &&
          m_iLifetime           == right.m_iLifetime &&
          m_iMaxRecordings      == right.m_iMaxRecordings &&
          m_strFileNameAndPath  == right.m_strFileNameAndPath &&
          m_strTitle            == right.m_strTitle &&
          m_strEpgSearchString  == right.m_strEpgSearchString &&
          m_bFullTextEpgSearch  == right.m_bFullTextEpgSearch &&
          m_strDirectory        == right.m_strDirectory &&
          m_iClientId           == right.m_iClientId &&
          m_iMarginStart        == right.m_iMarginStart &&
          m_iMarginEnd          == right.m_iMarginEnd &&
          m_state               == right.m_state &&
          m_timerType           == right.m_timerType &&
          m_iTimerId            == right.m_iTimerId &&
          m_strSeriesLink       == right.m_strSeriesLink &&
          m_iEpgUid             == right.m_iEpgUid &&
          m_iActiveChildTimers  == right.m_iActiveChildTimers &&
          m_bHasChildConflictNOK== right.m_bHasChildConflictNOK &&
          m_bHasChildRecording  == right.m_bHasChildRecording &&
          m_bHasChildErrors     == right.m_bHasChildErrors);
}

/**
 * Compare not equal for two CPVRTimerInfoTag
 */
bool CPVRTimerInfoTag::operator !=(const CPVRTimerInfoTag& right) const
{
  return !(*this == right);
}

void CPVRTimerInfoTag::Serialize(CVariant &value) const
{
  value["channelid"] = m_channel != NULL ? m_channel->ChannelID() : -1;
  value["summary"] = m_strSummary;
  value["isradio"] = m_bIsRadio;
  value["preventduplicateepisodes"] = m_iPreventDupEpisodes;
  value["starttime"] = m_StartTime.IsValid() ? m_StartTime.GetAsDBDateTime() : "";
  value["endtime"] = m_StopTime.IsValid() ? m_StopTime.GetAsDBDateTime() : "";
  value["startanytime"] = m_bStartAnyTime;
  value["endanytime"] = m_bEndAnyTime;
  value["runtime"] = m_StartTime.IsValid() && m_StopTime.IsValid() ? (m_StopTime - m_StartTime).GetSecondsTotal() : 0;
  value["firstday"] = m_FirstDay.IsValid() ? m_FirstDay.GetAsDBDate() : "";

  CVariant weekdays(CVariant::VariantTypeArray);
  if (m_iWeekdays & PVR_WEEKDAY_MONDAY)
    weekdays.push_back("monday");
  if (m_iWeekdays & PVR_WEEKDAY_TUESDAY)
    weekdays.push_back("tuesday");
  if (m_iWeekdays & PVR_WEEKDAY_WEDNESDAY)
    weekdays.push_back("wednesday");
  if (m_iWeekdays & PVR_WEEKDAY_THURSDAY)
    weekdays.push_back("thursday");
  if (m_iWeekdays & PVR_WEEKDAY_FRIDAY)
    weekdays.push_back("friday");
  if (m_iWeekdays & PVR_WEEKDAY_SATURDAY)
    weekdays.push_back("saturday");
  if (m_iWeekdays & PVR_WEEKDAY_SUNDAY)
    weekdays.push_back("sunday");
  value["weekdays"] = weekdays;

  value["priority"] = m_iPriority;
  value["lifetime"] = m_iLifetime;
  value["title"] = m_strTitle;
  value["directory"] = m_strDirectory;
  value["startmargin"] = m_iMarginStart;
  value["endmargin"] = m_iMarginEnd;

  value["timerid"] = m_iTimerId;

  switch (m_state)
  {
  case PVR_TIMER_STATE_NEW:
    value["state"] = "new";
    break;
  case PVR_TIMER_STATE_SCHEDULED:
    value["state"] = "scheduled";
    break;
  case PVR_TIMER_STATE_RECORDING:
    value["state"] = "recording";
    break;
  case PVR_TIMER_STATE_COMPLETED:
    value["state"] = "completed";
    break;
  case PVR_TIMER_STATE_ABORTED:
    value["state"] = "aborted";
    break;
  case PVR_TIMER_STATE_CANCELLED:
    value["state"] = "cancelled";
    break;
  case PVR_TIMER_STATE_CONFLICT_OK:
    value["state"] = "conflict_ok";
    break;
  case PVR_TIMER_STATE_CONFLICT_NOK:
    value["state"] = "conflict_notok";
    break;
  case PVR_TIMER_STATE_ERROR:
    value["state"] = "error";
    break;
  case PVR_TIMER_STATE_DISABLED:
    value["state"] = "disabled";
    break;
  default:
    value["state"] = "unknown";
    break;
  }

  value["istimerrule"] = m_timerType && m_timerType->IsTimerRule();
  value["ismanual"] = m_timerType && m_timerType->IsManual();
  value["isreadonly"] = m_timerType && m_timerType->IsReadOnly();

  value["epgsearchstring"]   = m_strEpgSearchString;
  value["fulltextepgsearch"] = m_bFullTextEpgSearch;
  value["recordinggroup"]    = m_iRecordingGroup;
  value["maxrecordings"]     = m_iMaxRecordings;
  value["epguid"]            = m_iEpgUid;
  value["serieslink"]        = m_strSeriesLink;
}

void CPVRTimerInfoTag::UpdateEpgInfoTag(void)
{
  CSingleLock lock(m_critSection);
  m_epgTag.reset();
  GetEpgInfoTag();
}

void CPVRTimerInfoTag::UpdateSummary(void)
{
  CSingleLock lock(m_critSection);
  m_strSummary.clear();

  const std::string startDate(StartAsLocalTime().GetAsLocalizedDate());
  const std::string endDate(EndAsLocalTime().GetAsLocalizedDate());

  if (m_bEndAnyTime)
  {
    m_strSummary = StringUtils::Format("%s %s %s",
        m_iWeekdays != PVR_WEEKDAY_NONE ?
          GetWeekdaysString().c_str() : startDate.c_str(), //for "Any day" set PVR_WEEKDAY_ALLDAYS
        g_localizeStrings.Get(19107).c_str(), // "at"
        m_bStartAnyTime ?
          g_localizeStrings.Get(19161).c_str() /* "any time" */ : StartAsLocalTime().GetAsLocalizedTime("", false).c_str());
  }
  else if ((m_iWeekdays != PVR_WEEKDAY_NONE) || (startDate == endDate))
  {
    m_strSummary = StringUtils::Format("%s %s %s %s %s",
        m_iWeekdays != PVR_WEEKDAY_NONE ?
          GetWeekdaysString().c_str() : startDate.c_str(), //for "Any day" set PVR_WEEKDAY_ALLDAYS
        g_localizeStrings.Get(19159).c_str(), // "from"
        m_bStartAnyTime ?
          g_localizeStrings.Get(19161).c_str() /* "any time" */ : StartAsLocalTime().GetAsLocalizedTime("", false).c_str(),
        g_localizeStrings.Get(19160).c_str(), // "to"
        m_bEndAnyTime ?
          g_localizeStrings.Get(19161).c_str() /* "any time" */ : EndAsLocalTime().GetAsLocalizedTime("", false).c_str());
  }
  else
  {
    m_strSummary = StringUtils::Format("%s %s %s %s %s %s",
        startDate.c_str(),
        g_localizeStrings.Get(19159).c_str(), // "from"
        m_bStartAnyTime ?
          g_localizeStrings.Get(19161).c_str() /* "any time" */ : StartAsLocalTime().GetAsLocalizedTime("", false).c_str(),
        g_localizeStrings.Get(19160).c_str(), // "to"
        endDate.c_str(),
        m_bEndAnyTime ?
          g_localizeStrings.Get(19161).c_str() /* "any time" */ : EndAsLocalTime().GetAsLocalizedTime("", false).c_str());
  }
}

void CPVRTimerInfoTag::SetTimerType(const CPVRTimerTypePtr &type)
{
  CSingleLock lock(m_critSection);
  m_timerType = type;

  if (m_timerType && m_iClientIndex == PVR_TIMER_NO_CLIENT_INDEX)
  {
    m_iPriority           = m_timerType->GetPriorityDefault();
    m_iLifetime           = m_timerType->GetLifetimeDefault();
    m_iMaxRecordings      = m_timerType->GetMaxRecordingsDefault();
    m_iPreventDupEpisodes = m_timerType->GetPreventDuplicateEpisodesDefault();
    m_iRecordingGroup     = m_timerType->GetRecordingGroupDefault();
  }

  if (m_timerType && !m_timerType->IsTimerRule())
    m_iWeekdays = PVR_WEEKDAY_NONE;
}

/**
 * Get the status string of this Timer, is used by the GUIInfoManager
 */
std::string CPVRTimerInfoTag::GetStatus() const
{
  std::string strReturn = g_localizeStrings.Get(305);
  CSingleLock lock(m_critSection);
  if (URIUtils::PathEquals(m_strFileNameAndPath, CPVRTimersPath::PATH_ADDTIMER))
    strReturn = g_localizeStrings.Get(19026);
  else if (m_state == PVR_TIMER_STATE_CANCELLED || m_state == PVR_TIMER_STATE_ABORTED)
    strReturn = g_localizeStrings.Get(13106);
  else if (m_state == PVR_TIMER_STATE_RECORDING)
    strReturn = g_localizeStrings.Get(19162);
  else if (m_state == PVR_TIMER_STATE_CONFLICT_OK)
    strReturn = g_localizeStrings.Get(19275);
  else if (m_state == PVR_TIMER_STATE_CONFLICT_NOK)
    strReturn = g_localizeStrings.Get(19276);
  else if (m_state == PVR_TIMER_STATE_ERROR)
    strReturn = g_localizeStrings.Get(257);
  else if (m_state == PVR_TIMER_STATE_DISABLED)
    strReturn = g_localizeStrings.Get(13106);
  else if (m_state == PVR_TIMER_STATE_COMPLETED)
    if (m_bHasChildRecording)
      strReturn = g_localizeStrings.Get(19162); // "Recording active"
    else
      strReturn = g_localizeStrings.Get(19256); // "Completed"
  else if (m_state == PVR_TIMER_STATE_SCHEDULED || m_state == PVR_TIMER_STATE_NEW)
  {
    if (m_bHasChildRecording)
      strReturn = g_localizeStrings.Get(19162); // "Recording active"
    else if (m_bHasChildErrors)
      strReturn = g_localizeStrings.Get(257);   // "Error"
    else if (m_bHasChildConflictNOK)
      strReturn = g_localizeStrings.Get(19276); // "Conflict error"
    else if (m_iActiveChildTimers > 0)
      strReturn = StringUtils::Format(g_localizeStrings.Get(19255).c_str(), m_iActiveChildTimers); // "%d scheduled"
  }

  return strReturn;
}

/**
 * Get the type string of this timer
 */
std::string CPVRTimerInfoTag::GetTypeAsString() const
{
  CSingleLock lock(m_critSection);
  return m_timerType ? m_timerType->GetDescription() : "";
}

namespace
{
void AppendDay(std::string &strReturn, unsigned int iId)
{
  if (!strReturn.empty())
    strReturn += "-";

  if (iId > 0)
    strReturn += g_localizeStrings.Get(iId).c_str();
  else
    strReturn += "__";
}
} // unnamed namespace

std::string CPVRTimerInfoTag::GetWeekdaysString(unsigned int iWeekdays, bool bEpgBased, bool bLongMultiDaysFormat)
{
  std::string strReturn;

  if (iWeekdays == PVR_WEEKDAY_NONE)
    return strReturn;
  else if (iWeekdays == PVR_WEEKDAY_ALLDAYS)
    strReturn = bEpgBased
              ? g_localizeStrings.Get(807)  // "Any day"
              : g_localizeStrings.Get(808); // "Every day"
  else if (iWeekdays == PVR_WEEKDAY_MONDAY)
    strReturn = g_localizeStrings.Get(831); // "Mondays"
  else if (iWeekdays == PVR_WEEKDAY_TUESDAY)
    strReturn = g_localizeStrings.Get(832); // "Tuesdays"
  else if (iWeekdays == PVR_WEEKDAY_WEDNESDAY)
    strReturn = g_localizeStrings.Get(833); // "Wednesdays"
  else if (iWeekdays == PVR_WEEKDAY_THURSDAY)
    strReturn = g_localizeStrings.Get(834); // "Thursdays"
  else if (iWeekdays == PVR_WEEKDAY_FRIDAY)
    strReturn = g_localizeStrings.Get(835); // "Fridays"
  else if (iWeekdays == PVR_WEEKDAY_SATURDAY)
    strReturn = g_localizeStrings.Get(836); // "Saturdays"
  else if (iWeekdays == PVR_WEEKDAY_SUNDAY)
    strReturn = g_localizeStrings.Get(837); // "Sundays"
  else
  {
    // Any other combination. Assemble custom string.
    if (iWeekdays & PVR_WEEKDAY_MONDAY)
      AppendDay(strReturn, 19149); // Mo
    else if (bLongMultiDaysFormat)
      AppendDay(strReturn, 0);

    if (iWeekdays & PVR_WEEKDAY_TUESDAY)
      AppendDay(strReturn, 19150); // Tu
    else if (bLongMultiDaysFormat)
      AppendDay(strReturn, 0);

    if (iWeekdays & PVR_WEEKDAY_WEDNESDAY)
      AppendDay(strReturn, 19151); // We
    else if (bLongMultiDaysFormat)
      AppendDay(strReturn, 0);

    if (iWeekdays & PVR_WEEKDAY_THURSDAY)
      AppendDay(strReturn, 19152); // Th
    else if (bLongMultiDaysFormat)
      AppendDay(strReturn, 0);

    if (iWeekdays & PVR_WEEKDAY_FRIDAY)
      AppendDay(strReturn, 19153); // Fr
    else if (bLongMultiDaysFormat)
      AppendDay(strReturn, 0);

    if (iWeekdays & PVR_WEEKDAY_SATURDAY)
      AppendDay(strReturn, 19154); // Sa
    else if (bLongMultiDaysFormat)
      AppendDay(strReturn, 0);

    if (iWeekdays & PVR_WEEKDAY_SUNDAY)
      AppendDay(strReturn, 19155); // So
    else if (bLongMultiDaysFormat)
      AppendDay(strReturn, 0);
  }
  return strReturn;
}

std::string CPVRTimerInfoTag::GetWeekdaysString() const
{
  CSingleLock lock(m_critSection);
  return GetWeekdaysString(m_iWeekdays, m_timerType ? m_timerType->IsEpgBased() : false, false);
}

bool CPVRTimerInfoTag::AddToClient(void) const
{
  const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  if (client)
    return client->AddTimer(*this) == PVR_ERROR_NO_ERROR;
  return false;
}

TimerOperationResult CPVRTimerInfoTag::DeleteFromClient(bool bForce /* = false */) const
{
  const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  PVR_ERROR error = PVR_ERROR_UNKNOWN;

  if (client)
    error = client->DeleteTimer(*this, bForce);

  if (error == PVR_ERROR_RECORDING_RUNNING)
    return TimerOperationResult::RECORDING;

  return (error == PVR_ERROR_NO_ERROR) ? TimerOperationResult::OK : TimerOperationResult::FAILED;
}

bool CPVRTimerInfoTag::RenameOnClient(const std::string &strNewName)
{
  {
    // set the new timer title locally
    CSingleLock lock(m_critSection);
    m_strTitle = strNewName;
  }

  // update timer data in the backend
  return UpdateOnClient();
}

bool CPVRTimerInfoTag::UpdateEntry(const CPVRTimerInfoTagPtr &tag)
{
  CSingleLock lock(m_critSection);

  m_iClientId           = tag->m_iClientId;
  m_iClientIndex        = tag->m_iClientIndex;
  m_iParentClientIndex  = tag->m_iParentClientIndex;
  m_strTitle            = tag->m_strTitle;
  m_strEpgSearchString  = tag->m_strEpgSearchString;
  m_bFullTextEpgSearch  = tag->m_bFullTextEpgSearch;
  m_strDirectory        = tag->m_strDirectory;
  m_iClientChannelUid   = tag->m_iClientChannelUid;
  m_StartTime           = tag->m_StartTime;
  m_StopTime            = tag->m_StopTime;
  m_bStartAnyTime       = tag->m_bStartAnyTime;
  m_bEndAnyTime         = tag->m_bEndAnyTime;
  m_FirstDay            = tag->m_FirstDay;
  m_iPriority           = tag->m_iPriority;
  m_iLifetime           = tag->m_iLifetime;
  m_iMaxRecordings      = tag->m_iMaxRecordings;
  m_state               = tag->m_state;
  m_iPreventDupEpisodes = tag->m_iPreventDupEpisodes;
  m_iRecordingGroup     = tag->m_iRecordingGroup;
  m_iWeekdays           = tag->m_iWeekdays;
  m_bIsRadio            = tag->m_bIsRadio;
  m_iMarginStart        = tag->m_iMarginStart;
  m_iMarginEnd          = tag->m_iMarginEnd;
  m_strSeriesLink       = tag->m_strSeriesLink;
  m_iEpgUid             = tag->m_iEpgUid;
  m_epgTag              = tag->m_epgTag;
  m_strSummary          = tag->m_strSummary;
  m_channel             = tag->m_channel;

  SetTimerType(tag->m_timerType);

  if (m_strSummary.empty())
    UpdateSummary();

  UpdateEpgInfoTag();

  return true;
}

bool CPVRTimerInfoTag::UpdateChildState(const CPVRTimerInfoTagPtr &childTimer)
{
  if (!childTimer || childTimer->m_iParentClientIndex != m_iClientIndex)
    return false;

  switch (childTimer->m_state)
  {
  case PVR_TIMER_STATE_NEW:
  case PVR_TIMER_STATE_SCHEDULED:
  case PVR_TIMER_STATE_CONFLICT_OK:
    m_iActiveChildTimers++;
    break;
  case PVR_TIMER_STATE_RECORDING:
    m_iActiveChildTimers++;
    m_bHasChildRecording = true;
    break;
  case PVR_TIMER_STATE_CONFLICT_NOK:
    m_bHasChildConflictNOK = true;
    break;
  case PVR_TIMER_STATE_ERROR:
    m_bHasChildErrors = true;
    break;
  case PVR_TIMER_STATE_COMPLETED:
  case PVR_TIMER_STATE_ABORTED:
  case PVR_TIMER_STATE_CANCELLED:
  case PVR_TIMER_STATE_DISABLED:
    //these are not the child timers we are looking for
    break;
  }
  return true;
}

void CPVRTimerInfoTag::ResetChildState()
{
  m_iActiveChildTimers = 0;
  m_bHasChildConflictNOK = false;
  m_bHasChildRecording = false;
  m_bHasChildErrors = false;
}

bool CPVRTimerInfoTag::UpdateOnClient()
{
  const CPVRClientPtr client = CServiceBroker::GetPVRManager().GetClient(m_iClientId);
  return client && (client->UpdateTimer(*this) == PVR_ERROR_NO_ERROR);
}

std::string CPVRTimerInfoTag::ChannelName() const
{
  std::string strReturn;
  CPVRChannelPtr channeltag = Channel();
  if (channeltag)
    strReturn = channeltag->ChannelName();
  else if (m_timerType && m_timerType->IsEpgBasedTimerRule())
    strReturn = StringUtils::Format("(%s)", g_localizeStrings.Get(809).c_str()); // "Any channel"

  return strReturn;
}

std::string CPVRTimerInfoTag::ChannelIcon() const
{
  std::string strReturn;
  CPVRChannelPtr channeltag = Channel();
  if (channeltag)
    strReturn = channeltag->IconPath();
  return strReturn;
}

static const time_t INSTANT_TIMER_START = 0; // PVR addon API: special start time value to denote an instant timer

CPVRTimerInfoTagPtr CPVRTimerInfoTag::CreateInstantTimerTag(const CPVRChannelPtr &channel, int iDuration /* = DEFAULT_PVRRECORD_INSTANTRECORDTIME */)
{
  if (!channel)
  {
    CLog::LogF(LOGERROR, "No channel");
    return CPVRTimerInfoTagPtr();
  }

  CPVREpgInfoTagPtr epgTag(channel->GetEPGNow());
  CPVRTimerInfoTagPtr newTimer;
  if (epgTag)
  {
    if (epgTag->IsRecordable())
    {
      newTimer = CreateFromEpg(epgTag);
    }
    else
    {
      CLog::LogF(LOGERROR, "EPG tag is not recordable");
      return CPVRTimerInfoTagPtr();
    }
  }

  if (!newTimer)
  {
    newTimer.reset(new CPVRTimerInfoTag);

    newTimer->m_iClientIndex       = PVR_TIMER_NO_CLIENT_INDEX;
    newTimer->m_iParentClientIndex = PVR_TIMER_NO_PARENT;
    newTimer->m_channel            = channel;
    newTimer->m_strTitle           = channel->ChannelName();
    newTimer->m_iClientChannelUid  = channel->UniqueID();
    newTimer->m_iClientId          = channel->ClientID();
    newTimer->m_bIsRadio           = channel->IsRadio();

    // timertype: manual one-shot timer for given client
    CPVRTimerTypePtr timerType(CPVRTimerType::CreateFromAttributes(
      PVR_TIMER_TYPE_IS_MANUAL, PVR_TIMER_TYPE_IS_REPEATING | PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES, channel->ClientID()));
    if (!timerType)
    {
      CLog::LogF(LOGERROR, "Unable to create one shot manual timer type");
      return CPVRTimerInfoTagPtr();
    }

    newTimer->SetTimerType(timerType);

    if (epgTag)
      newTimer->SetEpgTag(epgTag);
    else
      newTimer->UpdateEpgInfoTag();
  }

  /* no matter the timer was created from an epg tag, overwrite timer start and end times. */
  CDateTime now(CDateTime::GetUTCDateTime());
  newTimer->SetStartFromUTC(now);

  if (iDuration == DEFAULT_PVRRECORD_INSTANTRECORDTIME)
    iDuration = CServiceBroker::GetSettings().GetInt(CSettings::SETTING_PVRRECORD_INSTANTRECORDTIME);

  CDateTime endTime = now + CDateTimeSpan(0, 0, iDuration ? iDuration : 120, 0);
  newTimer->SetEndFromUTC(endTime);

  /* update summary string according to instant recording start/end time */
  newTimer->UpdateSummary();
  newTimer->m_strSummary = StringUtils::Format(g_localizeStrings.Get(19093).c_str(), newTimer->Summary().c_str());

  CDateTime startTime(INSTANT_TIMER_START);
  newTimer->SetStartFromUTC(startTime);
  newTimer->m_iMarginStart = 0;
  newTimer->m_iMarginEnd = 0;

  /* unused only for reference */
  newTimer->m_strFileNameAndPath = CPVRTimersPath::PATH_NEW;

  return newTimer;
}

CPVRTimerInfoTagPtr CPVRTimerInfoTag::CreateFromEpg(const CPVREpgInfoTagPtr &tag, bool bCreateRule /* = false */)
{
  /* create a new timer */
  CPVRTimerInfoTagPtr newTag(new CPVRTimerInfoTag());

  /* check if a valid channel is set */
  CPVRChannelPtr channel = tag->Channel();
  if (!channel)
  {
    CLog::LogF(LOGERROR, "EPG tag has no channel");
    return CPVRTimerInfoTagPtr();
  }

  /* set the timer data */
  CDateTime newStart = tag->StartAsUTC();
  CDateTime newEnd = tag->EndAsUTC();
  newTag->m_iClientIndex       = PVR_TIMER_NO_CLIENT_INDEX;
  newTag->m_iParentClientIndex = PVR_TIMER_NO_PARENT;
  newTag->m_strTitle           = tag->Title().empty() ? channel->ChannelName() : tag->Title();
  newTag->m_iClientChannelUid  = channel->UniqueID();
  newTag->m_iClientId          = channel->ClientID();
  newTag->m_bIsRadio           = channel->IsRadio();
  newTag->m_channel            = channel;
  newTag->m_strSeriesLink      = tag->SeriesLink();
  newTag->m_iEpgUid            = tag->UniqueBroadcastID();
  newTag->SetStartFromUTC(newStart);
  newTag->SetEndFromUTC(newEnd);

  const int iMustNotHaveAttribs = PVR_TIMER_TYPE_IS_MANUAL |
                                  PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES |
                                  PVR_TIMER_TYPE_FORBIDS_EPG_TAG_ON_CREATE;
  CPVRTimerTypePtr timerType;
  if (bCreateRule)
  {
    // create epg-based timer rule, prefer rule using series link if available.

    const int iMustHaveAttribs = PVR_TIMER_TYPE_IS_REPEATING;

    if (!tag->SeriesLink().empty())
      timerType = CPVRTimerType::CreateFromAttributes(iMustHaveAttribs | PVR_TIMER_TYPE_REQUIRES_EPG_SERIESLINK_ON_CREATE,
                                                      iMustNotHaveAttribs,
                                                      channel->ClientID());
    if (!timerType)
      timerType = CPVRTimerType::CreateFromAttributes(iMustHaveAttribs,
                                                      iMustNotHaveAttribs | PVR_TIMER_TYPE_REQUIRES_EPG_SERIESLINK_ON_CREATE,
                                                      channel->ClientID());
    if (timerType)
    {
      if (timerType->SupportsEpgTitleMatch())
        newTag->m_strEpgSearchString = newTag->m_strTitle;

      if (timerType->SupportsWeekdays())
        newTag->m_iWeekdays = PVR_WEEKDAY_ALLDAYS;

      if (timerType->SupportsStartAnyTime())
        newTag->m_bStartAnyTime = true;

      if (timerType->SupportsEndAnyTime())
        newTag->m_bEndAnyTime = true;
    }
  }
  else
  {
    // create one-shot epg-based timer
    timerType = CPVRTimerType::CreateFromAttributes(PVR_TIMER_TYPE_ATTRIBUTE_NONE,
                                                    PVR_TIMER_TYPE_IS_REPEATING | iMustNotHaveAttribs,
                                                    channel->ClientID());
  }

  if (!timerType)
  {
    CLog::LogF(LOGERROR, "Unable to create any epg-based timer type");
    return CPVRTimerInfoTagPtr();
  }

  newTag->SetTimerType(timerType);
  newTag->UpdateSummary();
  newTag->SetEpgTag(tag);

  /* unused only for reference */
  newTag->m_strFileNameAndPath = CPVRTimersPath::PATH_NEW;

  return newTag;
}

CDateTime CPVRTimerInfoTag::StartAsUTC(void) const
{
  CDateTime retVal = m_StartTime;
  return retVal;
}

CDateTime CPVRTimerInfoTag::StartAsLocalTime(void) const
{
  CDateTime retVal;
  retVal.SetFromUTCDateTime(m_StartTime);
  return retVal;
}

CDateTime CPVRTimerInfoTag::EndAsUTC(void) const
{
  CDateTime retVal = m_StopTime;
  return retVal;
}

CDateTime CPVRTimerInfoTag::EndAsLocalTime(void) const
{
  CDateTime retVal;
  retVal.SetFromUTCDateTime(m_StopTime);
  return retVal;
}

CDateTime CPVRTimerInfoTag::FirstDayAsUTC(void) const
{
  CDateTime retVal = m_FirstDay;
  return retVal;
}

CDateTime CPVRTimerInfoTag::FirstDayAsLocalTime(void) const
{
  CDateTime retVal;
  retVal.SetFromUTCDateTime(m_FirstDay);
  return retVal;
}

void CPVRTimerInfoTag::GetNotificationText(std::string &strText) const
{
  CSingleLock lock(m_critSection);

  int stringID = 0;

  switch (m_state)
  {
  case PVR_TIMER_STATE_ABORTED:
  case PVR_TIMER_STATE_CANCELLED:
      stringID = 19224; // Recording aborted
    break;
  case PVR_TIMER_STATE_SCHEDULED:
    if (IsTimerRule())
      stringID = 19058; // Timer enabled
    else
      stringID = 19225; // Recording scheduled
    break;
  case PVR_TIMER_STATE_RECORDING:
    stringID = 19226; // Recording started
    break;
  case PVR_TIMER_STATE_COMPLETED:
    stringID = 19227; // Recording completed
    break;
  case PVR_TIMER_STATE_CONFLICT_OK:
  case PVR_TIMER_STATE_CONFLICT_NOK:
    stringID = 19277; // Recording conflict
    break;
  case PVR_TIMER_STATE_ERROR:
    stringID = 19278; // Recording error
    break;
  case PVR_TIMER_STATE_DISABLED:
    stringID = 19057; // Timer disabled
    break;
  default:
    break;
  }
  if (stringID != 0)
    strText = StringUtils::Format("%s: '%s'", g_localizeStrings.Get(stringID).c_str(), m_strTitle.c_str());
}

std::string CPVRTimerInfoTag::GetDeletedNotificationText() const
{
  CSingleLock lock(m_critSection);

  int stringID = 0;
  // The state in this case is the state the timer had when it was last seen
  switch (m_state)
  {
  case PVR_TIMER_STATE_RECORDING:
    stringID = 19227; // Recording completed
    break;
  case PVR_TIMER_STATE_SCHEDULED:
  default:
    if (IsTimerRule())
      stringID = 828; // Timer rule deleted
    else
      stringID = 19228; // Timer deleted
  }

  return StringUtils::Format("%s: '%s'", g_localizeStrings.Get(stringID).c_str(), m_strTitle.c_str());
}

CPVREpgInfoTagPtr CPVRTimerInfoTag::GetEpgInfoTag(bool bCreate /* = true */) const
{
  if (!m_epgTag && bCreate)
  {
    CPVRChannelPtr channel(m_channel);
    if (!channel)
    {
      channel = CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bIsRadio)->GetGroupAll()->GetByUniqueID(m_iClientChannelUid, m_iClientId);

      CSingleLock lock(m_critSection);
      m_channel = channel;
    }

    if (channel)
    {
      const CPVREpgPtr epg(channel->GetEPG());
      if (epg)
      {
        CSingleLock lock(m_critSection);
        if (!m_epgTag)
        {
          if (m_iEpgUid != EPG_TAG_INVALID_UID)
          {
            m_epgTag = epg->GetTagByBroadcastId(m_iEpgUid);
          }
          else
          {
            time_t startTime = 0;
            time_t endTime = 0;

            StartAsUTC().GetAsTime(startTime);
            if (startTime > 0)
              EndAsUTC().GetAsTime(endTime);

            if (startTime > 0 && endTime > 0)
            {
              // if no epg uid present, try to find a tag according to timer's start/end time
              CPVREpgInfoTagPtr epgTag = epg->GetTagBetween(StartAsUTC() - CDateTimeSpan(0, 0, 2, 0), EndAsUTC() + CDateTimeSpan(0, 0, 2, 0));
              if (epgTag)
              {
                bool bTagMatches = !IsTimerRule();
                if (!bTagMatches)
                {
                  // Check whether the tag actually is an event that belongs to a child of this timer rule
                  const CPVRTimerInfoTagPtr timer = epgTag->Timer();
                  if (timer && (timer->GetTimerRuleId() == m_iClientIndex))
                  {
                    bTagMatches = true;
                  }
                }

                if (bTagMatches)
                {
                  m_epgTag = epgTag;
                  m_iEpgUid = m_epgTag->UniqueBroadcastID();
                }
              }
            }
          }
        }
      }
    }
  }
  return m_epgTag;
}

void CPVRTimerInfoTag::SetEpgTag(const CPVREpgInfoTagPtr &tag)
{
  CPVREpgInfoTagPtr previousTag;
  {
    CSingleLock lock(m_critSection);
    previousTag = m_epgTag;
    m_epgTag = tag;
  }

  if (previousTag)
    previousTag->ClearTimer();
}

void CPVRTimerInfoTag::ClearEpgTag(void)
{
  SetEpgTag(CPVREpgInfoTagPtr());
}

bool CPVRTimerInfoTag::HasChannel() const
{
  return m_channel.get() != nullptr;
}

CPVRChannelPtr CPVRTimerInfoTag::Channel() const
{
  return m_channel;
}

CPVRChannelPtr CPVRTimerInfoTag::UpdateChannel(void)
{
  const CPVRChannelPtr channel(CServiceBroker::GetPVRManager().ChannelGroups()->Get(m_bIsRadio)->GetGroupAll()->GetByUniqueID(m_iClientChannelUid, m_iClientId));

  CSingleLock lock(m_critSection);
  m_channel = channel;
  return m_channel;
}

const std::string& CPVRTimerInfoTag::Title(void) const
{
  return m_strTitle;
}

const std::string& CPVRTimerInfoTag::Summary(void) const
{
  return m_strSummary;
}

const std::string& CPVRTimerInfoTag::Path(void) const
{
  return m_strFileNameAndPath;
}

const std::string& CPVRTimerInfoTag::SeriesLink() const
{
  return m_strSeriesLink;
}
