/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PictureThumbLoader.h"
#include "windows/GUIMediaWindow.h"

class CGUIDialogProgress;

class CGUIWindowPictures : public CGUIMediaWindow, public IBackgroundLoaderObserver
{
public:
  CGUIWindowPictures(void);
  ~CGUIWindowPictures(void) override;
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
