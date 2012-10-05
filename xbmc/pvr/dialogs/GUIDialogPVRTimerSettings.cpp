/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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
#include "guilib/GUIKeyboardFactory.h"
#include "dialogs/GUIDialogNumeric.h"
#include "settings/GUISettings.h"
#include "guilib/LocalizeStrings.h"

#include "pvr/PVRManager.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"

using namespace std;
using namespace PVR;

#define CONTROL_TMR_ACTIVE              20
#define CONTROL_TMR_CHNAME_TV           21
#define CONTROL_TMR_DAY                 22
#define CONTROL_TMR_BEGIN               23
#define CONTROL_TMR_END                 24
#define CONTROL_TMR_PRIORITY            26
#define CONTROL_TMR_LIFETIME            27
#define CONTROL_TMR_FIRST_DAY           28
#define CONTROL_TMR_NAME                29
#define CONTROL_TMR_DIR                 30
#define CONTROL_TMR_RADIO               50
#define CONTROL_TMR_CHNAME_RADIO        51

CGUIDialogPVRTimerSettings::CGUIDialogPVRTimerSettings(void)
  : CGUIDialogSettings(WINDOW_DIALOG_PVR_TIMER_SETTING, "DialogPVRTimerSettings.xml")
{
  m_cancelled = true;
  m_tmp_day   = 11;
  m_loadType = LOAD_EVERY_TIME;
}

