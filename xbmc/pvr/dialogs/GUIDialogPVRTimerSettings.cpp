/*
 *      Copyright (C) 2012-2014 Team XBMC
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

#include "GUIDialogPVRTimerSettings.h"
#include "FileItem.h"
#include "dialogs/GUIDialogNumeric.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace PVR;

#define SETTING_TMR_ACTIVE              "timer.active"
#define SETTING_TMR_CHNAME              "timer.channelname"
#define SETTING_TMR_DAY                 "timer.day"
#define SETTING_TMR_BEGIN               "timer.begin"
#define SETTING_TMR_END                 "timer.end"
#define SETTING_TMR_PRIORITY            "timer.priority"
#define SETTING_TMR_LIFETIME            "timer.lifetime"
#define SETTING_TMR_FIRST_DAY           "timer.firstday"
#define SETTING_TMR_NAME                "timer.name"
#define SETTING_TMR_DIR                 "timer.directory"

CGUIDialogPVRTimerSettings::CGUIDialogPVRTimerSettings(void)
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_PVR_TIMER_SETTING, "DialogPVRTimerSettings.xml"),
    m_tmp_iFirstDay(0),
    m_tmp_day(11),
    m_bTimerActive(false),
    m_selectedChannelEntry(0),
    m_timerItem(NULL)
{
  m_loadType = LOAD_EVERY_TIME;
}

void CGUIDialogPVRTimerSettings::SetTimer(CFileItem *item)
{
  m_timerItem         = item;

  m_timerItem->GetPVRTimerInfoTag()->StartAsLocalTime().GetAsSystemTime(m_timerStartTime);
  m_timerItem->GetPVRTimerInfoTag()->EndAsLocalTime().GetAsSystemTime(m_timerEndTime);
  m_timerStartTimeStr   = m_timerItem->GetPVRTimerInfoTag()->StartAsLocalTime().GetAsLocalizedTime("", false);
  m_timerEndTimeStr     = m_timerItem->GetPVRTimerInfoTag()->EndAsLocalTime().GetAsLocalizedTime("", false);

  m_tmp_iFirstDay     = 0;
  m_tmp_day           = 11;
}

void CGUIDialogPVRTimerSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  CPVRTimerInfoTagPtr tag = m_timerItem->GetPVRTimerInfoTag();
  if (tag == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_TMR_ACTIVE)
    m_bTimerActive = static_cast<const CSettingBool*>(setting)->GetValue();
  if (settingId == SETTING_TMR_CHNAME)
    m_selectedChannelEntry = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_DAY)
  {
    m_tmp_day = static_cast<const CSettingInt*>(setting)->GetValue();

    if (m_tmp_day <= 10)
      SetTimerFromWeekdaySetting(*tag);
    else
    {
      CDateTime time = CDateTime::GetCurrentDateTime();
      CDateTime timestart = m_timerStartTime;
      CDateTime timestop = m_timerEndTime;
      int m_tmp_diff;

      // get diffence of timer in days between today and timer start date
      tm time_cur; time.GetAsTm(time_cur);
      tm time_tmr; timestart.GetAsTm(time_tmr);

      m_tmp_diff = time_tmr.tm_yday - time_cur.tm_yday;
      if (time_tmr.tm_yday - time_cur.tm_yday < 0)
        m_tmp_diff = 365;

      CDateTime newStart = timestart + CDateTimeSpan(m_tmp_day - 11 - m_tmp_diff, 0, 0, 0);
      CDateTime newEnd = timestop + CDateTimeSpan(m_tmp_day - 11 - m_tmp_diff, 0, 0, 0);

      // add a day to end time if end time is before start time
      // TODO: this should be removed after separate end date control was added
      if (newEnd < newStart)
        newEnd += CDateTimeSpan(1, 0, 0, 0);

      tag->SetStartFromLocalTime(newStart);
      tag->SetEndFromLocalTime(newEnd);

      tag->m_bIsRepeating = false;
      tag->m_iWeekdays = 0;
    }
  }
  else if (settingId == SETTING_TMR_PRIORITY)
    tag->m_iPriority = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_LIFETIME)
    tag->m_iLifetime = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_FIRST_DAY)
  {
    m_tmp_iFirstDay = static_cast<const CSettingInt*>(setting)->GetValue();

    CDateTime newFirstDay;
    if (m_tmp_iFirstDay > 0)
      newFirstDay = CDateTime::GetCurrentDateTime() + CDateTimeSpan(m_tmp_iFirstDay - 1, 0, 0, 0);

    tag->SetFirstDayFromLocalTime(newFirstDay);
  }
  else if (settingId == SETTING_TMR_NAME)
    tag->m_strTitle = static_cast<const CSettingString*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_DIR)
    tag->m_strDirectory = static_cast<const CSettingString*>(setting)->GetValue();

  tag->UpdateSummary();
}

void CGUIDialogPVRTimerSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  CPVRTimerInfoTagPtr tag = m_timerItem->GetPVRTimerInfoTag();
  if (tag == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_TMR_BEGIN)
  {
    if (CGUIDialogNumeric::ShowAndGetTime(m_timerStartTime, g_localizeStrings.Get(14066)))
    {
      CDateTime timestart = m_timerStartTime;
      int start_day       = tag->StartAsLocalTime().GetDay();
      int start_month     = tag->StartAsLocalTime().GetMonth();
      int start_year      = tag->StartAsLocalTime().GetYear();
      int start_hour      = timestart.GetHour();
      int start_minute    = timestart.GetMinute();
      CDateTime newStart(start_year, start_month, start_day, start_hour, start_minute, 0);
      tag->SetStartFromLocalTime(newStart);

      m_timerStartTimeStr = tag->StartAsLocalTime().GetAsLocalizedTime("", false);
      setButtonLabels();
    }
  }
  else if (settingId == SETTING_TMR_END)
  {
    if (CGUIDialogNumeric::ShowAndGetTime(m_timerEndTime, g_localizeStrings.Get(14066)))
    {
      CDateTime timestop = m_timerEndTime;
      // TODO: add separate end date control to schedule a show with more then 24 hours
      int start_day       = tag->StartAsLocalTime().GetDay();
      int start_month     = tag->StartAsLocalTime().GetMonth();
      int start_year      = tag->StartAsLocalTime().GetYear();
      int start_hour      = timestop.GetHour();
      int start_minute    = timestop.GetMinute();
      CDateTime newEnd(start_year, start_month, start_day, start_hour, start_minute, 0);

      // add a day to end time if end time is before start time
      // TODO: this should be removed after separate end date control was added
      if (newEnd < tag->StartAsLocalTime())
        newEnd += CDateTimeSpan(1, 0, 0, 0);

      tag->SetEndFromLocalTime(newEnd);

      m_timerEndTimeStr = tag->EndAsLocalTime().GetAsLocalizedTime("", false);
      setButtonLabels();
    }
  }

  tag->UpdateSummary();
}

void CGUIDialogPVRTimerSettings::Save()
{
  CPVRTimerInfoTagPtr tag = m_timerItem->GetPVRTimerInfoTag();

  // Set the timer's channel
  std::map<std::pair<bool, int>, int>::iterator itc = m_channelEntries.find(std::make_pair(tag->m_bIsRadio, m_selectedChannelEntry));
  if (itc != m_channelEntries.end())
  {
    CPVRChannelPtr channel =  g_PVRChannelGroups->GetChannelById(itc->second);
    if (channel)
    {
      tag->m_iClientChannelUid = channel->UniqueID();
      tag->m_iClientId         = channel->ClientID();
      tag->m_bIsRadio          = channel->IsRadio();
      tag->m_iChannelNumber    = channel->ChannelNumber();

      // Update channel pointer from above values
      tag->UpdateChannel();
    }
  }

  // Set the timer's title to the channel name if it's 'New Timer' or empty
  if (tag->m_strTitle == g_localizeStrings.Get(19056) || tag->m_strTitle.empty())
  {
    CPVRChannelPtr channel = g_PVRChannelGroups->GetByUniqueID(tag->m_iClientChannelUid, tag->m_iClientId);
    if (channel)
      tag->m_strTitle = channel->ChannelName();
  }

  if (m_bTimerActive)
    tag->m_state = PVR_TIMER_STATE_SCHEDULED;
  else
    tag->m_state = PVR_TIMER_STATE_CANCELLED;
}

void CGUIDialogPVRTimerSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  setButtonLabels();
}

void CGUIDialogPVRTimerSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("pvrtimersettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings: unable to setup settings");
    return;
  }

  CSettingGroup *group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings: unable to setup settings");
    return;
  }

  // add a condition
  m_settingsManager->AddCondition("IsTimerDayRepeating", IsTimerDayRepeating);

  CPVRTimerInfoTagPtr tag = m_timerItem->GetPVRTimerInfoTag();

  m_selectedChannelEntry = 0;
  m_channelEntries.clear();
  m_bTimerActive = tag->IsActive();

  AddToggle(group, SETTING_TMR_ACTIVE, 19074, 0, m_bTimerActive);
  AddEdit(group, SETTING_TMR_NAME, 19075, 0, tag->m_strTitle, false, false, 19097);

  if (tag->SupportsFolders())
    AddEdit(group, SETTING_TMR_DIR, 19076, 0, tag->m_strDirectory, true, false, 19104);

  /// Channel names
  {
    AddChannelNames(group, tag->m_bIsRadio);
  }

  /// Day
  {
    // get diffence of timer in days between today and timer start date
    tm time_cur; CDateTime::GetCurrentDateTime().GetAsTm(time_cur);
    tm time_tmr; tag->StartAsLocalTime().GetAsTm(time_tmr);

    m_tmp_day += time_tmr.tm_yday - time_cur.tm_yday;
    if (time_tmr.tm_yday - time_cur.tm_yday < 0)
      m_tmp_day += 365;

    SetWeekdaySettingFromTimer(*tag);

    AddSpinner(group, SETTING_TMR_DAY, 19079, 0, m_tmp_day, DaysOptionsFiller);
  }

  AddButton(group, SETTING_TMR_BEGIN, 19080, 0);
  AddButton(group, SETTING_TMR_END, 19081, 0);
  AddSpinner(group, SETTING_TMR_PRIORITY, 19082, 0, tag->m_iPriority, 0, 1, 99);
  AddSpinner(group, SETTING_TMR_LIFETIME, 19083, 0, tag->m_iLifetime, 0, 1, 365);

  /// First day
  {
    CDateTime time = CDateTime::GetCurrentDateTime();
    CDateTime timestart = tag->FirstDayAsLocalTime();

    // get diffence of timer in days between today and timer start date
    if (time < timestart)
    {
      tm time_cur; time.GetAsTm(time_cur);
      tm time_tmr; timestart.GetAsTm(time_tmr);

      m_tmp_iFirstDay += time_tmr.tm_yday - time_cur.tm_yday + 1;
      if (time_tmr.tm_yday - time_cur.tm_yday < 0)
        m_tmp_iFirstDay += 365;
    }

    CSettingInt *settingFirstDay = AddSpinner(group, SETTING_TMR_FIRST_DAY, 19084, 0, m_tmp_iFirstDay, DaysOptionsFiller);

    // define an enable dependency with m_tmp_day <= 10
    CSettingDependency depdendencyFirstDay(SettingDependencyTypeEnable, m_settingsManager);
    depdendencyFirstDay.And()
      ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition("IsTimerDayRepeating", "true", SETTING_TMR_DAY, false, m_settingsManager)));
    SettingDependencies deps;
    deps.push_back(depdendencyFirstDay);
    settingFirstDay->SetDependencies(deps);
  }
}

CSetting* CGUIDialogPVRTimerSettings::AddChannelNames(CSettingGroup *group, bool bRadio)
{
  std::vector< std::pair<std::string, int> > options;
  getChannelNames(bRadio, options, m_selectedChannelEntry, true);

  // select the correct channel
  int timerChannelID = 0;
  if (m_timerItem->GetPVRTimerInfoTag()->ChannelTag())
    timerChannelID = m_timerItem->GetPVRTimerInfoTag()->ChannelTag()->ChannelID();

  for (std::vector< std::pair<std::string, int> >::const_iterator option = options.begin(); option != options.end(); ++option)
  {
    std::map<std::pair<bool, int>, int>::const_iterator channelEntry = m_channelEntries.find(std::make_pair(bRadio, option->second));
    if (channelEntry != m_channelEntries.end() && channelEntry->second == timerChannelID)
    {
      m_selectedChannelEntry = option->second;
      break;
    }
  }

  CSettingInt *setting = AddSpinner(group, SETTING_TMR_CHNAME, 19078, 0, m_selectedChannelEntry, ChannelNamesOptionsFiller);
  if (setting == NULL)
    return NULL;

  return setting;
}

void CGUIDialogPVRTimerSettings::SetWeekdaySettingFromTimer(const CPVRTimerInfoTag &timer)
{
  if (timer.m_bIsRepeating)
  {
    if (timer.m_iWeekdays == 0x01)
      m_tmp_day = 0;
    else if (timer.m_iWeekdays == 0x02)
      m_tmp_day = 1;
    else if (timer.m_iWeekdays == 0x04)
      m_tmp_day = 2;
    else if (timer.m_iWeekdays == 0x08)
      m_tmp_day = 3;
    else if (timer.m_iWeekdays == 0x10)
      m_tmp_day = 4;
    else if (timer.m_iWeekdays == 0x20)
      m_tmp_day = 5;
    else if (timer.m_iWeekdays == 0x40)
      m_tmp_day = 6;
    else if (timer.m_iWeekdays == 0x1F)
      m_tmp_day = 7;
    else if (timer.m_iWeekdays == 0x3F)
      m_tmp_day = 8;
    else if (timer.m_iWeekdays == 0x7F)
      m_tmp_day = 9;
    else if (timer.m_iWeekdays == 0x60)
      m_tmp_day = 10;
  }
}

void CGUIDialogPVRTimerSettings::SetTimerFromWeekdaySetting(CPVRTimerInfoTag &timer)
{
  timer.m_bIsRepeating = true;

  if (m_tmp_day == 0)
    timer.m_iWeekdays = 0x01;
  else if (m_tmp_day == 1)
    timer.m_iWeekdays = 0x02;
  else if (m_tmp_day == 2)
    timer.m_iWeekdays = 0x04;
  else if (m_tmp_day == 3)
    timer.m_iWeekdays = 0x08;
  else if (m_tmp_day == 4)
    timer.m_iWeekdays = 0x10;
  else if (m_tmp_day == 5)
    timer.m_iWeekdays = 0x20;
  else if (m_tmp_day == 6)
    timer.m_iWeekdays = 0x40;
  else if (m_tmp_day == 7)
    timer.m_iWeekdays = 0x1F;
  else if (m_tmp_day == 8)
    timer.m_iWeekdays = 0x3F;
  else if (m_tmp_day == 9)
    timer.m_iWeekdays = 0x7F;
  else if (m_tmp_day == 10)
    timer.m_iWeekdays = 0x60;
  else
    timer.m_iWeekdays = 0;
}

void CGUIDialogPVRTimerSettings::getChannelNames(bool bRadio, std::vector< std::pair<std::string, int> > &list, int &current, bool updateChannelEntries /* = false */)
{
  CFileItemList channelsList;
  g_PVRChannelGroups->GetGroupAll(bRadio)->GetMembers(channelsList);

  int entry = 0;

  for (int i = 0; i < channelsList.Size(); i++)
  {
    const CPVRChannelPtr channel(channelsList[i]->GetPVRChannelInfoTag());

    list.push_back(std::make_pair(StringUtils::Format("%i %s", channel->ChannelNumber(), channel->ChannelName().c_str()), entry));
    if (updateChannelEntries)
      m_channelEntries.insert(std::make_pair(std::make_pair(bRadio, entry), channel->ChannelID()));
    ++entry;
  }
}

