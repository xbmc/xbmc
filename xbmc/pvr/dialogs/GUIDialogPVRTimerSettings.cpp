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
#include "pvr/addons/PVRClients.h"
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
#define SETTING_TMR_CHNAME_RADIO        "timer.radiochannelname"
#define SETTING_TMR_TYPE                "timer.type"
#define SETTING_TMR_NEW_EPISODES        "timer.newepisodes"
#define SETTING_TMR_BEGIN_PRE           "timer.startmargin"
#define SETTING_TMR_END_POST            "timer.endmargin"

#define OPENREADONLY 20

CGUIDialogPVRTimerSettings::CGUIDialogPVRTimerSettings(void)
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_PVR_TIMER_SETTING, "DialogPVRTimerSettings.xml"),
    m_tmp_iFirstDay(0),
    m_tmp_day(0),
    m_tmp_type(0),
    m_bTimerActive(false),
    m_selectedChannelEntry(0),
    m_bIsNewTimer(true),
    m_bIsManualTimer(true),
    m_timerItem(NULL)
{
  m_loadType = LOAD_EVERY_TIME;
}

void CGUIDialogPVRTimerSettings::SetTimer(CFileItem *item)
{
  m_timerItem = item;

  m_timerItem->GetPVRTimerInfoTag()->StartAsLocalTime().GetAsSystemTime(m_timerStartTime);
  m_timerItem->GetPVRTimerInfoTag()->EndAsLocalTime().GetAsSystemTime(m_timerEndTime);
  m_timerStartTimeStr   = m_timerItem->GetPVRTimerInfoTag()->StartAsLocalTime().GetAsLocalizedTime("", false);
  m_timerEndTimeStr     = m_timerItem->GetPVRTimerInfoTag()->EndAsLocalTime().GetAsLocalizedTime("", false);

  m_tmp_iFirstDay     = 0;
  m_tmp_day           = 0;
  m_tmp_type          = 0;
}

void CGUIDialogPVRTimerSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  CPVRTimerInfoTag* tag = m_timerItem->GetPVRTimerInfoTag();
  if (tag == NULL)
    return;

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_TMR_ACTIVE)
    m_bTimerActive = static_cast<const CSettingBool*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_CHNAME_TV || settingId == SETTING_TMR_CHNAME_RADIO)
  {
    m_selectedChannelEntry = static_cast<const CSettingInt*>(setting)->GetValue();

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
  }
  else if (settingId == SETTING_TMR_DAY)
  {
    m_tmp_day = static_cast<const CSettingInt*>(setting)->GetValue();

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

    CDateTime newStart = timestart + CDateTimeSpan(m_tmp_day - m_tmp_diff, 0, 0, 0);
    CDateTime newEnd = timestop + CDateTimeSpan(m_tmp_day - m_tmp_diff, 0, 0, 0);

    // add a day to end time if end time is before start time
    // TODO: this should be removed after separate end date control was added
    if (newEnd < newStart)
      newEnd += CDateTimeSpan(1, 0, 0, 0);

    tag->SetStartFromLocalTime(newStart);
    tag->SetEndFromLocalTime(newEnd);
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
  else if (settingId == SETTING_TMR_TYPE)
  {
    m_tmp_type = static_cast<const CSettingInt*>(setting)->GetValue();
    SetTimerFromWeekdayAndType(*tag);
  }
  else if (settingId == SETTING_TMR_BEGIN_PRE)
    tag->m_iMarginStart = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_END_POST)
    tag->m_iMarginEnd = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_NEW_EPISODES)
    tag->m_bNewEpisodesOnly = static_cast<const CSettingInt*>(setting)->GetValue();

  tag->UpdateSummary();
}

void CGUIDialogPVRTimerSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
    return;

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  CPVRTimerInfoTag* tag = m_timerItem->GetPVRTimerInfoTag();
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
  CPVRTimerInfoTag* tag = m_timerItem->GetPVRTimerInfoTag();

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

  CPVRTimerInfoTag* tag = m_timerItem->GetPVRTimerInfoTag();

  m_bIsNewTimer = (tag->m_state == PVR_TIMER_STATE_NEW);
  m_bIsManualTimer = (tag->m_type == PVR_TIMER_TYPE_NONE
                    || tag->m_type == PVR_TIMER_TYPE_ONCE_MANUAL
                    || tag->m_type == PVR_TIMER_TYPE_SERIE_MANUAL);

  tag->m_bIsRepeating = !(tag->m_type == PVR_TIMER_TYPE_NONE
                        || tag->m_type == PVR_TIMER_TYPE_ONCE_MANUAL
                        || tag->m_type == PVR_TIMER_TYPE_ONCE_EPG) ;

  m_selectedChannelEntry = 0;
  m_channelEntries.clear();
  m_bTimerActive = tag->IsActive();

  // m_tmp_type = UNSUPPORTED_NUM if we opened an unsupported timer
  // in this case the dialog is opened in a sort of 'read-only' mode (all controls are disabled)
  // this is done with the settings dependencies underneath
  // stop time, end time, title, day and channel are always in 'read-only' mode if this is an epg based timer
  SetTypeFromTimer(*tag);

  /// Timer type
  if (m_bIsManualTimer)
    AddSpinner(group, SETTING_TMR_TYPE, 804, 0, m_tmp_type, TypesManualOptionsFiller);
  else
    AddSpinner(group, SETTING_TMR_TYPE, 804, 0, m_tmp_type, TypesEpgOptionsFiller);

  /// Timer active selection, only for existing timers
  if (!m_bIsNewTimer)
    AddToggle(group, SETTING_TMR_ACTIVE, 19074, 0, m_bTimerActive);

  /// Tile
  CSettingString *settingTitle = AddEdit(group, SETTING_TMR_NAME, 19075, 0, tag->m_strTitle, false, false, 19097);

  // grey out when we open an unsupported timer (SETTING_TMR_TYPE = 20)
  CSettingDependency dependencytitle(SettingDependencyTypeEnable, m_settingsManager);
  dependencytitle.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, "20", SettingDependencyOperatorEquals, (m_tmp_type == OPENREADONLY || m_bIsManualTimer), m_settingsManager)));
  SettingDependencies depsTitle;
  depsTitle.push_back(dependencytitle);
  settingTitle->SetDependencies(depsTitle);

  /// Recording folder
  if (tag->SupportsFolders())
  {
    CSettingString *settingFolder = AddEdit(group, SETTING_TMR_DIR, 19076, 0, tag->m_strDirectory, true, false, 19104);

    // grey out when we open an unsupported timer (SETTING_TMR_TYPE = 20)
    CSettingDependency dependencyfolder(SettingDependencyTypeEnable, m_settingsManager);
    dependencyfolder.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, "20", SettingDependencyOperatorEquals, (m_tmp_type == OPENREADONLY || m_bIsManualTimer) , m_settingsManager)));
    SettingDependencies depsFolder;
    depsFolder.push_back(dependencyfolder);
    settingFolder->SetDependencies(depsFolder);
  }

  /// Channel names
  AddChannelNames(group, tag->m_bIsRadio);

  /// Day
  // get difference of timer in days between today and timer start date
  tm time_cur; CDateTime::GetCurrentDateTime().GetAsTm(time_cur);
  tm time_tmr; tag->StartAsLocalTime().GetAsTm(time_tmr);

  m_tmp_day += time_tmr.tm_yday - time_cur.tm_yday;
  if (time_tmr.tm_yday - time_cur.tm_yday < 0)
    m_tmp_day += 365;

  CSettingInt *settingDay = AddSpinner(group, SETTING_TMR_DAY, 19079, 0, m_tmp_day, DaysOptionsFiller);

  // grey out when we open an unsupported timer (SETTING_TMR_TYPE = 20)
  CSettingDependency dependencyday(SettingDependencyTypeEnable, m_settingsManager);
  dependencyday.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, "20", SettingDependencyOperatorEquals, (m_tmp_type == OPENREADONLY || m_bIsManualTimer), m_settingsManager)));
  CSettingDependency dependency2day(SettingDependencyTypeVisible, m_settingsManager);
  dependency2day.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, "0", SettingDependencyOperatorEquals, false, m_settingsManager)));
  SettingDependencies depsDay;
  depsDay.push_back(dependencyday);

  if (m_bIsManualTimer)
    depsDay.push_back(dependency2day);

  settingDay->SetDependencies(depsDay);

  /// First day (only for manual timers)
  if (m_bIsManualTimer)
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

    // define an enable dependency with timer type set to "!once" or unsupported timer (=20)
    CSettingDependency dependencyFirstDay(SettingDependencyTypeVisible, m_settingsManager);
    dependencyFirstDay.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, "0", SettingDependencyOperatorEquals, true, m_settingsManager)));
    CSettingDependency dependency2FirstDay(SettingDependencyTypeEnable, m_settingsManager);
    dependency2FirstDay.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, "20", SettingDependencyOperatorEquals, true, m_settingsManager)));
    SettingDependencies depsFirstDay;
    depsFirstDay.push_back(dependencyFirstDay);
    depsFirstDay.push_back(dependency2FirstDay);
    settingFirstDay->SetDependencies(depsFirstDay);
  }

  /// Begin and end time
  if (!m_bIsManualTimer || m_tmp_type == OPENREADONLY)
  {
    // add greyed out edit boxes for epg based timers (just for displaying info to user)
    CSettingString *settingBegin = AddEdit(group, SETTING_TMR_BEGIN, 19080, 0, m_timerStartTimeStr, false, false);
    CSettingString *settingEnd = AddEdit(group, SETTING_TMR_END, 19081, 0, m_timerEndTimeStr, false, false);

    CSettingDependency dependencybegin(SettingDependencyTypeEnable, m_settingsManager);
    dependencybegin.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, "20", SettingDependencyOperatorEquals, (m_tmp_type == OPENREADONLY), m_settingsManager)));
    SettingDependencies depsBegin;
    depsBegin.push_back(dependencybegin);
    settingBegin->SetDependencies(depsBegin);

    CSettingDependency dependencyend(SettingDependencyTypeEnable, m_settingsManager);
    dependencyend.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, "20", SettingDependencyOperatorEquals, (m_tmp_type == OPENREADONLY), m_settingsManager)));
    SettingDependencies depsEnd;
    depsEnd.push_back(dependencyend);
    settingEnd->SetDependencies(depsEnd);
  }
  else
  {
    // manual timer uses non greyed out boxes
    AddButton(group, SETTING_TMR_BEGIN, 19080, 0);
    AddButton(group, SETTING_TMR_END, 19081, 0);
  }

  /// "New episodes only" rule (only for epg timers)
  if (!m_bIsManualTimer && g_PVRClients->HasNewEpisodesTimerSupport(tag->m_iClientId))
  {
    CSettingBool *settingNewEpisodesOnly = AddToggle(group, SETTING_TMR_NEW_EPISODES, 821, 0, tag->m_bNewEpisodesOnly);

    // only show button when timer is repeating and gray out when we opened an unsupported timer (SETTING_TMR_TYPE = 20)
    CSettingDependency dependencyNewEpisodes(SettingDependencyTypeVisible, m_settingsManager);
    dependencyNewEpisodes.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, "0", SettingDependencyOperatorEquals, true, m_settingsManager)));
    CSettingDependency dependency2NewEpisodes(SettingDependencyTypeEnable, m_settingsManager);
    dependency2NewEpisodes.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, "20", SettingDependencyOperatorEquals, true, m_settingsManager)));
    SettingDependencies depsRepeat;
    depsRepeat.push_back(dependency2NewEpisodes);
    depsRepeat.push_back(dependencyNewEpisodes);
    settingNewEpisodesOnly->SetDependencies(depsRepeat);
  }

  /// Pre and post time (only for epg timers)
  if (!m_bIsManualTimer)
  {
    CSettingInt *settingExtraStart = AddSpinner(group, SETTING_TMR_BEGIN_PRE, 822, 0, 0, tag->m_iMarginStart,1,60);
    CSettingInt *settingExtraEnd = AddSpinner(group, SETTING_TMR_END_POST, 823, 0, 0, tag->m_iMarginEnd,1,60);

    // grey out when we open an unsupported timer (SETTING_TMR_TYPE = 20)
    CSettingDependency dependencyextrastart(SettingDependencyTypeEnable, m_settingsManager);
    dependencyextrastart.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, "20", SettingDependencyOperatorEquals, true, m_settingsManager)));
    SettingDependencies depsExtraStart;
    depsExtraStart.push_back(dependencyextrastart);
    settingExtraStart->SetDependencies(depsExtraStart);

    // grey out when we open an unsupported timer (SETTING_TMR_TYPE = 20)
    CSettingDependency dependencyextraend(SettingDependencyTypeEnable, m_settingsManager);
    dependencyextraend.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, "20", SettingDependencyOperatorEquals, true, m_settingsManager)));
    SettingDependencies depsExtraEnd;
    depsExtraEnd.push_back(dependencyextraend);
    settingExtraEnd->SetDependencies(depsExtraEnd);
  }

  /// Priority and lifetime
  CSettingInt *settingPrio = AddSpinner(group, SETTING_TMR_PRIORITY, 19082, 0, tag->m_iPriority, 0, 1, 99);
  CSettingInt *settingLifetime = AddSpinner(group, SETTING_TMR_LIFETIME, 19083, 0, tag->m_iLifetime, 0, 1, 365);

  // grey out when we open an unsupported timer (SETTING_TMR_TYPE = 20)
  CSettingDependency dependencypri(SettingDependencyTypeEnable, m_settingsManager);
  dependencypri.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, "20", SettingDependencyOperatorEquals, true, m_settingsManager)));
  SettingDependencies depsPri;
  depsPri.push_back(dependencypri);
  settingPrio->SetDependencies(depsPri);

  // grey out when we open an unsupported timer (SETTING_TMR_TYPE = 20)
  CSettingDependency dependencylife(SettingDependencyTypeEnable, m_settingsManager);
  dependencylife.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, "20", SettingDependencyOperatorEquals, true, m_settingsManager)));
  SettingDependencies depsLife;
  depsLife.push_back(dependencylife);
  settingLifetime->SetDependencies(depsLife);
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

  CSettingInt *setting = AddSpinner(group, bRadio ? SETTING_TMR_CHNAME_RADIO : SETTING_TMR_CHNAME_TV, 19078, 0, m_selectedChannelEntry, ChannelNamesOptionsFiller);
  if (setting == NULL)
    return NULL;

  // grey out when we opened an unsupported timer (SETTING_TMR_TYPE = 20) or an epg based timer */
  CSettingDependency dependencyChannel(SettingDependencyTypeEnable, m_settingsManager);
  dependencyChannel.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, "20", SettingDependencyOperatorEquals, (m_tmp_type == OPENREADONLY || m_bIsManualTimer), m_settingsManager)));
  SettingDependencies deps;
  deps.push_back(dependencyChannel);
  setting->SetDependencies(deps);

  return setting;
}

