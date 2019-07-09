/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/DirectoryHistory.h"
#include "filesystem/VirtualDirectory.h"
#include "guilib/GUIDialog.h"
#include "pictures/PictureThumbLoader.h"
#include "view/GUIViewControl.h"

#include <string>
#include <vector>

class CFileItem;
class CFileItemList;

class CGUIDialogFileBrowser : public CGUIDialog, public IBackgroundLoaderObserver
{
public:
  CGUIDialogFileBrowser(void);
  ~CGUIDialogFileBrowser(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;
  bool OnBack(int actionID) override;
  void FrameMove() override;
  void OnWindowLoaded() override;
  void OnWindowUnload() override;
  bool IsConfirmed() { return m_bConfirmed; };
  void SetHeading(const std::string &heading);

  static bool ShowAndGetDirectory(const VECSOURCES &shares, const std::string &heading, std::string &path, bool bWriteOnly=false);
  static bool ShowAndGetFile(const VECSOURCES &shares, const std::string &mask, const std::string &heading, std::string &path, bool useThumbs = false, bool useFileDirectories = false);
  static bool ShowAndGetFile(const std::string &directory, const std::string &mask, const std::string &heading, std::string &path, bool useThumbs = false, bool useFileDirectories = false, bool singleList = false);
  static bool ShowAndGetSource(std::string &path, bool allowNetworkShares, VECSOURCES* additionalShare = NULL, const std::string& strType="");
  static bool ShowAndGetFileList(const VECSOURCES &shares, const std::string &mask, const std::string &heading, std::vector<std::string> &path, bool useThumbs = false, bool useFileDirectories = false);
  static bool ShowAndGetImage(const VECSOURCES &shares, const std::string &heading, std::string &path);
  static bool ShowAndGetImage(const CFileItemList &items, const VECSOURCES &shares, const std::string &heading, std::string &path, bool* flip=NULL, int label=21371);
  static bool ShowAndGetImageList(const VECSOURCES &shares, const std::string &heading, std::vector<std::string> &path);

  void SetSources(const VECSOURCES &shares);

  void OnItemLoaded(CFileItem *item) override {};

  bool HasListItems() const override { return true; };
  CFileItemPtr GetCurrentListItem(int offset = 0) override;
  int GetViewContainerID() const override { return m_viewControl.GetCurrentControl(); };

protected:
  void GoParentFolder();
  void OnClick(int iItem);
  void OnSort();
  void ClearFileItems();
  void Update(const std::string &strDirectory);
  bool HaveDiscOrConnection( int iDriveType );
  bool OnPopupMenu(int iItem);
  void OnAddNetworkLocation();
  void OnAddMediaSource();
  void OnEditMediaSource(CFileItem* pItem);
  CGUIControl *GetFirstFocusableControl(int id) override;

  VECSOURCES m_shares;
  XFILE::CVirtualDirectory m_rootDir;
  CFileItemList* m_vecItems;
  CFileItem* m_Directory;
  std::string m_strParentPath;
  std::string m_selectedPath;
  CDirectoryHistory m_history;
  int m_browsingForFolders; // 0 - no, 1 - yes, 2 - yes, only writable
  bool m_bConfirmed;
  int m_bFlip;
  bool m_addNetworkShareEnabled;
  bool m_flipEnabled;
  std::string m_addSourceType;
  bool m_browsingForImages;
  bool m_useFileDirectories;
  bool m_singleList;              // if true, we have no shares or anything
  bool m_multipleSelection;
  std::vector<std::string> m_markedPath;

  CPictureThumbLoader m_thumbLoader;
  CGUIViewControl m_viewControl;
};
