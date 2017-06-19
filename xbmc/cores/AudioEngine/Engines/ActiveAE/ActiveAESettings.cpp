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
#include "guilib/LocalizeStrings.h"
#include "cores/AudioEngine/Interfaces/AE.h"

#include "ServiceBroker.h"
#include "cores/AudioEngine/Engines/ActiveAE/ActiveAE.h"


namespace ActiveAE
{
void CActiveAESettings::SettingOptionsAudioDevicesFiller(SettingConstPtr setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  SettingOptionsAudioDevicesFillerGeneral(setting, list, current, false);
}

void CActiveAESettings::SettingOptionsAudioDevicesPassthroughFiller(SettingConstPtr setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  SettingOptionsAudioDevicesFillerGeneral(setting, list, current, true);
}

void CActiveAESettings::SettingOptionsAudioQualityLevelsFiller(SettingConstPtr setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  if(CServiceBroker::GetActiveAE().SupportsQualityLevel(AE_QUALITY_LOW))
    list.push_back(std::make_pair(g_localizeStrings.Get(13506), AE_QUALITY_LOW));
  if(CServiceBroker::GetActiveAE().SupportsQualityLevel(AE_QUALITY_MID))
    list.push_back(std::make_pair(g_localizeStrings.Get(13507), AE_QUALITY_MID));
  if(CServiceBroker::GetActiveAE().SupportsQualityLevel(AE_QUALITY_HIGH))
    list.push_back(std::make_pair(g_localizeStrings.Get(13508), AE_QUALITY_HIGH));
  if(CServiceBroker::GetActiveAE().SupportsQualityLevel(AE_QUALITY_REALLYHIGH))
    list.push_back(std::make_pair(g_localizeStrings.Get(13509), AE_QUALITY_REALLYHIGH));
  if(CServiceBroker::GetActiveAE().SupportsQualityLevel(AE_QUALITY_GPU))
    list.push_back(std::make_pair(g_localizeStrings.Get(38010), AE_QUALITY_GPU));
}

void CActiveAESettings::SettingOptionsAudioStreamsilenceFiller(SettingConstPtr setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data)
{
  list.push_back(std::make_pair(g_localizeStrings.Get(20422), XbmcThreads::EndTime::InfiniteValue));
  list.push_back(std::make_pair(g_localizeStrings.Get(13551), 0));

  if (CServiceBroker::GetActiveAE().SupportsSilenceTimeout())
  {
    list.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(13554).c_str(), 1), 1));
    for (int i = 2; i <= 10; i++)
    {
      list.push_back(std::make_pair(StringUtils::Format(g_localizeStrings.Get(13555).c_str(), i), i));
    }
  }
}

bool CActiveAESettings::IsSettingVisible(const std::string & condition, const std::string & value, SettingConstPtr  setting, void * data)
{
  if (setting == NULL || value.empty())
    return false;

  return CServiceBroker::GetActiveAE().IsSettingVisible(value);
}

bool CActiveAESettings::SupportsQualitySetting(void)
{
    return ((CServiceBroker::GetActiveAE().SupportsQualityLevel(AE_QUALITY_LOW) ? 1 : 0) +
            (CServiceBroker::GetActiveAE().SupportsQualityLevel(AE_QUALITY_MID) ? 1 : 0) +
            (CServiceBroker::GetActiveAE().SupportsQualityLevel(AE_QUALITY_HIGH) ? 1 : 0)) >= 2;
}

void CActiveAESettings::SettingOptionsAudioDevicesFillerGeneral(SettingConstPtr setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, bool passthrough)
{
  current = std::static_pointer_cast<const CSettingString>(setting)->GetValue();
  std::string firstDevice;

  bool foundValue = false;
  AEDeviceList sinkList;
  CServiceBroker::GetActiveAE().EnumerateOutputDevices(sinkList, passthrough);
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
