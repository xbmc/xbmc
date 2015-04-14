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
#define SETTING_TMR_CHNAME_TV           "timer.tvchannelname"
#define SETTING_TMR_DAY                 "timer.day"
#define SETTING_TMR_BEGIN               "timer.begin"
#define SETTING_TMR_END                 "timer.end"
#define SETTING_TMR_PRIORITY            "timer.priority"
#define SETTING_TMR_LIFETIME            "timer.lifetime"
#define SETTING_TMR_FIRST_DAY           "timer.firstday"
#define SETTING_TMR_NAME                "timer.name"
#define SETTING_TMR_DIR                 "timer.directory"
#define SETTING_TMR_RADIO               "timer.radio"
#define SETTING_TMR_CHNAME_RADIO        "timer.radiochannelname"

CGUIDialogPVRTimerSettings::CGUIDialogPVRTimerSettings(void)
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_PVR_TIMER_SETTING, "DialogPVRTimerSettings.xml"),
    m_bIsRadio(false),
    m_bIsRepeating(false),
    m_bSupportsFolders(false),
    m_iWeekdays(0),
    m_iPriority(0),
    m_iLifetime(0),
    m_tmp_iFirstDay(0),
    m_tmp_day(11),
    m_bTimerActive(false),
    m_selectedChannelEntry(0)
{
  m_loadType = LOAD_EVERY_TIME;
}

void CGUIDialogPVRTimerSettings::SetTimer(CFileItem *item)
{
  m_InfoTag = item->GetPVRTimerInfoTag();

  // Copy over data from tag. We must not change the tag until Save()
  // because it is not a copy, but the (one and only) original.

  m_bIsRadio         = m_InfoTag->m_bIsRadio;
  m_bIsRepeating     = m_InfoTag->m_bIsRepeating;
  m_iWeekdays        = m_InfoTag->m_iWeekdays;
  m_iPriority        = m_InfoTag->m_iPriority;
  m_iLifetime        = m_InfoTag->m_iLifetime;
  m_strTitle         = m_InfoTag->m_strTitle;
  m_strDirectory     = m_InfoTag->m_strDirectory;

  m_FirstDay         = m_InfoTag->FirstDayAsLocalTime();
  m_StartTime        = m_InfoTag->StartAsLocalTime();
  m_EndTime          = m_InfoTag->EndAsLocalTime();

  m_bTimerActive     = m_InfoTag->IsActive();
  m_bSupportsFolders = m_InfoTag->SupportsFolders();

  m_Channel          = m_InfoTag->ChannelTag();

  m_StartTime.GetAsSystemTime(m_timerStartTime);
  m_EndTime.GetAsSystemTime(m_timerEndTime);
  m_timerStartTimeStr = m_StartTime.GetAsLocalizedTime("", false);
  m_timerEndTimeStr   = m_EndTime.GetAsLocalizedTime("", false);

  m_selectedChannelEntry = 0;
  m_channelEntries.clear();

  m_tmp_iFirstDay     = 0;
  m_tmp_day           = 11;
}

void CGUIDialogPVRTimerSettings::SetChannelFromSelectedEntry(int iSelectedEntry, bool bRadio)
{
  const std::map<std::pair<bool, int>, int>::const_iterator itc(m_channelEntries.find(std::make_pair(bRadio, iSelectedEntry)));
  if (itc != m_channelEntries.end())
    m_Channel = g_PVRChannelGroups->GetChannelById(itc->second);
}

void CGUIDialogPVRTimerSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_TMR_ACTIVE)
    m_bTimerActive = static_cast<const CSettingBool*>(setting)->GetValue();
  if (settingId == SETTING_TMR_RADIO || settingId == SETTING_TMR_CHNAME_TV || settingId == SETTING_TMR_CHNAME_RADIO)
  {
    if (settingId == SETTING_TMR_RADIO)
    {
      m_bIsRadio = static_cast<const CSettingBool*>(setting)->GetValue();
      m_selectedChannelEntry = 0;
    }
    else
    {
      m_selectedChannelEntry = static_cast<const CSettingInt*>(setting)->GetValue();
    }
    SetChannelFromSelectedEntry(m_selectedChannelEntry, m_bIsRadio);
  }
  else if (settingId == SETTING_TMR_DAY)
  {
    m_tmp_day = static_cast<const CSettingInt*>(setting)->GetValue();

    if (m_tmp_day <= 10)
      SetTimerFromWeekdaySetting();
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

      m_StartTime = newStart;
      m_EndTime   = newEnd;

      m_bIsRepeating = false;
      m_iWeekdays    = 0;
    }
  }
  else if (settingId == SETTING_TMR_PRIORITY)
    m_iPriority = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_LIFETIME)
    m_iLifetime = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_FIRST_DAY)
  {
    m_tmp_iFirstDay = static_cast<const CSettingInt*>(setting)->GetValue();

    CDateTime newFirstDay;
    if (m_tmp_iFirstDay > 0)
      newFirstDay = CDateTime::GetCurrentDateTime() + CDateTimeSpan(m_tmp_iFirstDay - 1, 0, 0, 0);

    m_FirstDay = newFirstDay;
  }
  else if (settingId == SETTING_TMR_NAME)
    m_strTitle = static_cast<const CSettingString*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_DIR)
    m_strDirectory = static_cast<const CSettingString*>(setting)->GetValue();
}

void CGUIDialogPVRTimerSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_TMR_BEGIN)
  {
    if (CGUIDialogNumeric::ShowAndGetTime(m_timerStartTime, g_localizeStrings.Get(14066)))
    {
      CDateTime timestart = m_timerStartTime;
      int start_day       = m_StartTime.GetDay();
      int start_month     = m_StartTime.GetMonth();
      int start_year      = m_StartTime.GetYear();
      int start_hour      = timestart.GetHour();
      int start_minute    = timestart.GetMinute();
      CDateTime newStart(start_year, start_month, start_day, start_hour, start_minute, 0);
      m_StartTime = newStart;

      m_timerStartTimeStr = m_StartTime.GetAsLocalizedTime("", false);
      setButtonLabels();
    }
  }
  else if (settingId == SETTING_TMR_END)
  {
    if (CGUIDialogNumeric::ShowAndGetTime(m_timerEndTime, g_localizeStrings.Get(14066)))
    {
      CDateTime timestop = m_timerEndTime;
      // TODO: add separate end date control to schedule a show with more then 24 hours
      int start_day       = m_StartTime.GetDay();
      int start_month     = m_StartTime.GetMonth();
      int start_year      = m_StartTime.GetYear();
      int start_hour      = timestop.GetHour();
      int start_minute    = timestop.GetMinute();
      CDateTime newEnd(start_year, start_month, start_day, start_hour, start_minute, 0);
      
      // add a day to end time if end time is before start time
      // TODO: this should be removed after separate end date control was added
      if (newEnd < m_StartTime)
        newEnd += CDateTimeSpan(1, 0, 0, 0);

      m_EndTime = newEnd;

      m_timerEndTimeStr = m_EndTime.GetAsLocalizedTime("", false);
      setButtonLabels();
    }
  }
}

