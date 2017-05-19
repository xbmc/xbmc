/*
*      Copyright (C) 2005-2017 Team XBMC
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
#include "FontResource.h"

#include "AddonManager.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIWindowManager.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "ServiceBroker.h"
#include "settings/Settings.h"

using namespace XFILE;
using namespace KODI::MESSAGING;

using KODI::MESSAGING::HELPERS::DialogResponse;

namespace ADDON
{

std::unique_ptr<CFontResource> CFontResource::FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext)
{
  return std::unique_ptr<CFontResource>(new CFontResource(std::move(addonInfo)));
}

void CFontResource::OnPostInstall(bool update, bool modal)
{
  std::string skin = CServiceBroker::GetSettings().GetString(CSettings::SETTING_LOOKANDFEEL_SKIN);
  const auto& deps = CServiceBroker::GetAddonMgr().GetDepsRecursive(skin);
  for (const auto& it : deps)
    if (it.id == ID())
      CApplicationMessenger::GetInstance().PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, "ReloadSkin");
}

bool CFontResource::GetFont(const std::string& file, std::string& path) const
{
  std::string result = CSpecialProtocol::TranslatePathConvertCase(Path()+"/resources/"+file);
  if (CFile::Exists(result))
  {
    path = result;
    return true;
  }

  return false;
}

}
