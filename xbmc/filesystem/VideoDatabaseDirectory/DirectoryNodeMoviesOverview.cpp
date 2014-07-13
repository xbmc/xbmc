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

#include "DirectoryNodeMoviesOverview.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbUrl.h"
#include "utils/StringUtils.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;
using namespace std;

Node MovieChildren[] = {
                        { NODE_TYPE_GENRE,        "genres",     135 },
                        { NODE_TYPE_TITLE_MOVIES, "titles",     369 },
                        { NODE_TYPE_YEAR,         "years",      562 },
                        { NODE_TYPE_ACTOR,        "actors",     344 },
                        { NODE_TYPE_DIRECTOR,     "directors",  20348 },
                        { NODE_TYPE_STUDIO,       "studios",    20388 },
                        { NODE_TYPE_SETS,         "sets",       20434 },
                        { NODE_TYPE_COUNTRY,      "countries",  20451 },
                        { NODE_TYPE_TAGS,         "tags",       20459 }
                       };

CDirectoryNodeMoviesOverview::CDirectoryNodeMoviesOverview(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_MOVIES_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeMoviesOverview::GetChildType() const
{
  for (unsigned int i = 0; i < sizeof(MovieChildren) / sizeof(Node); ++i)
    if (GetName() == MovieChildren[i].id)
      return MovieChildren[i].node;
  
  return NODE_TYPE_NONE;
}

std::string CDirectoryNodeMoviesOverview::GetLocalizedName() const
{
  for (unsigned int i = 0; i < sizeof(MovieChildren) / sizeof(Node); ++i)
    if (GetName() == MovieChildren[i].id)
      return g_localizeStrings.Get(MovieChildren[i].label);
  return "";
}

bool CDirectoryNodeMoviesOverview::GetContent(CFileItemList& items) const
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(BuildPath()))
    return false;
  
  for (unsigned int i = 0; i < sizeof(MovieChildren) / sizeof(Node); ++i)
  {
    if (i == 6)
    {
      CVideoDatabase db;
      if (db.Open() && !db.HasSets())
        continue;
    }

    CVideoDbUrl itemUrl = videoUrl;
    std::string strDir = StringUtils::Format("%s/", MovieChildren[i].id.c_str());
    itemUrl.AppendPath(strDir);

    CFileItemPtr pItem(new CFileItem(itemUrl.ToString(), true));
    pItem->SetLabel(g_localizeStrings.Get(MovieChildren[i].label));
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
