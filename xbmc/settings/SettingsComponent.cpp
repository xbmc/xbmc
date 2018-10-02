/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingsComponent.h"

#include "AdvancedSettings.h"
#include "ServiceBroker.h"
#include "Settings.h"

CSettingsComponent::CSettingsComponent()
{
  m_advancedSettings.reset(new CAdvancedSettings());
  m_settings.reset(new CSettings());
}

CSettingsComponent::~CSettingsComponent()
{
  Deinit();
}

void CSettingsComponent::Init(const CAppParamParser &params)
{
  m_advancedSettings->Initialize(params);
  CServiceBroker::RegisterSettingsComponent(this);
}

void CSettingsComponent::Deinit()
{
  CServiceBroker::UnregisterSettingsComponent();
  m_advancedSettings->Clear();
}

std::shared_ptr<CSettings> CSettingsComponent::GetSettings()
{
  return m_settings;
}

std::shared_ptr<CAdvancedSettings> CSettingsComponent::GetAdvancedSettings()
{
  return m_advancedSettings;
}
