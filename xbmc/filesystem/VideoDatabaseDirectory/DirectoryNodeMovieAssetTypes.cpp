/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeMovieAssetTypes.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeMovieAssetTypes::CDirectoryNodeMovieAssetTypes(const std::string& strName,
                                                             CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::MOVIE_ASSET_TYPES, strName, pParent)
{
}

NodeType CDirectoryNodeMovieAssetTypes::GetChildType() const
{
  return NodeType::MOVIE_ASSETS;
}
