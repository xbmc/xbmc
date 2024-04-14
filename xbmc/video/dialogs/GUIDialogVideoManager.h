/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "video/VideoDatabase.h"

#include <memory>

class CFileItem;
class CFileItemList;
class CMediaSource;

enum class VideoAssetType;

class CGUIDialogVideoManager : public CGUIDialog
{
public:
  explicit CGUIDialogVideoManager(int windowId);
  ~CGUIDialogVideoManager() override = default;

  virtual void SetVideoAsset(const std::shared_ptr<CFileItem>& item);
  virtual void SetSelectedVideoAsset(const std::shared_ptr<CFileItem>& asset);
  virtual bool HasUpdatedItems() const { return m_hasUpdatedItems; }

protected:
  void OnInitWindow() override;
  void OnDeinitWindow(int nextWindowID) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction& action) override;

  virtual VideoAssetType GetVideoAssetType() = 0;
  virtual int GetHeadingId() = 0;

  virtual void Clear();
  virtual void Refresh();
  virtual void UpdateButtons();
  virtual void UpdateAssetsList();

  virtual void Play();
  virtual void Remove();
  virtual void Rename();
  virtual void ChooseArt();

  void DisableRemove();
  void EnableRemove();

  void UpdateControls();

  static int ChooseVideoAsset(const std::shared_ptr<CFileItem>& item,
                              VideoAssetType assetType,
                              const std::string& defaultName);
  void AppendItemFolderToFileBrowserSources(std::vector<CMediaSource>& sources);
  void RefreshSelectedVideoAsset();

  CVideoDatabase m_database;
  std::shared_ptr<CFileItem> m_videoAsset;
  std::unique_ptr<CFileItemList> m_videoAssetsList;
  std::shared_ptr<CFileItem> m_selectedVideoAsset;
  bool m_hasUpdatedItems{false};

private:
  CGUIDialogVideoManager() = delete;

  void CloseAll();
  bool UpdateSelectedAsset();
};
