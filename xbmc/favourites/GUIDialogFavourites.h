#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "guilib/GUIDialog.h"
#include "favourites/FavouritesService.h"

class CFileItem;
class CFileItemList;

class CGUIDialogFavourites :
      public CGUIDialog
{
public:
  CGUIDialogFavourites(void);
  ~CGUIDialogFavourites(void) override;
  bool OnMessage(CGUIMessage &message) override;
  void OnInitWindow() override;

  CFileItemPtr GetCurrentListItem(int offset = 0) override;

  bool HasListItems() const override { return true; }

  static bool ChooseAndSetNewName(const CFileItemPtr &item);
  static bool ChooseAndSetNewThumbnail(const CFileItemPtr &item);

protected:
  int GetSelectedItem();
  void OnClick(int item);
  void OnPopupMenu(int item);
  void OnMoveItem(int item, int amount);
  void OnDelete(int item);
  void OnRename(int item);
  void OnSetThumb(int item);
  void UpdateList();

private:
  CFileItemList* m_favourites;
  CFavouritesService& m_favouritesService;
};
