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

class CGUIWindowFavourites : public CGUIMediaWindow
{
public:
  CGUIWindowFavourites();
  ~CGUIWindowFavourites() override;

protected:
  std::string GetRootPath() const override { return "favourites://"; }

  bool OnSelect(int item) override;
  bool OnAction(const CAction& action) override;
  bool OnMessage(CGUIMessage& message) override;

  bool Update(const std::string& strDirectory, bool updateFilterPath = true) override;

private:
  void OnFavouritesEvent(const CFavouritesService::FavouritesUpdated& event);
  bool MoveItem(int item, int amount);
  bool RemoveItem(int item);
};
