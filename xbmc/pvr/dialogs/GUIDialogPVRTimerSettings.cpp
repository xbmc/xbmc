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
#include "addons/include/xbmc_pvr_types.h"
#include "dialogs/GUIDialogNumeric.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimerType.h"
#include "settings/SettingUtils.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace PVR;

#define SETTING_TMR_TYPE          "timer.type"
#define SETTING_TMR_ACTIVE        "timer.active"
#define SETTING_TMR_NAME          "timer.name"
#define SETTING_TMR_EPGSEARCH     "timer.epgsearch"
#define SETTING_TMR_FULLTEXT      "timer.fulltext"
#define SETTING_TMR_CHANNEL       "timer.channel"
#define SETTING_TMR_ANYTIME       "timer.anytime"
#define SETTING_TMR_START_DAY     "timer.startday"
#define SETTING_TMR_END_DAY       "timer.endday"
#define SETTING_TMR_BEGIN         "timer.begin"
#define SETTING_TMR_END           "timer.end"
#define SETTING_TMR_WEEKDAYS      "timer.weekdays"
#define SETTING_TMR_FIRST_DAY     "timer.firstday"
#define SETTING_TMR_NEW_EPISODES  "timer.newepisodes"
#define SETTING_TMR_BEGIN_PRE     "timer.startmargin"
#define SETTING_TMR_END_POST      "timer.endmargin"
#define SETTING_TMR_PRIORITY      "timer.priority"
#define SETTING_TMR_LIFETIME      "timer.lifetime"
#define SETTING_TMR_DIR           "timer.directory"

#define TYPE_DEP_VISIBI_COND_ID_POSTFIX     "visibi.typedep"
#define TYPE_DEP_ENABLE_COND_ID_POSTFIX     "enable.typedep"
#define CHANNEL_DEP_VISIBI_COND_ID_POSTFIX  "visibi.channeldep"
#define ANYTIME_DEP_ENABLE_COND_ID_POSTFIX  "enable.anytimedep"

#define ENTRY_ANY_CHANNEL (-1)

CGUIDialogPVRTimerSettings::CGUIDialogPVRTimerSettings() :
  CGUIDialogSettingsManualBase(WINDOW_DIALOG_PVR_TIMER_SETTING, "DialogPVRTimerSettings.xml"),
  m_bIsRadio(false),
  m_bIsNewTimer(true),
  m_bTimerActive(false),
  m_bStartAnytime(true),
  m_bEndAnytime(true),
  m_bFullTextEpgSearch(true),
  m_iStartDay(0),
  m_iEndDay(0),
  m_iWeekdays(PVR_WEEKDAY_NONE),
  m_iFirstDay(0),
  m_iPreventDupEpisodes(0),
  m_iMarginStart(0),
  m_iMarginEnd(0),
  m_iPriority(0),
  m_iLifetime(0)
{
  m_loadType = LOAD_EVERY_TIME;
}

// virtual
CGUIDialogPVRTimerSettings::~CGUIDialogPVRTimerSettings()
{
}

