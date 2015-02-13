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


CGUIDialogPVRTimerSettings::CGUIDialogPVRTimerSettings(void)
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_PVR_TIMER_SETTING, "DialogPVRTimerSettings.xml"),
    m_tmp_iFirstDay(0),
    m_tmp_day(0),
    m_tmp_type(0),
    m_bTimerActive(false),
    m_selectedChannelEntry(0),
    m_bIsNewTimer(false),
    m_bIsManualTimer(false),
    m_timerItem(NULL),
    m_tmp_iLifetime(0),
    m_tmp_iPriority(0),
    m_tmp_iMarginStart(0),
    m_tmp_iMarginEnd(0),
    m_tmp_bNewEpisodesOnly(false)
{
  m_loadType = LOAD_EVERY_TIME;
}

void CGUIDialogPVRTimerSettings::SetTimer(CFileItem *item)
{
  m_timerItem = item;

  m_timerItem->GetPVRTimerInfoTag()->StartAsLocalTime().GetAsSystemTime(m_timerStartTime);
  m_timerItem->GetPVRTimerInfoTag()->EndAsLocalTime().GetAsSystemTime(m_timerEndTime);
  m_timerStartTimeStr     = m_timerItem->GetPVRTimerInfoTag()->StartAsLocalTime().GetAsLocalizedTime("", false);
  m_timerEndTimeStr       = m_timerItem->GetPVRTimerInfoTag()->EndAsLocalTime().GetAsLocalizedTime("", false);

  m_tmp_strTitle          = m_timerItem->GetPVRTimerInfoTag()->m_strTitle;
  m_tmp_strDirectory      = m_timerItem->GetPVRTimerInfoTag()->m_strDirectory;
  m_tmp_iMarginStart      = m_timerItem->GetPVRTimerInfoTag()->m_iMarginStart;
  m_tmp_iMarginEnd        = m_timerItem->GetPVRTimerInfoTag()->m_iMarginEnd;
  m_tmp_iPriority         = m_timerItem->GetPVRTimerInfoTag()->m_iPriority;
  m_tmp_iLifetime         = m_timerItem->GetPVRTimerInfoTag()->m_iLifetime;
  m_tmp_bNewEpisodesOnly  = m_timerItem->GetPVRTimerInfoTag()->m_bNewEpisodesOnly;
  m_bTimerActive          = m_timerItem->GetPVRTimerInfoTag()->IsActive();

  m_bIsManualTimer        = false;
  m_bIsNewTimer           = false;
  m_selectedChannelEntry  = 0;
  m_tmp_iFirstDay         = 0;
  m_tmp_day               = 0;
  m_channelEntries.clear();


  /// Set manual and new timer flags
  if (m_timerItem->GetPVRTimerInfoTag()->m_state == PVR_TIMER_STATE_NEW)
  {
    m_bIsManualTimer = (m_timerItem->GetPVRTimerInfoTag()->GetEpgInfoTag() == NULL);
    m_bIsNewTimer = true;
  }
  else
    m_bIsManualTimer = (m_timerItem->GetPVRTimerInfoTag()->m_iTimerType  == PVR_TIMERTYPE_MANUAL_ONCE
                     || m_timerItem->GetPVRTimerInfoTag()->m_iTimerType  == PVR_TIMERTYPE_MANUAL_SERIE);


  /// Set day and first day
  CDateTime time = CDateTime::GetCurrentDateTime();
  CDateTime timeFirstDay = m_timerItem->GetPVRTimerInfoTag()->FirstDayAsLocalTime();
  CDateTime timestart = m_timerItem->GetPVRTimerInfoTag()->StartAsLocalTime();
  tm time_tmr; timestart.GetAsTm(time_tmr);
  tm time_cur; time.GetAsTm(time_cur);

  if (time < timeFirstDay)
  {
    // get difference of timer in days between today and timer start date
    tm time_tmr_first_day; timeFirstDay.GetAsTm(time_tmr_first_day);

    m_tmp_iFirstDay = time_tmr_first_day.tm_yday - time_cur.tm_yday;
    if (time_tmr_first_day.tm_yday - time_cur.tm_yday < 0)
      m_tmp_iFirstDay += 365;
  }

  m_tmp_day = time_tmr.tm_yday - time_cur.tm_yday;
  if (time_tmr.tm_yday - time_cur.tm_yday < 0)
    m_tmp_day += 365;
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
    m_selectedChannelEntry = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_DAY)
    m_tmp_day = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_PRIORITY)
    m_tmp_iPriority = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_LIFETIME)
    m_tmp_iLifetime = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_FIRST_DAY)
    m_tmp_iFirstDay = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_NAME)
    m_tmp_strTitle = static_cast<const CSettingString*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_DIR)
    m_tmp_strDirectory = static_cast<const CSettingString*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_TYPE)
    m_tmp_type = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_BEGIN_PRE)
    m_tmp_iMarginStart = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_END_POST)
    m_tmp_iMarginEnd = static_cast<const CSettingInt*>(setting)->GetValue();
  else if (settingId == SETTING_TMR_NEW_EPISODES)
    m_tmp_bNewEpisodesOnly = static_cast<const CSettingInt*>(setting)->GetValue();
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
      m_timerStartTimeStr = timestart.GetAsLocalizedTime("", false);
      setButtonLabels();
    }
  }
  else if (settingId == SETTING_TMR_END)
  {
    if (CGUIDialogNumeric::ShowAndGetTime(m_timerEndTime, g_localizeStrings.Get(14066)))
    {
      CDateTime timestop = m_timerEndTime;
      m_timerEndTimeStr = timestop.GetAsLocalizedTime("", false);
      setButtonLabels();
    }
  }
}