void CGUIDialogPVRTimerSettings::Save()
{
  m_InfoTag->m_bIsRadio     = m_bIsRadio;
  m_InfoTag->m_bIsRepeating = m_bIsRepeating;
  m_InfoTag->m_iWeekdays    = m_iWeekdays;
  m_InfoTag->m_iPriority    = m_iPriority;
  m_InfoTag->m_iLifetime    = m_iLifetime;

  m_InfoTag->SetFirstDayFromLocalTime(m_FirstDay);
  m_InfoTag->SetStartFromLocalTime(m_StartTime);
  m_InfoTag->SetEndFromLocalTime(m_EndTime);

  m_InfoTag->SetChannel(m_Channel);

  // Set the timer's title to the channel name if it's 'New Timer' or empty
  if (m_strTitle == g_localizeStrings.Get(19056) || m_strTitle.empty())
    m_strTitle = m_Channel->ChannelName();

  m_InfoTag->m_strTitle     = m_strTitle;
  m_InfoTag->m_strDirectory = m_strDirectory;

  m_InfoTag->m_state = m_bTimerActive ? PVR_TIMER_STATE_SCHEDULED : PVR_TIMER_STATE_CANCELLED;

  m_InfoTag->UpdateSummary();
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

  AddToggle(group, SETTING_TMR_ACTIVE, 19074, 0, m_bTimerActive);
  AddEdit(group, SETTING_TMR_NAME, 19075, 0, m_strTitle, false, false, 19097);

  if (m_bSupportsFolders)
    AddEdit(group, SETTING_TMR_DIR, 19076, 0, m_strDirectory, true, false, 19104);

  AddToggle(group, SETTING_TMR_RADIO, 19077, 0, m_bIsRadio);

  /// Channel names
  {
    // For TV
    AddChannelNames(group, false);

    // For Radio
    AddChannelNames(group, true);
  }

  /// Day
  {
    // get diffence of timer in days between today and timer start date
    tm time_cur; CDateTime::GetCurrentDateTime().GetAsTm(time_cur);
    tm time_tmr; m_StartTime.GetAsTm(time_tmr);

    m_tmp_day += time_tmr.tm_yday - time_cur.tm_yday;
    if (time_tmr.tm_yday - time_cur.tm_yday < 0)
      m_tmp_day += 365;

    SetWeekdaySettingFromTimer();

    AddSpinner(group, SETTING_TMR_DAY, 19079, 0, m_tmp_day, DaysOptionsFiller);
  }

  AddButton(group, SETTING_TMR_BEGIN, 19080, 0);
  AddButton(group, SETTING_TMR_END, 19081, 0);
  AddSpinner(group, SETTING_TMR_PRIORITY, 19082, 0, m_iPriority, 0, 1, 99);
  AddSpinner(group, SETTING_TMR_LIFETIME, 19083, 0, m_iLifetime, 0, 1, 365);

  /// First day
  {
    CDateTime time = CDateTime::GetCurrentDateTime();
    CDateTime timestart = m_FirstDay;

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
  if (m_Channel)
  {
    int timerChannelID = m_Channel->ChannelID();

    for (std::vector< std::pair<std::string, int> >::const_iterator option = options.begin(); option != options.end(); ++option)
    {
      std::map<std::pair<bool, int>, int>::const_iterator channelEntry = m_channelEntries.find(std::make_pair(bRadio, option->second));
      if (channelEntry != m_channelEntries.end() && channelEntry->second == timerChannelID)
      {
        m_selectedChannelEntry = option->second;
        SetChannelFromSelectedEntry(m_selectedChannelEntry, bRadio);
        break;
      }
    }
  }
  else
  {
    // New timer -> tag does not yet contain channel data
    m_selectedChannelEntry = 0;
    SetChannelFromSelectedEntry(m_selectedChannelEntry, bRadio);
  }

  CSettingInt *setting = AddSpinner(group, bRadio ? SETTING_TMR_CHNAME_RADIO : SETTING_TMR_CHNAME_TV, 19078, 0, m_selectedChannelEntry, ChannelNamesOptionsFiller);
  if (setting == NULL)
    return NULL;

  // define an enable dependency with bIsRadio
  CSettingDependency depdendencyIsRadio(SettingDependencyTypeEnable, m_settingsManager);
  depdendencyIsRadio.And()
    ->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_RADIO, "true", SettingDependencyOperatorEquals, !bRadio, m_settingsManager)));
  SettingDependencies deps;
  deps.push_back(depdendencyIsRadio);
  setting->SetDependencies(deps);

  return setting;
}

void CGUIDialogPVRTimerSettings::SetWeekdaySettingFromTimer()
{
  if (m_bIsRepeating)
  {
    if (m_iWeekdays == 0x01)
      m_tmp_day = 0;
    else if (m_iWeekdays == 0x02)
      m_tmp_day = 1;
    else if (m_iWeekdays == 0x04)
      m_tmp_day = 2;
    else if (m_iWeekdays == 0x08)
      m_tmp_day = 3;
    else if (m_iWeekdays == 0x10)
      m_tmp_day = 4;
    else if (m_iWeekdays == 0x20)
      m_tmp_day = 5;
    else if (m_iWeekdays == 0x40)
      m_tmp_day = 6;
    else if (m_iWeekdays == 0x1F)
      m_tmp_day = 7;
    else if (m_iWeekdays == 0x3F)
      m_tmp_day = 8;
    else if (m_iWeekdays == 0x7F)
      m_tmp_day = 9;
    else if (m_iWeekdays == 0x60)
      m_tmp_day = 10;
  }
}

void CGUIDialogPVRTimerSettings::SetTimerFromWeekdaySetting()
{
  m_bIsRepeating = true;

  if (m_tmp_day == 0)
    m_iWeekdays = 0x01;
  else if (m_tmp_day == 1)
    m_iWeekdays = 0x02;
  else if (m_tmp_day == 2)
    m_iWeekdays = 0x04;
  else if (m_tmp_day == 3)
    m_iWeekdays = 0x08;
  else if (m_tmp_day == 4)
    m_iWeekdays = 0x10;
  else if (m_tmp_day == 5)
    m_iWeekdays = 0x20;
  else if (m_tmp_day == 6)
    m_iWeekdays = 0x40;
  else if (m_tmp_day == 7)
    m_iWeekdays = 0x1F;
  else if (m_tmp_day == 8)
    m_iWeekdays = 0x3F;
  else if (m_tmp_day == 9)
    m_iWeekdays = 0x7F;
  else if (m_tmp_day == 10)
    m_iWeekdays = 0x60;
  else
    m_iWeekdays = 0;
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

  dialog->getChannelNames(setting->GetId() == SETTING_TMR_CHNAME_RADIO, list, current, false);
}

void CGUIDialogPVRTimerSettings::DaysOptionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  if (setting == NULL || data == NULL)
    return;

  CGUIDialogPVRTimerSettings *dialog = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (dialog == NULL)
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