void CGUIDialogPVRTimerSettings::SetTypeFromTimer(const CPVRTimerInfoTag &timer)
{
  if (timer.m_bIsRepeating)
  {
    if (m_bIsManualTimer)
    {
      switch (timer.m_iWeekdays)
      {
      case BITFLAG_MONDAY:
        m_tmp_type = 1;
        break;
      case BITFLAG_TUESDAY:
        m_tmp_type = 2;
        break;
      case BITFLAG_WEDNESDAY:
        m_tmp_type = 3;
        break;
      case BITFLAG_THURSDAY:
        m_tmp_type = 4;
        break;
      case BITFLAG_FRIDAY:
        m_tmp_type = 5;
        break;
      case BITFLAG_SATURDAY:
        m_tmp_type = 6;
        break;
      case BITFLAG_SUNDAY:
        m_tmp_type = 7;
        break;
      case BITFLAG_WEEKDAYS:
        m_tmp_type = 8;
        break;
      case BITFLAG_WEEKENDS:
        m_tmp_type = 9;
        break;
      case BITFLAG_ALL_DAYS:
        m_tmp_type = 10;
        break;
      default:
        m_tmp_type = OPENREADONLY;     //not supported
        break;
      }

      /* supported by kodi but marked as unsupported by pvr client */
      if (!(PVR_TIMER_TYPE_SERIE_MANUAL & g_PVRClients->GetSupportedTimers(timer.m_iClientId)))
        m_tmp_type = OPENREADONLY;
    }
    else
    {
      switch (timer.m_type)
      {
      case PVR_TIMER_TYPE_SERIE_EPG_ANYTIME_THIS_CHANNEL:
        m_tmp_type = 1;
        break;
      case PVR_TIMER_TYPE_SERIE_EPG_ANYTIME_ANY_CHANNEL:
        m_tmp_type = 2;
        break;
      case PVR_TIMER_TYPE_SERIE_EPG_WEEKLY_AROUND_TIME:
        m_tmp_type = 3;
        break;
      case PVR_TIMER_TYPE_SERIE_EPG_DAILY_AROUND_TIME:
        m_tmp_type = 4;
        break;
      case PVR_TIMER_TYPE_SERIE_EPG_WEEKDAYS:
        m_tmp_type = 5;
        break;
      case PVR_TIMER_TYPE_SERIE_EPG_WEEKENDS:
        m_tmp_type = 6;
        break;
      default:
        m_tmp_type = OPENREADONLY;     //not supported
        break;
      }

      /* supported by kodi but marked as unsupported by pvr client */
      if (!(timer.m_type & g_PVRClients->GetSupportedTimers(timer.m_iClientId)))
        m_tmp_type = OPENREADONLY;
    }
  }
  else
    m_tmp_type = 0;    //rec once
}

