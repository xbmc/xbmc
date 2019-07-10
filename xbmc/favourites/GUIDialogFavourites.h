/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "favourites/FavouritesService.h"
#include "guilib/GUIDialog.h"

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