void CGUIDialogPVRTimerSettings::setButtonLabels()
{
  // timer start time
  BaseSettingControlPtr settingControl = GetSettingControl(SETTING_TMR_BEGIN);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_LABEL2(settingControl->GetID(), m_timerStartTimeStr);

  // timer end time
  settingControl = GetSettingControl(SETTING_TMR_END);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_LABEL2(settingControl->GetID(), m_timerEndTimeStr);
}

 bool CGUIDialogPVRTimerSettings::IsTimerDayRepeating(const std::string &condition, const std::string &value, const CSetting *setting)
 {
   if (setting == NULL || setting->GetType() != SettingTypeInteger)
     return false;

   bool result = static_cast<const CSettingInt*>(setting)->GetValue() <= 10;
   return result == StringUtils::EqualsNoCase(value, "true");
 }

void CGUIDialogPVRTimerSettings::ChannelNamesOptionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  if (data == NULL)
    return;

  CGUIDialogPVRTimerSettings *dialog = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (dialog == NULL)
    return;

  const CPVRTimerInfoTagPtr tag = dialog->m_timerItem->GetPVRTimerInfoTag();
  if (tag == NULL)
    return;

  dialog->getChannelNames(tag->m_bIsRadio, list, current, false);
}

void CGUIDialogPVRTimerSettings::DaysOptionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  if (setting == NULL || data == NULL)
    return;

  CGUIDialogPVRTimerSettings *dialog = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (dialog == NULL)
    return;

  const CPVRTimerInfoTagPtr tag = dialog->m_timerItem->GetPVRTimerInfoTag();
  if (tag == NULL)
    return;

  if (setting->GetId() == SETTING_TMR_DAY)
  {
    for (unsigned int iDayPtr = 19086; iDayPtr <= 19096; iDayPtr++)
      list.push_back(std::make_pair(g_localizeStrings.Get(iDayPtr), list.size()));
  }
  else if (setting->GetId() == SETTING_TMR_FIRST_DAY)
    list.push_back(std::make_pair(g_localizeStrings.Get(19030), 0));

  CDateTime time = CDateTime::GetCurrentDateTime();
  for (int i = 1; i < 365; ++i)
  {
    list.push_back(std::make_pair(time.GetAsLocalizedDate(), list.size()));
    time += CDateTimeSpan(1, 0, 0, 0);
  }
}
