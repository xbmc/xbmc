/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogPVRTimerSettings.h"

#include "ServiceBroker.h"
#include "dialogs/GUIDialogNumeric.h"
#include "guilib/GUIMessage.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "pvr/PVRManager.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/addons/PVRClients.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimerType.h"
#include "settings/SettingUtils.h"
#include "settings/dialogs/GUIDialogSettingsBase.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingsManager.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace PVR;
using namespace KODI::MESSAGING;

#define SETTING_TMR_TYPE "timer.type"
#define SETTING_TMR_ACTIVE "timer.active"
#define SETTING_TMR_NAME "timer.name"
#define SETTING_TMR_EPGSEARCH "timer.epgsearch"
#define SETTING_TMR_FULLTEXT "timer.fulltext"
#define SETTING_TMR_CHANNEL "timer.channel"
#define SETTING_TMR_START_ANYTIME "timer.startanytime"
#define SETTING_TMR_END_ANYTIME "timer.endanytime"
#define SETTING_TMR_START_DAY "timer.startday"
#define SETTING_TMR_END_DAY "timer.endday"
#define SETTING_TMR_BEGIN "timer.begin"
#define SETTING_TMR_END "timer.end"
#define SETTING_TMR_WEEKDAYS "timer.weekdays"
#define SETTING_TMR_FIRST_DAY "timer.firstday"
#define SETTING_TMR_NEW_EPISODES "timer.newepisodes"
#define SETTING_TMR_BEGIN_PRE "timer.startmargin"
#define SETTING_TMR_END_POST "timer.endmargin"
#define SETTING_TMR_PRIORITY "timer.priority"
#define SETTING_TMR_LIFETIME "timer.lifetime"
#define SETTING_TMR_MAX_REC "timer.maxrecordings"
#define SETTING_TMR_DIR "timer.directory"
#define SETTING_TMR_REC_GROUP "timer.recgroup"

#define TYPE_DEP_VISIBI_COND_ID_POSTFIX "visibi.typedep"
#define TYPE_DEP_ENABLE_COND_ID_POSTFIX "enable.typedep"
#define CHANNEL_DEP_VISIBI_COND_ID_POSTFIX "visibi.channeldep"
#define START_ANYTIME_DEP_VISIBI_COND_ID_POSTFIX "visibi.startanytimedep"
#define END_ANYTIME_DEP_VISIBI_COND_ID_POSTFIX "visibi.endanytimedep"

CGUIDialogPVRTimerSettings::CGUIDialogPVRTimerSettings()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_PVR_TIMER_SETTING, "DialogSettings.xml"),
    m_iWeekdays(PVR_WEEKDAY_NONE)
{
  m_loadType = LOAD_EVERY_TIME;
}

CGUIDialogPVRTimerSettings::~CGUIDialogPVRTimerSettings() = default;

bool CGUIDialogPVRTimerSettings::CanBeActivated() const
{
  if (!m_timerInfoTag)
  {
    CLog::LogF(LOGERROR, "No timer info tag");
    return false;
  }
  return true;
}

void CGUIDialogPVRTimerSettings::SetTimer(const std::shared_ptr<CPVRTimerInfoTag>& timer)
{
  if (!timer)
  {
    CLog::LogF(LOGERROR, "No timer given");
    return;
  }

  m_timerInfoTag = timer;

  // Copy data we need from tag. Do not modify the tag itself until Save()!
  m_timerType = m_timerInfoTag->GetTimerType();
  m_bIsRadio = m_timerInfoTag->m_bIsRadio;
  m_bIsNewTimer = m_timerInfoTag->m_iClientIndex == PVR_TIMER_NO_CLIENT_INDEX;
  m_bTimerActive = m_bIsNewTimer || !m_timerType->SupportsEnableDisable() ||
                   !(m_timerInfoTag->m_state == PVR_TIMER_STATE_DISABLED);
  m_bStartAnyTime =
      m_bIsNewTimer || !m_timerType->SupportsStartAnyTime() || m_timerInfoTag->m_bStartAnyTime;
  m_bEndAnyTime =
      m_bIsNewTimer || !m_timerType->SupportsEndAnyTime() || m_timerInfoTag->m_bEndAnyTime;
  m_strTitle = m_timerInfoTag->m_strTitle;

  m_startLocalTime = m_timerInfoTag->StartAsLocalTime();
  m_endLocalTime = m_timerInfoTag->EndAsLocalTime();

  m_timerStartTimeStr = m_startLocalTime.GetAsLocalizedTime("", false);
  m_timerEndTimeStr = m_endLocalTime.GetAsLocalizedTime("", false);
  m_firstDayLocalTime = m_timerInfoTag->FirstDayAsLocalTime();

  m_strEpgSearchString = m_timerInfoTag->m_strEpgSearchString;
  if (!m_bIsNewTimer && m_strEpgSearchString.empty())
    m_strEpgSearchString = m_strTitle;

  m_bFullTextEpgSearch = m_timerInfoTag->m_bFullTextEpgSearch;

  m_iWeekdays = m_timerInfoTag->m_iWeekdays;
  if ((m_bIsNewTimer || !m_timerType->SupportsWeekdays()) && m_iWeekdays == PVR_WEEKDAY_NONE)
    m_iWeekdays = PVR_WEEKDAY_ALLDAYS;

  m_iPreventDupEpisodes = m_timerInfoTag->m_iPreventDupEpisodes;
  m_iMarginStart = m_timerInfoTag->m_iMarginStart;
  m_iMarginEnd = m_timerInfoTag->m_iMarginEnd;
  m_iPriority = m_timerInfoTag->m_iPriority;
  m_iLifetime = m_timerInfoTag->m_iLifetime;
  m_iMaxRecordings = m_timerInfoTag->m_iMaxRecordings;

  if (m_bIsNewTimer && m_timerInfoTag->m_strDirectory.empty() &&
      m_timerType->SupportsRecordingFolders())
    m_strDirectory = m_strTitle;
  else
    m_strDirectory = m_timerInfoTag->m_strDirectory;

  m_iRecordingGroup = m_timerInfoTag->m_iRecordingGroup;

  InitializeChannelsList();
  InitializeTypesList();

  // Channel
  m_channel = ChannelDescriptor();

  if (m_timerInfoTag->m_iClientChannelUid == PVR_CHANNEL_INVALID_UID)
  {
    if (m_timerType->SupportsAnyChannel())
    {
      // Select first matching "Any channel" entry.
      const auto it = std::find_if(m_channelEntries.cbegin(), m_channelEntries.cend(),
                                   [this](const auto& channel) {
                                     return channel.second.channelUid == PVR_CHANNEL_INVALID_UID &&
                                            channel.second.clientId == m_timerInfoTag->m_iClientId;
                                   });

      if (it != m_channelEntries.cend())
      {
        m_channel = (*it).second;
      }
      else
      {
        CLog::LogF(LOGERROR, "Unable to map PVR_CHANNEL_INVALID_UID to channel entry!");
      }
    }
    else if (m_bIsNewTimer)
    {
      // Select first matching regular (not "Any channel") entry.
      const auto it = std::find_if(m_channelEntries.cbegin(), m_channelEntries.cend(),
                                   [this](const auto& channel) {
                                     return channel.second.channelUid != PVR_CHANNEL_INVALID_UID &&
                                            channel.second.clientId == m_timerInfoTag->m_iClientId;
                                   });

      if (it != m_channelEntries.cend())
      {
        m_channel = (*it).second;
      }
      else
      {
        CLog::LogF(LOGERROR, "Unable to map PVR_CHANNEL_INVALID_UID to channel entry!");
      }
    }
  }
  else
  {
    // Find matching channel entry
    const auto it = std::find_if(
        m_channelEntries.cbegin(), m_channelEntries.cend(), [this](const auto& channel) {
          return channel.second.channelUid == m_timerInfoTag->m_iClientChannelUid &&
                 channel.second.clientId == m_timerInfoTag->m_iClientId;
        });

    if (it != m_channelEntries.cend())
    {
      m_channel = (*it).second;
    }
    else
    {
      CLog::LogF(LOGERROR, "Unable to map channel uid to channel entry!");
    }
  }
}

