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

#include "DirectoryNodeYearAlbum.h"
#include "DatabaseManager.h"
#include "QueryParams.h"
#include "music/MusicDatabase.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeYearAlbum::CDirectoryNodeYearAlbum(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_YEAR_ALBUM, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeYearAlbum::GetChildType() const
{
  if (GetName()=="-1")
    return NODE_TYPE_YEAR_SONG;

  return NODE_TYPE_SONG;
}

std::string CDirectoryNodeYearAlbum::GetLocalizedName() const
{
  if (GetID() == -1)
    return g_localizeStrings.Get(15102); // All Albums
  CMusicDatabase *database = CDatabaseManager::Get().GetMusicDatabase();
  return database->GetAlbumById(GetID());
}

bool CDirectoryNodeYearAlbum::GetContent(CFileItemList& items) const
{
  CQueryParams params;
  CollectQueryParams(params);
  CMusicDatabase *database = CDatabaseManager::Get().GetMusicDatabase();

  return database->GetAlbumsByYear(BuildPath(), items, params.GetYear());
}
