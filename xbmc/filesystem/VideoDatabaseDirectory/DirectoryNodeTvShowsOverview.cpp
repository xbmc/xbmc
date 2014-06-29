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

#include "DirectoryNodeTvShowsOverview.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "video/VideoDbUrl.h"
#include "utils/StringUtils.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

Node TvShowChildren[] = {
                          { NODE_TYPE_GENRE,         "genres",   135 },
                          { NODE_TYPE_TITLE_TVSHOWS, "titles",   369 },
                          { NODE_TYPE_YEAR,          "years",    562 },
                          { NODE_TYPE_ACTOR,         "actors",   344 },
                          { NODE_TYPE_STUDIO,        "studios",  20388 },
                          { NODE_TYPE_TAGS,          "tags",     20459 }
                        };

CDirectoryNodeTvShowsOverview::CDirectoryNodeTvShowsOverview(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_TVSHOWS_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeTvShowsOverview::GetChildType() const
{
  if (GetName()=="0")
    return NODE_TYPE_EPISODES;

  for (unsigned int i = 0; i < sizeof(TvShowChildren) / sizeof(Node); ++i)
    if (GetName() == TvShowChildren[i].id)
      return TvShowChildren[i].node;

  return NODE_TYPE_NONE;
}

std::string CDirectoryNodeTvShowsOverview::GetLocalizedName() const
{
  for (unsigned int i = 0; i < sizeof(TvShowChildren) / sizeof(Node); ++i)
    if (GetName() == TvShowChildren[i].id)
      return g_localizeStrings.Get(TvShowChildren[i].label);
  return "";
}

bool CDirectoryNodeTvShowsOverview::GetContent(CFileItemList& items) const
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(BuildPath()))
    return false;

  for (unsigned int i = 0; i < sizeof(TvShowChildren) / sizeof(Node); ++i)
  {
    CFileItemPtr pItem(new CFileItem(g_localizeStrings.Get(TvShowChildren[i].label)));

    CVideoDbUrl itemUrl = videoUrl;
    std::string strDir = StringUtils::Format("%s/", TvShowChildren[i].id.c_str());
    itemUrl.AppendPath(strDir);
    pItem->SetPath(itemUrl.ToString());

    pItem->m_bIsFolder = true;
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
