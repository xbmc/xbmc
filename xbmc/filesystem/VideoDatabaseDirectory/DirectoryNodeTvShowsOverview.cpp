/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeTvShowsOverview.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "video/VideoDbUrl.h"
#include "utils/StringUtils.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

Node TvShowChildren[] = {
                          { NODE_TYPE_GENRE,         "genres",   135 },
                          { NODE_TYPE_TITLE_TVSHOWS, "titles",   10024 },
                          { NODE_TYPE_YEAR,          "years",    652 },
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