void CGUIDialogPVRTimerSettings::SetTimer(CFileItem *item)
{
  if (item == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::SetTimer - No item");
    return;
  }

  m_timerInfoTag = item->GetPVRTimerInfoTag();

  if (!m_timerInfoTag)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::SetTimer - No timer info tag");
    return;
  }

  // Copy data we need from tag. Do not modify the tag itself until Save()!
  m_timerType     = m_timerInfoTag->GetTimerType();
  m_bIsRadio      = m_timerInfoTag->m_bIsRadio;
  m_bIsNewTimer   = m_timerInfoTag->m_state == PVR_TIMER_STATE_NEW;
  m_bStartAnytime = m_bIsNewTimer || m_timerInfoTag->IsStartAtAnyTime();
  m_bEndAnytime   = m_bIsNewTimer || m_timerInfoTag->IsEndAtAnyTime();
  m_bTimerActive  = m_bIsNewTimer || m_timerInfoTag->IsActive();
  m_strTitle      = m_timerInfoTag->m_strTitle;

  CDateTime now(CDateTime::GetCurrentDateTime());
  CDateTime startLocalTime = m_timerInfoTag->IsStartAtAnyTime() ? now : m_timerInfoTag->StartAsLocalTime();
  CDateTime endLocalTime   = m_timerInfoTag->IsEndAtAnyTime() ? now : m_timerInfoTag->EndAsLocalTime();
  startLocalTime.GetAsSystemTime(m_timerStartTime);
  endLocalTime.GetAsSystemTime(m_timerEndTime);
  m_timerStartTimeStr  = startLocalTime.GetAsLocalizedTime("", false);
  m_timerEndTimeStr    = endLocalTime.GetAsLocalizedTime("", false);
  m_iStartDay          = InitializeDay(startLocalTime);
  m_iEndDay            = InitializeDay(endLocalTime);
  m_iFirstDay          = InitializeDay(m_timerInfoTag->FirstDayAsLocalTime());

  m_strEpgSearchString = m_timerInfoTag->m_strEpgSearchString;

  if (m_bIsNewTimer && m_strEpgSearchString.empty() && (m_strTitle != g_localizeStrings.Get(19056)))
    m_strEpgSearchString = m_strTitle;

  m_bFullTextEpgSearch  = m_timerInfoTag->m_bFullTextEpgSearch;
  m_iWeekdays           = m_timerInfoTag->m_iWeekdays;

  if (m_bIsNewTimer && (m_iWeekdays == PVR_WEEKDAY_NONE))
    m_iWeekdays = PVR_WEEKDAY_ALLDAYS;

  m_iPreventDupEpisodes = m_timerInfoTag->m_iPreventDupEpisodes;
  m_iMarginStart        = m_timerInfoTag->m_iMarginStart;
  m_iMarginEnd          = m_timerInfoTag->m_iMarginEnd;
  m_iPriority           = m_timerInfoTag->m_iPriority;
  m_iLifetime           = m_timerInfoTag->m_iLifetime;
  m_strDirectory        = m_timerInfoTag->m_strDirectory;

  InitializeChannelsList();
  InitializeTypesList();

  // Channel
  m_channel = ChannelDescriptor();

  bool bAnyChannel(m_timerInfoTag->m_iClientChannelUid == PVR_INVALID_CHANNEL_UID);
  if (bAnyChannel)
  {
    auto it = m_channelEntries.find(ENTRY_ANY_CHANNEL);
    if (it != m_channelEntries.end())
      m_channel = it->second;
  }
  else
  {
    for (auto it = m_channelEntries.begin(); it != m_channelEntries.end(); ++it)
    {
      if ((it->second.channelUid == m_timerInfoTag->m_iClientChannelUid) &&
          (it->second.clientId == m_timerInfoTag->m_iClientId))
      {
        m_channel = it->second;
        break;
      }
    }
  }

  if (!bAnyChannel && (m_channel.channelUid == PVR_INVALID_CHANNEL_UID))
  {
    // As fallback, select first real channel entry.
    auto it = m_channelEntries.begin();
    while (it != m_channelEntries.end())
    {
      if (it->second.channelUid != PVR_INVALID_CHANNEL_UID)
      {
        m_channel = it->second;
        break;
      }
      ++it;
    }
  }

  if (!bAnyChannel && (m_channel.channelUid == PVR_INVALID_CHANNEL_UID))
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::SetTimer - Invalid channel uid!");

  if (!m_timerType)
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::SetTimer - No timer type!");
}

// virtual
void CGUIDialogPVRTimerSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();
  SetButtonLabels();
}