void CGUIDialogPVRTimerSettings::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();
  SetHeading(19065);
  SET_CONTROL_HIDDEN(CONTROL_SETTINGS_CUSTOM_BUTTON);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 186);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222);
  SetButtonLabels();
}

void CGUIDialogPVRTimerSettings::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  const std::shared_ptr<CSettingCategory> category = AddCategory("pvrtimersettings", -1);
  if (category == NULL)
  {
    CLog::LogF(LOGERROR, "Unable to add settings category");
    return;
  }

  const std::shared_ptr<CSettingGroup> group = AddGroup(category);
  if (group == NULL)
  {
    CLog::LogF(LOGERROR, "Unable to add settings group");
    return;
  }

  std::shared_ptr<CSetting> setting = NULL;

  // Timer type
  bool useDetails = false;
  bool foundClientSupportingTimers = false;

  const CPVRClientMap clients = CServiceBroker::GetPVRManager().Clients()->GetCreatedClients();
  for (const auto& client : clients)
  {
    if (client.second->GetClientCapabilities().SupportsTimers())
    {
      if (foundClientSupportingTimers)
      {
        // found second client supporting timers, use detailed timer type list layout
        useDetails = true;
        break;
      }
      foundClientSupportingTimers = true;
    }
  }

  setting = AddList(group, SETTING_TMR_TYPE, 803, SettingLevel::Basic, 0, TypesFiller, 803, true,
                    -1, useDetails);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_TYPE);

  // Timer enabled/disabled
  setting = AddToggle(group, SETTING_TMR_ACTIVE, 305, SettingLevel::Basic, m_bTimerActive);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_ACTIVE);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_ACTIVE);

  // Name
  setting =
      AddEdit(group, SETTING_TMR_NAME, 19075, SettingLevel::Basic, m_strTitle, true, false, 19097);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_NAME);

  // epg search string (only for epg-based timer rules)
  setting = AddEdit(group, SETTING_TMR_EPGSEARCH, 804, SettingLevel::Basic, m_strEpgSearchString,
                    true, false, 805);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_EPGSEARCH);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_EPGSEARCH);

  // epg fulltext search (only for epg-based timer rules)
  setting = AddToggle(group, SETTING_TMR_FULLTEXT, 806, SettingLevel::Basic, m_bFullTextEpgSearch);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_FULLTEXT);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_FULLTEXT);

  // Channel
  setting =
      AddList(group, SETTING_TMR_CHANNEL, 19078, SettingLevel::Basic, 0, ChannelsFiller, 19078);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_CHANNEL);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_CHANNEL);

  // Days of week (only for timer rules)
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

  setting = AddList(group, SETTING_TMR_WEEKDAYS, 19079, SettingLevel::Basic, weekdaysPreselect,
                    WeekdaysFiller, 19079, 1, -1, true, -1, WeekdaysValueFormatter);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_WEEKDAYS);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_WEEKDAYS);

  // "Start any time" (only for timer rules)
  setting = AddToggle(group, SETTING_TMR_START_ANYTIME, 810, SettingLevel::Basic, m_bStartAnyTime);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_START_ANYTIME);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_START_ANYTIME);

  // Start day (day + month + year only, no hours, minutes)
  setting = AddSpinner(group, SETTING_TMR_START_DAY, 19128, SettingLevel::Basic,
                       GetDateAsIndex(m_startLocalTime), DaysFiller);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_START_DAY);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_START_DAY);
  AddStartAnytimeDependentVisibilityCondition(setting, SETTING_TMR_START_DAY);

  // Start time (hours + minutes only, no day, month, year)
  setting = AddButton(group, SETTING_TMR_BEGIN, 19126, SettingLevel::Basic);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_BEGIN);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_BEGIN);
  AddStartAnytimeDependentVisibilityCondition(setting, SETTING_TMR_BEGIN);

  // "End any time" (only for timer rules)
  setting = AddToggle(group, SETTING_TMR_END_ANYTIME, 817, SettingLevel::Basic, m_bEndAnyTime);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_END_ANYTIME);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_END_ANYTIME);

  // End day (day + month + year only, no hours, minutes)
  setting = AddSpinner(group, SETTING_TMR_END_DAY, 19129, SettingLevel::Basic,
                       GetDateAsIndex(m_endLocalTime), DaysFiller);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_END_DAY);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_END_DAY);
  AddEndAnytimeDependentVisibilityCondition(setting, SETTING_TMR_END_DAY);

  // End time (hours + minutes only, no day, month, year)
  setting = AddButton(group, SETTING_TMR_END, 19127, SettingLevel::Basic);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_END);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_END);
  AddEndAnytimeDependentVisibilityCondition(setting, SETTING_TMR_END);

  // First day (only for timer rules)
  setting = AddSpinner(group, SETTING_TMR_FIRST_DAY, 19084, SettingLevel::Basic,
                       GetDateAsIndex(m_firstDayLocalTime), DaysFiller);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_FIRST_DAY);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_FIRST_DAY);

  // "Prevent duplicate episodes" (only for timer rules)
  setting = AddList(group, SETTING_TMR_NEW_EPISODES, 812, SettingLevel::Basic,
                    m_iPreventDupEpisodes, DupEpisodesFiller, 812);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_NEW_EPISODES);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_NEW_EPISODES);

  // Pre and post record time
  setting = AddList(group, SETTING_TMR_BEGIN_PRE, 813, SettingLevel::Basic, m_iMarginStart,
                    MarginTimeFiller, 813);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_BEGIN_PRE);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_BEGIN_PRE);

  setting = AddList(group, SETTING_TMR_END_POST, 814, SettingLevel::Basic, m_iMarginEnd,
                    MarginTimeFiller, 814);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_END_POST);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_END_POST);

  // Priority
  setting = AddList(group, SETTING_TMR_PRIORITY, 19082, SettingLevel::Basic, m_iPriority,
                    PrioritiesFiller, 19082);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_PRIORITY);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_PRIORITY);

  // Lifetime
  setting = AddList(group, SETTING_TMR_LIFETIME, 19083, SettingLevel::Basic, m_iLifetime,
                    LifetimesFiller, 19083);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_LIFETIME);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_LIFETIME);

  // MaxRecordings
  setting = AddList(group, SETTING_TMR_MAX_REC, 818, SettingLevel::Basic, m_iMaxRecordings,
                    MaxRecordingsFiller, 818);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_MAX_REC);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_MAX_REC);

  // Recording folder
  setting = AddEdit(group, SETTING_TMR_DIR, 19076, SettingLevel::Basic, m_strDirectory, true, false,
                    19104);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_DIR);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_DIR);

  // Recording group
  setting = AddList(group, SETTING_TMR_REC_GROUP, 811, SettingLevel::Basic, m_iRecordingGroup,
                    RecordingGroupFiller, 811);
  AddTypeDependentVisibilityCondition(setting, SETTING_TMR_REC_GROUP);
  AddTypeDependentEnableCondition(setting, SETTING_TMR_REC_GROUP);
}

