/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "windows/GUIMediaWindow.h"

class CFileItem;
class CFileItemList;

class CGUIWindowFavourites : public CGUIMediaWindow
{
public:
  CGUIWindowFavourites();
  ~CGUIWindowFavourites() override = default;

  static bool ChooseAndSetNewName(CFileItem& item);
  static bool ChooseAndSetNewThumbnail(CFileItem& item);
  static bool MoveItem(CFileItemList& items, const CFileItem& item, int amount);
  static bool ShouldEnableMoveItems();

protected:
  std::string GetRootPath() const override { return "favourites://"; }

  bool OnSelect(int item) override;
  bool OnPopupMenu(int iItem) override;

  bool Update(const std::string& strDirectory, bool updateFilterPath = true) override;
};