// virtual
void CGUIDialogPVRTimerSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  CSettingCategory *category = AddCategory("pvrtimersettings", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::InitializeSettings - Unable to add settings category");
    return;
  }

  CSettingGroup *group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::InitializeSettings - Unable to add settings group");
    return;
  }

  CSetting *setting = NULL;

  // Timer type
  setting = AddList(group, SETTING_TMR_TYPE, 803, 0, 0, TypesFiller, 803);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_TYPE);

  // Timer enabled/disabled
  setting = AddToggle(group, SETTING_TMR_ACTIVE, 19074, 0, m_bTimerActive);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_ACTIVE);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_ACTIVE);

  // Name
  setting = AddEdit(group, SETTING_TMR_NAME, 19075, 0, m_strTitle, true, false, 19097);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_NAME);

  // epg search string (only for epg-based repeating timers)
  setting = AddEdit(group, SETTING_TMR_EPGSEARCH, 804, 0, m_strEpgSearchString, true, false, 805);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_EPGSEARCH);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_EPGSEARCH);

  // epg fulltext search (only for epg-based repeating timers)
  setting = AddToggle(group, SETTING_TMR_FULLTEXT, 806, 0, m_bFullTextEpgSearch);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_FULLTEXT);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_FULLTEXT);

  // Channel
  setting = AddList(group, SETTING_TMR_CHANNEL, 19078, 0, 0, ChannelsFiller, 19078);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_CHANNEL);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_CHANNEL);

  // Days of week (only for repeating timers)
  std::vector<int> weekdaysPreselect;
  if (m_iWeekdays & PVR_WEEKDAY_MONDAY)
    weekdaysPreselect.push_back(PVR_WEEKDAY_MONDAY);
  if (m_iWeekdays & PVR_WEEKDAY_TUESDAY)
    weekdaysPreselect.push_back(PVR_WEEKDAY_TUESDAY);
  if (m_iWeekdays & PVR_WEEKDAY_WEDNESDAY)
    weekdaysPreselect.push_back(PVR_WEEKDAY_WEDNESDAY);
  if (m_iWeekdays & PVR_WEEKDAY_THURSDAY)
    weekdaysPreselect.push_back(PVR_WEEKDAY_THURSDAY);
  if (m_iWeekdays & PVR_WEEKDAY_FRIDAY)
    weekdaysPreselect.push_back(PVR_WEEKDAY_FRIDAY);
  if (m_iWeekdays & PVR_WEEKDAY_SATURDAY)
    weekdaysPreselect.push_back(PVR_WEEKDAY_SATURDAY);
  if (m_iWeekdays & PVR_WEEKDAY_SUNDAY)
    weekdaysPreselect.push_back(PVR_WEEKDAY_SUNDAY);

  setting = AddList(group, SETTING_TMR_WEEKDAYS, 19079, 0, weekdaysPreselect, WeekdaysFiller, 19079, 1);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_WEEKDAYS);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_WEEKDAYS);

  // "Any time" (only for repeating timers)
  setting = AddToggle(group, SETTING_TMR_ANYTIME, 810, 0, m_bStartAnytime);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_ANYTIME);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_ANYTIME);

  // Start day (day + month + year only, hours, minutes)
  setting = AddSpinner(group, SETTING_TMR_START_DAY, 19128, 0, m_iStartDay, DaysFiller);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_START_DAY);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_START_DAY);
  AddAnytimeDependentVisibilityCondition(setting, SETTING_TMR_START_DAY);

  // Start time (hours + minutes only, no day, month, year)
  setting = AddButton(group, SETTING_TMR_BEGIN, 19126, 0);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_BEGIN);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_BEGIN);
  AddAnytimeDependentVisibilityCondition(setting, SETTING_TMR_BEGIN);

  // End day (day + month + year only, hours, minutes)
  setting = AddSpinner(group, SETTING_TMR_END_DAY, 19129, 0, m_iEndDay, DaysFiller);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_END_DAY);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_END_DAY);
  AddAnytimeDependentVisibilityCondition(setting, SETTING_TMR_END_DAY);

  // End time (hours + minutes only, no day, month, year)
  setting = AddButton(group, SETTING_TMR_END, 19127, 0);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_END);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_END);
  AddAnytimeDependentVisibilityCondition(setting, SETTING_TMR_END);

  // First day (only for repeating timers)
  setting = AddSpinner(group, SETTING_TMR_FIRST_DAY, 19084, 0, m_iFirstDay, DaysFiller);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_FIRST_DAY);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_FIRST_DAY);

  // "Prevent duplicate episodes" (only for repeating timers)
  setting = AddList(group, SETTING_TMR_NEW_EPISODES, 812, 0, m_iPreventDupEpisodes, DupEpisodesFiller, 812);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_NEW_EPISODES);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_NEW_EPISODES);

  // Pre and post record time
  setting = AddSpinner(group, SETTING_TMR_BEGIN_PRE, 813, 0, 0, m_iMarginStart, 1, 60, 14044);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_BEGIN_PRE);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_BEGIN_PRE);

  setting = AddSpinner(group, SETTING_TMR_END_POST,  814, 0, 0, m_iMarginEnd,   1, 60, 14044);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_END_POST);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_END_POST);

  // Priority
  setting = AddSpinner(group, SETTING_TMR_PRIORITY, 19082, 0, m_iPriority, PrioritiesFiller);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_PRIORITY);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_PRIORITY);

  // Lifetime
  setting = AddSpinner(group, SETTING_TMR_LIFETIME, 19083, 0, m_iLifetime, LifetimesFiller);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_LIFETIME);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_LIFETIME);

  // Recording folder
  setting = AddEdit(group, SETTING_TMR_DIR, 19076, 0, m_strDirectory, true, false, 19104);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_DIR);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_DIR);
}