int CGUIDialogPVRTimerSettings::GetWeekdaysFromSetting(const SettingConstPtr& setting)
{
  std::shared_ptr<const CSettingList> settingList =
      std::static_pointer_cast<const CSettingList>(setting);
  if (settingList->GetElementType() != SettingType::Integer)
  {
    CLog::LogF(LOGERROR, "Wrong weekdays element type");
    return 0;
  }
  int weekdays = 0;
  std::vector<CVariant> list = CSettingUtils::GetList(settingList);
  for (const auto& value : list)
  {
    if (!value.isInteger())
    {
      CLog::LogF(LOGERROR, "Wrong weekdays value type");
      return 0;
    }
    weekdays += static_cast<int>(value.asInteger());
  }

  return weekdays;
}

void CGUIDialogPVRTimerSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
  {
    CLog::LogF(LOGERROR, "No setting");
    return;
  }

  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  const std::string& settingId = setting->GetId();

  if (settingId == SETTING_TMR_TYPE)
  {
    int idx = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    const auto it = m_typeEntries.find(idx);
    if (it != m_typeEntries.end())
    {
      m_timerType = it->second;

      // reset certain settings to the defaults of the new timer type

      if (m_timerType->SupportsPriority())
        m_iPriority = m_timerType->GetPriorityDefault();

      if (m_timerType->SupportsLifetime())
        m_iLifetime = m_timerType->GetLifetimeDefault();

      if (m_timerType->SupportsMaxRecordings())
        m_iMaxRecordings = m_timerType->GetMaxRecordingsDefault();

      if (m_timerType->SupportsRecordingGroup())
        m_iRecordingGroup = m_timerType->GetRecordingGroupDefault();

      if (m_timerType->SupportsRecordOnlyNewEpisodes())
        m_iPreventDupEpisodes = m_timerType->GetPreventDuplicateEpisodesDefault();

      if (m_timerType->IsTimerRule() && (m_iWeekdays == PVR_WEEKDAY_ALLDAYS))
        SetButtonLabels(); // update "Any day" vs. "Every day"
    }
    else
    {
      CLog::LogF(LOGERROR, "Unable to get 'type' value");
    }
  }
  else if (settingId == SETTING_TMR_ACTIVE)
  {
    m_bTimerActive = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_NAME)
  {
    m_strTitle = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_EPGSEARCH)
  {
    m_strEpgSearchString = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_FULLTEXT)
  {
    m_bFullTextEpgSearch = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_CHANNEL)
  {
    int idx = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    const auto it = m_channelEntries.find(idx);
    if (it != m_channelEntries.end())
    {
      m_channel = it->second;
    }
    else
    {
      CLog::LogF(LOGERROR, "Unable to get 'type' value");
    }
  }
  else if (settingId == SETTING_TMR_WEEKDAYS)
  {
    m_iWeekdays = GetWeekdaysFromSetting(setting);
  }
  else if (settingId == SETTING_TMR_START_ANYTIME)
  {
    m_bStartAnyTime = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_END_ANYTIME)
  {
    m_bEndAnyTime = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_START_DAY)
  {
    SetDateFromIndex(m_startLocalTime,
                     std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
  }
  else if (settingId == SETTING_TMR_END_DAY)
  {
    SetDateFromIndex(m_endLocalTime,
                     std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
  }
  else if (settingId == SETTING_TMR_FIRST_DAY)
  {
    SetDateFromIndex(m_firstDayLocalTime,
                     std::static_pointer_cast<const CSettingInt>(setting)->GetValue());
  }
  else if (settingId == SETTING_TMR_NEW_EPISODES)
  {
    m_iPreventDupEpisodes = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_BEGIN_PRE)
  {
    m_iMarginStart = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_END_POST)
  {
    m_iMarginEnd = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_PRIORITY)
  {
    m_iPriority = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_LIFETIME)
  {
    m_iLifetime = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_MAX_REC)
  {
    m_iMaxRecordings = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_DIR)
  {
    m_strDirectory = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  }
  else if (settingId == SETTING_TMR_REC_GROUP)
  {
    m_iRecordingGroup = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  }
}

void CGUIDialogPVRTimerSettings::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == NULL)
  {
    CLog::LogF(LOGERROR, "No setting");
    return;
  }

  CGUIDialogSettingsManualBase::OnSettingAction(setting);

  const std::string& settingId = setting->GetId();
  if (settingId == SETTING_TMR_BEGIN)
  {
    KODI::TIME::SystemTime timerStartTime;
    m_startLocalTime.GetAsSystemTime(timerStartTime);
    if (CGUIDialogNumeric::ShowAndGetTime(timerStartTime, g_localizeStrings.Get(14066)))
    {
      SetTimeFromSystemTime(m_startLocalTime, timerStartTime);
      m_timerStartTimeStr = m_startLocalTime.GetAsLocalizedTime("", false);
      SetButtonLabels();
    }
  }
  else if (settingId == SETTING_TMR_END)
  {
    KODI::TIME::SystemTime timerEndTime;
    m_endLocalTime.GetAsSystemTime(timerEndTime);
    if (CGUIDialogNumeric::ShowAndGetTime(timerEndTime, g_localizeStrings.Get(14066)))
    {
      SetTimeFromSystemTime(m_endLocalTime, timerEndTime);
      m_timerEndTimeStr = m_endLocalTime.GetAsLocalizedTime("", false);
      SetButtonLabels();
    }
  }
}

