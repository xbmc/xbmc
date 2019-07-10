/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ThumbLoader.h"
#include "dialogs/GUIDialogProgress.h"
#include "windows/GUIMediaWindow.h"

class CGUIWindowPrograms :
      public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowPrograms(void);
  ~CGUIWindowPrograms(void) override;
  bool OnMessage(CGUIMessage& message) override;
  virtual void OnItemInfo(int iItem);
protected:
  void OnItemLoaded(CFileItem* pItem) override {};
  bool Update(const std::string& strDirectory, bool updateFilterPath = true) override;
  bool OnPlayMedia(int iItem, const std::string& = "") override;
  void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  bool OnAddMediaSource() override;
  std::string GetStartFolder(const std::string &dir) override;

  CGUIDialogProgress* m_dlgProgress;

  CProgramThumbLoader m_thumbLoader;
};