// virtual
void CGUIDialogPVRTimerSettings::OnSettingChanged(const CSetting *setting)
{
  if (setting == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::OnSettingChanged - No setting");
    return;
  }

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string &settingId = setting->GetId();

  if (settingId == SETTING_TMR_TYPE)
  {
    int idx = dynamic_cast<const CSettingInt*>(setting)->GetValue();
    auto entry = m_typeEntries.find(idx);
    if (entry != m_typeEntries.end())
    {
      m_timerType = entry->second;

      if (m_timerType->IsRepeating() && (m_iWeekdays == PVR_WEEKDAY_ALLDAYS))
        SetButtonLabels(); // update "Any day" vs. "Every day"
    }
    else
    {
      CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::OnSettingChanged - Unable to get 'type' value");
    }
  }
  else if (settingId == SETTING_TMR_ACTIVE)
  {
    m_bTimerActive = dynamic_cast<const CSettingBool*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_NAME)
  {
    m_strTitle = dynamic_cast<const CSettingString*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_EPGSEARCH)
  {
    m_strEpgSearchString = dynamic_cast<const CSettingString*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_FULLTEXT)
  {
    m_bFullTextEpgSearch = dynamic_cast<const CSettingBool*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_CHANNEL)
  {
    int idx = dynamic_cast<const CSettingInt*>(setting)->GetValue();
    auto entry = m_channelEntries.find(idx);
    if (entry != m_channelEntries.end())
    {
      m_channel = entry->second;
    }
    else
    {
      CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::OnSettingChanged - Unable to get 'type' value");
    }
  }
  else if (settingId == SETTING_TMR_WEEKDAYS)
  {
    const CSettingList *settingList = dynamic_cast<const CSettingList*>(setting);
    if (settingList->GetElementType() != SettingTypeInteger)
    {
      CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::OnSettingChanged - wrong weekdays element type");
      return;
    }
    int weekdays = 0;
    std::vector<CVariant> list = CSettingUtils::GetList(settingList);
    for (auto itValue = list.begin(); itValue != list.end(); ++itValue)
    {
      if (!itValue->isInteger())
      {
        CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::OnSettingChanged - wrong weekdays value type");
        return;
      }
      weekdays += static_cast<int>(itValue->asInteger());
    }
    m_iWeekdays = weekdays;
  }
  else if (settingId == SETTING_TMR_ANYTIME)
  {
    m_bStartAnytime = m_bEndAnytime = dynamic_cast<const CSettingBool*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_START_DAY)
  {
    m_iStartDay = dynamic_cast<const CSettingInt*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_END_DAY)
  {
    m_iEndDay = dynamic_cast<const CSettingInt*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_FIRST_DAY)
  {
    m_iFirstDay = dynamic_cast<const CSettingInt*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_NEW_EPISODES)
  {
    m_iPreventDupEpisodes = dynamic_cast<const CSettingInt*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_BEGIN_PRE)
  {
    m_iMarginStart = dynamic_cast<const CSettingInt*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_END_POST)
  {
    m_iMarginEnd = dynamic_cast<const CSettingInt*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_PRIORITY)
  {
    m_iPriority = dynamic_cast<const CSettingInt*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_LIFETIME)
  {
    m_iLifetime = dynamic_cast<const CSettingInt*>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_DIR)
  {
    m_strDirectory = dynamic_cast<const CSettingString*>(setting)->GetValue();
  }
}

// virtual
void CGUIDialogPVRTimerSettings::OnSettingAction(const CSetting *setting)
{
  if (setting == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::OnSettingAction - No setting");
    return;
  }

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string &settingId = setting->GetId();
  if (settingId == SETTING_TMR_BEGIN)
  {
    if (CGUIDialogNumeric::ShowAndGetTime(m_timerStartTime, g_localizeStrings.Get(14066)))
    {
      CDateTime timestart(m_timerStartTime);
      m_timerStartTimeStr = timestart.GetAsLocalizedTime("", false);
      SetButtonLabels();
    }
  }
  else if (settingId == SETTING_TMR_END)
  {
    if (CGUIDialogNumeric::ShowAndGetTime(m_timerEndTime, g_localizeStrings.Get(14066)))
    {
      CDateTime timeend(m_timerEndTime);
      m_timerEndTimeStr = timeend.GetAsLocalizedTime("", false);
      SetButtonLabels();
    }
  }
}

