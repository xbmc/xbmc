/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeMovieAssets.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "QueryParams.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoManagerTypes.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

//! @todo does the node type need to match the type requested by the parent node?
//! this type of node is used for MOVIE_ASSETS MOVIE_ASSETS_VERSIONS MOVIE_ASSETS_EXTRAS
CDirectoryNodeMovieAssets::CDirectoryNodeMovieAssets(const std::string& strName,
                                                     CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::MOVIE_ASSETS, strName, pParent)
{
}

bool CDirectoryNodeMovieAssets::GetContent(CFileItemList& items) const
{
  CVideoDatabase videoDatabase;
  if (!videoDatabase.Open())
  {
    CLog::LogF(LOGERROR, "Error opening the video database");
    return false;
  }

  CQueryParams params;
  CollectQueryParams(params);

  const int details{items.GetProperty("set_videodb_details").asInteger32(VideoDbDetailsStream)};

  const std::string path{BuildPath()};

  bool success = videoDatabase.GetMoviesNav(
      path, items, params.GetGenreId(), params.GetYear(), params.GetActorId(),
      params.GetDirectorId(), params.GetStudioId(), params.GetCountryId(), params.GetSetId(),
      params.GetTagId(), SortDescription(), details);

  videoDatabase.Close();

  // Add virtual folder for extras
  if (params.GetVideoAssetType() == static_cast<long>(VideoAssetType::VERSIONSANDEXTRASFOLDER) &&
      items.Size() && items[0]->HasVideoExtras() && GetParent() &&
      GetParent()->GetType() == NodeType::MOVIE_ASSET_TYPES)
  {
    const std::string pathExtras{URIUtils::AddFileToFolder(
        URIUtils::GetParentPath(path), std::to_string(static_cast<int>(VideoAssetType::EXTRA)))};

    const auto item{std::make_shared<CFileItem>(pathExtras, true)};
    item->SetLabel(g_localizeStrings.Get(40211)); // "Extras"
    item->SetLabelPreformatted(true); //! @todo not sure, but used elsewhere

    //! @todo icon too small for nice display, add new skin icon in bigger size? ex. DefaultAddSource.png
    //! @todo wrong art type? some Estuary views don't show it
    item->SetArt("icon", "icons/infodialogs/extras.png");

    items.Add(item);
  }

  return success;
}
