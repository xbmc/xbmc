/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeMoviesOverview.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbUrl.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

// clang-format off
Node MovieChildren[] = {
                        { NODE_TYPE_GENRE,        "genres",           135 },
                        { NODE_TYPE_TITLE_MOVIES, "titles",           10024 },
                        { NODE_TYPE_YEAR,         "years",            652 },
                        { NODE_TYPE_ACTOR,        "actors",           344 },
                        { NODE_TYPE_DIRECTOR,     "directors",        20348 },
                        { NODE_TYPE_STUDIO,       "studios",          20388 },
                        { NODE_TYPE_SETS,         "sets",             20434 },
                        { NODE_TYPE_COUNTRY,      "countries",        20451 },
                        { NODE_TYPE_TAGS,         "tags",             20459 },
                        { NODE_TYPE_VIDEOVERSIONS,"videoversions",    40000 },
                       };
// clang-format on

CDirectoryNodeMoviesOverview::CDirectoryNodeMoviesOverview(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_MOVIES_OVERVIEW, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeMoviesOverview::GetChildType() const
{
  for (const Node& node : MovieChildren)
    if (GetName() == node.id)
      return node.node;

  return NODE_TYPE_NONE;
}

std::string CDirectoryNodeMoviesOverview::GetLocalizedName() const
{
  for (const Node& node : MovieChildren)
    if (GetName() == node.id)
      return g_localizeStrings.Get(node.label);
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
    std::string strDir = StringUtils::Format("{}/", MovieChildren[i].id);
    itemUrl.AppendPath(strDir);

    CFileItemPtr pItem(new CFileItem(itemUrl.ToString(), true));
    pItem->SetLabel(g_localizeStrings.Get(MovieChildren[i].label));
    pItem->SetCanQueue(false);
    items.Add(pItem);
  }

  return true;
}
