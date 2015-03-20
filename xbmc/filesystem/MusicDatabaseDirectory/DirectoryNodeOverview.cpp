/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DirectoryNodeOverview.h"
#include "FileItem.h"
#include "music/MusicDatabase.h"
#include "guilib/LocalizeStrings.h"
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
                                { NODE_TYPE_ALBUM_COMPILATIONS,    "compilations",         521 },
                              };
  };
};

using namespace std;
using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeOverview::CDirectoryNodeOverview(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeOverview::GetChildType() const
{
  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); ++i)
    if (GetName() == OverviewChildren[i].id)
      return OverviewChildren[i].node;
  return NODE_TYPE_NONE;
}

std::string CDirectoryNodeOverview::GetLocalizedName() const
{
  for (unsigned int i = 0; i < sizeof(OverviewChildren) / sizeof(Node); ++i)
    if (GetName() == OverviewChildren[i].id)
      return g_localizeStrings.Get(OverviewChildren[i].label);
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
    std::string strDir = StringUtils::Format("%s/", OverviewChildren[i].id.c_str());
    pItem->SetPath(BuildPath() + strDir);
    pItem->m_bIsFolder = true;
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