bool CGUIDialogPVRTimerSettings::Validate()
{
  // @todo: Timer rules may have no date (time-only), so we can't check those for now.
  //        We need to extend the api with additional attributes to properly fix this
  if (m_timerType->IsTimerRule())
    return true;

  bool bStartAnyTime = m_bStartAnyTime;
  bool bEndAnyTime = m_bEndAnyTime;

  if (!m_timerType->SupportsStartAnyTime() ||
      !m_timerType->IsEpgBased()) // Start anytime toggle is not displayed
    bStartAnyTime = false; // Assume start time change needs checking for

  if (!m_timerType->SupportsEndAnyTime() ||
      !m_timerType->IsEpgBased()) // End anytime toggle is not displayed
    bEndAnyTime = false; // Assume end time change needs checking for

  // Begin and end time
  if (!bStartAnyTime && !bEndAnyTime)
  {
    if (m_timerType->SupportsStartTime() && m_timerType->SupportsEndTime() &&
        m_endLocalTime < m_startLocalTime)
    {
      HELPERS::ShowOKDialogText(CVariant{19065}, // "Timer settings"
                                CVariant{19072}); // In order to add/update a timer
      return false;
    }
  }

  return true;
}

bool CGUIDialogPVRTimerSettings::Save()
{
  if (!Validate())
    return false;

  // Timer type
  m_timerInfoTag->SetTimerType(m_timerType);

  // Timer active/inactive
  m_timerInfoTag->m_state = m_bTimerActive ? PVR_TIMER_STATE_SCHEDULED : PVR_TIMER_STATE_DISABLED;

  // Name
  m_timerInfoTag->m_strTitle = m_strTitle;

  // epg search string (only for epg-based timer rules)
  m_timerInfoTag->m_strEpgSearchString = m_strEpgSearchString;

  // epg fulltext search, instead of just title match. (only for epg-based timer rules)
  m_timerInfoTag->m_bFullTextEpgSearch = m_bFullTextEpgSearch;

  // Channel
  m_timerInfoTag->m_iClientChannelUid = m_channel.channelUid;
  m_timerInfoTag->m_iClientId = m_channel.clientId;
  m_timerInfoTag->m_bIsRadio = m_bIsRadio;
  m_timerInfoTag->UpdateChannel();

  if (!m_timerType->SupportsStartAnyTime() ||
      !m_timerType->IsEpgBased()) // Start anytime toggle is not displayed
    m_bStartAnyTime = false; // Assume start time change needs checking for
  m_timerInfoTag->m_bStartAnyTime = m_bStartAnyTime;

  if (!m_timerType->SupportsEndAnyTime() ||
      !m_timerType->IsEpgBased()) // End anytime toggle is not displayed
    m_bEndAnyTime = false; // Assume end time change needs checking for
  m_timerInfoTag->m_bEndAnyTime = m_bEndAnyTime;

  // Begin and end time
  if (!m_bStartAnyTime && !m_bEndAnyTime)
  {
    if (m_timerType->SupportsStartTime() && // has start clock entry
        m_timerType->SupportsEndTime() && // and end clock entry
        m_timerType->IsTimerRule()) // but no associated start/end day spinners
    {
      if (m_endLocalTime < m_startLocalTime) // And the end clock is earlier than the start clock
      {
        CLog::LogFC(LOGDEBUG, LOGPVR, "End before start, adding a day.");
        m_endLocalTime += CDateTimeSpan(1, 0, 0, 0);
        if (m_endLocalTime < m_startLocalTime)
        {
          CLog::Log(LOGWARNING,
                    "Timer settings dialog: End before start. Setting end time to start time.");
          m_endLocalTime = m_startLocalTime;
        }
      }
      else if (m_endLocalTime >
               (m_startLocalTime + CDateTimeSpan(1, 0, 0, 0))) // Or the duration is more than a day
      {
        CLog::LogFC(LOGDEBUG, LOGPVR, "End > 1 day after start, removing a day.");
        m_endLocalTime -= CDateTimeSpan(1, 0, 0, 0);
        if (m_endLocalTime > (m_startLocalTime + CDateTimeSpan(1, 0, 0, 0)))
        {
          CLog::Log(
              LOGWARNING,
              "Timer settings dialog: End > 1 day after start. Setting end time to start time.");
          m_endLocalTime = m_startLocalTime;
        }
      }
    }
    else if (m_endLocalTime < m_startLocalTime)
    {
      // this case will fail validation so this can't be reached.
    }
    m_timerInfoTag->SetStartFromLocalTime(m_startLocalTime);
    m_timerInfoTag->SetEndFromLocalTime(m_endLocalTime);
  }
  else if (!m_bStartAnyTime)
    m_timerInfoTag->SetStartFromLocalTime(m_startLocalTime);
  else if (!m_bEndAnyTime)
    m_timerInfoTag->SetEndFromLocalTime(m_endLocalTime);

  // Days of week (only for timer rules)
  if (m_timerType->IsTimerRule())
    m_timerInfoTag->m_iWeekdays = m_iWeekdays;
  else
    m_timerInfoTag->m_iWeekdays = PVR_WEEKDAY_NONE;

  // First day (only for timer rules)
  m_timerInfoTag->SetFirstDayFromLocalTime(m_firstDayLocalTime);

  // "New episodes only" (only for timer rules)
  m_timerInfoTag->m_iPreventDupEpisodes = m_iPreventDupEpisodes;

  // Pre and post record time
  m_timerInfoTag->m_iMarginStart = m_iMarginStart;
  m_timerInfoTag->m_iMarginEnd = m_iMarginEnd;

  // Priority
  m_timerInfoTag->m_iPriority = m_iPriority;

  // Lifetime
  m_timerInfoTag->m_iLifetime = m_iLifetime;

  // MaxRecordings
  m_timerInfoTag->m_iMaxRecordings = m_iMaxRecordings;

  // Recording folder
  m_timerInfoTag->m_strDirectory = m_strDirectory;

  // Recording group
  m_timerInfoTag->m_iRecordingGroup = m_iRecordingGroup;

  // Set the timer's title to the channel name if it's empty or 'New Timer'
  if (m_strTitle.empty() || m_strTitle == g_localizeStrings.Get(19056))
  {
    const std::string channelName = m_timerInfoTag->ChannelName();
    if (!channelName.empty())
      m_timerInfoTag->m_strTitle = channelName;
  }

  // Update summary
  m_timerInfoTag->UpdateSummary();

  return true;
}

void CGUIDialogPVRTimerSettings::SetButtonLabels()
{
  // timer start time
  BaseSettingControlPtr settingControl = GetSettingControl(SETTING_TMR_BEGIN);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
  {
    SET_CONTROL_LABEL2(settingControl->GetID(), m_timerStartTimeStr);
  }

  // timer end time
  settingControl = GetSettingControl(SETTING_TMR_END);
  if (settingControl != NULL && settingControl->GetControl() != NULL)
  {
    SET_CONTROL_LABEL2(settingControl->GetID(), m_timerEndTimeStr);
  }
}