// virtual
void CGUIDialogPVRTimerSettings::Save()
{
  // Timer type
  m_timerInfoTag->SetTimerType(m_timerType);

  // Timer active/inactive
  m_timerInfoTag->m_state = m_bTimerActive ? PVR_TIMER_STATE_SCHEDULED : PVR_TIMER_STATE_DISABLED;

  // Name
  m_timerInfoTag->m_strTitle = m_strTitle;

  // epg search string (only for epg-based repeating timers)
  m_timerInfoTag->m_strEpgSearchString = m_strEpgSearchString;

  // epg fulltext search, instead of just title match. (only for epg-based repeating timers)
  m_timerInfoTag->m_bFullTextEpgSearch = m_bFullTextEpgSearch;

  // Channel
  CPVRChannelPtr channel(g_PVRChannelGroups->GetByUniqueID(m_channel.channelUid, m_channel.clientId));
  if (channel)
  {
    m_timerInfoTag->m_iClientChannelUid = channel->UniqueID();
    m_timerInfoTag->m_iClientId         = channel->ClientID();
    m_timerInfoTag->m_bIsRadio          = channel->IsRadio();
    m_timerInfoTag->m_iChannelNumber    = channel->ChannelNumber();

    m_timerInfoTag->UpdateChannel();
  }
  else
  {
    if (m_timerType->IsOnetime() || m_timerType->IsManual())
      CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::Save - No channel");

    m_timerInfoTag->m_iClientChannelUid = m_channel.channelUid;
    m_timerInfoTag->m_iClientId         = m_timerType->GetClientId();
  }

  // Begin and end time
  if (m_bStartAnytime && m_timerType->IsRepeatingEpgBased())
  {
    time_t time = 0;
    CDateTime datetime(time);
    m_timerInfoTag->SetStartFromLocalTime(datetime);
    m_timerInfoTag->SetEndFromLocalTime(datetime);
  }
  else
  {
    CDateTime now(CDateTime::GetCurrentDateTime());
    tm time_now; now.GetAsTm(time_now);

    CDateTime timestart(m_timerStartTime);
    tm time_start; timestart.GetAsTm(time_start);

    CDateTime timeend(m_timerEndTime);

    int time_diff = time_start.tm_yday - time_now.tm_yday;
    if (time_diff < 0)
      time_diff = 365;

    CDateTime newStart(timestart + CDateTimeSpan(m_iStartDay - time_diff, 0, 0, -timestart.GetSecond()));
    CDateTime newEnd  (timeend   + CDateTimeSpan(m_iEndDay   - time_diff, 0, 0, -timeend.GetSecond()));

    // add a day to end time if end time is before start time
    if (newEnd < newStart)
      newEnd += CDateTimeSpan(1, 0, 0, 0);

    m_timerInfoTag->SetStartFromLocalTime(newStart);
    m_timerInfoTag->SetEndFromLocalTime(newEnd);
  }

  // Days of week (only for repeating timers)
  if (m_timerType->IsRepeating())
    m_timerInfoTag->m_iWeekdays = m_iWeekdays;
  else
    m_timerInfoTag->m_iWeekdays = PVR_WEEKDAY_NONE;

  // First day (only for repeating timers)
  CDateTime newFirstDay;
  if (m_iFirstDay > 0)
    newFirstDay = CDateTime::GetCurrentDateTime() + CDateTimeSpan(m_iFirstDay - 1, 0, 0, 0);

  m_timerInfoTag->SetFirstDayFromLocalTime(newFirstDay);

  // "New episodes only" (only for repeating timers)
  m_timerInfoTag->m_iPreventDupEpisodes = m_iPreventDupEpisodes;

  // Pre and post record time
  m_timerInfoTag->m_iMarginStart = m_iMarginStart;
  m_timerInfoTag->m_iMarginEnd   = m_iMarginEnd;

  // Priority
  m_timerInfoTag->m_iPriority = m_iPriority;

  // Lifetime
  m_timerInfoTag->m_iLifetime = m_iLifetime;

  // Recording folder
  m_timerInfoTag->m_strDirectory = m_strDirectory;

  // Set the timer's title to the channel name if it's empty or 'New Timer'
  if (channel && (m_strTitle.empty() || m_strTitle == g_localizeStrings.Get(19056)))
    m_timerInfoTag->m_strTitle = channel->ChannelName();

  // Update summary
  m_timerInfoTag->UpdateSummary();
}

void CGUIDialogPVRTimerSettings::SetButtonLabels()
{
  // timer start time
  BaseSettingControlPtr settingControl = GetSettingControl(SETTING_TMR_BEGIN);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
  {
    if (!m_bIsNewTimer && m_bStartAnytime)
      SET_CONTROL_LABEL2(settingControl->GetID(), g_localizeStrings.Get(19161)); // "any time"
    else
      SET_CONTROL_LABEL2(settingControl->GetID(), m_timerStartTimeStr);
  }

  // timer end time
  settingControl = GetSettingControl(SETTING_TMR_END);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
  {
    if (!m_bIsNewTimer && m_bEndAnytime)
      SET_CONTROL_LABEL2(settingControl->GetID(), g_localizeStrings.Get(19161)); // "any time"
    else
      SET_CONTROL_LABEL2(settingControl->GetID(), m_timerEndTimeStr);
  }

  // weekdays
  settingControl = GetSettingControl(SETTING_TMR_WEEKDAYS);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
    SET_CONTROL_LABEL2(settingControl->GetID(),
                       CPVRTimerInfoTag::GetWeekdaysString(
                        m_iWeekdays, m_timerType->IsEpgBased(), true));
}

