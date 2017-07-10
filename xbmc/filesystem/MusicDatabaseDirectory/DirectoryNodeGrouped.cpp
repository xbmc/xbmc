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
  if (!db.Open())
    return "";
  
  std::string name = db.GetItemById(GetContentType(), GetID());
  db.Close();
  return name;
  
}

bool CDirectoryNodeGrouped::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  bool bSuccess = musicdatabase.GetItems(BuildPath(), GetContentType(), items);
  
  musicdatabase.Close();
  
  return bSuccess;
}

std::string CDirectoryNodeGrouped::GetContentType() const
{
  switch (GetType())
  {
    case NODE_TYPE_GENRE:
      return "genres";
    case NODE_TYPE_ROLE:
      return "roles";
    case NODE_TYPE_YEAR:
      return "years";
    default:
      break;
  }

  return "";
}
