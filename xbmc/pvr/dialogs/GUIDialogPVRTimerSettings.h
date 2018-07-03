/*
 *      Copyright (C) 2012-2014 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h" // PVR_CHANNEL_INVALID_UID
#include "settings/SettingConditions.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"
#include "settings/lib/SettingDependency.h"

#include "pvr/PVRTypes.h"

class CSetting;

namespace PVR
{
  class CGUIDialogPVRTimerSettings : public CGUIDialogSettingsManualBase
  {
  public:
    CGUIDialogPVRTimerSettings();
    ~CGUIDialogPVRTimerSettings() override;

    bool CanBeActivated() const override;

    void SetTimer(const CPVRTimerInfoTagPtr &timer);

  protected:
    // implementation of ISettingCallback
    void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
    void OnSettingAction(std::shared_ptr<const CSetting> setting) override;

    // specialization of CGUIDialogSettingsBase
    bool AllowResettingSettings() const override { return false; }
    void Save() override;
    void SetupView() override;

    // specialization of CGUIDialogSettingsManualBase
    void InitializeSettings() override;

  private:
    void InitializeTypesList();
    void InitializeChannelsList();
    void SetButtonLabels();

    static int  GetDateAsIndex(const CDateTime &datetime);
    static void SetDateFromIndex(CDateTime &datetime, int date);
    static void SetTimeFromSystemTime(CDateTime &datetime, const SYSTEMTIME &time);

    static int GetWeekdaysFromSetting(std::shared_ptr<const CSetting> setting);

    static void TypesFiller(
      std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
    static void ChannelsFiller(
      std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
    static void DaysFiller(
      std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
    static void DupEpisodesFiller(
      std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
    static void WeekdaysFiller(
      std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
    static void PrioritiesFiller(
      std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
    static void LifetimesFiller(
      std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
    static void MaxRecordingsFiller(
      std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
    static void RecordingGroupFiller(
      std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
    static void MarginTimeFiller(
      std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

    static std::string WeekdaysValueFormatter(std::shared_ptr<const CSetting> setting);

    void AddCondition(
      std::shared_ptr<CSetting> setting, const std::string &identifier, SettingConditionCheck condition,
      SettingDependencyType depType, const std::string &settingId);

    void AddTypeDependentEnableCondition(std::shared_ptr<CSetting> setting, const std::string &identifier);
    static bool TypeReadOnlyCondition(
      const std::string &condition, const std::string &value, std::shared_ptr<const CSetting> setting, void *data);

    void AddTypeDependentVisibilityCondition(std::shared_ptr<CSetting> setting, const std::string &identifier);
    static bool TypeSupportsCondition(
      const std::string &condition, const std::string &value, std::shared_ptr<const CSetting> setting, void *data);

    void AddStartAnytimeDependentVisibilityCondition(std::shared_ptr<CSetting> setting, const std::string &identifier);
    static bool StartAnytimeSetCondition(
      const std::string &condition, const std::string &value, std::shared_ptr<const CSetting> setting, void *data);
    void AddEndAnytimeDependentVisibilityCondition(std::shared_ptr<CSetting> setting, const std::string &identifier);
    static bool EndAnytimeSetCondition(
      const std::string &condition, const std::string &value, std::shared_ptr<const CSetting> setting, void *data);

    typedef std::map<int, CPVRTimerTypePtr>  TypeEntriesMap;

    typedef struct ChannelDescriptor
    {
      int         channelUid;
      int         clientId;
      std::string description;

      ChannelDescriptor(int _channelUid = PVR_CHANNEL_INVALID_UID,
                        int _clientId   = -1,
                        const std::string& _description = "")
      : channelUid(_channelUid),
        clientId(_clientId),
        description(_description)
      {}

      inline bool operator ==(const ChannelDescriptor& right) const
      {
        return (channelUid  == right.channelUid &&
                clientId    == right.clientId   &&
                description == right.description);
      }

    } ChannelDescriptor;

    typedef std::map <int, ChannelDescriptor> ChannelEntriesMap;

    CPVRTimerInfoTagPtr m_timerInfoTag;
    TypeEntriesMap      m_typeEntries;
    ChannelEntriesMap   m_channelEntries;
    std::string         m_timerStartTimeStr;
    std::string         m_timerEndTimeStr;

    CPVRTimerTypePtr    m_timerType;
    bool                m_bIsRadio = false;
    bool                m_bIsNewTimer = true;
    bool                m_bTimerActive = false;
    std::string         m_strTitle;
    std::string         m_strEpgSearchString;
    bool                m_bFullTextEpgSearch = true;
    ChannelDescriptor   m_channel;
    CDateTime           m_startLocalTime;
    CDateTime           m_endLocalTime;
    bool                m_bStartAnyTime = false;
    bool                m_bEndAnyTime = false;
    unsigned int        m_iWeekdays;
    CDateTime           m_firstDayLocalTime;
    unsigned int        m_iPreventDupEpisodes = 0;
    unsigned int        m_iMarginStart = 0;
    unsigned int        m_iMarginEnd = 0;
    int                 m_iPriority = 0;
    int                 m_iLifetime = 0;
    int                 m_iMaxRecordings = 0;
    std::string         m_strDirectory;
    unsigned int        m_iRecordingGroup = 0;
  };
} // namespace PVR
