/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeOverview.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicDatabase.h"
#include "utils/StringUtils.h"

namespace XFILE
{
  namespace MUSICDATABASEDIRECTORY
  {
  Node OverviewChildren[] = {
      {NodeType::GENRE, "genres", 135},
      {NodeType::ARTIST, "artists", 133},
      {NodeType::ALBUM, "albums", 132},
      {NodeType::SINGLES, "singles", 1050},
      {NodeType::SONG, "songs", 134},
      {NodeType::YEAR, "years", 652},
      {NodeType::TOP100, "top100", 271},
      {NodeType::ALBUM_RECENTLY_ADDED, "recentlyaddedalbums", 359},
      {NodeType::ALBUM_RECENTLY_PLAYED, "recentlyplayedalbums", 517},
      {NodeType::ALBUM, "compilations", 521},
      {NodeType::ROLE, "roles", 38033},
      {NodeType::SOURCE, "sources", 39031},
      {NodeType::DISC, "discs", 14087},
      {NodeType::YEAR, "originalyears", 38078},
  };
  };
};

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeOverview::CDirectoryNodeOverview(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::OVERVIEW, strName, pParent)
{

}

NodeType CDirectoryNodeOverview::GetChildType() const
{
  for (const Node& node : OverviewChildren)
    if (GetName() == node.id)
      return node.node;
  return NodeType::NONE;
}

std::string CDirectoryNodeOverview::GetLocalizedName() const
{
  for (const Node& node : OverviewChildren)
    if (GetName() == node.id)
      return g_localizeStrings.Get(node.label);
  return "";
}

bool CDirectoryNodeOverview::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicDatabase;
  musicDatabase.Open();

  bool hasSingles = (musicDatabase.GetSinglesCount() > 0);
  bool hasCompilations = (musicDatabase.GetCompilationAlbumsCount() > 0);

  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); ++i)
  {
    if (i == 3 && !hasSingles)
      continue;
    if (i == 9 && !hasCompilations)
      continue;

    CFileItemPtr pItem(new CFileItem(g_localizeStrings.Get(OverviewChildren[i].label)));
    std::string strDir = StringUtils::Format("{}/", OverviewChildren[i].id);
    pItem->SetPath(BuildPath() + strDir);
    pItem->m_bIsFolder = true;
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
