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
#include "pvr/channels/PVRChannelGroupInternal.h"
#include "epg/EpgContainer.h"
#include "pvr/addons/PVRClients.h"

#include "epg/Epg.h"

using namespace PVR;
using namespace EPG;

CPVRTimerInfoTag::CPVRTimerInfoTag(bool bRadio /* = false */) :
  m_strTitle(g_localizeStrings.Get(19056)), // New Timer
  m_strDirectory("/")
{
  m_iClientId          = g_PVRClients->GetFirstConnectedClientID();
  m_iClientIndex       = -1;
  m_iClientChannelUid  = PVR_INVALID_CHANNEL_UID;
  m_iPriority          = CSettings::Get().GetInt("pvrrecord.defaultpriority");
  m_iLifetime          = CSettings::Get().GetInt("pvrrecord.defaultlifetime");
  m_bIsRepeating       = false;
  m_iWeekdays          = 0;
  m_iChannelNumber     = 0;
  m_bIsRadio           = bRadio;
  CEpgInfoTagPtr emptyTag;
  m_epgTag             = emptyTag;
  m_iMarginStart       = CSettings::Get().GetInt("pvrrecord.marginstart");
  m_iMarginEnd         = CSettings::Get().GetInt("pvrrecord.marginend");
  m_iGenreType         = 0;
  m_iGenreSubType      = 0;
  m_StartTime          = CDateTime::GetUTCDateTime();
  m_StopTime           = m_StartTime;
  m_state              = PVR_TIMER_STATE_SCHEDULED;
  m_FirstDay.SetValid(false);
  m_iTimerId           = 0;
}

