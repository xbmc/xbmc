/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeGrouped.h"

#include "music/MusicDatabase.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeGrouped::CDirectoryNodeGrouped(NodeType type,
                                             const std::string& strName,
                                             CDirectoryNode* pParent)
  : CDirectoryNode(type, strName, pParent)
{ }

NodeType CDirectoryNodeGrouped::GetChildType() const
{
  if (GetType() == NodeType::YEAR)
    return NodeType::ALBUM;

  return NodeType::ARTIST;
}

std::string CDirectoryNodeGrouped::GetLocalizedName() const
{
  CMusicDatabase db;
  if (db.Open())
    return db.GetItemById(GetContentType(), GetID());
  return "";
}

bool CDirectoryNodeGrouped::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  return musicdatabase.GetItems(BuildPath(), GetContentType(), items);
}

std::string CDirectoryNodeGrouped::GetContentType() const
{
  switch (GetType())
  {
    case NodeType::GENRE:
      return "genres";
    case NodeType::SOURCE:
      return "sources";
    case NodeType::ROLE:
      return "roles";
    case NodeType::YEAR:
      return "years";
    default:
      break;
  }

  return "";
}
