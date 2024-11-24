/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeTvShowsOverview.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "video/VideoDbUrl.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

Node TvShowChildren[] = {
    {NodeType::GENRE, "genres", 135},     {NodeType::TITLE_TVSHOWS, "titles", 10024},
    {NodeType::YEAR, "years", 652},       {NodeType::ACTOR, "actors", 344},
    {NodeType::STUDIO, "studios", 20388}, {NodeType::TAGS, "tags", 20459}};

CDirectoryNodeTvShowsOverview::CDirectoryNodeTvShowsOverview(const std::string& strName,
                                                             CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::TVSHOWS_OVERVIEW, strName, pParent)
{

}

NodeType CDirectoryNodeTvShowsOverview::GetChildType() const
{
  if (GetName()=="0")
    return NodeType::EPISODES;

  for (const Node& node : TvShowChildren)
    if (GetName() == node.id)
      return node.node;

  return NodeType::NONE;
}

std::string CDirectoryNodeTvShowsOverview::GetLocalizedName() const
{
  for (const Node& node : TvShowChildren)
    if (GetName() == node.id)
      return g_localizeStrings.Get(node.label);
  return "";
}

bool CDirectoryNodeTvShowsOverview::GetContent(CFileItemList& items) const
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(BuildPath()))
    return false;

  for (const Node& node : TvShowChildren)
  {
    CFileItemPtr pItem(new CFileItem(g_localizeStrings.Get(node.label)));

    CVideoDbUrl itemUrl = videoUrl;
    std::string strDir = StringUtils::Format("{}/", node.id);
    itemUrl.AppendPath(strDir);
    pItem->SetPath(itemUrl.ToString());

    pItem->m_bIsFolder = true;
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
