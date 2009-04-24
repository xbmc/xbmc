/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "GUIDialogTVTimerSettings.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogNumeric.h"
#include "GUISettings.h"
#include "PVRManager.h"
#include "utils/TVTimerInfoTag.h"

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

CGUIDialogTVTimerSettings::CGUIDialogTVTimerSettings(void)
    : CGUIDialogSettings(WINDOW_DIALOG_TV_TIMER_SETTING, "DialogTimerSettings.xml")
{
  m_cancelled         = true;
  m_tmp_day           = 11;
  m_tmp_iStartTime    = 0;
  m_tmp_iStopTime     = 0;
}

CGUIDialogTVTimerSettings::~CGUIDialogTVTimerSettings(void)
{
}

void CGUIDialogTVTimerSettings::CreateSettings()
{
  CTVTimerInfoTag* tag = m_timerItem->GetTVTimerInfoTag();

  // clear out any old settings
  m_settings.clear();

  // create our settings controls
  AddBool(CONTROL_TMR_ACTIVE, 18401, &tag->m_Active);
  AddButton(CONTROL_TMR_NAME, 18063, &tag->m_strTitle, true);
  AddBool(CONTROL_TMR_RADIO, 18409, &tag->m_Radio);

  /// Channel names
  {
    // For TV
    CFileItemList channelslist_tv;
    SETTINGSTRINGS channelstrings_tv;
    /*CPVRManager::GetInstance()->GetTVChannels(&channelslist_tv, -1, false, !g_guiSettings.GetBool("pvrmenu.ftaonly"));*/

    channelstrings_tv.push_back("0 dummy");

    for (int i = 0; i < channelslist_tv.Size(); i++)
    {
      CStdString string;
      CFileItemPtr item = channelslist_tv[i];
      string.Format("%i %s", item->GetTVChannelInfoTag()->m_iChannelNum, item->GetTVChannelInfoTag()->m_strChannel.c_str());
      channelstrings_tv.push_back(string);
    }

    AddSpin(CONTROL_TMR_CHNAME_TV, 18402, &tag->m_channelNum, channelstrings_tv.size(), channelstrings_tv);
    EnableSettings(CONTROL_TMR_CHNAME_TV, !tag->m_Radio);

	// For Radio
    CFileItemList channelslist_radio;
    SETTINGSTRINGS channelstrings_radio;
    /*CPVRManager::GetInstance()->GetRadioChannels(&channelslist_radio, -1, false, !g_guiSettings.GetBool("pvrmenu.ftaonly"));*/

    channelstrings_radio.push_back("0 dummy");

    for (int i = 0; i < channelslist_radio.Size(); i++)
    {
      CStdString string;
      CFileItemPtr item = channelslist_radio[i];
      string.Format("%i %s", item->GetTVChannelInfoTag()->m_iChannelNum, item->GetTVChannelInfoTag()->m_strChannel.c_str());
      channelstrings_radio.push_back(string);
    }

    AddSpin(CONTROL_TMR_CHNAME_RADIO, 18402, &tag->m_channelNum, channelstrings_radio.size(), channelstrings_radio);
    EnableSettings(CONTROL_TMR_CHNAME_RADIO, tag->m_Radio);
  }

  /// Day
  {
    SETTINGSTRINGS daystrings;
    tm time_cur;
    tm time_tmr;

    daystrings.push_back(g_localizeStrings.Get(18300));
    daystrings.push_back(g_localizeStrings.Get(18301));
    daystrings.push_back(g_localizeStrings.Get(18302));
    daystrings.push_back(g_localizeStrings.Get(18303));
    daystrings.push_back(g_localizeStrings.Get(18304));
    daystrings.push_back(g_localizeStrings.Get(18305));
    daystrings.push_back(g_localizeStrings.Get(18306));
    daystrings.push_back(g_localizeStrings.Get(18307));
    daystrings.push_back(g_localizeStrings.Get(18308));
    daystrings.push_back(g_localizeStrings.Get(18309));
    daystrings.push_back(g_localizeStrings.Get(18310));
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

    if (tag->m_Repeat)
    {
      if (tag->m_Repeat_Mon && !tag->m_Repeat_Tue && !tag->m_Repeat_Wed &&
          !tag->m_Repeat_Thu && !tag->m_Repeat_Fri && !tag->m_Repeat_Sat &&
          !tag->m_Repeat_Sun)
      {
        m_tmp_day = 0;
      }
      else if (!tag->m_Repeat_Mon &&  tag->m_Repeat_Tue && !tag->m_Repeat_Wed &&
               !tag->m_Repeat_Thu && !tag->m_Repeat_Fri && !tag->m_Repeat_Sat &&
               !tag->m_Repeat_Sun)
      {
        m_tmp_day = 1;
      }
      else if (!tag->m_Repeat_Mon && !tag->m_Repeat_Tue &&  tag->m_Repeat_Wed &&
               !tag->m_Repeat_Thu && !tag->m_Repeat_Fri && !tag->m_Repeat_Sat &&
               !tag->m_Repeat_Sun)
      {
        m_tmp_day = 2;
      }
      else if (!tag->m_Repeat_Mon && !tag->m_Repeat_Tue && !tag->m_Repeat_Wed &&
               tag->m_Repeat_Thu && !tag->m_Repeat_Fri && !tag->m_Repeat_Sat &&
               !tag->m_Repeat_Sun)
      {
        m_tmp_day = 3;
      }
      else if (!tag->m_Repeat_Mon && !tag->m_Repeat_Tue && !tag->m_Repeat_Wed &&
               !tag->m_Repeat_Thu &&  tag->m_Repeat_Fri && !tag->m_Repeat_Sat &&
               !tag->m_Repeat_Sun)
      {
        m_tmp_day = 4;
      }
      else if (!tag->m_Repeat_Mon && !tag->m_Repeat_Tue && !tag->m_Repeat_Wed &&
               !tag->m_Repeat_Thu && !tag->m_Repeat_Fri && tag->m_Repeat_Sat &&
               !tag->m_Repeat_Sun)
      {
        m_tmp_day = 5;
      }
      else if (!tag->m_Repeat_Mon && !tag->m_Repeat_Tue && !tag->m_Repeat_Wed &&
               !tag->m_Repeat_Thu && !tag->m_Repeat_Fri && !tag->m_Repeat_Sat &&
               tag->m_Repeat_Sun)
      {
        m_tmp_day = 6;
      }
      else if (tag->m_Repeat_Mon && tag->m_Repeat_Tue && tag->m_Repeat_Wed &&
               tag->m_Repeat_Thu && tag->m_Repeat_Fri && !tag->m_Repeat_Sat &&
               !tag->m_Repeat_Sun)
      {
        m_tmp_day = 7;
      }
      else if (tag->m_Repeat_Mon && tag->m_Repeat_Tue && tag->m_Repeat_Wed &&
               tag->m_Repeat_Thu && tag->m_Repeat_Fri && tag->m_Repeat_Sat &&
               !tag->m_Repeat_Sun)
      {
        m_tmp_day = 8;
      }
      else if (tag->m_Repeat_Mon && tag->m_Repeat_Tue && tag->m_Repeat_Wed &&
               tag->m_Repeat_Thu && tag->m_Repeat_Fri && tag->m_Repeat_Sat &&
               tag->m_Repeat_Sun)
      {
        m_tmp_day = 9;
      }
      else if (!tag->m_Repeat_Mon && !tag->m_Repeat_Tue && !tag->m_Repeat_Wed &&
               !tag->m_Repeat_Thu && !tag->m_Repeat_Fri &&  tag->m_Repeat_Sat &&
               tag->m_Repeat_Sun)
      {
        m_tmp_day = 10;
      }
    }

    AddSpin(CONTROL_TMR_DAY, 18403, &m_tmp_day, daystrings.size(), daystrings);
  }

  AddButton(CONTROL_TMR_BEGIN, 18404, &timerStartTimeStr, true);
  AddButton(CONTROL_TMR_END, 18405, &timerEndTimeStr, true);
  AddSpin(CONTROL_TMR_PRIORITY, 18406, &tag->m_Priority, 0, 99);
  AddSpin(CONTROL_TMR_LIFETIME, 18407, &tag->m_Lifetime, 0, 365);

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

    daystrings.push_back(g_localizeStrings.Get(18150));

    for (int i = 1; i < 365; ++i)
    {
      CStdString string = time.GetAsLocalizedDate();
      daystrings.push_back(string);
      time += CDateTimeSpan(1, 0, 0, 0);
    }

    AddSpin(CONTROL_TMR_FIRST_DAY, 18408, &m_tmp_iFirstDay, daystrings.size(), daystrings);

    if (tag->m_Repeat)
      EnableSettings(CONTROL_TMR_FIRST_DAY, true);
    else
      EnableSettings(CONTROL_TMR_FIRST_DAY, false);
  }
}

