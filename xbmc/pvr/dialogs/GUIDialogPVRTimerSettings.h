#pragma once
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

#include <map>

#include "settings/dialogs/GUIDialogSettingsManualBase.h"

class CFileItem;
class CSetting;
class CSettingGroup;

namespace PVR
{
  class CPVRTimerInfoTag;

  class CGUIDialogPVRTimerSettings : public CGUIDialogSettingsManualBase
  {
  public:
    CGUIDialogPVRTimerSettings();
    virtual ~CGUIDialogPVRTimerSettings() { }

    void SetTimer(CFileItem *item);

  protected:
    // implementations of ISettingCallback
    virtual void OnSettingChanged(const CSetting *setting);
    virtual void OnSettingAction(const CSetting *setting);

    // specialization of CGUIDialogSettingsBase
    virtual bool AllowResettingSettings() const { return false; }
    virtual void Save();
    virtual void SetupView();

    // specialization of CGUIDialogSettingsManualBase
    virtual void InitializeSettings();

    virtual CSetting* AddChannelNames(CSettingGroup *group, bool bRadio);
    virtual void SetWeekdaySettingFromTimer(const CPVRTimerInfoTag &timer);
    virtual void SetTimerFromWeekdaySetting(CPVRTimerInfoTag &timer);

    void getChannelNames(bool bRadio, std::vector< std::pair<std::string, int> > &list, int &current, bool updateChannelEntries = false);
    void setButtonLabels();

    static bool IsTimerDayRepeating(const std::string &condition, const std::string &value, const CSetting *setting);

    static void ChannelNamesOptionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
    static void DaysOptionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

    SYSTEMTIME                          m_timerStartTime;
    SYSTEMTIME                          m_timerEndTime;
    std::string                         m_timerStartTimeStr;
    std::string                         m_timerEndTimeStr;
    int                                 m_tmp_iFirstDay;
    int                                 m_tmp_day;
    bool                                m_bTimerActive;
    int                                 m_selectedChannelEntry;
    std::map<std::pair<bool, int>, int> m_channelEntries;

    CFileItem                          *m_timerItem;
  };
}
