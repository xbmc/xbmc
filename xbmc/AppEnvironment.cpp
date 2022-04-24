/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AppEnvironment.h"

#include "ServiceBroker.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

void CAppEnvironment::SetUp(const std::shared_ptr<CAppParams>& appParams)
{
  CServiceBroker::RegisterAppParams(appParams);
  CServiceBroker::CreateLogging();
  const auto settingsComponent = std::make_shared<CSettingsComponent>();
  settingsComponent->Initialize();
  CServiceBroker::RegisterSettingsComponent(settingsComponent);
}

void CAppEnvironment::TearDown()
{
  CServiceBroker::GetLogging().UnregisterFromSettings();
  CServiceBroker::GetSettingsComponent()->Deinitialize();
  CServiceBroker::UnregisterSettingsComponent();
  CServiceBroker::GetLogging().Deinitialize();
  CServiceBroker::DestroyLogging();
  CServiceBroker::UnregisterAppParams();
}