void CGUIDialogPVRTimerSettings::AddCondition(const std::shared_ptr<CSetting>& setting,
                                              const std::string& identifier,
                                              SettingConditionCheck condition,
                                              SettingDependencyType depType,
                                              const std::string& settingId)
{
  GetSettingsManager()->AddDynamicCondition(identifier, condition, this);
  CSettingDependency dep(depType, GetSettingsManager());
  dep.And()->Add(std::make_shared<CSettingDependencyCondition>(identifier, "true", settingId, false,
                                                               GetSettingsManager()));
  SettingDependencies deps(setting->GetDependencies());
  deps.push_back(dep);
  setting->SetDependencies(deps);
}

int CGUIDialogPVRTimerSettings::GetDateAsIndex(const CDateTime& datetime)
{
  const CDateTime date(datetime.GetYear(), datetime.GetMonth(), datetime.GetDay(), 0, 0, 0);
  time_t t(0);
  date.GetAsTime(t);
  return static_cast<int>(t);
}

void CGUIDialogPVRTimerSettings::SetDateFromIndex(CDateTime& datetime, int date)
{
  const CDateTime newDate(static_cast<time_t>(date));
  datetime.SetDateTime(newDate.GetYear(), newDate.GetMonth(), newDate.GetDay(), datetime.GetHour(),
                       datetime.GetMinute(), datetime.GetSecond());
}

void CGUIDialogPVRTimerSettings::SetTimeFromSystemTime(CDateTime& datetime,
                                                       const KODI::TIME::SystemTime& time)
{
  const CDateTime newTime(time);
  datetime.SetDateTime(datetime.GetYear(), datetime.GetMonth(), datetime.GetDay(),
                       newTime.GetHour(), newTime.GetMinute(), newTime.GetSecond());
}

void CGUIDialogPVRTimerSettings::InitializeTypesList()
{
  m_typeEntries.clear();

  // If timer is read-only or was created by a timer rule, only add current type, for information. Type can't be changed.
  if (m_timerType->IsReadOnly() || m_timerInfoTag->HasParent())
  {
    m_typeEntries.insert(std::make_pair(0, m_timerType));
    return;
  }

  bool bFoundThisType(false);
  int idx(0);
  const std::vector<std::shared_ptr<CPVRTimerType>> types(CPVRTimerType::GetAllTypes());
  for (const auto& type : types)
  {
    // Type definition prohibits created of new instances.
    // But the dialog can act as a viewer for these types.
    if (type->ForbidsNewInstances())
      continue;

    // Read-only timers cannot be created using this dialog.
    // But the dialog can act as a viewer for read-only types.
    if (type->IsReadOnly())
      continue;

    // Drop TimerTypes that require EPGInfo, if none is populated
    if (type->RequiresEpgTagOnCreate() && !m_timerInfoTag->GetEpgInfoTag())
      continue;

    // Drop TimerTypes without 'Series' EPG attributes if none are set
    if (type->RequiresEpgSeriesOnCreate())
    {
      const std::shared_ptr<const CPVREpgInfoTag> epgTag(m_timerInfoTag->GetEpgInfoTag());
      if (epgTag && !epgTag->IsSeries())
        continue;
    }

    // Drop TimerTypes which need series link if none is set
    if (type->RequiresEpgSeriesLinkOnCreate())
    {
      const std::shared_ptr<const CPVREpgInfoTag> epgTag(m_timerInfoTag->GetEpgInfoTag());
      if (!epgTag || epgTag->SeriesLink().empty())
        continue;
    }

    // Drop TimerTypes that forbid EPGInfo, if it is populated
    if (type->ForbidsEpgTagOnCreate() && m_timerInfoTag->GetEpgInfoTag())
      continue;

    // Drop TimerTypes that aren't rules and cannot be recorded
    if (!type->IsTimerRule())
    {
      const std::shared_ptr<const CPVREpgInfoTag> epgTag(m_timerInfoTag->GetEpgInfoTag());
      bool bCanRecord = epgTag ? epgTag->IsRecordable()
                               : m_timerInfoTag->EndAsLocalTime() > CDateTime::GetCurrentDateTime();
      if (!bCanRecord)
        continue;
    }

    if (!bFoundThisType && *type == *m_timerType)
      bFoundThisType = true;

    m_typeEntries.insert(std::make_pair(idx++, type));
  }

  if (!bFoundThisType)
    m_typeEntries.insert(std::make_pair(idx++, m_timerType));
}

void CGUIDialogPVRTimerSettings::InitializeChannelsList()
{
  m_channelEntries.clear();

  int index = 0;

  // Add special "any channel" entries - one for every client (used for epg-based timer rules),
  // and for reminder rules another one representing any channel from any client.
  const CPVRClientMap clients = CServiceBroker::GetPVRManager().Clients()->GetCreatedClients();
  if (clients.size() > 1)
    m_channelEntries.insert({index++, ChannelDescriptor(PVR_CHANNEL_INVALID_UID, PVR_ANY_CLIENT_ID,
                                                        // Any channel from any client
                                                        g_localizeStrings.Get(854))});

  for (const auto& client : clients)
  {
    m_channelEntries.insert(
        {index, ChannelDescriptor(PVR_CHANNEL_INVALID_UID, client.second->GetID(),
                                  clients.size() == 1
                                      // Any channel
                                      ? g_localizeStrings.Get(809)
                                      // Any channel from client "X"
                                      : StringUtils::Format(g_localizeStrings.Get(853),
                                                            client.second->GetFullClientName()))});
    ++index;
  }

  // Add regular channels
  const std::shared_ptr<const CPVRChannelGroup> allGroup =
      CServiceBroker::GetPVRManager().ChannelGroups()->GetGroupAll(m_bIsRadio);
  const std::vector<std::shared_ptr<CPVRChannelGroupMember>> groupMembers =
      allGroup->GetMembers(CPVRChannelGroup::Include::ONLY_VISIBLE);
  for (const auto& groupMember : groupMembers)
  {
    const std::shared_ptr<const CPVRChannel> channel = groupMember->Channel();
    const std::string channelDescription = StringUtils::Format(
        "{} {}", groupMember->ChannelNumber().FormattedChannelNumber(), channel->ChannelName());
    m_channelEntries.insert(
        {index, ChannelDescriptor(channel->UniqueID(), channel->ClientID(), channelDescription)});
    ++index;
  }
}

