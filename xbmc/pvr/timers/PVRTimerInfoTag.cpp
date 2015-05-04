/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include "PVRTimers.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "epg/EpgContainer.h"
#include "pvr/addons/PVRClients.h"

#include "epg/Epg.h"

using namespace PVR;
using namespace EPG;

CPVRTimerInfoTag::CPVRTimerInfoTag(bool bRadio /* = false */) :
  m_strTitle(g_localizeStrings.Get(19056)), // New Timer
  m_bFullTextEpgSearch(false)
{
  m_iClientId           = g_PVRClients->GetFirstConnectedClientID();
  m_iClientIndex        = -1;
  m_iParentClientIndex  = 0;
  m_iClientChannelUid   = PVR_INVALID_CHANNEL_UID;
  m_iPriority           = CSettings::Get().GetInt("pvrrecord.defaultpriority");
  m_iLifetime           = CSettings::Get().GetInt("pvrrecord.defaultlifetime");
  m_iPreventDupEpisodes = CSettings::Get().GetInt("pvrrecord.preventduplicateepisodes");
  m_iRecordingGroup     = 0;
  m_iChannelNumber      = 0;
  m_bIsRadio            = bRadio;
  m_iMarginStart        = CSettings::Get().GetInt("pvrrecord.marginstart");
  m_iMarginEnd          = CSettings::Get().GetInt("pvrrecord.marginend");
  m_iGenreType          = 0;
  m_iGenreSubType       = 0;
  m_StartTime           = CDateTime::GetUTCDateTime();
  m_StopTime            = m_StartTime;
  m_state               = PVR_TIMER_STATE_NEW;
  m_FirstDay.SetValid(false);
  m_iTimerId            = 0;

  if (g_PVRClients->SupportsTimers(m_iClientId))
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
      CLog::Log(LOGERROR, "%s: no timer type, although timers are supported by client %d!", __FUNCTION__, m_iClientId);
  }

  m_iWeekdays = (m_timerType && m_timerType->IsRepeating()) ? PVR_WEEKDAY_ALLDAYS : PVR_WEEKDAY_NONE;
}

CPVRTimerInfoTag::CPVRTimerInfoTag(const PVR_TIMER &timer, const CPVRChannelPtr &channel, unsigned int iClientId) :
  m_strTitle(timer.strTitle),
  m_strEpgSearchString(timer.strEpgSearchString),
  m_bFullTextEpgSearch(timer.bFullTextEpgSearch),
  m_strDirectory(timer.strDirectory)
{
  m_iClientId           = iClientId;
  m_iClientIndex        = timer.iClientIndex;
  m_iParentClientIndex  = timer.iParentClientIndex;
  m_iClientChannelUid   = channel ? channel->UniqueID() : (timer.iClientChannelUid > 0) ? timer.iClientChannelUid : PVR_INVALID_CHANNEL_UID;
  m_iChannelNumber      = channel ? g_PVRChannelGroups->GetGroupAll(channel->IsRadio())->GetChannelNumber(channel) : 0;
  m_StartTime           = timer.startTime + g_advancedSettings.m_iPVRTimeCorrection;
  m_StopTime            = timer.endTime + g_advancedSettings.m_iPVRTimeCorrection;
  m_iPreventDupEpisodes = timer.iPreventDuplicateEpisodes;
  m_iRecordingGroup     = timer.iRecordingGroup;
  m_FirstDay            = timer.firstDay + g_advancedSettings.m_iPVRTimeCorrection;
  m_iWeekdays           = timer.iWeekdays;
  m_iPriority           = timer.iPriority;
  m_iLifetime           = timer.iLifetime;
  m_iMarginStart        = timer.iMarginStart;
  m_iMarginEnd          = timer.iMarginEnd;
  m_genre               = StringUtils::Split(CEpg::ConvertGenreIdToString(timer.iGenreType, timer.iGenreSubType), g_advancedSettings.m_videoItemSeparator);
  m_iGenreType          = timer.iGenreType;
  m_iGenreSubType       = timer.iGenreSubType;
  m_channel             = channel;
  m_bIsRadio            = channel && channel->IsRadio();
  m_state               = timer.state;
  m_strFileNameAndPath  = StringUtils::Format("pvr://client%i/timers/%i", m_iClientId, m_iClientIndex);
  m_iTimerId            = 0;

  if (g_PVRClients->SupportsTimers(m_iClientId))
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

      if (timer.iWeekdays != PVR_WEEKDAY_NONE)
        iMustHave |= PVR_TIMER_TYPE_IS_REPEATING;
      else
        iMustNotHave |= PVR_TIMER_TYPE_IS_REPEATING;

      if (timer.iEpgUid == 0)
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
      CLog::Log(LOGERROR, "%s: no timer type, although timers are supported by client %d!", __FUNCTION__, m_iClientId);
  }

  UpdateSummary();
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
          m_iRecordingGroup     == m_iRecordingGroup &&
          m_StartTime           == right.m_StartTime &&
          m_StopTime            == right.m_StopTime &&
          m_FirstDay            == right.m_FirstDay &&
          m_iWeekdays           == right.m_iWeekdays &&
          m_iPriority           == right.m_iPriority &&
          m_iLifetime           == right.m_iLifetime &&
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
          m_iTimerId            == right.m_iTimerId);
}

