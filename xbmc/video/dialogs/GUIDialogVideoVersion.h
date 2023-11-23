/*
 *  Copyright (C) 2005-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "MediaSource.h"
#include "guilib/GUIDialog.h"
#include "media/MediaType.h"
#include "video/VideoDatabase.h"

#include <memory>

class CFileItem;
class CFileItemList;
class CVideoDatabase;

class CGUIDialogVideoVersion : public CGUIDialog
{
public:
  CGUIDialogVideoVersion(int id);
  ~CGUIDialogVideoVersion(void) override;
  bool OnMessage(CGUIMessage& message) override;
  void SetVideoItem(const std::shared_ptr<CFileItem>& item, bool playMode);
  static std::tuple<int, std::string> NewVideoVersion();
  static bool ConvertVideoVersion(const std::shared_ptr<CFileItem>& item);
  static bool ProcessVideoVersion(VideoDbContentType itemType, int dbId);
  static int SelectVideoVersion(const std::shared_ptr<CFileItem>& item);
  static void ManageVideoVersion(const std::shared_ptr<CFileItem>& item);
  static void PlayVideoItem(const std::shared_ptr<CFileItem>& item);
  static std::string GenerateExtrasVideoVersion(const std::string& extrasRoot,
                                                const std::string& extrasPath);
  static std::string GenerateExtrasVideoVersion(const std::string& extrasPath);

  using VideoVersionPlayCallback = std::function<void(const std::shared_ptr<CFileItem>& item)>;
  static void PlayVideoVersion(const std::shared_ptr<CFileItem>& item,
                               VideoVersionPlayCallback play);

  static int ManageVideoVersionContextMenu(const std::shared_ptr<CFileItem>& version);

protected:
  void OnInitWindow() override;

private:
  void SetDefaultVideoVersion(CFileItem& version);
  void SetSelectedVideoVersion(const std::shared_ptr<CFileItem>& version);
  void ClearVideoVersionList();
  void RefreshVideoVersionList();
  void SetPlayCallback(VideoVersionPlayCallback callback);
  void AddVideoVersion(bool primary);
  void Play();
  void AddVersion();
  void AddExtras();
  void Rename();
  void SetDefault();
  void Remove();
  void ChooseArt();

  VideoVersionPlayCallback m_playCallback;

  std::shared_ptr<CFileItem> m_videoItem;
  bool m_playMode{false};

  CVideoDatabase m_database;
  std::unique_ptr<CFileItemList> m_primaryVideoVersionList;
  std::unique_ptr<CFileItemList> m_extrasVideoVersionList;
  std::unique_ptr<CFileItem> m_defaultVideoVersion;
  std::unique_ptr<CFileItem> m_selectedVideoVersion;
};
