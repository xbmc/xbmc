/*
 *      Copyright (C) 2010-2016 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


#include "ActiveAESettings.h"

#include "utils/StringUtils.h"
#include "settings/Settings.h"
#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "cores/AudioEngine/Interfaces/AE.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"
#include "settings/lib/SettingsManager.h"
#include "threads/SingleLock.h"

namespace ActiveAE
{

CActiveAESettings* CActiveAESettings::m_instance = nullptr;

CActiveAESettings::CActiveAESettings(CActiveAE &ae) : m_audioEngine(ae)
{
  CSingleLock lock(m_cs);
  m_instance = this;

  std::set<std::string> settingSet;
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_CONFIG);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_SAMPLERATE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_CHANNELS);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_PROCESSQUALITY);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_ATEMPOTHRESHOLD);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_GUISOUNDMODE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_STEREOUPMIX);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_AC3PASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_AC3TRANSCODE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_EAC3PASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_DTSPASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_TRUEHDPASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_DTSHDPASSTHROUGH);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_AUDIODEVICE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_STREAMSILENCE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_STREAMNOISE);
  settingSet.insert(CSettings::SETTING_AUDIOOUTPUT_MAINTAINORIGINALVOLUME);
  CServiceBroker::GetSettings().GetSettingsManager()->RegisterCallback(this, settingSet);

  CServiceBroker::GetSettings().GetSettingsManager()->RegisterSettingOptionsFiller("aequalitylevels",
                                                                                   SettingOptionsAudioQualityLevelsFiller);
  CServiceBroker::GetSettings().GetSettingsManager()->RegisterSettingOptionsFiller("audiodevices",
                                                                                   SettingOptionsAudioDevicesFiller);
  CServiceBroker::GetSettings().GetSettingsManager()->RegisterSettingOptionsFiller("audiodevicespassthrough",
                                                                                   SettingOptionsAudioDevicesPassthroughFiller);
  CServiceBroker::GetSettings().GetSettingsManager()->RegisterSettingOptionsFiller("audiostreamsilence",
                                                                                   SettingOptionsAudioStreamsilenceFiller);
}

CActiveAESettings::~CActiveAESettings()
{
  CSingleLock lock(m_cs);
  CServiceBroker::GetSettings().GetSettingsManager()->UnregisterSettingOptionsFiller("aequalitylevels");
  CServiceBroker::GetSettings().GetSettingsManager()->UnregisterSettingOptionsFiller("audiodevices");
  CServiceBroker::GetSettings().GetSettingsManager()->UnregisterSettingOptionsFiller("audiodevicespassthrough");
  CServiceBroker::GetSettings().GetSettingsManager()->UnregisterSettingOptionsFiller("audiostreamsilence");
  CServiceBroker::GetSettings().GetSettingsManager()->UnregisterCallback(this);
  m_instance = nullptr;
}

void CActiveAESettings::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
  CSingleLock lock(m_cs);
  m_instance->m_audioEngine.OnSettingsChange();
}

void CActiveAESettings::SettingOptionsAudioDevicesFiller(SettingConstPtr setting,
                                                         std::vector< std::pair<std::string, std::string> > &list,
                                                         std::string &current, void *data)
{
  SettingOptionsAudioDevicesFillerGeneral(setting, list, current, false);
}

void CActiveAESettings::SettingOptionsAudioDevicesPassthroughFiller(SettingConstPtr setting,
                                                                    std::vector< std::pair<std::string, std::string> > &list,
                                                                    std::string &current, void *data)
{
  SettingOptionsAudioDevicesFillerGeneral(setting, list, current, true);
}

void CActiveAESettings::SettingOptionsAudioQualityLevelsFiller(SettingConstPtr setting,
                                                               std::vector< std::pair<std::string, int> > &list,
                                                               int &current, void *data)
{
  CSingleLock lock(m_instance->m_cs);

  if (m_instance->m_audioEngine.SupportsQualityLevel(AE_QUALITY_LOW))
    list.push_back(std::make_pair(g_localizeStrings.Get(13506), AE_QUALITY_LOW));
  if (m_instance->m_audioEngine.SupportsQualityLevel(AE_QUALITY_MID))
    list.push_back(std::make_pair(g_localizeStrings.Get(13507), AE_QUALITY_MID));
  if (m_instance->m_audioEngine.SupportsQualityLevel(AE_QUALITY_HIGH))
    list.push_back(std::make_pair(g_localizeStrings.Get(13508), AE_QUALITY_HIGH));
  if (m_instance->m_audioEngine.SupportsQualityLevel(AE_QUALITY_REALLYHIGH))
    list.push_back(std::make_pair(g_localizeStrings.Get(13509), AE_QUALITY_REALLYHIGH));
  if (m_instance->m_audioEngine.SupportsQualityLevel(AE_QUALITY_GPU))
    list.push_back(std::make_pair(g_localizeStrings.Get(38010), AE_QUALITY_GPU));
}

void CActiveAESettings::SettingOptionsAudioStreamsilenceFiller(SettingConstPtr setting,
                                                               std::vector< std::pair<std::string, int> > &list,
                                                               int &current, void *data)
{
  CSingleLock lock(m_instance->m_cs);

  list.push_back(std::make_pair(g_localizeStrings.Get(20422), XbmcThreads::EndTime::InfiniteValue));
  list.push_back(std::make_pair(g_localizeStrings.Get(13551), 0));

  if (m_instance->m_audioEngine.SupportsSilenceTimeout())
  {
    list.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(13554).c_str(), 1), 1));
    for (int i = 2; i <= 10; i++)
    {
      list.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(13555).c_str(), i), i));
    }
  }
}

bool CActiveAESettings::IsSettingVisible(const std::string & condition, const std::string & value,
                                         SettingConstPtr  setting, void * data)
{
  if (setting == NULL || value.empty())
    return false;

  CSingleLock lock(m_instance->m_cs);
  if (!m_instance)
    return false;

  return m_instance->m_audioEngine.IsSettingVisible(value);
}

void CActiveAESettings::SettingOptionsAudioDevicesFillerGeneral(SettingConstPtr setting,
                                                                std::vector< std::pair<std::string, std::string> > &list,
                                                                std::string &current, bool passthrough)
{
  current = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  std::string firstDevice;

  CSingleLock lock(m_instance->m_cs);

  bool foundValue = false;
  AEDeviceList sinkList;
  m_instance->m_audioEngine.EnumerateOutputDevices(sinkList, passthrough);
  if (sinkList.empty())
    list.push_back(std::make_pair("Error - no devices found", "error"));
  else
  {
    for (AEDeviceList::const_iterator sink = sinkList.begin(); sink != sinkList.end(); ++sink)
    {
      if (sink == sinkList.begin())
        firstDevice = sink->second;

      list.push_back(std::make_pair(sink->first, sink->second));

      if (StringUtils::EqualsNoCase(current, sink->second))
        foundValue = true;
    }
  }

  if (!foundValue)
    current = firstDevice;
}
}
