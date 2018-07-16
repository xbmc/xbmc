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

CDirectoryNodeGrouped::CDirectoryNodeGrouped(NODE_TYPE type, const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(type, strName, pParent)
{ }

NODE_TYPE CDirectoryNodeGrouped::GetChildType() const
{
  if (GetType() == NODE_TYPE_YEAR)
    return NODE_TYPE_YEAR_ALBUM;

  return NODE_TYPE_ARTIST;
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
    case NODE_TYPE_GENRE:
      return "genres";
    case NODE_TYPE_SOURCE:
      return "sources";
    case NODE_TYPE_ROLE:
      return "roles";
    case NODE_TYPE_YEAR:
      return "years";
    default:
      break;
  }

  return "";
}
