/*
 *  Copyright (C) 2009-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DPMSSupport.h"

#include "Application.h"
#include "interfaces/AnnouncementManager.h"
#include "ServiceBroker.h"
#include "settings/lib/Setting.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

#include <array>
#include <string>

CDPMSSupport::CDPMSSupport()
{
  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (settingsComponent)
  {
    auto settings = settingsComponent->GetSettings();
    if (settings)
    {
      auto setting = settings->GetSetting(CSettings::SETTING_POWERMANAGEMENT_DISPLAYSOFF);
      if (setting)
        setting->SetRequirementsMet(true);
    }
  }
}

bool CDPMSSupport::IsModeSupported(PowerSavingMode mode) const
{
  for (const auto& supportedModes : m_supportedModes)
  {
    if (supportedModes == mode)
      return true;
  }

  return false;
}

void CDPMSSupport::Deactivate()
{
  if (!m_active)
    return;

  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::GUI, "xbmc", "OnDPMSDeactivated");
  DisablePowerSaving();
  g_application.SetRenderGUI(true);
  m_active = false;
}

void CDPMSSupport::Activate()
{
  if (m_active)
    return;

  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::GUI, "xbmc", "OnDPMSActivated");
  EnablePowerSaving(GetSupportedModes().front());
  g_application.SetRenderGUI(false);
  m_active = true;
}