void CGUIDialogPVRTimerSettings::TypesFiller(const SettingConstPtr& setting,
                                             std::vector<IntegerSettingOption>& list,
                                             int& current,
                                             void* data)
{
  CGUIDialogPVRTimerSettings* pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();
    current = 0;

    static const std::vector<std::pair<std::string, CVariant>> reminderTimerProps{
        std::make_pair("PVR.IsRemindingTimer", CVariant{true})};
    static const std::vector<std::pair<std::string, CVariant>> recordingTimerProps{
        std::make_pair("PVR.IsRecordingTimer", CVariant{true})};

    const auto clients = CServiceBroker::GetPVRManager().Clients();

    bool foundCurrent(false);
    for (const auto& typeEntry : pThis->m_typeEntries)
    {
      std::string clientName;

      const auto client = clients->GetCreatedClient(typeEntry.second->GetClientId());
      if (client)
        clientName = client->GetFullClientName();

      list.emplace_back(typeEntry.second->GetDescription(), clientName, typeEntry.first,
                        typeEntry.second->IsReminder() ? reminderTimerProps : recordingTimerProps);

      if (!foundCurrent && (*(pThis->m_timerType) == *(typeEntry.second)))
      {
        current = typeEntry.first;
        foundCurrent = true;
      }
    }
  }
  else
    CLog::LogF(LOGERROR, "No dialog");
}

void CGUIDialogPVRTimerSettings::ChannelsFiller(const SettingConstPtr& setting,
                                                std::vector<IntegerSettingOption>& list,
                                                int& current,
                                                void* data)
{
  CGUIDialogPVRTimerSettings* pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();
    current = 0;

    bool foundCurrent(false);
    for (const auto& channelEntry : pThis->m_channelEntries)
    {
      // Only include channels for the currently selected timer type or all channels if type is client-independent.
      if (pThis->m_timerType->GetClientId() == PVR_ANY_CLIENT_ID || // client-independent
          pThis->m_timerType->GetClientId() == channelEntry.second.clientId)
      {
        // Do not add "any channel" entry if not supported by selected timer type.
        if (channelEntry.second.channelUid == PVR_CHANNEL_INVALID_UID &&
            !pThis->m_timerType->SupportsAnyChannel())
          continue;

        // Do not add "any channel from any client" entry for reminder rules.
        if (channelEntry.second.channelUid == PVR_CHANNEL_INVALID_UID &&
            channelEntry.second.clientId == PVR_ANY_CLIENT_ID &&
            !pThis->m_timerType->IsReminder() && !pThis->m_timerType->IsTimerRule())
          continue;

        list.emplace_back(channelEntry.second.description, channelEntry.first);
      }

      if (!foundCurrent && (pThis->m_channel == channelEntry.second))
      {
        current = channelEntry.first;
        foundCurrent = true;
      }
    }

    if (foundCurrent)
    {
      // Verify m_channel is still valid. Update if not.
      if (std::find_if(list.cbegin(), list.cend(),
                       [&current](const auto& channel)
                       { return channel.value == current; }) == list.cend())
      {
        // Set m_channel and current to first valid channel in list
        const int first{list.front().value};
        const auto it =
            std::find_if(pThis->m_channelEntries.cbegin(), pThis->m_channelEntries.cend(),
                         [first](const auto& channel) { return channel.first == first; });

        if (it != pThis->m_channelEntries.cend())
        {
          current = (*it).first;
          pThis->m_channel = (*it).second;
        }
        else
        {
          CLog::LogF(LOGERROR, "Unable to find channel to select");
        }
      }
    }
  }
  else
    CLog::LogF(LOGERROR, "No dialog");
}

void CGUIDialogPVRTimerSettings::DaysFiller(const SettingConstPtr& setting,
                                            std::vector<IntegerSettingOption>& list,
                                            int& current,
                                            void* data)
{
  CGUIDialogPVRTimerSettings* pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();
    current = 0;

    // Data range: "today" until "yesterday next year"
    const CDateTime now(CDateTime::GetCurrentDateTime());
    CDateTime time(now.GetYear(), now.GetMonth(), now.GetDay(), 0, 0, 0);
    const CDateTime yesterdayPlusOneYear(CDateTime(time.GetYear() + 1, time.GetMonth(),
                                                   time.GetDay(), time.GetHour(), time.GetMinute(),
                                                   time.GetSecond()) -
                                         CDateTimeSpan(1, 0, 0, 0));

    CDateTime oldCDateTime;
    if (setting->GetId() == SETTING_TMR_FIRST_DAY)
      oldCDateTime = pThis->m_timerInfoTag->FirstDayAsLocalTime();
    else if (setting->GetId() == SETTING_TMR_START_DAY)
      oldCDateTime = pThis->m_timerInfoTag->StartAsLocalTime();
    else
      oldCDateTime = pThis->m_timerInfoTag->EndAsLocalTime();
    const CDateTime oldCDate(oldCDateTime.GetYear(), oldCDateTime.GetMonth(), oldCDateTime.GetDay(),
                             0, 0, 0);

    if ((oldCDate < time) || (oldCDate > yesterdayPlusOneYear))
      list.emplace_back(oldCDate.GetAsLocalizedDate(true /*long date*/), GetDateAsIndex(oldCDate));

    while (time <= yesterdayPlusOneYear)
    {
      list.emplace_back(time.GetAsLocalizedDate(true /*long date*/), GetDateAsIndex(time));
      time += CDateTimeSpan(1, 0, 0, 0);
    }

    if (setting->GetId() == SETTING_TMR_FIRST_DAY)
      current = GetDateAsIndex(pThis->m_firstDayLocalTime);
    else if (setting->GetId() == SETTING_TMR_START_DAY)
      current = GetDateAsIndex(pThis->m_startLocalTime);
    else
      current = GetDateAsIndex(pThis->m_endLocalTime);
  }
  else
    CLog::LogF(LOGERROR, "No dialog");
}

void CGUIDialogPVRTimerSettings::DupEpisodesFiller(const SettingConstPtr& setting,
                                                   std::vector<IntegerSettingOption>& list,
                                                   int& current,
                                                   void* data)
{
  CGUIDialogPVRTimerSettings* pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();

    std::vector<std::pair<std::string, int>> values;
    pThis->m_timerType->GetPreventDuplicateEpisodesValues(values);
    std::transform(values.cbegin(), values.cend(), std::back_inserter(list), [](const auto& value) {
      return IntegerSettingOption(value.first, value.second);
    });

    current = pThis->m_iPreventDupEpisodes;
  }
  else
    CLog::LogF(LOGERROR, "No dialog");
}

void CGUIDialogPVRTimerSettings::WeekdaysFiller(const SettingConstPtr& setting,
                                                std::vector<IntegerSettingOption>& list,
                                                int& current,
                                                void* data)
{
  CGUIDialogPVRTimerSettings* pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();
    list.emplace_back(g_localizeStrings.Get(831), PVR_WEEKDAY_MONDAY); // "Mondays"
    list.emplace_back(g_localizeStrings.Get(832), PVR_WEEKDAY_TUESDAY); // "Tuesdays"
    list.emplace_back(g_localizeStrings.Get(833), PVR_WEEKDAY_WEDNESDAY); // "Wednesdays"
    list.emplace_back(g_localizeStrings.Get(834), PVR_WEEKDAY_THURSDAY); // "Thursdays"
    list.emplace_back(g_localizeStrings.Get(835), PVR_WEEKDAY_FRIDAY); // "Fridays"
    list.emplace_back(g_localizeStrings.Get(836), PVR_WEEKDAY_SATURDAY); // "Saturdays"
    list.emplace_back(g_localizeStrings.Get(837), PVR_WEEKDAY_SUNDAY); // "Sundays"

    current = pThis->m_iWeekdays;
  }
  else
    CLog::LogF(LOGERROR, "No dialog");
}

