/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIWindowMusicBase.h"
#include "windows/GUIWindowPlayList.h"

class CGUIWindowMusicPlayList : public CGUIWindowPlayList<CGUIWindowMusicBase>
{
public:
  CGUIWindowMusicPlayList();
  ~CGUIWindowMusicPlayList() override;

  bool OnMessage(CGUIMessage& message) override;

protected:
  void OnItemLoaded(CFileItem* pItem) override;
  bool Update(const std::string& strDirectory, bool updateFilterPath = true) override;
  bool StopLoadingItems() override;
  void StartLoadingItems() override;
};
