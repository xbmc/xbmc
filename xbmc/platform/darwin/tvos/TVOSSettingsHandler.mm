/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "TVOSSettingsHandler.h"

#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"

#import "platform/darwin/tvos/XBMCController.h"
#import "platform/darwin/tvos/input/LibInputHandler.h"
#import "platform/darwin/tvos/input/LibInputSettings.h"

#include <mutex>

namespace
{

std::mutex singletonMutex;

}

CTVOSInputSettings* CTVOSInputSettings::m_instance = nullptr;

CTVOSInputSettings& CTVOSInputSettings::GetInstance()
{
  std::lock_guard<std::mutex> lock(singletonMutex);
  if (!m_instance)
    m_instance = new CTVOSInputSettings();

  return *m_instance;
}

void CTVOSInputSettings::Initialize()
{
  bool idleTimerEnabled = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
      CSettings::SETTING_INPUT_SIRIREMOTEIDLETIMERENABLED);
  [g_xbmcController.inputHandler.inputSettings setSiriRemoteIdleTimerEnabled:idleTimerEnabled];
  int idleTime = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_INPUT_SIRIREMOTEIDLETIME);
  [g_xbmcController.inputHandler.inputSettings setSiriRemoteIdleTime:idleTime];
  int panHorizontalSensitivity = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_INPUT_SIRIREMOTEHORIZONTALSENSITIVITY);
  g_xbmcController.inputHandler.inputSettings.siriRemoteHorizontalSensitivity =
      panHorizontalSensitivity;
  int panVerticalSensitivity = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      CSettings::SETTING_INPUT_SIRIREMOTEVERTICALSENSITIVITY);
  g_xbmcController.inputHandler.inputSettings.siriRemoteVerticalSensitivity =
      panVerticalSensitivity;
}

void CTVOSInputSettings::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  if (setting == nullptr)
    return;

  const std::string& settingId = setting->GetId();
  if (settingId == CSettings::SETTING_INPUT_SIRIREMOTEIDLETIMERENABLED)
  {
    bool idleTimerEnabled = std::dynamic_pointer_cast<const CSettingBool>(setting)->GetValue();
    [g_xbmcController.inputHandler.inputSettings setSiriRemoteIdleTimerEnabled:idleTimerEnabled];
  }
  else if (settingId == CSettings::SETTING_INPUT_SIRIREMOTEIDLETIME)
  {
    int idleTime = std::dynamic_pointer_cast<const CSettingInt>(setting)->GetValue();
    [g_xbmcController.inputHandler.inputSettings setSiriRemoteIdleTime:idleTime];
  }
  else if (settingId == CSettings::SETTING_INPUT_SIRIREMOTEHORIZONTALSENSITIVITY)
  {
    int panHorizontalSensitivity =
        std::dynamic_pointer_cast<const CSettingInt>(setting)->GetValue();
    g_xbmcController.inputHandler.inputSettings.siriRemoteHorizontalSensitivity =
        panHorizontalSensitivity;
  }
  else if (settingId == CSettings::SETTING_INPUT_SIRIREMOTEVERTICALSENSITIVITY)
  {
    int panVerticalSensitivity = std::dynamic_pointer_cast<const CSettingInt>(setting)->GetValue();
    g_xbmcController.inputHandler.inputSettings.siriRemoteVerticalSensitivity =
        panVerticalSensitivity;
  }
}
