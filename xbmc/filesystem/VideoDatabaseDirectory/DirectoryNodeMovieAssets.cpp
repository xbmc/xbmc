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
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  int details = items.HasProperty("set_videodb_details")
                    ? items.GetProperty("set_videodb_details").asInteger32()
                    : VideoDbDetailsNone;

  const std::string path{BuildPath()};

  bool bSuccess = videodatabase.GetMoviesNav(
      path, items, params.GetGenreId(), params.GetYear(), params.GetActorId(),
      params.GetDirectorId(), params.GetStudioId(), params.GetCountryId(), params.GetSetId(),
      params.GetTagId(), SortDescription(), details);

  videodatabase.Close();

  if (params.GetVideoAssetType() == -2 && items.Size() && items[0]->HasVideoExtras() &&
      GetParent() && GetParent()->GetType() == NodeType::MOVIE_ASSET_TYPES)
  {
    const std::string pathExtras{URIUtils::AddFileToFolder(
        URIUtils::GetParentPath(path), std::to_string(static_cast<int>(VideoAssetType::EXTRA)))};

    CFileItemPtr pItem(new CFileItem(pathExtras, true));
    pItem->SetLabel(g_localizeStrings.Get(40211)); // "Extras"
    pItem->SetLabelPreformatted(true); //! @todo not sure, but used elsewhere

    //! @todo icon too small for nice display, add new skin icon in bigger size? ex. DefaultAddSource.png
    //! @todo wrong art type? some Estuary views don't show it
    pItem->SetArt("icon", "icons/infodialogs/extras.png");

    items.Add(pItem);
  }

  return bSuccess;
}
/*
NodeType CDirectoryNodeTitleMovies::GetChildType() const
{
  return NodeType::MOVIE_ASSET_TYPE;
}
*/