void CGUIDialogPVRTimerSettings::SetTimerFromWeekdayAndType(CPVRTimerInfoTag &timer)
{
  if (m_tmp_type == 0) //once
  {
    timer.m_bIsRepeating = false;
    timer.m_type = m_bIsManualTimer ? PVR_TIMER_TYPE_ONCE_MANUAL : PVR_TIMER_TYPE_ONCE_EPG;
    timer.m_iWeekdays = 0;
  }
  else
  {
    timer.m_bIsRepeating = true;

    if (m_bIsManualTimer)
    {
      timer.m_type = PVR_TIMER_TYPE_SERIE_MANUAL;

      switch (m_tmp_type)
      {
      case 1:
        timer.m_iWeekdays = BITFLAG_MONDAY;
        break;
      case 2:
        timer.m_iWeekdays = BITFLAG_TUESDAY;
        break;
      case 3:
        timer.m_iWeekdays = BITFLAG_WEDNESDAY;
        break;
      case 4:
        timer.m_iWeekdays = BITFLAG_THURSDAY;
        break;
      case 5:
        timer.m_iWeekdays = BITFLAG_FRIDAY;
        break;
      case 6:
        timer.m_iWeekdays = BITFLAG_SATURDAY;
        break;
      case 7:
        timer.m_iWeekdays = BITFLAG_SUNDAY;
        break;
      case 8:
        timer.m_iWeekdays = BITFLAG_WEEKDAYS;
        break;
      case 9:
        timer.m_iWeekdays = BITFLAG_WEEKENDS;
        break;
      case 10:
        timer.m_iWeekdays = BITFLAG_ALL_DAYS;
        break;
      default:
        {
          timer.m_iWeekdays = 0;
          timer.m_type = PVR_TIMER_TYPE_NONE;
        }
        break;
      }
    }
    else
    {
      timer.m_iWeekdays = 0;

      switch (m_tmp_type)
      {
      case 1:
        timer.m_type = PVR_TIMER_TYPE_SERIE_EPG_ANYTIME_THIS_CHANNEL;
        break;
      case 2:
        timer.m_type = PVR_TIMER_TYPE_SERIE_EPG_ANYTIME_ANY_CHANNEL;
        break;
      case 3:
        timer.m_type = PVR_TIMER_TYPE_SERIE_EPG_WEEKLY_AROUND_TIME ;
        break;
      case 4:
        timer.m_type = PVR_TIMER_TYPE_SERIE_EPG_DAILY_AROUND_TIME;
        break;
      case 5:
        timer.m_type = PVR_TIMER_TYPE_SERIE_EPG_WEEKDAYS;
        break;
      case 6:
        timer.m_type = PVR_TIMER_TYPE_SERIE_EPG_WEEKENDS;
        break;
      default:
        timer.m_type = PVR_TIMER_TYPE_NONE;
        break;
      }
    }
  }
}