void CGUIDialogPVRTimerSettings::AddCondition(
  CSetting *setting, const std::string &identifier, SettingConditionCheck condition,
  SettingDependencyType depType, const std::string &settingId)
{
  m_settingsManager->AddCondition(identifier, condition, this);
  CSettingDependency dep(depType, m_settingsManager);
  dep.And()->Add(
    CSettingDependencyConditionPtr(
      new CSettingDependencyCondition(identifier, "true", settingId, false, m_settingsManager)));
  SettingDependencies deps(setting->GetDependencies());
  deps.push_back(dep);
  setting->SetDependencies(deps);
}

// static
int CGUIDialogPVRTimerSettings::InitializeDay(const CDateTime &datetime)
{
  CDateTime now(CDateTime::GetCurrentDateTime());
  tm time_now; now.GetAsTm(time_now);

  CDateTime dt(datetime);
  if (now > dt)
    dt = now;

  tm time_start; dt.GetAsTm(time_start);
  int ret = time_start.tm_yday - time_now.tm_yday;
  if (ret < 0)
    ret += 365;

  return ret;
}

void CGUIDialogPVRTimerSettings::InitializeTypesList()
{
  m_typeEntries.clear();

  int idx = 0;
  const std::vector<CPVRTimerTypePtr> types(CPVRTimerType::GetAllTypes());
  for (auto it = types.begin(); it != types.end(); ++it)
  {
    // Read-only timers cannot be created using this dialog.
    // But the dialog can act as a viewer for read-only types (m_bIsNewTimer is false in this case)
    if ((*it)->IsReadOnly() && m_bIsNewTimer)
      continue;

    // For new timers, skip one time epg-based types. Those cannot be created using this dialog (yet).
    if (m_bIsNewTimer && (*it)->IsOnetimeEpgBased())
      continue;

    m_typeEntries.insert(std::make_pair(idx++, *it));
  }
}

void CGUIDialogPVRTimerSettings::InitializeChannelsList()
{
  m_channelEntries.clear();

  CFileItemList channelsList;
  g_PVRChannelGroups->GetGroupAll(m_bIsRadio)->GetMembers(channelsList);

  for (int i = 0; i < channelsList.Size(); ++i)
  {
    const CPVRChannelPtr channel(channelsList[i]->GetPVRChannelInfoTag());
    std::string channelDescription(
      StringUtils::Format("%i %s", channel->ChannelNumber(), channel->ChannelName().c_str()));
    m_channelEntries.insert(
      std::make_pair(i, ChannelDescriptor(channel->UniqueID(), channel->ClientID(), channelDescription)));
  }

  // Add special "any channel" entry (used for epg-based repeating timers).
  m_channelEntries.insert(
    std::make_pair(
      ENTRY_ANY_CHANNEL, ChannelDescriptor(PVR_INVALID_CHANNEL_UID, 0, g_localizeStrings.Get(809))));
}

// static
void CGUIDialogPVRTimerSettings::TypesFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::TypesFiller - No dialog");
    return;
  }

  list.clear();
  current = 0;

  bool foundCurrent(false);
  for (auto it = pThis->m_typeEntries.begin(); it != pThis->m_typeEntries.end(); ++it)
  {
    list.push_back(std::make_pair(it->second->GetDescription(), it->first));

    if (!foundCurrent && (*(pThis->m_timerType) == *(it->second)))
    {
      current = it->first;
      foundCurrent = true;
    }
  }
}

// static
void CGUIDialogPVRTimerSettings::ChannelsFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::ChannelsFiller - No dialog");
    return;
  }

  list.clear();
  current = 0;

  bool foundCurrent(false);
  for (auto it = pThis->m_channelEntries.begin(); it != pThis->m_channelEntries.end(); ++it)
  {
    if (it->first == ENTRY_ANY_CHANNEL)
    {
      // For repeating epg-based timers only, add an "any channel" entry.
      if (pThis->m_timerType->IsRepeatingEpgBased())
        list.push_back(std::make_pair(it->second.description, it->first));
      else
        continue;
    }
    else
    {
      // Only include channels supplied by the currently active PVR client.
      if (it->second.clientId == pThis->m_timerType->GetClientId())
        list.push_back(std::make_pair(it->second.description, it->first));
    }

    if (!foundCurrent && (pThis->m_channel == it->second))
    {
      current = it->first;
      foundCurrent = true;
    }
  }
}