void CGUIDialogPVRTimerSettings::Save()
{
  CPVRTimerInfoTag* tag = m_timerItem->GetPVRTimerInfoTag();

  tag->m_strTitle         = m_tmp_strTitle;
  tag->m_strDirectory     = m_tmp_strDirectory;
  tag->m_iMarginStart     = m_tmp_iMarginStart;
  tag->m_iMarginEnd       = m_tmp_iMarginEnd;
  tag->m_iPriority        = m_tmp_iPriority;
  tag->m_iLifetime        = m_tmp_iLifetime;
  tag->m_bNewEpisodesOnly = m_tmp_bNewEpisodesOnly;
  tag->m_iTimerType       = m_tmp_type;

  /// Set the timer's title to the channel name if it's 'New Timer' or empty
  if (tag->m_strTitle == g_localizeStrings.Get(19056) || tag->m_strTitle.empty())
  {
    CPVRChannelPtr channel = g_PVRChannelGroups->GetByUniqueID(tag->m_iClientChannelUid, tag->m_iClientId);
    if (channel)
      tag->m_strTitle = channel->ChannelName();
  }

  /// Timer active
  if (m_bTimerActive)
    tag->m_state = PVR_TIMER_STATE_SCHEDULED;
  else
    tag->m_state = PVR_TIMER_STATE_CANCELLED;

  // Set the timer type
  if (m_bIsManualTimer && m_tmp_type != PVR_TIMERTYPE_MANUAL_ONCE)
  {
    tag->m_iWeekdays = m_tmp_type;
    tag->m_iTimerType = PVR_TIMERTYPE_MANUAL_SERIE;
  }
  else
  {
    tag->m_iWeekdays = 0;
    tag->m_iTimerType = m_tmp_type;
  }

  /// Channel
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
      tag->UpdateChannel();
    }
  }

  /// First day
  CDateTime newFirstDay;
  if (m_tmp_iFirstDay > 0)
    newFirstDay = CDateTime::GetCurrentDateTime() + CDateTimeSpan(m_tmp_iFirstDay - 1, 0, 0, 0);

  tag->SetFirstDayFromLocalTime(newFirstDay);

  /// Start and stop time
  CDateTime time = CDateTime::GetCurrentDateTime();
  CDateTime timestart = m_timerStartTime;
  CDateTime timestop = m_timerEndTime;
  int m_tmp_diff;

  // get difference of timer in days between today and timer start date
  tm time_cur; time.GetAsTm(time_cur);
  tm time_tmr; timestart.GetAsTm(time_tmr);

  m_tmp_diff = time_tmr.tm_yday - time_cur.tm_yday;
  if (time_tmr.tm_yday - time_cur.tm_yday < 0)
    m_tmp_diff = 365;

  CDateTime newStart = timestart + CDateTimeSpan(m_tmp_day - m_tmp_diff, 0, 0, -timestart.GetSecond());
  CDateTime newEnd = timestop + CDateTimeSpan(m_tmp_day - m_tmp_diff, 0, 0, -timestop.GetSecond());

  // add a day to end time if end time is before start time
  // TODO: this should be removed after separate end date control was added
  if (newEnd < newStart)
    newEnd += CDateTimeSpan(1, 0, 0, 0);

  tag->SetStartFromLocalTime(newStart);
  tag->SetEndFromLocalTime(newEnd);

  /// Update summary
  tag->UpdateSummary();
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
  PVR_TIMERTYPES *types = g_PVRClients->GetTimerTypes(tag->m_iClientId);

  CSettingString  *settingFolder;
  CSettingString  *settingTitle;
  CSettingInt     *settingFirstDay;
  CSettingString  *settingBegin;
  CSettingString  *settingEnd;
  CSettingInt     *settingDay;
  CSettingInt     *settingType;
  CSettingBool    *settingNewEpisodesOnly;
  CSettingInt     *settingPrio;
  CSettingInt     *settingLifetime;
  CSettingInt     *settingExtraStart;
  CSettingInt     *settingExtraEnd;

  if (!m_bIsManualTimer)
  {
    bool oneTypeExcist  = false;
    bool unKnownTypeExcist  = false;
    bool isUnKnownTimer = true;

    // Check if we have a 'once' and 'unknown' timertype,
    // we have to add them if this is not the case
    // We need them even if pvr addons don't push them
    for (unsigned int i = 0; i < types->size(); i++)
    {
      if (types->at(i).iTypeId == PVR_TIMERTYPE_EPG_ONCE)
        oneTypeExcist = true;
      if (types->at(i).iTypeId == PVR_TIMERTYPE_UNKNOWN)
        unKnownTypeExcist = true;
      if (types->at(i).iTypeId == tag->m_iTimerType)
        isUnKnownTimer = false;
    }

    /// Set to unknown, pvr addon doesn't seems to support this type (type id not pushed)
    if (isUnKnownTimer && tag->m_iTimerType != PVR_TIMERTYPE_EPG_ONCE)
      tag->m_iTimerType = PVR_TIMERTYPE_UNKNOWN;

    // add dummy type for a one time recording
    if (!oneTypeExcist)
    {
      PVR_TIMERTYPE onceType;
      onceType.iTypeId = PVR_TIMERTYPE_EPG_ONCE;
      onceType.bDependsNewEpisodes = false;
      onceType.bDependsChannel = true;
      onceType.bDependsTime = true;
      onceType.bReadOnly = false;
      types->push_back(onceType);
    }

    // add dummy type for an unknown timer schedule
    if (!unKnownTypeExcist)
    {
      PVR_TIMERTYPE unknownType;
      unknownType.iTypeId = PVR_TIMERTYPE_UNKNOWN;
      unknownType.bDependsNewEpisodes = false;
      unknownType.bDependsChannel = false;
      unknownType.bDependsTime = false;
      unknownType.bReadOnly = true;
      types->push_back(unknownType);
    }
  }

  /// Set spinner type to the correct type
  if (m_bIsNewTimer && m_bIsManualTimer)
    m_tmp_type = PVR_TIMERTYPE_MANUAL_ONCE;
  else if (tag->m_iTimerType == PVR_TIMERTYPE_MANUAL_SERIE)
    m_tmp_type = tag->m_iWeekdays;
  else
    m_tmp_type = tag->m_iTimerType;

  /// Timer type
  if (m_bIsManualTimer)
    settingType = AddSpinner(group, SETTING_TMR_TYPE, 804, 0, m_tmp_type, TypesManualOptionsFiller);
  else
    settingType = AddSpinner(group, SETTING_TMR_TYPE, 804, 0, m_tmp_type, TypesEpgOptionsFiller);

  /// Timer active selection, only for existing timers
  if (!m_bIsNewTimer)
    AddToggle(group, SETTING_TMR_ACTIVE, 19074, 0, m_bTimerActive);

  /// Title
  settingTitle = AddEdit(group, SETTING_TMR_NAME, 19075, 0, m_tmp_strTitle, false, false, 19097);

  /// Channel names
  AddChannelNames(group, tag->m_bIsRadio);

  /// Begin and end time
  if (!m_bIsManualTimer)
  {
    settingBegin = AddEdit(group, SETTING_TMR_BEGIN, 19080, 0, m_timerStartTimeStr, false, false);
    settingEnd = AddEdit(group, SETTING_TMR_END, 19081, 0, m_timerEndTimeStr, false, false);
  }
  else
  {
    // Manual timer uses non greyed out boxes
    AddButton(group, SETTING_TMR_BEGIN, 19080, 0);
    AddButton(group, SETTING_TMR_END, 19081, 0);
  }

  /// First day setting
  if (m_bIsManualTimer)
    settingFirstDay = AddSpinner(group, SETTING_TMR_FIRST_DAY, 19084, 0, m_tmp_iFirstDay, DaysOptionsFiller);

  /// Day setting
  settingDay = AddSpinner(group, SETTING_TMR_DAY, 19079, 0, m_tmp_day, DaysOptionsFiller);

  /// "New episodes only" rule
  if (!m_bIsManualTimer)
    settingNewEpisodesOnly = AddToggle(group, SETTING_TMR_NEW_EPISODES, 821, 0, m_tmp_bNewEpisodesOnly);

  /// Pre and post time
  if (!m_bIsManualTimer)
  {
    settingExtraStart = AddSpinner(group, SETTING_TMR_BEGIN_PRE, 822, 0, 0, m_tmp_iMarginStart,1,60);
    settingExtraEnd = AddSpinner(group, SETTING_TMR_END_POST, 823, 0, 0, m_tmp_iMarginEnd,1,60);
  }

  /// Priority and lifetime
  settingPrio = AddSpinner(group, SETTING_TMR_PRIORITY, 19082, 0, m_tmp_iPriority, 0, 1, 99);
  settingLifetime = AddSpinner(group, SETTING_TMR_LIFETIME, 19083, 0, m_tmp_iLifetime, 0, 1, 365);

  /// Recording folder
  if (tag->SupportsFolders())
    settingFolder = AddEdit(group, SETTING_TMR_DIR, 19076, 0, m_tmp_strDirectory, true, false, 19104);

  /// Set visible and enable dependencies
  if (m_bIsManualTimer)
  {
    SettingDependencies depsDay;
    CSettingDependency dependencyvisible(SettingDependencyTypeVisible, m_settingsManager);
    dependencyvisible.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, StringUtils::Format("%i", PVR_TIMERTYPE_MANUAL_ONCE).c_str(), SettingDependencyOperatorEquals, false, m_settingsManager)));
    depsDay.push_back(dependencyvisible);
    settingDay->SetDependencies(depsDay);

    if (m_bIsNewTimer || tag->m_iTimerType  == PVR_TIMERTYPE_MANUAL_ONCE)
    {
      depsDay.clear();
      CSettingDependency dependencyvisible2(SettingDependencyTypeVisible, m_settingsManager);
      dependencyvisible2.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, StringUtils::Format("%i", PVR_TIMERTYPE_MANUAL_ONCE).c_str(), SettingDependencyOperatorEquals, true, m_settingsManager)));
      depsDay.push_back(dependencyvisible2);
      settingFirstDay->SetDependencies(depsDay);
    }
    else
      settingFirstDay->SetEnabled(false);
  }
  else
  {
    SettingDependencies depsTitle;
    SettingDependencies depsPriLifeExtr;
    SettingDependencies depsTime;
    SettingDependencies depsEpisodes;
    SettingDependencies depsDay;

    for (unsigned int i = 0; i < types->size(); i++)
    {
      if (!m_bIsNewTimer
          && types->at(i).bReadOnly
          && types->at(i).iTypeId == tag->m_iTimerType)
      {
        // We opened a read only timer, disable controls
        CSettingDependency dependencyenable(SettingDependencyTypeEnable, m_settingsManager);
        dependencyenable.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, StringUtils::Format("%i", types->at(i).iTypeId).c_str(), SettingDependencyOperatorEquals, true, m_settingsManager)));
        depsPriLifeExtr.push_back(dependencyenable);
        depsTitle.push_back(dependencyenable);
        depsTime.push_back(dependencyenable);
        depsEpisodes.push_back(dependencyenable);
      }

      if (!m_bIsNewTimer)
      {
        CSettingDependency dependencyenable(SettingDependencyTypeEnable, m_settingsManager);
        dependencyenable.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, StringUtils::Format("%i", types->at(i).iTypeId).c_str(), SettingDependencyOperatorEquals, true, m_settingsManager)));
        depsEpisodes.push_back(dependencyenable);
      }

      if (!types->at(i).bDependsTime)
      {
        CSettingDependency dependencyvisible(SettingDependencyTypeVisible, m_settingsManager);
        dependencyvisible.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, StringUtils::Format("%i", types->at(i).iTypeId).c_str(), SettingDependencyOperatorEquals, true, m_settingsManager)));
        depsTime.push_back(dependencyvisible);
      }

      if (!types->at(i).bDependsNewEpisodes)
      {
        CSettingDependency dependencyvisible(SettingDependencyTypeVisible, m_settingsManager);
        dependencyvisible.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, StringUtils::Format("%i", types->at(i).iTypeId).c_str(), SettingDependencyOperatorEquals, true, m_settingsManager)));
        depsEpisodes.push_back(dependencyvisible);
      }

      // Disable day for repeating timers
      if (types->at(i).iTypeId != PVR_TIMERTYPE_EPG_ONCE)
      {
        CSettingDependency dependencyvisible(SettingDependencyTypeVisible, m_settingsManager);
        dependencyvisible.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, StringUtils::Format("%i", types->at(i).iTypeId).c_str(), SettingDependencyOperatorEquals, true, m_settingsManager)));
        depsDay.push_back(dependencyvisible);
      }

      // Disable time and title controls for all epg based timers
      CSettingDependency dependencyenable(SettingDependencyTypeEnable, m_settingsManager);
      dependencyenable.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, StringUtils::Format("%i", types->at(i).iTypeId).c_str(), SettingDependencyOperatorEquals, true, m_settingsManager)));
      depsTitle.push_back(dependencyenable);
      depsTime.push_back(dependencyenable);
      depsDay.push_back(dependencyenable);
    }

    settingPrio->SetDependencies(depsPriLifeExtr);
    settingLifetime->SetDependencies(depsPriLifeExtr);
    settingExtraStart->SetDependencies(depsPriLifeExtr);
    settingExtraEnd->SetDependencies(depsPriLifeExtr);
    settingNewEpisodesOnly->SetDependencies(depsEpisodes);
    settingTitle->SetDependencies(depsTitle);
    settingBegin->SetDependencies(depsTime);
    settingEnd->SetDependencies(depsTime);
    settingDay->SetDependencies(depsDay);

    if (tag->SupportsFolders())
      settingFolder->SetDependencies(depsPriLifeExtr);
  }

  // Should we allow type and first day switching for existing timers??
  if (!m_bIsNewTimer)
    settingType->SetEnabled(false);

  // Only 'once' is available
  if (!m_bIsManualTimer && types->size() == 2)
    settingType->SetEnabled(false);
}

