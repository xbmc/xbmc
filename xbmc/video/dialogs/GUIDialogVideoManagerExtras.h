/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/dialogs/GUIDialogVideoManager.h"

#include <memory>
#include <string>

class CFileItem;

enum class VideoAssetType;

class CGUIDialogVideoManagerExtras : public CGUIDialogVideoManager
{
public:
  CGUIDialogVideoManagerExtras();
  ~CGUIDialogVideoManagerExtras() override = default;

  void SetVideoAsset(const std::shared_ptr<CFileItem>& item) override;

  static void ManageVideoExtra(const std::shared_ptr<CFileItem>& item);
  static std::string GenerateVideoExtra(const std::string& extrasRoot,
                                        const std::string& extrasPath);

protected:
  bool OnMessage(CGUIMessage& message) override;

  VideoAssetType GetVideoAssetType() override;
  int GetHeadingId() override { return 40025; } // Extras:

  void UpdateButtons() override;

private:
  void AddVideoExtra();
  static std::string GenerateVideoExtra(const std::string& extrasPath);
};
