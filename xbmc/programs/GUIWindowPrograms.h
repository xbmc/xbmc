/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#pragma once

#include "windows/GUIMediaWindow.h"
#include "dialogs/GUIDialogProgress.h"
#include "ThumbLoader.h"

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