CPVRTimerInfoTag::CPVRTimerInfoTag(const PVR_TIMER &timer, const CPVRChannelPtr &channel, unsigned int iClientId) :
  m_strTitle(timer.strTitle),
  m_strDirectory(timer.strDirectory)
{
  m_iClientId          = iClientId;
  m_iClientIndex       = timer.iClientIndex;
  m_iClientChannelUid  = channel ? channel->UniqueID() : timer.iClientChannelUid;
  m_iChannelNumber     = channel ? g_PVRChannelGroups->GetGroupAll(channel->IsRadio())->GetChannelNumber(channel) : 0;
  m_StartTime          = timer.startTime + g_advancedSettings.m_iPVRTimeCorrection;
  m_StopTime           = timer.endTime + g_advancedSettings.m_iPVRTimeCorrection;
  m_bIsRepeating       = timer.bIsRepeating;
  m_FirstDay           = timer.firstDay + g_advancedSettings.m_iPVRTimeCorrection;
  m_iWeekdays          = timer.iWeekdays;
  m_iPriority          = timer.iPriority;
  m_iLifetime          = timer.iLifetime;
  m_iMarginStart       = timer.iMarginStart;
  m_iMarginEnd         = timer.iMarginEnd;
  m_genre              = StringUtils::Split(CEpg::ConvertGenreIdToString(timer.iGenreType, timer.iGenreSubType), g_advancedSettings.m_videoItemSeparator);
  m_iGenreType         = timer.iGenreType;
  m_iGenreSubType      = timer.iGenreSubType;
  CEpgInfoTagPtr emptyTag;
  m_epgTag             = emptyTag;
  m_channel            = channel;
  m_bIsRadio           = channel && channel->IsRadio();
  m_state              = timer.state;
  m_strFileNameAndPath = StringUtils::Format("pvr://client%i/timers/%i", m_iClientId, m_iClientIndex);
  m_iTimerId           = 0;

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
          m_iClientIndex       == right.m_iClientIndex &&
          m_strSummary         == right.m_strSummary &&
          m_iClientChannelUid  == right.m_iClientChannelUid &&
          m_bIsRadio           == right.m_bIsRadio &&
          m_bIsRepeating       == right.m_bIsRepeating &&
          m_StartTime          == right.m_StartTime &&
          m_StopTime           == right.m_StopTime &&
          m_FirstDay           == right.m_FirstDay &&
          m_iWeekdays          == right.m_iWeekdays &&
          m_iPriority          == right.m_iPriority &&
          m_iLifetime          == right.m_iLifetime &&
          m_strFileNameAndPath == right.m_strFileNameAndPath &&
          m_strTitle           == right.m_strTitle &&
          m_strDirectory       == right.m_strDirectory &&
          m_iClientId          == right.m_iClientId &&
          m_iMarginStart       == right.m_iMarginStart &&
          m_iMarginEnd         == right.m_iMarginEnd &&
          m_state              == right.m_state &&
          m_iTimerId           == right.m_iTimerId);
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
  value["repeating"] = m_bIsRepeating;
  value["starttime"] = m_StartTime.IsValid() ? m_StartTime.GetAsDBDateTime() : "";
  value["endtime"] = m_StopTime.IsValid() ? m_StopTime.GetAsDBDateTime() : "";
  value["runtime"] = m_StartTime.IsValid() && m_StopTime.IsValid() ? (m_StopTime - m_StartTime).GetSecondsTotal() : 0;
  value["firstday"] = m_FirstDay.IsValid() ? m_FirstDay.GetAsDBDate() : "";

  CVariant weekdays(CVariant::VariantTypeArray);
  if (m_iWeekdays & 0x01)
    weekdays.push_back("monday");
  if (m_iWeekdays & 0x02)
    weekdays.push_back("tuesday");
  if (m_iWeekdays & 0x04)
    weekdays.push_back("wednesday");
  if (m_iWeekdays & 0x08)
    weekdays.push_back("thursday");
  if (m_iWeekdays & 0x10)
    weekdays.push_back("friday");
  if (m_iWeekdays & 0x20)
    weekdays.push_back("saturday");
  if (m_iWeekdays & 0x40)
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
  default:
    value["state"] = "unknown";
    break;
  }
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

  if (!m_bIsRepeating || !m_iWeekdays)
  {
    m_strSummary = StringUtils::Format("%s %s %s %s %s",
        StartAsLocalTime().GetAsLocalizedDate().c_str(),
        g_localizeStrings.Get(19159).c_str(),
        StartAsLocalTime().GetAsLocalizedTime("", false).c_str(),
        g_localizeStrings.Get(19160).c_str(),
        EndAsLocalTime().GetAsLocalizedTime("", false).c_str());
  }
  else if (m_FirstDay.IsValid())
  {
    m_strSummary = StringUtils::Format("%s-%s-%s-%s-%s-%s-%s %s %s %s %s %s %s",
        m_iWeekdays & 0x01 ? g_localizeStrings.Get(19149).c_str() : "__",
        m_iWeekdays & 0x02 ? g_localizeStrings.Get(19150).c_str() : "__",
        m_iWeekdays & 0x04 ? g_localizeStrings.Get(19151).c_str() : "__",
        m_iWeekdays & 0x08 ? g_localizeStrings.Get(19152).c_str() : "__",
        m_iWeekdays & 0x10 ? g_localizeStrings.Get(19153).c_str() : "__",
        m_iWeekdays & 0x20 ? g_localizeStrings.Get(19154).c_str() : "__",
        m_iWeekdays & 0x40 ? g_localizeStrings.Get(19155).c_str() : "__",
        g_localizeStrings.Get(19156).c_str(),
        FirstDayAsLocalTime().GetAsLocalizedDate(false).c_str(),
        g_localizeStrings.Get(19159).c_str(),
        StartAsLocalTime().GetAsLocalizedTime("", false).c_str(),
        g_localizeStrings.Get(19160).c_str(),
        EndAsLocalTime().GetAsLocalizedTime("", false).c_str());
  }
  else
  {
    m_strSummary = StringUtils::Format("%s-%s-%s-%s-%s-%s-%s %s %s %s %s",
        m_iWeekdays & 0x01 ? g_localizeStrings.Get(19149).c_str() : "__",
        m_iWeekdays & 0x02 ? g_localizeStrings.Get(19150).c_str() : "__",
        m_iWeekdays & 0x04 ? g_localizeStrings.Get(19151).c_str() : "__",
        m_iWeekdays & 0x08 ? g_localizeStrings.Get(19152).c_str() : "__",
        m_iWeekdays & 0x10 ? g_localizeStrings.Get(19153).c_str() : "__",
        m_iWeekdays & 0x20 ? g_localizeStrings.Get(19154).c_str() : "__",
        m_iWeekdays & 0x40 ? g_localizeStrings.Get(19155).c_str() : "__",
        g_localizeStrings.Get(19159).c_str(),
        StartAsLocalTime().GetAsLocalizedTime("", false).c_str(),
        g_localizeStrings.Get(19160).c_str(),
        EndAsLocalTime().GetAsLocalizedTime("", false).c_str());
  }
}

/**
 * Get the status string of this Timer, is used by the GUIInfoManager
 */
