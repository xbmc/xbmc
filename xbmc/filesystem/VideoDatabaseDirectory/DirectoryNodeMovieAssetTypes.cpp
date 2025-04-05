/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeMovieAssetTypes.h"

#include "QueryParams.h"
#include "video/VideoManagerTypes.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeMovieAssetTypes::CDirectoryNodeMovieAssetTypes(const std::string& strName,
                                                             CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::MOVIE_ASSET_TYPES, strName, pParent)
{
}

NodeType CDirectoryNodeMovieAssetTypes::GetChildType() const
{
  CQueryParams params;
  CollectQueryParams(params);

  switch (params.GetVideoAssetType())
  {
    case -2: // special value for all assets + extras virtual folder
    case static_cast<int>(VideoAssetType::VERSION):
      return NodeType::MOVIE_ASSETS_VERSIONS;
    case static_cast<int>(VideoAssetType::EXTRA):
      return NodeType::MOVIE_ASSETS_EXTRAS;
    case 0:
    default:
      return NodeType::MOVIE_ASSETS;
  }
}