CSetting* CGUIDialogPVRTimerSettings::AddChannelNames(CSettingGroup *group, bool bRadio)
{
  std::vector< std::pair<std::string, int> > options;
  getChannelNames(bRadio, options, m_selectedChannelEntry, true);

  if (m_timerItem->GetPVRTimerInfoTag()->ChannelTag())
  {
    int timerChannelID = m_timerItem->GetPVRTimerInfoTag()->ChannelTag()->ChannelID();
    for (std::vector< std::pair<std::string, int> >::const_iterator option = options.begin(); option != options.end(); ++option)
    {
      std::map<std::pair<bool, int>, int>::const_iterator channelEntry = m_channelEntries.find(std::make_pair(bRadio, option->second));
      if (channelEntry != m_channelEntries.end() && channelEntry->second == timerChannelID)
      {
        m_selectedChannelEntry = option->second;
        break;
      }
    }
  }

  CSettingInt *setting = AddSpinner(group, bRadio ? SETTING_TMR_CHNAME_RADIO : SETTING_TMR_CHNAME_TV, 19078, 0, m_selectedChannelEntry, ChannelNamesOptionsFiller);
  if (setting == NULL)
    return NULL;

  if (!m_bIsManualTimer)
  {
    CSettingDependency dependencyChannelV(SettingDependencyTypeVisible, m_settingsManager);
    CSettingDependency dependencyChannelE(SettingDependencyTypeEnable, m_settingsManager);
    SettingDependencies depsChannel;

    PVR_TIMERTYPES *types = g_PVRClients->GetTimerTypes(m_timerItem->GetPVRTimerInfoTag()->m_iClientId);

    for (unsigned int i = 0; i < types->size(); i++)
    {
      dependencyChannelE.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, StringUtils::Format("%i", types->at(i).iTypeId).c_str(), SettingDependencyOperatorEquals, true, m_settingsManager)));
      depsChannel.push_back(dependencyChannelE);

      if (!types->at(i).bDependsChannel)
      {
        dependencyChannelV.And()->Add(CSettingDependencyConditionPtr(new CSettingDependencyCondition(SETTING_TMR_TYPE, StringUtils::Format("%i", types->at(i).iTypeId).c_str(), SettingDependencyOperatorEquals, true, m_settingsManager)));
        depsChannel.push_back(dependencyChannelV);
      }
    }
    setting->SetDependencies(depsChannel);
  }

  // Should we allow channel switching for an existing timer??
  if (!m_bIsNewTimer)
    setting->SetEnabled(false);

  return setting;
}

