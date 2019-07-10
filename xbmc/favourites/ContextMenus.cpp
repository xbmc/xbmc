/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ContextMenus.h"

#include "FileItem.h"
#include "GUIDialogFavourites.h"
#include "ServiceBroker.h"
#include "utils/URIUtils.h"


namespace CONTEXTMENU
{
  bool CFavouriteContextMenuAction::IsVisible(const CFileItem& item) const
  {
    return URIUtils::IsProtocol(item.GetPath(), "favourites");
  }

  bool CFavouriteContextMenuAction::Execute(const CFileItemPtr& item) const
  {
    CFileItemList items;
    CServiceBroker::GetFavouritesService().GetAll(items);
    for (const auto& favourite : items)
    {
      if (favourite->GetPath() == item->GetPath())
      {
        if (DoExecute(items, favourite))
          return CServiceBroker::GetFavouritesService().Save(items);
      }
    }
    return false;
  }

  bool CRemoveFavourite::DoExecute(CFileItemList &items, const CFileItemPtr& item) const
  {
    int iBefore = items.Size();
    items.Remove(item.get());
    return items.Size() == iBefore - 1;
  }

  bool CRenameFavourite::DoExecute(CFileItemList&, const CFileItemPtr& item) const
  {
    return CGUIDialogFavourites::ChooseAndSetNewName(item);
  }

  bool CChooseThumbnailForFavourite::DoExecute(CFileItemList&, const CFileItemPtr& item) const
  {
    return CGUIDialogFavourites::ChooseAndSetNewThumbnail(item);
  }

} // namespace CONTEXTMENU
