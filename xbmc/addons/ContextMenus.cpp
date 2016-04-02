/*
 *      Copyright (C) 2016 Team Kodi
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

#include "ContextMenus.h"
#include "AddonManager.h"
#include "Repository.h"
#include "RepositoryUpdater.h"
#include "GUIDialogAddonInfo.h"
#include "GUIDialogAddonSettings.h"


namespace CONTEXTMENU
{

using namespace ADDON;

bool CAddonSettings::IsVisible(const CFileItem& item) const
{
  AddonPtr addon;
  return item.HasAddonInfo()
         && CAddonMgr::GetInstance().GetAddon(item.GetAddonInfo()->ID(), addon, ADDON_UNKNOWN, false)
         && addon->HasSettings();
}

bool CAddonSettings::Execute(const CFileItemPtr& item) const
{
  AddonPtr addon;
  return CAddonMgr::GetInstance().GetAddon(item->GetAddonInfo()->ID(), addon, ADDON_UNKNOWN, false)
         && CGUIDialogAddonSettings::ShowAndGetInput(addon);
}

bool CCheckForUpdates::IsVisible(const CFileItem& item) const
{
  return item.HasAddonInfo() && item.GetAddonInfo()->Type() == ADDON::ADDON_REPOSITORY;
}

bool CCheckForUpdates::Execute(const CFileItemPtr& item) const
{
  AddonPtr addon;
  if (item->HasAddonInfo() && CAddonMgr::GetInstance().GetAddon(item->GetAddonInfo()->ID(), addon, ADDON_REPOSITORY))
  {
    CRepositoryUpdater::GetInstance().CheckForUpdates(std::static_pointer_cast<CRepository>(addon), true);
    return true;
  }
  return false;
}
}