void CGUIDialogPVRTimerSettings::getChannelNames(bool bRadio, std::vector< std::pair<std::string, int> > &list, int &current, bool updateChannelEntries /* = false */)
{
  CFileItemList channelsList;
  g_PVRChannelGroups->GetGroupAll(bRadio)->GetMembers(channelsList);

  for (int i = 0; i < channelsList.Size(); i++)
  {
    const CPVRChannel *channel = channelsList[i]->GetPVRChannelInfoTag();
    list.push_back(std::make_pair(StringUtils::Format("%i %s", channel->ChannelNumber(), channel->ChannelName().c_str()), i));
    if (updateChannelEntries)
      m_channelEntries.insert(std::make_pair(std::make_pair(bRadio, i), channel->ChannelID()));
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

  PVR_TIMERTYPES *types = g_PVRClients->GetTimerTypes(tag->m_iClientId);

  for (unsigned int i = 0; i < types->size(); i++)
  {
    if (tag->m_iTimerType == PVR_TIMERTYPE_UNKNOWN || types->at(i).iTypeId != PVR_TIMERTYPE_UNKNOWN)
    {
      list.push_back(std::make_pair( types->at(i).iTypeId == PVR_TIMERTYPE_EPG_ONCE ? g_localizeStrings.Get(805)
          : (types->at(i).iTypeId == PVR_TIMERTYPE_UNKNOWN ? g_localizeStrings.Get(812)
          : g_PVRClients->GetClientLocalizedString(tag->m_iClientId, types->at(i).iLocalizedStringId)), (int)types->at(i).iTypeId));
    }
  }
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

  if (tag->m_iTimerType  == PVR_TIMERTYPE_MANUAL_SERIE
      && tag->m_iWeekdays  != BITFLAG_MONDAY    && tag->m_iWeekdays  != BITFLAG_TUESDAY
      && tag->m_iWeekdays  != BITFLAG_WEDNESDAY && tag->m_iWeekdays  != BITFLAG_THURSDAY
      && tag->m_iWeekdays  != BITFLAG_FRIDAY    && tag->m_iWeekdays  != BITFLAG_SATURDAY
      && tag->m_iWeekdays  != BITFLAG_SUNDAY    && tag->m_iWeekdays  != BITFLAG_WEEKDAYS
      && tag->m_iWeekdays  != BITFLAG_WEEKENDS  && tag->m_iWeekdays  != BITFLAG_ALL_DAYS)
  {
    // Add a new type specially for this weekday flag
    list.push_back(std::make_pair(tag->GetWeekdayString().c_str(), tag->m_iWeekdays));
  }

  list.push_back(std::make_pair(g_localizeStrings.Get(805), PVR_TIMERTYPE_MANUAL_ONCE));
  list.push_back(std::make_pair(g_localizeStrings.Get(813), BITFLAG_MONDAY));
  list.push_back(std::make_pair(g_localizeStrings.Get(814), BITFLAG_TUESDAY));
  list.push_back(std::make_pair(g_localizeStrings.Get(815), BITFLAG_WEDNESDAY));
  list.push_back(std::make_pair(g_localizeStrings.Get(816), BITFLAG_THURSDAY));
  list.push_back(std::make_pair(g_localizeStrings.Get(817), BITFLAG_FRIDAY));
  list.push_back(std::make_pair(g_localizeStrings.Get(818), BITFLAG_SATURDAY));
  list.push_back(std::make_pair(g_localizeStrings.Get(819), BITFLAG_SUNDAY));
  list.push_back(std::make_pair(g_localizeStrings.Get(810), BITFLAG_WEEKDAYS));
  list.push_back(std::make_pair(g_localizeStrings.Get(811), BITFLAG_WEEKENDS));
  list.push_back(std::make_pair(g_localizeStrings.Get(820), BITFLAG_ALL_DAYS));
}
