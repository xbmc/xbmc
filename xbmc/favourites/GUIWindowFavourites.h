/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "favourites/FavouritesService.h"
#include "windows/GUIMediaWindow.h"

class CFileItem;
class CFileItemList;

class CGUIWindowFavourites : public CGUIMediaWindow
{
public:
  CGUIWindowFavourites();
  ~CGUIWindowFavourites() override;

  static bool ChooseAndSetNewName(CFileItem& item);
  static bool ChooseAndSetNewThumbnail(CFileItem& item);
  static bool MoveItem(CFileItemList& items, const CFileItem& item, int amount);
  static bool ShouldEnableMoveItems();

protected:
  std::string GetRootPath() const override { return "favourites://"; }

  bool OnSelect(int item) override;
  bool OnAction(const CAction& action) override;
  bool OnMessage(CGUIMessage& message) override;

  bool Update(const std::string& strDirectory, bool updateFilterPath = true) override;

private:
  void OnFavouritesEvent(const CFavouritesService::FavouritesUpdated& event);
};
