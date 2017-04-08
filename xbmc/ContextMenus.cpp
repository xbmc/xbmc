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

#include "FileItem.h"
#include "dialogs/GUIDialogFavourites.h"
#include "filesystem/FavouritesDirectory.h"
#include "storage/MediaManager.h"
#include "utils/URIUtils.h"

#include "ContextMenus.h"

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
    g_mediaManager.ToggleTray(g_mediaManager.TranslateDevicePath(item->GetPath())[0]);
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
    return g_mediaManager.Eject(item->GetPath());
  }

  bool CFavouriteContextMenuAction::IsVisible(const CFileItem& item) const
  {
    return URIUtils::IsProtocol(item.GetPath(), "favourites");
  }

  bool CFavouriteContextMenuAction::Execute(const CFileItemPtr& item) const
  {
    CFileItemList items;
    if (XFILE::CFavouritesDirectory::Load(items))
    {
      for (auto favourite : items)
      {
        if (favourite->GetPath() == item->GetPath())
        {
          if (DoExecute(items, favourite))
            return XFILE::CFavouritesDirectory::Save(items);
        }
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