void CGUIDialogPVRTimerSettings::PrioritiesFiller(const SettingConstPtr& setting,
                                                  std::vector<IntegerSettingOption>& list,
                                                  int& current,
                                                  void* data)
{
  CGUIDialogPVRTimerSettings* pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();

    std::vector<std::pair<std::string, int>> values;
    pThis->m_timerType->GetPriorityValues(values);
    std::transform(values.cbegin(), values.cend(), std::back_inserter(list), [](const auto& value) {
      return IntegerSettingOption(value.first, value.second);
    });

    current = pThis->m_iPriority;

    auto it = list.begin();
    while (it != list.end())
    {
      if (it->value == current)
        break; // value already in list

      ++it;
    }

    if (it == list.end())
    {
      // PVR backend supplied value is not in the list of predefined values. Insert it.
      list.insert(it, IntegerSettingOption(std::to_string(current), current));
    }
  }
  else
    CLog::LogF(LOGERROR, "No dialog");
}

void CGUIDialogPVRTimerSettings::LifetimesFiller(const SettingConstPtr& setting,
                                                 std::vector<IntegerSettingOption>& list,
                                                 int& current,
                                                 void* data)
{
  CGUIDialogPVRTimerSettings* pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();

    std::vector<std::pair<std::string, int>> values;
    pThis->m_timerType->GetLifetimeValues(values);
    std::transform(values.cbegin(), values.cend(), std::back_inserter(list), [](const auto& value) {
      return IntegerSettingOption(value.first, value.second);
    });

    current = pThis->m_iLifetime;

    auto it = list.begin();
    while (it != list.end())
    {
      if (it->value == current)
        break; // value already in list

      ++it;
    }

    if (it == list.end())
    {
      // PVR backend supplied value is not in the list of predefined values. Insert it.
      list.insert(it, IntegerSettingOption(
                          StringUtils::Format(g_localizeStrings.Get(17999), current) /* {} days */,
                          current));
    }
  }
  else
    CLog::LogF(LOGERROR, "No dialog");
}

void CGUIDialogPVRTimerSettings::MaxRecordingsFiller(const SettingConstPtr& setting,
                                                     std::vector<IntegerSettingOption>& list,
                                                     int& current,
                                                     void* data)
{
  CGUIDialogPVRTimerSettings* pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();

    std::vector<std::pair<std::string, int>> values;
    pThis->m_timerType->GetMaxRecordingsValues(values);
    std::transform(values.cbegin(), values.cend(), std::back_inserter(list), [](const auto& value) {
      return IntegerSettingOption(value.first, value.second);
    });

    current = pThis->m_iMaxRecordings;

    auto it = list.begin();
    while (it != list.end())
    {
      if (it->value == current)
        break; // value already in list

      ++it;
    }

    if (it == list.end())
    {
      // PVR backend supplied value is not in the list of predefined values. Insert it.
      list.insert(it, IntegerSettingOption(std::to_string(current), current));
    }
  }
  else
    CLog::LogF(LOGERROR, "No dialog");
}

void CGUIDialogPVRTimerSettings::RecordingGroupFiller(const SettingConstPtr& setting,
                                                      std::vector<IntegerSettingOption>& list,
                                                      int& current,
                                                      void* data)
{
  CGUIDialogPVRTimerSettings* pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();

    std::vector<std::pair<std::string, int>> values;
    pThis->m_timerType->GetRecordingGroupValues(values);
    std::transform(values.cbegin(), values.cend(), std::back_inserter(list), [](const auto& value) {
      return IntegerSettingOption(value.first, value.second);
    });

    current = pThis->m_iRecordingGroup;
  }
  else
    CLog::LogF(LOGERROR, "No dialog");
}

void CGUIDialogPVRTimerSettings::MarginTimeFiller(const SettingConstPtr& setting,
                                                  std::vector<IntegerSettingOption>& list,
                                                  int& current,
                                                  void* data)
{
  CGUIDialogPVRTimerSettings* pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis)
  {
    list.clear();

    // Get global settings values
    CPVRSettings::MarginTimeFiller(setting, list, current, data);

    if (setting->GetId() == SETTING_TMR_BEGIN_PRE)
      current = pThis->m_iMarginStart;
    else
      current = pThis->m_iMarginEnd;

    bool bInsertValue = true;
    auto it = list.begin();
    while (it != list.end())
    {
      if (it->value == current)
      {
        bInsertValue = false;
        break; // value already in list
      }

      if (it->value > current)
        break;

      ++it;
    }

    if (bInsertValue)
    {
      // PVR backend supplied value is not in the list of predefined values. Insert it.
      list.insert(it, IntegerSettingOption(
                          StringUtils::Format(g_localizeStrings.Get(14044), current) /* {} min */,
                          current));
    }
  }
  else
    CLog::LogF(LOGERROR, "No dialog");
}

std::string CGUIDialogPVRTimerSettings::WeekdaysValueFormatter(const SettingConstPtr& setting)
{
  return CPVRTimerInfoTag::GetWeekdaysString(GetWeekdaysFromSetting(setting), true, true);
}

void CGUIDialogPVRTimerSettings::AddTypeDependentEnableCondition(
    const std::shared_ptr<CSetting>& setting, const std::string& identifier)
{
  // Enable setting depending on read-only attribute of the selected timer type
  std::string id(identifier);
  id.append(TYPE_DEP_ENABLE_COND_ID_POSTFIX);
  AddCondition(setting, id, TypeReadOnlyCondition, SettingDependencyType::Enable, SETTING_TMR_TYPE);
}

bool CGUIDialogPVRTimerSettings::TypeReadOnlyCondition(const std::string& condition,
                                                       const std::string& value,
                                                       const SettingConstPtr& setting,
                                                       void* data)
{
  if (setting == NULL)
    return false;

  CGUIDialogPVRTimerSettings* pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::LogF(LOGERROR, "No dialog");
    return false;
  }

  if (!StringUtils::EqualsNoCase(value, "true"))
    return false;

  std::string cond(condition);
  cond.erase(cond.find(TYPE_DEP_ENABLE_COND_ID_POSTFIX));

  // If only one type is available, disable type selector.
  if (pThis->m_typeEntries.size() == 1)
  {
    if (cond == SETTING_TMR_TYPE)
      return false;
  }

  // For existing one time epg-based timers, disable editing of epg-filled data.
  if (!pThis->m_bIsNewTimer && pThis->m_timerType->IsEpgBasedOnetime())
  {
    if ((cond == SETTING_TMR_NAME) || (cond == SETTING_TMR_CHANNEL) ||
        (cond == SETTING_TMR_START_DAY) || (cond == SETTING_TMR_END_DAY) ||
        (cond == SETTING_TMR_BEGIN) || (cond == SETTING_TMR_END))
      return false;
  }

  /* Always enable enable/disable, if supported by the timer type. */
  if (pThis->m_timerType->SupportsEnableDisable() && !pThis->m_timerInfoTag->IsBroken())
  {
    if (cond == SETTING_TMR_ACTIVE)
      return true;
  }

  // Let the PVR client decide...
  int idx = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  const auto entry = pThis->m_typeEntries.find(idx);
  if (entry != pThis->m_typeEntries.end())
    return !entry->second->IsReadOnly();
  else
    CLog::LogF(LOGERROR, "No type entry");

  return false;
}

