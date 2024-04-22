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
                                { NODE_TYPE_GENRE,                 "genres",               135 },
                                { NODE_TYPE_ARTIST,                "artists",              133 },
                                { NODE_TYPE_ALBUM,                 "albums",               132 },
                                { NODE_TYPE_SINGLES,               "singles",              1050 },
                                { NODE_TYPE_SONG,                  "songs",                134 },
                                { NODE_TYPE_YEAR,                  "years",                652 },
                                { NODE_TYPE_TOP100,                "top100",               271 },
                                { NODE_TYPE_ALBUM_RECENTLY_ADDED,  "recentlyaddedalbums",  359 },
                                { NODE_TYPE_ALBUM_RECENTLY_PLAYED, "recentlyplayedalbums", 517 },
                                { NODE_TYPE_ALBUM,                 "compilations",         521 },
                                { NODE_TYPE_ROLE,                  "roles",              38033 },
                                { NODE_TYPE_SOURCE,                "sources",            39031 },
                                { NODE_TYPE_DISC,                  "discs",              14087 },
                                { NODE_TYPE_YEAR,                  "originalyears",      38078 },
                              };
  };
};

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeOverview::CDirectoryNodeOverview(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeOverview::GetChildType() const
{
  for (const Node& node : OverviewChildren)
    if (GetName() == node.id)
      return node.node;
  return NODE_TYPE_NONE;
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
