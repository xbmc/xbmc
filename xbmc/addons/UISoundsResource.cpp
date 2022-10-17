/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "UISoundsResource.h"

#include "ServiceBroker.h"
#include "addons/addoninfo/AddonType.h"
#include "guilib/GUIAudioManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"


namespace ADDON
{

CUISoundsResource::CUISoundsResource(const AddonInfoPtr& addonInfo)
  : CResource(addonInfo, AddonType::RESOURCE_UISOUNDS)
{
}

bool CUISoundsResource::IsAllowed(const std::string& file) const
{
  return StringUtils::EqualsNoCase(file, "sounds.xml")
      || URIUtils::HasExtension(file, ".wav");
}

bool CUISoundsResource::IsInUse() const
{
  return CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN) == ID();
}

void CUISoundsResource::OnPostInstall(bool update, bool modal)
{
  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (IsInUse() && gui)
    gui->GetAudioManager().Load();
}

}
