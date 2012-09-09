#pragma once
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

#include "XBDateTime.h"
#include "settings/GUIDialogSettings.h"
#include "guilib/GUIListItem.h"

class CFileItem;

namespace PVR
{
  class CPVRTimerInfoTag;

  class CGUIDialogPVRTimerSettings : public CGUIDialogSettings
  {
  public:
    CGUIDialogPVRTimerSettings(void);
    virtual ~CGUIDialogPVRTimerSettings(void) {}
    void SetTimer(CFileItem *item);
    bool GetOK() { return !m_cancelled; }

  protected:
    virtual void CreateSettings();
    virtual void OnSettingChanged(SettingInfo &setting);
    virtual void OnOkay();
    virtual void OnCancel() { m_cancelled = true; }
    virtual void AddChannelNames(CFileItemList &channelsList, SETTINGSTRINGS &channelNames, bool bRadio);
    virtual void SetWeekdaySettingFromTimer(const CPVRTimerInfoTag &timer);
    virtual void SetTimerFromWeekdaySetting(CPVRTimerInfoTag &timer);

    SYSTEMTIME      timerStartTime;
    SYSTEMTIME      timerEndTime;
    CStdString      timerStartTimeStr;
    CStdString      timerEndTimeStr;
    int             m_tmp_iFirstDay;;
    int             m_tmp_day;
    bool            m_bTimerActive;

    CFileItem      *m_timerItem;
    bool            m_cancelled;
  };
}