std::string CPVRTimerInfoTag::GetStatus() const
{
  std::string strReturn = g_localizeStrings.Get(305);
  CSingleLock lock(m_critSection);
  if (URIUtils::PathEquals(m_strFileNameAndPath, "pvr://timers/addtimer/"))
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

  return strReturn;
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

bool CPVRTimerInfoTag::DeleteFromClient(bool bForce /* = false */) const
{
  PVR_ERROR error = g_PVRClients->DeleteTimer(*this, bForce);
  if (error == PVR_ERROR_RECORDING_RUNNING)
  {
    // recording running. ask the user if it should be deleted anyway
    if (!CGUIDialogYesNo::ShowAndGetInput(122,0,19122,0))
      return false;

    error = g_PVRClients->DeleteTimer(*this, true);
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

  m_iClientId         = tag->m_iClientId;
  m_iClientIndex      = tag->m_iClientIndex;
  m_strTitle          = tag->m_strTitle;
  m_strDirectory      = tag->m_strDirectory;
  m_iClientChannelUid = tag->m_iClientChannelUid;
  m_StartTime         = tag->m_StartTime;
  m_StopTime          = tag->m_StopTime;
  m_FirstDay          = tag->m_FirstDay;
  m_iPriority         = tag->m_iPriority;
  m_iLifetime         = tag->m_iLifetime;
  m_state             = tag->m_state;
  m_bIsRepeating      = tag->m_bIsRepeating;
  m_iWeekdays         = tag->m_iWeekdays;
  m_iChannelNumber    = tag->m_iChannelNumber;
  m_bIsRadio          = tag->m_bIsRadio;
  m_iMarginStart      = tag->m_iMarginStart;
  m_iMarginEnd        = tag->m_iMarginEnd;
  m_epgTag            = tag->m_epgTag;
  m_genre             = tag->m_genre;
  m_iGenreType        = tag->m_iGenreType;
  m_iGenreSubType     = tag->m_iGenreSubType;
  m_strSummary        = tag->m_strSummary;

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
    CGUIDialogOK::ShowAndGetInput(19033,19111,19110,0); /* print info dialog "Server error!" */
  else if (err == PVR_ERROR_REJECTED)
    CGUIDialogOK::ShowAndGetInput(19033,19109,19110,0); /* print info dialog "Couldn't delete timer!" */
  else if (err == PVR_ERROR_ALREADY_PRESENT)
    CGUIDialogOK::ShowAndGetInput(19033,19109,0,19067); /* print info dialog */
  else
    CGUIDialogOK::ShowAndGetInput(19033,19147,19110,0); /* print info dialog "Unknown error!" */
}

void CPVRTimerInfoTag::SetEpgInfoTag(CEpgInfoTagPtr &tag)
{
  CSingleLock lock(m_critSection);
  if (tag && *m_epgTag != *tag)
    CLog::Log(LOGINFO, "cPVRTimerInfoTag: timer %s set to epg event %s", m_strTitle.c_str(), tag->Title().c_str());
  else if (!tag && m_epgTag)
    CLog::Log(LOGINFO, "cPVRTimerInfoTag: timer %s set to no epg event", m_strTitle.c_str());
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

CPVRTimerInfoTagPtr CPVRTimerInfoTag::CreateFromEpg(const CEpgInfoTagPtr &tag)
{
  /* create a new timer */
  CPVRTimerInfoTagPtr newTag(new CPVRTimerInfoTag());
  if (!newTag)
  {
    CLog::Log(LOGERROR, "%s - couldn't create new timer", __FUNCTION__);
    return CPVRTimerInfoTagPtr();
  }

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
  newTag->m_iClientIndex      = -1;
  newTag->m_strTitle          = tag->Title().empty() ? channel->ChannelName() : tag->Title();
  newTag->m_iChannelNumber    = channel->ChannelNumber();
  newTag->m_iClientChannelUid = channel->UniqueID();
  newTag->m_iClientId         = channel->ClientID();
  newTag->m_bIsRadio          = channel->IsRadio();
  newTag->m_iGenreType        = tag->GenreType();
  newTag->m_iGenreSubType     = tag->GenreSubType();
  newTag->m_channel           = channel;
  newTag->SetStartFromUTC(newStart);
  newTag->SetEndFromUTC(newEnd);

  if (tag->Plot().empty())
  {
    newTag->m_strSummary= StringUtils::Format("%s %s %s %s %s",
                                              newTag->StartAsLocalTime().GetAsLocalizedDate().c_str(),
                                              g_localizeStrings.Get(19159).c_str(),
                                              newTag->StartAsLocalTime().GetAsLocalizedTime("", false).c_str(),
                                              g_localizeStrings.Get(19160).c_str(),
                                              newTag->EndAsLocalTime().GetAsLocalizedTime("", false).c_str());
  }
  else
  {
    newTag->m_strSummary = tag->Plot();
  }

  newTag->m_epgTag = g_EpgContainer.GetById(tag->EpgID())->GetTag(tag->StartAsUTC());

  /* unused only for reference */
  newTag->m_strFileNameAndPath = "pvr://timers/new";

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
  switch (m_state)
  {
  case PVR_TIMER_STATE_ABORTED:
  case PVR_TIMER_STATE_CANCELLED:
      strText = StringUtils::Format("%s: '%s'", g_localizeStrings.Get(19224).c_str(), m_strTitle.c_str());
    break;
  case PVR_TIMER_STATE_SCHEDULED:
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
  default:
    break;
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

bool CPVRTimerInfoTag::SupportsFolders() const
{
  return g_PVRClients->SupportsRecordingFolders(m_iClientId);
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