void CGUIDialogPVRTimerSettings::AddChannelNames(CFileItemList &channelsList, SETTINGSTRINGS &channelNames, bool bRadio)
{
  g_PVRChannelGroups->GetGroupAll(bRadio)->GetMembers(channelsList);

  channelNames.push_back("0 dummy");
  for (int i = 0; i < channelsList.Size(); i++)
  {
    CStdString string;
    CFileItemPtr item = channelsList[i];
    const CPVRChannel *channel = item->GetPVRChannelInfoTag();
    string.Format("%i %s", channel->ChannelNumber(), channel->ChannelName().c_str());
    channelNames.push_back(string);
  }

  int iControl = bRadio ? CONTROL_TMR_CHNAME_RADIO : CONTROL_TMR_CHNAME_TV;
  AddSpin(iControl, 19078, &m_timerItem->GetPVRTimerInfoTag()->m_iChannelNumber, channelNames.size(), channelNames);
  EnableSettings(iControl, m_timerItem->GetPVRTimerInfoTag()->m_bIsRadio == bRadio);
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

void CGUIDialogPVRTimerSettings::CreateSettings()
{
  CPVRTimerInfoTag* tag = m_timerItem->GetPVRTimerInfoTag();

  // clear out any old settings
  m_settings.clear();

  // create our settings controls
  m_bTimerActive = tag->IsActive();
  AddBool(CONTROL_TMR_ACTIVE, 19074, &m_bTimerActive);
  AddButton(CONTROL_TMR_NAME, 19075, &tag->m_strTitle, true);

  if (tag->SupportsFolders())
    AddButton(CONTROL_TMR_DIR, 19076, &tag->m_strDirectory, true);

  AddBool(CONTROL_TMR_RADIO, 19077, &tag->m_bIsRadio);

  /// Channel names
  {
    // For TV
    CFileItemList channelslist_tv;
    SETTINGSTRINGS channelstrings_tv;
    AddChannelNames(channelslist_tv, channelstrings_tv, false);

    // For Radio
    CFileItemList channelslist_radio;
    SETTINGSTRINGS channelstrings_radio;
    AddChannelNames(channelslist_radio, channelstrings_radio, true);
  }

  /// Day
  {
    SETTINGSTRINGS daystrings;
    tm time_cur;
    tm time_tmr;

    for (unsigned int iDayPtr = 19086; iDayPtr <= 19096; iDayPtr++)
      daystrings.push_back(g_localizeStrings.Get(iDayPtr));
    CDateTime time = CDateTime::GetCurrentDateTime();
    CDateTime timestart = tag->StartAsLocalTime();

    /* get diffence of timer in days between today and timer start date */
    time.GetAsTm(time_cur);
    timestart.GetAsTm(time_tmr);

    m_tmp_day += time_tmr.tm_yday - time_cur.tm_yday;
    if (time_tmr.tm_yday - time_cur.tm_yday < 0)
      m_tmp_day += 365;

    for (int i = 1; i < 365; ++i)
    {
      CStdString string = time.GetAsLocalizedDate();
      daystrings.push_back(string);
      time += CDateTimeSpan(1, 0, 0, 0);
    }

    SetWeekdaySettingFromTimer(*tag);

    AddSpin(CONTROL_TMR_DAY, 19079, &m_tmp_day, daystrings.size(), daystrings);
  }

  AddButton(CONTROL_TMR_BEGIN, 19080, &timerStartTimeStr, true);
  AddButton(CONTROL_TMR_END, 19081, &timerEndTimeStr, true);
  AddSpin(CONTROL_TMR_PRIORITY, 19082, &tag->m_iPriority, 0, 99);
  AddSpin(CONTROL_TMR_LIFETIME, 19083, &tag->m_iLifetime, 0, 365);

  /// First day
  {
    SETTINGSTRINGS daystrings;
    tm time_cur;
    tm time_tmr;

    CDateTime time = CDateTime::GetCurrentDateTime();
    CDateTime timestart = tag->FirstDayAsLocalTime();

    /* get diffence of timer in days between today and timer start date */
    if (time < timestart)
    {
      time.GetAsTm(time_cur);
      timestart.GetAsTm(time_tmr);

      m_tmp_iFirstDay += time_tmr.tm_yday - time_cur.tm_yday + 1;
      if (time_tmr.tm_yday - time_cur.tm_yday < 0)
        m_tmp_iFirstDay += 365;
    }

    daystrings.push_back(g_localizeStrings.Get(19030));

    for (int i = 1; i < 365; ++i)
    {
      CStdString string = time.GetAsLocalizedDate();
      daystrings.push_back(string);
      time += CDateTimeSpan(1, 0, 0, 0);
    }

    AddSpin(CONTROL_TMR_FIRST_DAY, 19084, &m_tmp_iFirstDay, daystrings.size(), daystrings);

    if (tag->m_bIsRepeating)
      EnableSettings(CONTROL_TMR_FIRST_DAY, true);
    else
      EnableSettings(CONTROL_TMR_FIRST_DAY, false);
  }
}

void CGUIDialogPVRTimerSettings::OnSettingChanged(SettingInfo &setting)
{
  CPVRTimerInfoTag* tag = m_timerItem->GetPVRTimerInfoTag();

  if (setting.id == CONTROL_TMR_NAME)
  {
    if (CGUIKeyboardFactory::ShowAndGetInput(tag->m_strTitle, g_localizeStrings.Get(19097), false))
    {
      UpdateSetting(CONTROL_TMR_NAME);
    }
  }
  if (setting.id == CONTROL_TMR_DIR && CGUIKeyboardFactory::ShowAndGetInput(tag->m_strDirectory, g_localizeStrings.Get(19104), false))
      UpdateSetting(CONTROL_TMR_DIR);
  else if (setting.id == CONTROL_TMR_RADIO || setting.id == CONTROL_TMR_CHNAME_TV || setting.id == CONTROL_TMR_CHNAME_RADIO)
  {
    if (setting.id == CONTROL_TMR_RADIO)
    {
      EnableSettings(CONTROL_TMR_CHNAME_TV, !tag->m_bIsRadio);
      EnableSettings(CONTROL_TMR_CHNAME_RADIO, tag->m_bIsRadio);
    }

    CFileItemPtr channel = g_PVRChannelGroups->GetGroupAll(tag->m_bIsRadio)->GetByChannelNumber(tag->m_iChannelNumber);
    if (channel && channel->HasPVRChannelInfoTag())
    {
      tag->m_iClientChannelUid = channel->GetPVRChannelInfoTag()->UniqueID();
      tag->m_iClientId         = channel->GetPVRChannelInfoTag()->ClientID();
      tag->m_bIsRadio          = channel->GetPVRChannelInfoTag()->IsRadio();
      tag->m_iChannelNumber    = channel->GetPVRChannelInfoTag()->ChannelNumber();
    }
  }
  else if (setting.id == CONTROL_TMR_DAY && m_tmp_day > 10)
  {
    CDateTime time = CDateTime::GetCurrentDateTime();
    CDateTime timestart = timerStartTime;
    CDateTime timestop = timerEndTime;
    int m_tmp_diff;
    tm time_cur;
    tm time_tmr;

    /* get diffence of timer in days between today and timer start date */
    time.GetAsTm(time_cur);
    timestart.GetAsTm(time_tmr);

    m_tmp_diff = time_tmr.tm_yday - time_cur.tm_yday;
    if (time_tmr.tm_yday - time_cur.tm_yday < 0)
      m_tmp_diff = 365;

    CDateTime newStart = timestart + CDateTimeSpan(m_tmp_day-11-m_tmp_diff, 0, 0, 0);
    CDateTime newEnd = timestop  + CDateTimeSpan(m_tmp_day-11-m_tmp_diff, 0, 0, 0);
    tag->SetStartFromLocalTime(newStart);
    tag->SetEndFromLocalTime(newEnd);

    EnableSettings(CONTROL_TMR_FIRST_DAY, false);

    tag->m_bIsRepeating = false;
    tag->m_iWeekdays = 0;
  }
  else if (setting.id == CONTROL_TMR_DAY && m_tmp_day <= 10)
  {
    EnableSettings(CONTROL_TMR_FIRST_DAY, true);
    SetTimerFromWeekdaySetting(*tag);
  }
  else if (setting.id == CONTROL_TMR_BEGIN)
  {
    if (CGUIDialogNumeric::ShowAndGetTime(timerStartTime, g_localizeStrings.Get(14066)))
    {
      CDateTime timestart = timerStartTime;
      int start_day       = tag->StartAsLocalTime().GetDay();
      int start_month     = tag->StartAsLocalTime().GetMonth();
      int start_year      = tag->StartAsLocalTime().GetYear();
      int start_hour      = timestart.GetHour();
      int start_minute    = timestart.GetMinute();
      CDateTime newStart(start_year, start_month, start_day, start_hour, start_minute, 0);
      tag->SetStartFromLocalTime(newStart);

      timerStartTimeStr = tag->StartAsLocalTime().GetAsLocalizedTime("", false);
      UpdateSetting(CONTROL_TMR_BEGIN);
    }
  }
  else if (setting.id == CONTROL_TMR_END)
  {
    if (CGUIDialogNumeric::ShowAndGetTime(timerEndTime, g_localizeStrings.Get(14066)))
    {
      CDateTime timestop = timerEndTime;
      int start_day       = tag->EndAsLocalTime().GetDay();
      int start_month     = tag->EndAsLocalTime().GetMonth();
      int start_year      = tag->EndAsLocalTime().GetYear();
      int start_hour      = timestop.GetHour();
      int start_minute    = timestop.GetMinute();
      CDateTime newEnd(start_year, start_month, start_day, start_hour, start_minute, 0);
      tag->SetEndFromLocalTime(newEnd);

      timerEndTimeStr = tag->EndAsLocalTime().GetAsLocalizedTime("", false);
      UpdateSetting(CONTROL_TMR_END);
    }
  }
  else if (setting.id == CONTROL_TMR_FIRST_DAY && m_tmp_day <= 10)
  {
    CDateTime newFirstDay;
    if (m_tmp_iFirstDay > 0)
      newFirstDay = CDateTime::GetCurrentDateTime() + CDateTimeSpan(m_tmp_iFirstDay-1, 0, 0, 0);

    tag->SetFirstDayFromLocalTime(newFirstDay);
  }

  tag->UpdateSummary();
}

void CGUIDialogPVRTimerSettings::SetTimer(CFileItem *item)
{
  m_timerItem         = item;
  m_cancelled         = true;

  m_timerItem->GetPVRTimerInfoTag()->StartAsLocalTime().GetAsSystemTime(timerStartTime);
  m_timerItem->GetPVRTimerInfoTag()->EndAsLocalTime().GetAsSystemTime(timerEndTime);
  timerStartTimeStr   = m_timerItem->GetPVRTimerInfoTag()->StartAsLocalTime().GetAsLocalizedTime("", false);
  timerEndTimeStr     = m_timerItem->GetPVRTimerInfoTag()->EndAsLocalTime().GetAsLocalizedTime("", false);

  m_tmp_iFirstDay     = 0;
  m_tmp_day           = 11;
}

void CGUIDialogPVRTimerSettings::OnOkay()
{
  m_cancelled = false;
  CPVRTimerInfoTag* tag = m_timerItem->GetPVRTimerInfoTag();
  if (tag->m_strTitle == g_localizeStrings.Get(19056))
    tag->m_strTitle = g_PVRChannelGroups->GetByUniqueID(tag->m_iClientChannelUid, tag->m_iClientId)->ChannelName();

  if (m_bTimerActive)
    tag->m_state = PVR_TIMER_STATE_SCHEDULED;
  else
    tag->m_state = PVR_TIMER_STATE_CANCELLED;
}