// static
void CGUIDialogPVRTimerSettings::DaysFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::DaysFiller - No dialog");
    return;
  }

  list.clear();
  current = 0;

  CDateTime time(CDateTime::GetCurrentDateTime());
  CDateTime yesterdayPlusOneYear(
    time.GetYear() + 1, time.GetMonth(), time.GetDay() - 1, time.GetHour(), time.GetMinute(), time.GetSecond());

  while (time <= yesterdayPlusOneYear)
  {
    list.push_back(std::make_pair(time.GetAsLocalizedDate(), list.size()));
    time += CDateTimeSpan(1, 0, 0, 0);
  }

  if (setting->GetId() == SETTING_TMR_FIRST_DAY)
    current = pThis->m_iFirstDay;
  else if (setting->GetId() == SETTING_TMR_START_DAY)
    current = pThis->m_iStartDay;
  else
    current = pThis->m_iEndDay;
}

// static
void CGUIDialogPVRTimerSettings::DupEpisodesFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::DupEpisodesFiller - No dialog");
    return;
  }

  list.clear();
  pThis->m_timerType->GetPreventDuplicateEpisodesValues(list);
  current = pThis->m_iPreventDupEpisodes;
}

// static
void CGUIDialogPVRTimerSettings::WeekdaysFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::WeekdaysFiller - No dialog");
    return;
  }

  list.clear();
  list.push_back(std::make_pair(g_localizeStrings.Get(831), PVR_WEEKDAY_MONDAY));    // "Mondays"
  list.push_back(std::make_pair(g_localizeStrings.Get(832), PVR_WEEKDAY_TUESDAY));   // "Tuesdays"
  list.push_back(std::make_pair(g_localizeStrings.Get(833), PVR_WEEKDAY_WEDNESDAY)); // "Wednesdays"
  list.push_back(std::make_pair(g_localizeStrings.Get(834), PVR_WEEKDAY_THURSDAY));  // "Thursdays"
  list.push_back(std::make_pair(g_localizeStrings.Get(835), PVR_WEEKDAY_FRIDAY));    // "Fridays"
  list.push_back(std::make_pair(g_localizeStrings.Get(836), PVR_WEEKDAY_SATURDAY));  // "Saturdays"
  list.push_back(std::make_pair(g_localizeStrings.Get(837), PVR_WEEKDAY_SUNDAY));    // "Sundays"

  current = pThis->m_iWeekdays;
}

// static
void CGUIDialogPVRTimerSettings::PrioritiesFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::PrioritiesFiller - No dialog");
    return;
  }

  list.clear();
  pThis->m_timerType->GetPriorityValues(list);
  current = pThis->m_iPriority;
}

// static
void CGUIDialogPVRTimerSettings::LifetimesFiller(
  const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::LifetimesFiller - No dialog");
    return;
  }

  list.clear();
  pThis->m_timerType->GetLifetimeValues(list);
  current = pThis->m_iLifetime;
}

void CGUIDialogPVRTimerSettings::AddTypeDependentEnableCondition(CSetting *setting, const std::string &identifier)
{
  // Enable setting depending on read-only attribute of the selected timer type
  std::string id(identifier);
  id.append(TYPE_DEP_ENABLE_COND_ID_POSTFIX);
  AddCondition(setting, id, TypeReadOnlyCondition, SettingDependencyTypeEnable, SETTING_TMR_TYPE);
}

// static
bool CGUIDialogPVRTimerSettings::TypeReadOnlyCondition(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  if (setting == NULL)
    return false;

  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::TypeReadOnlyCondition - No dialog");
    return false;
  }

  if (!StringUtils::EqualsNoCase(value, "true"))
    return false;

  std::string cond(condition);
  cond.erase(cond.find(TYPE_DEP_ENABLE_COND_ID_POSTFIX));

  // For existing timers, disable type selector (view/edit of existing timer).
  if (!pThis->m_bIsNewTimer)
  {
    if (cond == SETTING_TMR_TYPE)
      return false;
  }

  // If only one type is available, disable type selector.
  if (pThis->m_typeEntries.size() == 1)
  {
    if (cond == SETTING_TMR_TYPE)
      return false;
  }

  // For existing one time epg-based timers, disable editing of epg-filled data.
  if (!pThis->m_bIsNewTimer && pThis->m_timerType->IsOnetimeEpgBased())
  {
    if ((cond == SETTING_TMR_NAME)      ||
        (cond == SETTING_TMR_CHANNEL)   ||
        (cond == SETTING_TMR_START_DAY) ||
        (cond == SETTING_TMR_END_DAY)   ||
        (cond == SETTING_TMR_BEGIN)     ||
        (cond == SETTING_TMR_END))
      return false;
  }

  // Let the PVR client decide...
  int idx = dynamic_cast<const CSettingInt*>(setting)->GetValue();
  auto entry = pThis->m_typeEntries.find(idx);
  if (entry != pThis->m_typeEntries.end())
  {
    return !entry->second->IsReadOnly();
  }
  else
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::TypeReadOnlyCondition - No type entry");
  }
  return false;
}