void CGUIDialogPVRTimerSettings::AddTypeDependentVisibilityCondition(
    const std::shared_ptr<CSetting>& setting, const std::string& identifier)
{
  // Show or hide setting depending on attributes of the selected timer type
  std::string id(identifier);
  id.append(TYPE_DEP_VISIBI_COND_ID_POSTFIX);
  AddCondition(setting, id, TypeSupportsCondition, SettingDependencyType::Visible,
               SETTING_TMR_TYPE);
}

bool CGUIDialogPVRTimerSettings::TypeSupportsCondition(const std::string& condition,
                                                       const std::string& value,
                                                       const SettingConstPtr& setting,
                                                       void* data)
{
  if (setting == NULL)
    return false;

  CGUIDialogPVRTimerSettings* pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::LogF(LOGERROR, "No dialog");
    return false;
  }

  if (!StringUtils::EqualsNoCase(value, "true"))
    return false;

  int idx = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
  const auto entry = pThis->m_typeEntries.find(idx);
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
    else if (cond == SETTING_TMR_START_ANYTIME)
      return entry->second->SupportsStartAnyTime() && entry->second->IsEpgBased();
    else if (cond == SETTING_TMR_END_ANYTIME)
      return entry->second->SupportsEndAnyTime() && entry->second->IsEpgBased();
    else if (cond == SETTING_TMR_START_DAY)
      return entry->second->SupportsStartTime() && entry->second->IsOnetime();
    else if (cond == SETTING_TMR_END_DAY)
      return entry->second->SupportsEndTime() && entry->second->IsOnetime();
    else if (cond == SETTING_TMR_BEGIN)
      return entry->second->SupportsStartTime();
    else if (cond == SETTING_TMR_END)
      return entry->second->SupportsEndTime();
    else if (cond == SETTING_TMR_WEEKDAYS)
      return entry->second->SupportsWeekdays();
    else if (cond == SETTING_TMR_FIRST_DAY)
      return entry->second->SupportsFirstDay();
    else if (cond == SETTING_TMR_NEW_EPISODES)
      return entry->second->SupportsRecordOnlyNewEpisodes();
    else if (cond == SETTING_TMR_BEGIN_PRE)
      return entry->second->SupportsStartMargin();
    else if (cond == SETTING_TMR_END_POST)
      return entry->second->SupportsEndMargin();
    else if (cond == SETTING_TMR_PRIORITY)
      return entry->second->SupportsPriority();
    else if (cond == SETTING_TMR_LIFETIME)
      return entry->second->SupportsLifetime();
    else if (cond == SETTING_TMR_MAX_REC)
      return entry->second->SupportsMaxRecordings();
    else if (cond == SETTING_TMR_DIR)
      return entry->second->SupportsRecordingFolders();
    else if (cond == SETTING_TMR_REC_GROUP)
      return entry->second->SupportsRecordingGroup();
    else
      CLog::LogF(LOGERROR, "Unknown condition");
  }
  else
  {
    CLog::LogF(LOGERROR, "No type entry");
  }
  return false;
}

void CGUIDialogPVRTimerSettings::AddStartAnytimeDependentVisibilityCondition(
    const std::shared_ptr<CSetting>& setting, const std::string& identifier)
{
  // Show or hide setting depending on value of setting "any time"
  std::string id(identifier);
  id.append(START_ANYTIME_DEP_VISIBI_COND_ID_POSTFIX);
  AddCondition(setting, id, StartAnytimeSetCondition, SettingDependencyType::Visible,
               SETTING_TMR_START_ANYTIME);
}

bool CGUIDialogPVRTimerSettings::StartAnytimeSetCondition(const std::string& condition,
                                                          const std::string& value,
                                                          const SettingConstPtr& setting,
                                                          void* data)
{
  if (setting == NULL)
    return false;

  CGUIDialogPVRTimerSettings* pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::LogF(LOGERROR, "No dialog");
    return false;
  }

  if (!StringUtils::EqualsNoCase(value, "true"))
    return false;

  // "any time" setting is only relevant for epg-based timers.
  if (!pThis->m_timerType->IsEpgBased())
    return true;

  // If 'Start anytime' option isn't supported, don't hide start time
  if (!pThis->m_timerType->SupportsStartAnyTime())
    return true;

  std::string cond(condition);
  cond.erase(cond.find(START_ANYTIME_DEP_VISIBI_COND_ID_POSTFIX));

  if ((cond == SETTING_TMR_START_DAY) || (cond == SETTING_TMR_BEGIN))
  {
    bool bAnytime = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    return !bAnytime;
  }
  return false;
}

void CGUIDialogPVRTimerSettings::AddEndAnytimeDependentVisibilityCondition(
    const std::shared_ptr<CSetting>& setting, const std::string& identifier)
{
  // Show or hide setting depending on value of setting "any time"
  std::string id(identifier);
  id.append(END_ANYTIME_DEP_VISIBI_COND_ID_POSTFIX);
  AddCondition(setting, id, EndAnytimeSetCondition, SettingDependencyType::Visible,
               SETTING_TMR_END_ANYTIME);
}

bool CGUIDialogPVRTimerSettings::EndAnytimeSetCondition(const std::string& condition,
                                                        const std::string& value,
                                                        const SettingConstPtr& setting,
                                                        void* data)
{
  if (setting == NULL)
    return false;

  CGUIDialogPVRTimerSettings* pThis = static_cast<CGUIDialogPVRTimerSettings*>(data);
  if (pThis == NULL)
  {
    CLog::LogF(LOGERROR, "No dialog");
    return false;
  }

  if (!StringUtils::EqualsNoCase(value, "true"))
    return false;

  // "any time" setting is only relevant for epg-based timers.
  if (!pThis->m_timerType->IsEpgBased())
    return true;

  // If 'End anytime' option isn't supported, don't hide end time
  if (!pThis->m_timerType->SupportsEndAnyTime())
    return true;

  std::string cond(condition);
  cond.erase(cond.find(END_ANYTIME_DEP_VISIBI_COND_ID_POSTFIX));

  if ((cond == SETTING_TMR_END_DAY) || (cond == SETTING_TMR_END))
  {
    bool bAnytime = std::static_pointer_cast<const CSettingBool>(setting)->GetValue();
    return !bAnytime;
  }
  return false;
}
