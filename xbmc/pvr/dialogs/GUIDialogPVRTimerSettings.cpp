/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIDialogPVRTimerSettings.h"
#include "dialogs/GUIDialogKeyboard.h"
#include "dialogs/GUIDialogNumeric.h"
#include "settings/GUISettings.h"
#include "guilib/LocalizeStrings.h"

#include "pvr/PVRManager.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"

using namespace std;

#define CONTROL_TMR_ACTIVE              20
#define CONTROL_TMR_CHNAME_TV           21
#define CONTROL_TMR_DAY                 22
#define CONTROL_TMR_BEGIN               23
#define CONTROL_TMR_END                 24
#define CONTROL_TMR_PRIORITY            26
#define CONTROL_TMR_LIFETIME            27
#define CONTROL_TMR_FIRST_DAY           28
#define CONTROL_TMR_NAME                29
#define CONTROL_TMR_RADIO               50
#define CONTROL_TMR_CHNAME_RADIO        51

CGUIDialogPVRTimerSettings::CGUIDialogPVRTimerSettings(void)
  : CGUIDialogSettings(WINDOW_DIALOG_PVR_TIMER_SETTING, "DialogPVRTimerSettings.xml")
{
  m_cancelled = true;
  m_tmp_day   = 11;
}

void CGUIDialogPVRTimerSettings::CreateSettings()
{
  CPVRTimerInfoTag* tag = m_timerItem->GetPVRTimerInfoTag();

  // clear out any old settings
  m_settings.clear();

  // create our settings controls
  AddBool(CONTROL_TMR_ACTIVE, 19074, &tag->m_bIsActive);
  AddButton(CONTROL_TMR_NAME, 19075, &tag->m_strTitle, true);
  AddBool(CONTROL_TMR_RADIO, 19077, &tag->m_bIsRadio);

  /// Channel names
  {
    // For TV
    CFileItemList channelslist_tv;
    SETTINGSTRINGS channelstrings_tv;
    ((CPVRChannelGroup *) CPVRManager::GetChannelGroups()->GetGroupAll(false))->GetChannels(&channelslist_tv, -1);

    channelstrings_tv.push_back("0 dummy");

    for (int i = 0; i < channelslist_tv.Size(); i++)
    {
      CStdString string;
      CFileItemPtr item = channelslist_tv[i];
      string.Format("%i %s", item->GetPVRChannelInfoTag()->ChannelNumber(), item->GetPVRChannelInfoTag()->ChannelName().c_str());
      channelstrings_tv.push_back(string);
    }

    AddSpin(CONTROL_TMR_CHNAME_TV, 19078, &tag->m_iChannelNumber, channelstrings_tv.size(), channelstrings_tv);
    EnableSettings(CONTROL_TMR_CHNAME_TV, !tag->m_bIsRadio);

    // For Radio
    CFileItemList channelslist_radio;
    SETTINGSTRINGS channelstrings_radio;
    ((CPVRChannelGroup *) CPVRManager::GetChannelGroups()->GetGroupAll(true))->GetChannels(&channelslist_radio, -1);

    channelstrings_radio.push_back("0 dummy");

    for (int i = 0; i < channelslist_radio.Size(); i++)
    {
      CStdString string;
      CFileItemPtr item = channelslist_radio[i];
      string.Format("%i %s", item->GetPVRChannelInfoTag()->ChannelNumber(), item->GetPVRChannelInfoTag()->ChannelName().c_str());
      channelstrings_radio.push_back(string);
    }

    AddSpin(CONTROL_TMR_CHNAME_RADIO, 19078, &tag->m_iChannelNumber, channelstrings_radio.size(), channelstrings_radio);
    EnableSettings(CONTROL_TMR_CHNAME_RADIO, tag->m_bIsRadio);
  }

  /// Day
  {
    SETTINGSTRINGS daystrings;
    tm time_cur;
    tm time_tmr;

    daystrings.push_back(g_localizeStrings.Get(19086));
    daystrings.push_back(g_localizeStrings.Get(19087));
    daystrings.push_back(g_localizeStrings.Get(19088));
    daystrings.push_back(g_localizeStrings.Get(19089));
    daystrings.push_back(g_localizeStrings.Get(19090));
    daystrings.push_back(g_localizeStrings.Get(19091));
    daystrings.push_back(g_localizeStrings.Get(19092));
    daystrings.push_back(g_localizeStrings.Get(19093));
    daystrings.push_back(g_localizeStrings.Get(19094));
    daystrings.push_back(g_localizeStrings.Get(19095));
    daystrings.push_back(g_localizeStrings.Get(19096));
    CDateTime time = CDateTime::GetCurrentDateTime();
    CDateTime timestart = tag->m_StartTime;

    /* get diffence of timer in days between today and timer start date */
    time.GetAsTm(time_cur);
    timestart.GetAsTm(time_tmr);

    if (time_tmr.tm_yday - time_cur.tm_yday >= 0)
      m_tmp_day += time_tmr.tm_yday - time_cur.tm_yday;
    else
      m_tmp_day += time_tmr.tm_yday - time_cur.tm_yday + 365;

    for (int i = 1; i < 365; ++i)
    {
      CStdString string = time.GetAsLocalizedDate();
      daystrings.push_back(string);
      time += CDateTimeSpan(1, 0, 0, 0);
    }

    if (tag->m_bIsRepeating)
    {
      if (tag->m_iWeekdays == 0x01)
        m_tmp_day = 0;
      else if (tag->m_iWeekdays == 0x02)
        m_tmp_day = 1;
      else if (tag->m_iWeekdays == 0x04)
        m_tmp_day = 2;
      else if (tag->m_iWeekdays == 0x08)
        m_tmp_day = 3;
      else if (tag->m_iWeekdays == 0x10)
        m_tmp_day = 4;
      else if (tag->m_iWeekdays == 0x20)
        m_tmp_day = 5;
      else if (tag->m_iWeekdays == 0x40)
        m_tmp_day = 6;
      else if (tag->m_iWeekdays == 0x1F)
        m_tmp_day = 7;
      else if (tag->m_iWeekdays == 0x3F)
        m_tmp_day = 8;
      else if (tag->m_iWeekdays == 0x7F)
        m_tmp_day = 9;
      else if (tag->m_iWeekdays == 0x60)
        m_tmp_day = 10;
    }

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
    CDateTime timestart = tag->m_FirstDay;

    /* get diffence of timer in days between today and timer start date */
    if (time < timestart)
    {
      time.GetAsTm(time_cur);
      timestart.GetAsTm(time_tmr);

      if (time_tmr.tm_yday - time_cur.tm_yday >= 0)
      {
        m_tmp_iFirstDay += time_tmr.tm_yday - time_cur.tm_yday + 1;
      }
      else
      {
        m_tmp_iFirstDay += time_tmr.tm_yday - time_cur.tm_yday + 365 + 1;
      }
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
    if (CGUIDialogKeyboard::ShowAndGetInput(tag->m_strTitle, g_localizeStrings.Get(19097), false))
    {
      UpdateSetting(CONTROL_TMR_NAME);
    }
  }
  else if (setting.id == CONTROL_TMR_RADIO)
  {
    const CPVRChannel* channeltag = NULL;
    if (!tag->IsRadio())
    {
      EnableSettings(CONTROL_TMR_CHNAME_TV, true);
      EnableSettings(CONTROL_TMR_CHNAME_RADIO, false);
      channeltag = ((CPVRChannelGroup *) CPVRManager::GetChannelGroups()->GetGroupAll(false))->GetByChannelNumber(tag->Number());
    }
    else
    {
      EnableSettings(CONTROL_TMR_CHNAME_TV, false);
      EnableSettings(CONTROL_TMR_CHNAME_RADIO, true);
      channeltag = ((CPVRChannelGroup *) CPVRManager::GetChannelGroups()->GetGroupAll(true))->GetByChannelNumber(tag->Number());
    }

    if (channeltag)
    {
      tag->SetClientNumber(channeltag->ClientChannelNumber());
      tag->SetClientID(channeltag->ClientID());
      tag->SetRadio(channeltag->IsRadio());
      tag->SetNumber(channeltag->ChannelNumber());
    }
  }
  else if (setting.id == CONTROL_TMR_CHNAME_TV || setting.id == CONTROL_TMR_CHNAME_RADIO)
  {
    const CPVRChannel* channeltag = ((CPVRChannelGroup *) CPVRManager::GetChannelGroups()->GetGroupAll(tag->IsRadio()))->GetByChannelNumber(tag->Number());

    if (channeltag)
    {
      tag->SetClientNumber(channeltag->ClientChannelNumber());
      tag->SetClientID(channeltag->ClientID());
      tag->SetRadio(channeltag->IsRadio());
      tag->SetNumber(channeltag->ChannelNumber());
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

    if (time_tmr.tm_yday - time_cur.tm_yday >= 0)
      m_tmp_diff = time_tmr.tm_yday - time_cur.tm_yday;
    else
      m_tmp_diff = time_tmr.tm_yday - time_cur.tm_yday + 365;

    tag->m_StartTime = timestart + CDateTimeSpan(m_tmp_day-11-m_tmp_diff, 0, 0, 0);
    tag->m_StopTime  = timestop  + CDateTimeSpan(m_tmp_day-11-m_tmp_diff, 0, 0, 0);

    EnableSettings(CONTROL_TMR_FIRST_DAY, false);

    tag->m_bIsRepeating = false;
    tag->m_iWeekdays = 0;
  }
  else if (setting.id == CONTROL_TMR_DAY && m_tmp_day <= 10)
  {
    EnableSettings(CONTROL_TMR_FIRST_DAY, true);
    tag->m_bIsRepeating = true;

    if (m_tmp_day == 0)
      tag->m_iWeekdays = 0x01;
    else if (m_tmp_day == 1)
      tag->m_iWeekdays = 0x02;
    else if (m_tmp_day == 2)
      tag->m_iWeekdays = 0x04;
    else if (m_tmp_day == 3)
      tag->m_iWeekdays = 0x08;
    else if (m_tmp_day == 4)
      tag->m_iWeekdays = 0x10;
    else if (m_tmp_day == 5)
      tag->m_iWeekdays = 0x20;
    else if (m_tmp_day == 6)
      tag->m_iWeekdays = 0x40;
    else if (m_tmp_day == 7)
      tag->m_iWeekdays = 0x1F;
    else if (m_tmp_day == 8)
      tag->m_iWeekdays = 0x3F;
    else if (m_tmp_day == 9)
      tag->m_iWeekdays = 0x7F;
    else if (m_tmp_day == 10)
      tag->m_iWeekdays = 0x60;
    else
      tag->m_iWeekdays = 0;
  }
  else if (setting.id == CONTROL_TMR_BEGIN)
  {
    if (CGUIDialogNumeric::ShowAndGetTime(timerStartTime, g_localizeStrings.Get(14066)))
    {
      CDateTime timestart = timerStartTime;
      int start_day       = tag->m_StartTime.GetDay();
      int start_month     = tag->m_StartTime.GetMonth();
      int start_year      = tag->m_StartTime.GetYear();
      int start_hour      = timestart.GetHour();
      int start_minute    = timestart.GetMinute();
      tag->m_StartTime.SetDateTime(start_year, start_month, start_day, start_hour, start_minute, 0);

      timerStartTimeStr = tag->m_StartTime.GetAsLocalizedTime("", false);
      UpdateSetting(CONTROL_TMR_BEGIN);
    }
  }
  else if (setting.id == CONTROL_TMR_END)
  {
    if (CGUIDialogNumeric::ShowAndGetTime(timerEndTime, g_localizeStrings.Get(14066)))
    {
      CDateTime timestop = timerEndTime;
      int start_day       = tag->m_StopTime.GetDay();
      int start_month     = tag->m_StopTime.GetMonth();
      int start_year      = tag->m_StopTime.GetYear();
      int start_hour      = timestop.GetHour();
      int start_minute    = timestop.GetMinute();
      tag->m_StopTime.SetDateTime(start_year, start_month, start_day, start_hour, start_minute, 0);

      timerEndTimeStr = tag->m_StopTime.GetAsLocalizedTime("", false);
      UpdateSetting(CONTROL_TMR_END);
    }
  }
  else if (setting.id == CONTROL_TMR_FIRST_DAY && m_tmp_day <= 10)
  {
    if (m_tmp_iFirstDay > 0)
      tag->m_FirstDay = CDateTime::GetCurrentDateTime() + CDateTimeSpan(m_tmp_iFirstDay-1, 0, 0, 0);
    else
      tag->m_FirstDay = NULL;
  }
}

void CGUIDialogPVRTimerSettings::SetTimer(CFileItem *item)
{
  m_timerItem         = item;
  m_cancelled         = true;

  m_timerItem->GetPVRTimerInfoTag()->m_StartTime.GetAsSystemTime(timerStartTime);
  m_timerItem->GetPVRTimerInfoTag()->m_StopTime.GetAsSystemTime(timerEndTime);
  timerStartTimeStr   = m_timerItem->GetPVRTimerInfoTag()->m_StartTime.GetAsLocalizedTime("", false);
  timerEndTimeStr     = m_timerItem->GetPVRTimerInfoTag()->m_StopTime.GetAsLocalizedTime("", false);

  m_tmp_iFirstDay     = 0;
  m_tmp_day           = 11;
}

void CGUIDialogPVRTimerSettings::OnOkay()
{
  m_cancelled = false;
  CPVRTimerInfoTag* tag = m_timerItem->GetPVRTimerInfoTag();
  if (tag->Title() == g_localizeStrings.Get(19056))
    tag->SetTitle(CPVRManager::GetChannelGroups()->GetByClientFromAll(tag->ClientNumber(), tag->ClientID())->ChannelName());
}