void CGUIDialogPVRTimerSettings::getChannelNames(bool bRadio, std::vector< std::pair<std::string, int> > &list, int &current, bool updateChannelEntries /* = false */)
{
  CFileItemList channelsList;
  g_PVRChannelGroups->GetGroupAll(bRadio)->GetMembers(channelsList);
  
  int entry = 0;

  for (int i = 0; i < channelsList.Size(); i++)
  {
    const CPVRChannel *channel = channelsList[i]->GetPVRChannelInfoTag();

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

  const CPVRTimerInfoTag *tag = dialog->m_timerItem->GetPVRTimerInfoTag();
  if (tag == NULL)
    return;

  if (setting->GetId() == SETTING_TMR_FIRST_DAY)
    list.push_back(std::make_pair(g_localizeStrings.Get(19030), 0)); //now

  CDateTime time = CDateTime::GetCurrentDateTime();
  for (int i = 1; i < 365; ++i)
  {
    list.push_back(std::make_pair(time.GetAsLocalizedDate(), list.size()));
    time += CDateTimeSpan(1, 0, 0, 0);
  }
}

void CGUIDialogPVRTimerSettings::TypesEpgOptionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  if (setting == NULL || data == NULL)
    return;

  CGUIDialogPVRTimerSettings *dialog = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (dialog == NULL)
    return;

  const CPVRTimerInfoTag *tag = dialog->m_timerItem->GetPVRTimerInfoTag();
  if (tag == NULL)
    return;

  /* unsupported repeating timer*/
  if (current == OPENREADONLY)
  {
    list.push_back(std::make_pair(g_localizeStrings.Get(812), OPENREADONLY));
    return;
  }

  int mask = g_PVRClients->GetSupportedTimers(tag->m_iClientId);

  if (mask & PVR_TIMER_TYPE_ONCE_EPG)
    list.push_back(std::make_pair(g_localizeStrings.Get(805), 0));
  if (mask & PVR_TIMER_TYPE_SERIE_EPG_ANYTIME_THIS_CHANNEL)
    list.push_back(std::make_pair(g_localizeStrings.Get(806), 1));
  if (mask & PVR_TIMER_TYPE_SERIE_EPG_ANYTIME_ANY_CHANNEL)
    list.push_back(std::make_pair(g_localizeStrings.Get(807), 2));
  if (mask & PVR_TIMER_TYPE_SERIE_EPG_WEEKLY_AROUND_TIME)
    list.push_back(std::make_pair(g_localizeStrings.Get(808), 3));
  if (mask & PVR_TIMER_TYPE_SERIE_EPG_DAILY_AROUND_TIME)
    list.push_back(std::make_pair(g_localizeStrings.Get(809), 4));
  if (mask & PVR_TIMER_TYPE_SERIE_EPG_WEEKDAYS)
    list.push_back(std::make_pair(g_localizeStrings.Get(810), 5));
  if (mask & PVR_TIMER_TYPE_SERIE_EPG_WEEKENDS)
    list.push_back(std::make_pair(g_localizeStrings.Get(811), 6));
}


