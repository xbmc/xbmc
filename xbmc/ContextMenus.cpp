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
#include "input/WindowTranslator.h"
#include "storage/MediaManager.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

namespace CONTEXTMENU
{

  bool CEjectDisk::IsVisible(const CFileItem& item) const
  {
#ifdef HAS_DVD_DRIVE
    return item.IsRemovable() && (item.IsDVD() || item.IsCDDA());
#else
    return false;
#endif
  }

  bool CEjectDisk::Execute(const CFileItemPtr& item) const
  {
#ifdef HAS_DVD_DRIVE
    CServiceBroker::GetMediaManager().ToggleTray(
        CServiceBroker::GetMediaManager().TranslateDevicePath(item->GetPath())[0]);
#endif
    return true;
  }

  bool CEjectDrive::IsVisible(const CFileItem& item) const
  {
    // Must be HDD
    return item.IsRemovable() && !item.IsDVD() && !item.IsCDDA();
  }

  bool CEjectDrive::Execute(const CFileItemPtr& item) const
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
  return !item.IsParentFolder() &&
         !item.IsPath("add") &&
         !item.IsPath("newplaylist://") &&
         !URIUtils::IsProtocol(item.GetPath(), "favourites") &&
         !URIUtils::IsProtocol(item.GetPath(), "newsmartplaylist") &&
         !URIUtils::IsProtocol(item.GetPath(), "newtag") &&
         !URIUtils::IsProtocol(item.GetPath(), "musicsearch") &&
         !StringUtils::StartsWith(item.GetPath(), "pvr://guide/") &&
         !StringUtils::StartsWith(item.GetPath(), "pvr://timers/") &&
         !item.GetPath().empty();
}

bool CAddRemoveFavourite::Execute(const CFileItemPtr& item) const
{
  return CServiceBroker::GetFavouritesService().AddOrRemove(*item.get(), GetTargetWindowID(*item));
}

} // namespace CONTEXTMENU