void CGUIDialogPVRTimerSettings::AddTypeDependentVisibilityCondition(CSetting *setting, const std::string &identifier)
{
  // Show or hide setting depending on attributes of the selected timer type
  std::string id(identifier);
  id.append(TYPE_DEP_VISIBI_COND_ID_POSTFIX);
  AddCondition(setting, id, TypeSupportsCondition, SettingDependencyTypeVisible, SETTING_TMR_TYPE);
}

// static
bool CGUIDialogPVRTimerSettings::TypeSupportsCondition(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  if (setting == NULL)
    return false;

  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::TypeSupportsCondition - No dialog");
    return false;
  }

  if (!StringUtils::EqualsNoCase(value, "true"))
    return false;

  int idx = dynamic_cast<const CSettingInt*>(setting)->GetValue();
  auto entry = pThis->m_typeEntries.find(idx);
  if (entry != pThis->m_typeEntries.end())
  {
    std::string cond(condition);
    cond.erase(cond.find(TYPE_DEP_VISIBI_COND_ID_POSTFIX));

    if (cond == SETTING_TMR_EPGSEARCH)
      return entry->second->SupportsEpgTitleMatch() || entry->second->SupportsEpgFulltextMatch();
    else if (cond == SETTING_TMR_FULLTEXT)
      return entry->second->SupportsEpgFulltextMatch();
    else if (cond == SETTING_TMR_ACTIVE)
      return entry->second->SupportsEnableDisable();
    else if (cond == SETTING_TMR_CHANNEL)
      return entry->second->SupportsChannels();
    else if (cond == SETTING_TMR_ANYTIME)
      return entry->second->IsRepeatingEpgBased();
    else if (cond == SETTING_TMR_START_DAY ||
             cond == SETTING_TMR_END_DAY)
      return entry->second->SupportsStartEndTime() && !entry->second->IsRepeating();
    else if ((cond == SETTING_TMR_BEGIN) ||
             (cond == SETTING_TMR_END))
      return entry->second->SupportsStartEndTime();
    else if (cond == SETTING_TMR_WEEKDAYS)
      return entry->second->SupportsWeekdays();
    else if (cond == SETTING_TMR_FIRST_DAY)
      return entry->second->SupportsFirstDay();
    else if (cond == SETTING_TMR_NEW_EPISODES)
      return entry->second->SupportsRecordOnlyNewEpisodes();
    else if ((cond == SETTING_TMR_BEGIN_PRE) ||
             (cond == SETTING_TMR_END_POST))
      return entry->second->SupportsStartEndMargin();
    else if (cond == SETTING_TMR_PRIORITY)
      return entry->second->SupportsPriority();
    else if (cond == SETTING_TMR_LIFETIME)
      return entry->second->SupportsLifetime();
    else if (cond == SETTING_TMR_DIR)
      return entry->second->SupportsRecordingFolders();
    else
      CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::TypeSupportsCondition - Unknown condition");
  }
  else
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::TypeSupportsCondition - No type entry");
  }
  return false;
}

void CGUIDialogPVRTimerSettings::AddAnytimeDependentVisibilityCondition(CSetting *setting, const std::string &identifier)
{
  // Show or hide setting depending on value of setting "any time"
  std::string id(identifier);
  id.append(ANYTIME_DEP_ENABLE_COND_ID_POSTFIX);
  AddCondition(setting, id, AnytimeSetCondition, SettingDependencyTypeVisible, SETTING_TMR_ANYTIME);
}

// static
bool CGUIDialogPVRTimerSettings::AnytimeSetCondition(const std::string &condition, const std::string &value, const CSetting *setting, void *data)
{
  if (setting == NULL)
    return false;

  CGUIDialogPVRTimerSettings *pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogPVRTimerSettings::AnytimeSetCondition - No dialog");
    return false;
  }

  if (!StringUtils::EqualsNoCase(value, "true"))
    return false;

  // "any time" setting is only relevant for repeating epg-based timers.
  if (!pThis->m_timerType->IsRepeatingEpgBased())
    return true;

  std::string cond(condition);
  cond.erase(cond.find(ANYTIME_DEP_ENABLE_COND_ID_POSTFIX));

  if ((cond == SETTING_TMR_START_DAY) ||
      (cond == SETTING_TMR_END_DAY)   ||
      (cond == SETTING_TMR_BEGIN)     ||
      (cond == SETTING_TMR_END))
  {
    bool bAnytime = dynamic_cast<const CSettingBool*>(setting)->GetValue();
    return !bAnytime;
  }
  return false;
}