void CGUIDialogPVRTimerSettings::TypesManualOptionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  if (setting == NULL || data == NULL)
    return;

  CGUIDialogPVRTimerSettings *dialog = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (dialog == NULL)
    return;

  const CPVRTimerInfoTag *tag = dialog->m_timerItem->GetPVRTimerInfoTag();
  if (tag == NULL)
    return;

  /* unsupported repeating timer*/
  if (current == OPENREADONLY)
  {
    list.push_back(std::make_pair(g_localizeStrings.Get(812), OPENREADONLY));
    return;
  }

  list.push_back(std::make_pair(g_localizeStrings.Get(805), 0)); //once

  // manual repeating timer support?
  if (PVR_TIMER_TYPE_SERIE_MANUAL & g_PVRClients->GetSupportedTimers(tag->m_iClientId))
  {
    /* add days mon-sun */
    for (unsigned int iDayPtr = 813; iDayPtr <= 819; iDayPtr++)
      list.push_back(std::make_pair(g_localizeStrings.Get(iDayPtr), iDayPtr-812));

    /* add weekend and weekday */
    list.push_back(std::make_pair(g_localizeStrings.Get(810), 8));  //weekdays
    list.push_back(std::make_pair(g_localizeStrings.Get(811), 9));  //weekends
    list.push_back(std::make_pair(g_localizeStrings.Get(820), 10)); //all days
  }
}