CPVRTimerInfoTag::~CPVRTimerInfoTag(void)
{
  ClearEpgTag();
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

  if (m_timerType)
  {
    if (m_timerType->IsManual())
    {
      if (m_timerType->IsRepeating())
        value["type"] = "manual_repeating";
      else
        value["type"] = "manual_once";
    }
    else
    {
      if (m_timerType->IsRepeating())
        value["type"] = "epg_repeating";
      else
        value["type"] = "epg_once";
    }
  }

  value["epgsearchstring"]   = m_strEpgSearchString;
  value["fulltextepgsearch"] = m_bFullTextEpgSearch;
  value["recordinggroup"]    = m_iRecordingGroup;
}

int CPVRTimerInfoTag::Compare(const CPVRTimerInfoTag &timer) const
{
  CSingleLock lock(m_critSection);
  int iTimerDelta = 0;
  if (StartAsUTC() != timer.StartAsUTC())
  {
    CDateTimeSpan timerDelta = StartAsUTC() - timer.StartAsUTC();
    iTimerDelta = (timerDelta.GetSeconds() + timerDelta.GetMinutes() * 60 + timerDelta.GetHours() * 3600 + timerDelta.GetDays() * 86400);
  }

  /* if the start times are equal, compare the priority of the timers */
  return iTimerDelta == 0 ?
    timer.m_iPriority - m_iPriority :
    iTimerDelta;
}

void CPVRTimerInfoTag::UpdateSummary(void)
{
  CSingleLock lock(m_critSection);
  m_strSummary.clear();

  const std::string startDate(StartAsLocalTime().GetAsLocalizedDate());
  const std::string endDate(EndAsLocalTime().GetAsLocalizedDate());

  if ((m_iWeekdays != PVR_WEEKDAY_NONE) || (startDate == endDate))
  {
    m_strSummary = StringUtils::Format("%s %s %s %s %s",
        m_iWeekdays != PVR_WEEKDAY_NONE ?
          GetWeekdaysString().c_str() : startDate.c_str(),
        g_localizeStrings.Get(19159).c_str(),
        IsStartAtAnyTime() ?
          g_localizeStrings.Get(19161).c_str() : StartAsLocalTime().GetAsLocalizedTime("", false).c_str(),
        g_localizeStrings.Get(19160).c_str(),
        IsEndAtAnyTime() ?
          g_localizeStrings.Get(19161).c_str() : EndAsLocalTime().GetAsLocalizedTime("", false).c_str());
  }
  else
  {
    m_strSummary = StringUtils::Format("%s %s %s %s %s %s",
        startDate.c_str(),
        g_localizeStrings.Get(19159).c_str(),
        IsStartAtAnyTime() ?
          g_localizeStrings.Get(19161).c_str() : StartAsLocalTime().GetAsLocalizedTime("", false).c_str(),
        g_localizeStrings.Get(19160).c_str(),
        endDate.c_str(),
        IsEndAtAnyTime() ?
          g_localizeStrings.Get(19161).c_str() : EndAsLocalTime().GetAsLocalizedTime("", false).c_str());
  }
}

void CPVRTimerInfoTag::SetTimerType(const CPVRTimerTypePtr &type)
{
  CSingleLock lock(m_critSection);
  m_timerType = type;

  if (m_timerType && (m_state == PVR_TIMER_STATE_NEW))
  {
    m_iPriority           = m_timerType->GetPriorityDefault();
    m_iLifetime           = m_timerType->GetLifetimeDefault();
    m_iPreventDupEpisodes = m_timerType->GetPreventDuplicateEpisodesDefault();
  }

  if (m_timerType && !m_timerType->IsRepeating())
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
    strReturn = g_localizeStrings.Get(825);

  return strReturn;
}

