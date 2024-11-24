/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeMusicVideosOverview.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "video/VideoDbUrl.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

Node MusicVideoChildren[] = {{NodeType::GENRE, "genres", 135},
                             {NodeType::TITLE_MUSICVIDEOS, "titles", 10024},
                             {NodeType::YEAR, "years", 652},
                             {NodeType::ACTOR, "artists", 133},
                             {NodeType::MUSICVIDEOS_ALBUM, "albums", 132},
                             {NodeType::DIRECTOR, "directors", 20348},
                             {NodeType::STUDIO, "studios", 20388},
                             {NodeType::TAGS, "tags", 20459}};

CDirectoryNodeMusicVideosOverview::CDirectoryNodeMusicVideosOverview(const std::string& strName,
                                                                     CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::MUSICVIDEOS_OVERVIEW, strName, pParent)
{

}

NodeType CDirectoryNodeMusicVideosOverview::GetChildType() const
{
  for (const Node& node : MusicVideoChildren)
    if (GetName() == node.id)
      return node.node;

  return NodeType::NONE;
}

std::string CDirectoryNodeMusicVideosOverview::GetLocalizedName() const
{
  for (const Node& node : MusicVideoChildren)
    if (GetName() == node.id)
      return g_localizeStrings.Get(node.label);
  return "";
}

bool CDirectoryNodeMusicVideosOverview::GetContent(CFileItemList& items) const
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(BuildPath()))
    return false;

  for (const Node& node : MusicVideoChildren)
  {
    CFileItemPtr pItem(new CFileItem(g_localizeStrings.Get(node.label)));

    CVideoDbUrl itemUrl = videoUrl;
    std::string strDir = StringUtils::Format("{}/", node.id);
    itemUrl.AppendPath(strDir);
    pItem->SetPath(itemUrl.ToString());

    pItem->m_bIsFolder = true;
    pItem->SetCanQueue(false);
    pItem->SetSpecialSort(SortSpecialOnTop);
    items.Add(pItem);
  }

  return true;
}