void CGUIDialogTVTimerSettings::OnSettingChanged(unsigned int num)
{
  // setting has changed - update anything that needs it
  if (num >= m_settings.size()) return;

  SettingInfo &setting = m_settings.at(num);

  CTVTimerInfoTag* tag = m_timerItem->GetTVTimerInfoTag();

  if (setting.id == CONTROL_TMR_NAME)
  {
    if (CGUIDialogKeyboard::ShowAndGetInput(tag->m_strTitle, g_localizeStrings.Get(18207), false))
    {
      UpdateSetting(CONTROL_TMR_NAME);
    }
  }
  else if (setting.id == CONTROL_TMR_RADIO)
  {
    EnableSettings(CONTROL_TMR_CHNAME_TV, !tag->m_Radio);
    EnableSettings(CONTROL_TMR_CHNAME_RADIO, tag->m_Radio);
    /*tag->m_strChannel = CPVRManager::GetInstance()->GetNameForChannel(tag->m_channelNum, tag->m_Radio);
    tag->m_clientNum = CPVRManager::GetInstance()->GetClientChannelNumber(tag->m_channelNum, tag->m_Radio);*/
  }
  else if (setting.id == CONTROL_TMR_CHNAME_TV || setting.id == CONTROL_TMR_CHNAME_RADIO)
  {
    /*tag->m_strChannel = CPVRManager::GetInstance()->GetNameForChannel(tag->m_channelNum, tag->m_Radio);
    tag->m_clientNum = CPVRManager::GetInstance()->GetClientChannelNumber(tag->m_channelNum, tag->m_Radio);*/
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

	tag->m_Repeat = false;
    tag->m_Repeat_Mon = false;
    tag->m_Repeat_Tue = false;
    tag->m_Repeat_Wed = false;
    tag->m_Repeat_Thu = false;
    tag->m_Repeat_Fri = false;
    tag->m_Repeat_Sat = false;
    tag->m_Repeat_Sun = false;
  }
  else if (setting.id == CONTROL_TMR_DAY && m_tmp_day <= 10)
  {
    EnableSettings(CONTROL_TMR_FIRST_DAY, true);
    tag->m_Repeat = true;
    tag->m_Repeat_Mon = false;
    tag->m_Repeat_Tue = false;
    tag->m_Repeat_Wed = false;
    tag->m_Repeat_Thu = false;
    tag->m_Repeat_Fri = false;
    tag->m_Repeat_Sat = false;
    tag->m_Repeat_Sun = false;

    if (m_tmp_day == 0)
    {
      tag->m_Repeat_Mon = true;
    }
    else if (m_tmp_day == 1)
    {
      tag->m_Repeat_Tue = true;
    }
    else if (m_tmp_day == 2)
    {
      tag->m_Repeat_Wed = true;
    }
    else if (m_tmp_day == 3)
    {
      tag->m_Repeat_Thu = true;
    }
    else if (m_tmp_day == 4)
    {
      tag->m_Repeat_Fri = true;
    }
    else if (m_tmp_day == 5)
    {
      tag->m_Repeat_Sat = true;
    }
    else if (m_tmp_day == 6)
    {
      tag->m_Repeat_Sun = true;
    }
    else if (m_tmp_day == 7)
    {
      tag->m_Repeat_Mon = true;
      tag->m_Repeat_Tue = true;
      tag->m_Repeat_Wed = true;
      tag->m_Repeat_Thu = true;
      tag->m_Repeat_Fri = true;
    }
    else if (m_tmp_day == 8)
    {
      tag->m_Repeat_Mon = true;
      tag->m_Repeat_Tue = true;
      tag->m_Repeat_Wed = true;
      tag->m_Repeat_Thu = true;
      tag->m_Repeat_Fri = true;
      tag->m_Repeat_Sat = true;
    }
    else if (m_tmp_day == 9)
    {
      tag->m_Repeat_Mon = true;
      tag->m_Repeat_Tue = true;
      tag->m_Repeat_Wed = true;
      tag->m_Repeat_Thu = true;
      tag->m_Repeat_Fri = true;
      tag->m_Repeat_Sat = true;
      tag->m_Repeat_Sun = true;
    }
    else if (m_tmp_day == 10)
    {
      tag->m_Repeat_Sat = true;
      tag->m_Repeat_Sun = true;
    }
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
      int start_day       = tag->m_StartTime.GetDay();
      int start_month     = tag->m_StartTime.GetMonth();
      int start_year      = tag->m_StartTime.GetYear();
      int start_hour      = timestop.GetHour();
      int start_minute    = timestop.GetMinute();
      tag->m_StartTime.SetDateTime(start_year, start_month, start_day, start_hour, start_minute, 0);

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

void CGUIDialogTVTimerSettings::SetTimer(CFileItem *item)
{
  m_timerItem         = item;
  m_cancelled         = true;

  m_timerItem->GetTVTimerInfoTag()->m_StartTime.GetAsSystemTime(timerStartTime);
  m_timerItem->GetTVTimerInfoTag()->m_StopTime.GetAsSystemTime(timerEndTime);
  timerStartTimeStr   = m_timerItem->GetTVTimerInfoTag()->m_StartTime.GetAsLocalizedTime("", false);
  timerEndTimeStr     = m_timerItem->GetTVTimerInfoTag()->m_StopTime.GetAsLocalizedTime("", false);

  m_tmp_iStartTime    = 0;
  m_tmp_iStopTime     = 0;
  m_tmp_iFirstDay     = 0;
  m_tmp_day           = 11;
}
