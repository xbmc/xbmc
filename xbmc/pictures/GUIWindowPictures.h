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

#include "windows/GUIMediaWindow.h"
#include "PictureThumbLoader.h"

class CGUIDialogProgress;

class CGUIWindowPictures : public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowPictures(void);
  virtual ~CGUIWindowPictures(void);
  bool OnMessage(CGUIMessage& message) override;
  void OnInitWindow() override;

protected:
  bool GetDirectory(const std::string &strDirectory, CFileItemList& items) override;
  void OnItemInfo(int item);
  bool OnClick(int iItem, const std::string &player = "") override;
  void UpdateButtons() override;
  void OnPrepareFileItems(CFileItemList& items) override;
  bool Update(const std::string &strDirectory, bool updateFilterPath = true) override;
  void GetContextButtons(int itemNumber, CContextButtons &buttons) override;
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button) override;
  bool OnAddMediaSource() override;
  std::string GetStartFolder(const std::string &dir) override;

  void OnRegenerateThumbs();
  bool OnPlayMedia(int iItem, const std::string &player = "") override;
  bool ShowPicture(int iItem, bool startSlideShow);
  void OnShowPictureRecursive(const std::string& strPath);
  void OnSlideShow(const std::string& strPicture);
  void OnSlideShow();
  void OnSlideShowRecursive(const std::string& strPicture);
  void OnSlideShowRecursive();
  void OnItemLoaded(CFileItem* pItem) override;
  void LoadPlayList(const std::string& strPlayList) override;

  CGUIDialogProgress* m_dlgProgress;

  CPictureThumbLoader m_thumbLoader;
  bool m_slideShowStarted;
};
