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
                        { NodeType::GENRE,        "genres",           135 },
                        { NodeType::TITLE_MOVIES, "titles",           10024 },
                        { NodeType::YEAR,         "years",            652 },
                        { NodeType::ACTOR,        "actors",           344 },
                        { NodeType::DIRECTOR,     "directors",        20348 },
                        { NodeType::STUDIO,       "studios",          20388 },
                        { NodeType::SETS,         "sets",             20434 },
                        { NodeType::COUNTRY,      "countries",        20451 },
                        { NodeType::TAGS,         "tags",             20459 },
                        { NodeType::VIDEOVERSIONS,"videoversions",    40000 },
                       };
// clang-format on

CDirectoryNodeMoviesOverview::CDirectoryNodeMoviesOverview(const std::string& strName,
                                                           CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::MOVIES_OVERVIEW, strName, pParent)
{

}

NodeType CDirectoryNodeMoviesOverview::GetChildType() const
{
  for (const Node& node : MovieChildren)
    if (GetName() == node.id)
      return node.node;

  return NodeType::NONE;
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
