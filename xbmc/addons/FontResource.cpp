/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "FontResource.h"

#include "ServiceBroker.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "filesystem/SpecialProtocol.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/FileUtils.h"

namespace ADDON
{

CFontResource::CFontResource(const AddonInfoPtr& addonInfo)
  : CResource(addonInfo, AddonType::RESOURCE_FONT)
{
}

void CFontResource::OnPostInstall(bool update, bool modal)
{
  std::string skin = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_LOOKANDFEEL_SKIN);
  const auto& deps =
      CServiceBroker::GetAddonMgr().GetDepsRecursive(skin, OnlyEnabledRootAddon::CHOICE_YES);
  for (const auto& it : deps)
    if (it.id == ID())
      CServiceBroker::GetAppMessenger()->PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr,
                                                 "ReloadSkin");
}

bool CFontResource::GetFont(const std::string& file, std::string& path) const
{
  std::string result = CSpecialProtocol::TranslatePathConvertCase(Path()+"/resources/"+file);
  if (CFileUtils::Exists(result))
  {
    path = result;
    return true;
  }

  return false;
}

}
