/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenus.h"

#include "ContextMenuManager.h"
#include "FileItem.h"
#include "ServiceBroker.h"
#include "favourites/FavouritesService.h"
#include "favourites/FavouritesURL.h"
#include "favourites/FavouritesUtils.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/guilib/GUIContentUtils.h"
#include "video/VideoUtils.h"

using namespace CONTEXTMENU;

bool CFavouriteContextMenuAction::IsVisible(const CFileItem& item) const
{
  return URIUtils::IsProtocol(item.GetPath(), "favourites");
}

bool CFavouriteContextMenuAction::Execute(const std::shared_ptr<CFileItem>& item) const
{
  CFileItemList items;
  CServiceBroker::GetFavouritesService().GetAll(items);

  const auto it = std::find_if(items.cbegin(), items.cend(), [&item](const auto& favourite) {
    return favourite->GetPath() == item->GetPath();
  });

  if ((it != items.cend()) && DoExecute(items, *it))
    return CServiceBroker::GetFavouritesService().Save(items);

  return false;
}

bool CMoveUpFavourite::DoExecute(CFileItemList& items, const std::shared_ptr<CFileItem>& item) const
{
  return FAVOURITES_UTILS::MoveItem(items, item, -1);
}

bool CMoveUpFavourite::IsVisible(const CFileItem& item) const
{
  return CFavouriteContextMenuAction::IsVisible(item) && FAVOURITES_UTILS::ShouldEnableMoveItems();
}

bool CMoveDownFavourite::DoExecute(CFileItemList& items,
                                   const std::shared_ptr<CFileItem>& item) const
{
  return FAVOURITES_UTILS::MoveItem(items, item, +1);
}

bool CMoveDownFavourite::IsVisible(const CFileItem& item) const
{
  return CFavouriteContextMenuAction::IsVisible(item) && FAVOURITES_UTILS::ShouldEnableMoveItems();
}

bool CRemoveFavourite::DoExecute(CFileItemList& items, const std::shared_ptr<CFileItem>& item) const
{
  return FAVOURITES_UTILS::RemoveItem(items, item);
}

bool CRenameFavourite::DoExecute(CFileItemList&, const std::shared_ptr<CFileItem>& item) const
{
  return FAVOURITES_UTILS::ChooseAndSetNewName(*item);
}

bool CChooseThumbnailForFavourite::DoExecute(CFileItemList&,
                                             const std::shared_ptr<CFileItem>& item) const
{
  return FAVOURITES_UTILS::ChooseAndSetNewThumbnail(*item);
}

namespace
{
std::shared_ptr<CFileItem> ResolveFavouriteItem(const CFileItem& item)
{
  std::shared_ptr<CFileItem> targetItem{
      CServiceBroker::GetFavouritesService().ResolveFavourite(item)};
  if (targetItem)
    targetItem->SetProperty("hide_add_remove_favourite", CVariant{true});

  return targetItem;
}

bool IsPlayMediaFavourite(const CFileItem& item)
{
  if (item.IsFavourite())
  {
    const CFavouritesURL favURL{item, -1};
    if (favURL.IsValid())
      return favURL.GetAction() == CFavouritesURL::Action::PLAY_MEDIA;
  }
  return false;
}

bool IsActivateWindowFavourite(const CFileItem& item)
{
  if (item.IsFavourite())
  {
    const CFavouritesURL favURL{item, -1};
    if (favURL.IsValid())
      return favURL.GetAction() == CFavouritesURL::Action::ACTIVATE_WINDOW;
  }
  return false;
}
} // unnamed namespace

bool CFavouritesTargetBrowse::IsVisible(const CFileItem& item) const
{
  return IsActivateWindowFavourite(item);
}

bool CFavouritesTargetBrowse::Execute(const std::shared_ptr<CFileItem>& item) const
{
  return FAVOURITES_UTILS::ExecuteAction({*item, -1});
}

std::string CFavouritesTargetResume::GetLabel(const CFileItem& item) const
{
  const std::shared_ptr<CFileItem> targetItem{ResolveFavouriteItem(item)};
  if (targetItem)
    return VIDEO_UTILS::GetResumeString(*targetItem);

  return {};
}

bool CFavouritesTargetResume::IsVisible(const CFileItem& item) const
{
  if (IsPlayMediaFavourite(item))
  {
    const std::shared_ptr<CFileItem> targetItem{ResolveFavouriteItem(item)};
    if (targetItem)
      return VIDEO_UTILS::GetItemResumeInformation(*targetItem).isResumable;
  }
  return false;
}

bool CFavouritesTargetResume::Execute(const std::shared_ptr<CFileItem>& item) const
{
  const std::shared_ptr<CFileItem> targetItem{ResolveFavouriteItem(*item)};
  if (targetItem)
    return FAVOURITES_UTILS::ExecuteAction({"PlayMedia", *targetItem, "resume"});

  return false;
}

std::string CFavouritesTargetPlay::GetLabel(const CFileItem& item) const
{
  const std::shared_ptr<CFileItem> targetItem{ResolveFavouriteItem(item)};
  if (targetItem && VIDEO_UTILS::GetItemResumeInformation(*targetItem).isResumable)
    return g_localizeStrings.Get(12021); // Play from beginning

  return g_localizeStrings.Get(208); // Play
}

bool CFavouritesTargetPlay::IsVisible(const CFileItem& item) const
{
  return IsPlayMediaFavourite(item);
}

bool CFavouritesTargetPlay::Execute(const std::shared_ptr<CFileItem>& item) const
{
  const std::shared_ptr<CFileItem> targetItem{ResolveFavouriteItem(*item)};
  if (targetItem)
    return FAVOURITES_UTILS::ExecuteAction({"PlayMedia", *targetItem, "noresume"});

  return false;
}

bool CFavouritesTargetInfo::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<CFileItem> targetItem{ResolveFavouriteItem(item)};
  if (targetItem)
    return UTILS::GUILIB::CGUIContentUtils::HasInfoForItem(*targetItem);

  return false;
}

bool CFavouritesTargetInfo::Execute(const std::shared_ptr<CFileItem>& item) const
{
  const std::shared_ptr<CFileItem> targetItem{ResolveFavouriteItem(*item)};
  if (targetItem)
    return UTILS::GUILIB::CGUIContentUtils::ShowInfoForItem(*targetItem);

  return false;
}

bool CFavouritesTargetContextMenu::IsVisible(const CFileItem& item) const
{
  const std::shared_ptr<CFileItem> targetItem{ResolveFavouriteItem(item)};
  if (targetItem)
    return CONTEXTMENU::HasAnyMenuItemsFor(targetItem);

  return false;
}

bool CFavouritesTargetContextMenu::Execute(const std::shared_ptr<CFileItem>& item) const
{
  const std::shared_ptr<CFileItem> targetItem{ResolveFavouriteItem(*item)};
  if (targetItem)
  {
    CONTEXTMENU::ShowFor(targetItem);
    return true;
  }
  return false;
}
