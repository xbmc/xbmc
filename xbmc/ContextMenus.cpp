/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenus.h"

#include "ServiceBroker.h"
#include "favourites/FavouritesService.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/WindowTranslator.h"
#include "music/MusicFileItemClassify.h"
#include "storage/MediaManager.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

using namespace KODI;

namespace CONTEXTMENU
{

  bool CEjectDisk::IsVisible(const CFileItem& item) const
  {
#ifdef HAS_OPTICAL_DRIVE
    return item.IsRemovable() && (item.IsDVD() || MUSIC::IsCDDA(item));
#else
    return false;
#endif
  }

  bool CEjectDisk::Execute(const std::shared_ptr<CFileItem>& item) const
  {
#ifdef HAS_OPTICAL_DRIVE
    CServiceBroker::GetMediaManager().ToggleTray(
        CServiceBroker::GetMediaManager().TranslateDevicePath(item->GetPath())[0]);
#endif
    return true;
  }

  bool CEjectDrive::IsVisible(const CFileItem& item) const
  {
    // Must be HDD
    return item.IsRemovable() && !item.IsDVD() && !MUSIC::IsCDDA(item);
  }

  bool CEjectDrive::Execute(const std::shared_ptr<CFileItem>& item) const
  {
    return CServiceBroker::GetMediaManager().Eject(item->GetPath());
  }

namespace
{

int GetTargetWindowID(const CFileItem& item)
{
  int iTargetWindow = WINDOW_INVALID;

  const std::string targetWindow = item.GetProperty("targetwindow").asString();
  if (targetWindow.empty())
    iTargetWindow = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow();
  else
    iTargetWindow = CWindowTranslator::TranslateWindow(targetWindow);

  return iTargetWindow;
}

} // unnamed namespace

std::string CAddRemoveFavourite::GetLabel(const CFileItem& item) const
{
  return g_localizeStrings.Get(CServiceBroker::GetFavouritesService().IsFavourited(item, GetTargetWindowID(item))
                               ? 14077   /* Remove from favourites */
                               : 14076); /* Add to favourites */
}

bool CAddRemoveFavourite::IsVisible(const CFileItem& item) const
{
  if (item.GetProperty("hide_add_remove_favourite").asBoolean())
    return false;

  return (!item.GetPath().empty() && !item.IsParentFolder() && !item.IsPath("add") &&
          !item.IsPath("newplaylist://") && !URIUtils::IsProtocol(item.GetPath(), "favourites") &&
          !URIUtils::IsProtocol(item.GetPath(), "newsmartplaylist") &&
          !URIUtils::IsProtocol(item.GetPath(), "newtag") &&
          !URIUtils::IsProtocol(item.GetPath(), "musicsearch") &&
          // Hide this item for all PVR EPG/timers/search except EPG/timer/timer rules/search root
          // folders.
          !StringUtils::StartsWith(item.GetPath(), "pvr://guide/") &&
          !StringUtils::StartsWith(item.GetPath(), "pvr://timers/") &&
          !StringUtils::StartsWith(item.GetPath(), "pvr://search/")) ||
         item.GetPath() == "pvr://guide/tv/" || item.GetPath() == "pvr://guide/radio/" ||
         item.GetPath() == "pvr://timers/tv/timers/" ||
         item.GetPath() == "pvr://timers/radio/timers/" ||
         item.GetPath() == "pvr://timers/tv/rules/" ||
         item.GetPath() == "pvr://timers/radio/rules/" || item.GetPath() == "pvr://search/tv/" ||
         item.GetPath() == "pvr://search/radio/";
}

bool CAddRemoveFavourite::Execute(const std::shared_ptr<CFileItem>& item) const
{
  return CServiceBroker::GetFavouritesService().AddOrRemove(*item.get(), GetTargetWindowID(*item));
}

} // namespace CONTEXTMENU
