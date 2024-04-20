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

Node MusicVideoChildren[] = {
                              { NODE_TYPE_GENRE,             "genres",    135 },
                              { NODE_TYPE_TITLE_MUSICVIDEOS, "titles",    10024 },
                              { NODE_TYPE_YEAR,              "years",     652 },
                              { NODE_TYPE_ACTOR,             "artists",   133 },
                              { NODE_TYPE_MUSICVIDEOS_ALBUM, "albums",    132 },
                              { NODE_TYPE_DIRECTOR,          "directors", 20348 },
                              { NODE_TYPE_STUDIO,            "studios",   20388 },
                              { NODE_TYPE_TAGS,              "tags",      20459 }
                            };

CDirectoryNodeMusicVideosOverview::CDirectoryNodeMusicVideosOverview(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_MUSICVIDEOS_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeMusicVideosOverview::GetChildType() const
{
  for (const Node& node : MusicVideoChildren)
    if (GetName() == node.id)
      return node.node;

  return NODE_TYPE_NONE;
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