/**
 * Get the type string of this timer
 */
std::string CPVRTimerInfoTag::GetTypeAsString() const
{
  CSingleLock lock(m_critSection);
  return m_timerType ? m_timerType->GetDescription() : std::string();
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

// static
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
  PVR_ERROR error = g_PVRClients->AddTimer(*this);
  if (error != PVR_ERROR_NO_ERROR)
  {
    DisplayError(error);
    return false;
  }

  return true;
}

bool CPVRTimerInfoTag::DeleteFromClient(bool bForce /* = false */ , bool bDeleteSchedule /* = false */ ) const
{
  PVR_ERROR error = g_PVRClients->DeleteTimer(*this, bForce, bDeleteSchedule);
  if (error == PVR_ERROR_RECORDING_RUNNING)
  {
    // recording running. ask the user if it should be deleted anyway
    if (!CGUIDialogYesNo::ShowAndGetInput(122, 19122))
      return false;

    error = g_PVRClients->DeleteTimer(*this, true, bDeleteSchedule);
  }

  if (error != PVR_ERROR_NO_ERROR)
  {
    DisplayError(error);
    return false;
  }


  return true;
}

bool CPVRTimerInfoTag::RenameOnClient(const std::string &strNewName)
{
  {
    CSingleLock lock(m_critSection);
    m_strTitle = strNewName;
  }

  PVR_ERROR error = g_PVRClients->RenameTimer(*this, strNewName);
  if (error != PVR_ERROR_NO_ERROR)
  {
    if (error == PVR_ERROR_NOT_IMPLEMENTED)
      return UpdateOnClient();

    DisplayError(error);
    return false;
  }

  return true;
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
  m_FirstDay            = tag->m_FirstDay;
  m_iPriority           = tag->m_iPriority;
  m_iLifetime           = tag->m_iLifetime;
  m_state               = tag->m_state;
  m_iPreventDupEpisodes = tag->m_iPreventDupEpisodes;
  m_iRecordingGroup     = tag->m_iRecordingGroup;
  m_iWeekdays           = tag->m_iWeekdays;
  m_iChannelNumber      = tag->m_iChannelNumber;
  m_bIsRadio            = tag->m_bIsRadio;
  m_iMarginStart        = tag->m_iMarginStart;
  m_iMarginEnd          = tag->m_iMarginEnd;
  m_epgTag              = tag->m_epgTag;
  m_genre               = tag->m_genre;
  m_iGenreType          = tag->m_iGenreType;
  m_iGenreSubType       = tag->m_iGenreSubType;
  m_strSummary          = tag->m_strSummary;

  SetTimerType(tag->m_timerType);

  if (m_strSummary.empty())
    UpdateSummary();

  return true;
}

bool CPVRTimerInfoTag::UpdateOnClient()
{
  PVR_ERROR error = g_PVRClients->UpdateTimer(*this);
  if (error != PVR_ERROR_NO_ERROR)
  {
    DisplayError(error);
    return false;
  }

  return true;
}

void CPVRTimerInfoTag::DisplayError(PVR_ERROR err) const
{
  if (err == PVR_ERROR_SERVER_ERROR)
    CGUIDialogOK::ShowAndGetInput(19033, 19111); /* print info dialog "Server error!" */
  else if (err == PVR_ERROR_REJECTED)
    CGUIDialogOK::ShowAndGetInput(19033, 19109); /* print info dialog "Couldn't save timer!" */
  else if (err == PVR_ERROR_ALREADY_PRESENT)
    CGUIDialogOK::ShowAndGetInput(19033, 19067); /* print info dialog */
  else
    CGUIDialogOK::ShowAndGetInput(19033, 19110); /* print info dialog "Unknown error!" */
}

void CPVRTimerInfoTag::SetEpgInfoTag(CEpgInfoTagPtr &tag)
{
  CSingleLock lock(m_critSection);
  if (tag && *m_epgTag != *tag)
    CLog::Log(LOGINFO, "CPVRTimerInfoTag: timer %s set to epg event %s", m_strTitle.c_str(), tag->Title().c_str());
  else if (!tag && m_epgTag)
    CLog::Log(LOGINFO, "CPVRTimerInfoTag: timer %s set to no epg event", m_strTitle.c_str());
  m_epgTag = tag;
}

