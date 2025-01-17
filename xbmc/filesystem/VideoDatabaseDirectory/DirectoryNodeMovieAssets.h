/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DirectoryNode.h"

namespace XFILE::VIDEODATABASEDIRECTORY
{
class CDirectoryNodeMovieAssets : public CDirectoryNode
{
public:
  CDirectoryNodeMovieAssets(const std::string& strEntryName, CDirectoryNode* pParent);

protected:
  bool GetContent(CFileItemList& items) const override;
};
} // namespace XFILE::VIDEODATABASEDIRECTORY