int CPVRTimerInfoTag::ChannelNumber() const
{
  CPVRChannelPtr channeltag = ChannelTag();
  return channeltag ? channeltag->ChannelNumber() : 0;
}

std::string CPVRTimerInfoTag::ChannelName() const
{
  std::string strReturn;
  CPVRChannelPtr channeltag = ChannelTag();
  if (channeltag)
    strReturn = channeltag->ChannelName();
  else if (m_timerType && m_timerType->IsRepeatingEpgBased())
    strReturn = StringUtils::Format("(%s)", g_localizeStrings.Get(809).c_str()); // "Any channel"

  return strReturn;
}

std::string CPVRTimerInfoTag::ChannelIcon() const
{
  std::string strReturn;
  CPVRChannelPtr channeltag = ChannelTag();
  if (channeltag)
    strReturn = channeltag->IconPath();
  return strReturn;
}

bool CPVRTimerInfoTag::SetDuration(int iDuration)
{
  CSingleLock lock(m_critSection);
  if (m_StartTime.IsValid())
  {
    m_StopTime = m_StartTime + CDateTimeSpan(0, iDuration / 60, iDuration % 60, 0);
    return true;
  }

  return false;
}

CPVRTimerInfoTagPtr CPVRTimerInfoTag::CreateFromEpg(const CEpgInfoTagPtr &tag, bool bRepeating /* = false */)
{
  /* create a new timer */
  CPVRTimerInfoTagPtr newTag(new CPVRTimerInfoTag());

  /* check if a valid channel is set */
  CPVRChannelPtr channel = tag->ChannelTag();
  if (!channel)
  {
    CLog::Log(LOGERROR, "%s - no channel set", __FUNCTION__);
    return CPVRTimerInfoTagPtr();
  }

  /* check if the epg end date is in the future */
  if (tag->EndAsLocalTime() < CDateTime::GetCurrentDateTime())
  {
    CLog::Log(LOGERROR, "%s - end time is in the past", __FUNCTION__);
    return CPVRTimerInfoTagPtr();
  }

  /* set the timer data */
  CDateTime newStart = tag->StartAsUTC();
  CDateTime newEnd = tag->EndAsUTC();
  newTag->m_iClientIndex       = -1;
  newTag->m_iParentClientIndex = 0;
  newTag->m_strTitle           = tag->Title().empty() ? channel->ChannelName() : tag->Title();
  newTag->m_iChannelNumber     = channel->ChannelNumber();
  newTag->m_iClientChannelUid  = channel->UniqueID();
  newTag->m_iClientId          = channel->ClientID();
  newTag->m_bIsRadio           = channel->IsRadio();
  newTag->m_iGenreType         = tag->GenreType();
  newTag->m_iGenreSubType      = tag->GenreSubType();
  newTag->m_channel            = channel;
  newTag->SetStartFromUTC(newStart);
  newTag->SetEndFromUTC(newEnd);

  CPVRTimerTypePtr timerType;
  if (bRepeating)
  {
    // create repeating epg-based timer
    timerType = CPVRTimerType::CreateFromAttributes(
      PVR_TIMER_TYPE_IS_REPEATING,
      PVR_TIMER_TYPE_IS_MANUAL | PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES, channel->ClientID());
  }
  if (!timerType)
  {
    // create one-shot epg-based timer
    timerType = CPVRTimerType::CreateFromAttributes(
      PVR_TIMER_TYPE_ATTRIBUTE_NONE,
      PVR_TIMER_TYPE_IS_REPEATING | PVR_TIMER_TYPE_IS_MANUAL | PVR_TIMER_TYPE_FORBIDS_NEW_INSTANCES, channel->ClientID());
  }
  if (!timerType)
  {
    CLog::Log(LOGERROR, "%s - unable to create any epg-based timer type", __FUNCTION__);
    return CPVRTimerInfoTagPtr();
  }

  newTag->SetTimerType(timerType);

  if (tag->Plot().empty())
  {
    newTag->UpdateSummary();
  }
  else
  {
    newTag->m_strSummary = tag->Plot();
  }

  newTag->m_epgTag = g_EpgContainer.GetById(tag->EpgID())->GetTag(tag->StartAsUTC());

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

bool CPVRTimerInfoTag::IsStartAtAnyTime(void) const
{
  time_t time = 0;
  CDateTime start(m_StartTime);
  start.GetAsTime(time);
  return time == 0;
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

bool CPVRTimerInfoTag::IsEndAtAnyTime(void) const
{
  time_t time = 0;
  CDateTime stop(m_StopTime);
  stop.GetAsTime(time);
  return time == 0;
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
  switch (m_state)
  {
  case PVR_TIMER_STATE_ABORTED:
  case PVR_TIMER_STATE_CANCELLED:
      strText = StringUtils::Format("%s: '%s'", g_localizeStrings.Get(19224).c_str(), m_strTitle.c_str());
    break;
  case PVR_TIMER_STATE_SCHEDULED:
    if (IsRepeating())
      strText = StringUtils::Format("%s: '%s'", g_localizeStrings.Get(826).c_str(), m_strTitle.c_str());
    else
      strText = StringUtils::Format("%s: '%s'", g_localizeStrings.Get(19225).c_str(), m_strTitle.c_str());
    break;
  case PVR_TIMER_STATE_RECORDING:
    strText = StringUtils::Format("%s: '%s'", g_localizeStrings.Get(19226).c_str(), m_strTitle.c_str());
    break;
  case PVR_TIMER_STATE_COMPLETED:
    strText = StringUtils::Format("%s: '%s'", g_localizeStrings.Get(19227).c_str(), m_strTitle.c_str());
    break;
  case PVR_TIMER_STATE_CONFLICT_OK:
  case PVR_TIMER_STATE_CONFLICT_NOK:
    strText = StringUtils::Format("%s: '%s'", g_localizeStrings.Get(19277).c_str(), m_strTitle.c_str());
    break;
  case PVR_TIMER_STATE_ERROR:
    strText = StringUtils::Format("%s: '%s'", g_localizeStrings.Get(19278).c_str(), m_strTitle.c_str());
    break;
  case PVR_TIMER_STATE_DISABLED:
    strText = StringUtils::Format("%s: '%s'", g_localizeStrings.Get(827).c_str(), m_strTitle.c_str());
    break;
  default:
    break;
  }
}

std::string CPVRTimerInfoTag::GetDeletedNotificationText() const
{
  CSingleLock lock(m_critSection);

  // The state in this case is the state the timer had when it was last seen
  switch (m_state)
  {
  case PVR_TIMER_STATE_RECORDING:
    return StringUtils::Format("%s: '%s'", g_localizeStrings.Get(19227).c_str(), m_strTitle.c_str()); // Recording completed
  case PVR_TIMER_STATE_SCHEDULED:
  default:
    if (IsRepeating())
      return StringUtils::Format("%s: '%s'", g_localizeStrings.Get(828).c_str(), m_strTitle.c_str()); // Repeating timer deleted
    else
      return StringUtils::Format("%s: '%s'", g_localizeStrings.Get(19228).c_str(), m_strTitle.c_str()); // Timer deleted
  }
}

void CPVRTimerInfoTag::QueueNotification(void) const
{
  if (CSettings::Get().GetBool("pvrrecord.timernotifications"))
  {
    std::string strMessage;
    GetNotificationText(strMessage);

    if (!strMessage.empty())
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(19166), strMessage);
  }
}

CEpgInfoTagPtr CPVRTimerInfoTag::GetEpgInfoTag(void) const
{
  return m_epgTag;
}

bool CPVRTimerInfoTag::HasEpgInfoTag(void) const
{
  return m_epgTag != NULL;
}

void CPVRTimerInfoTag::ClearEpgTag(void)
{
  CEpgInfoTagPtr deletedTag;

  {
    CSingleLock lock(m_critSection);
    deletedTag = m_epgTag;

    CEpgInfoTagPtr emptyTag;
    m_epgTag = emptyTag;
  }

  if (deletedTag)
    deletedTag->ClearTimer();
}

CPVRChannelPtr CPVRTimerInfoTag::ChannelTag(void) const
{
  return m_channel;
}

void CPVRTimerInfoTag::UpdateChannel(void)
{
  CSingleLock lock(m_critSection);
  m_channel = g_PVRChannelGroups->Get(m_bIsRadio)->GetGroupAll()->GetByUniqueID(m_iClientChannelUid, m_iClientId);
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
