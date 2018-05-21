/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "DirectoryNodeAlbumRecentlyPlayed.h"

#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicDatabase.h"
#include "utils/StringUtils.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeAlbumRecentlyPlayed::CDirectoryNodeAlbumRecentlyPlayed(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ALBUM_RECENTLY_PLAYED, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeAlbumRecentlyPlayed::GetChildType() const
{
  if (GetName()=="-1")
    return NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS;

  return NODE_TYPE_SONG;
}

std::string CDirectoryNodeAlbumRecentlyPlayed::GetLocalizedName() const
{
  if (GetID() == -1)
    return g_localizeStrings.Get(15102); // All Albums
  CMusicDatabase db;
  if (db.Open())
    return db.GetAlbumById(GetID());
  return "";
}

bool CDirectoryNodeAlbumRecentlyPlayed::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  VECALBUMS albums;
  if (!musicdatabase.GetRecentlyPlayedAlbums(albums))
  {
    musicdatabase.Close();
    return false;
  }

  for (int i=0; i<(int)albums.size(); ++i)
  {
    CAlbum& album=albums[i];
    std::string strDir = StringUtils::Format("%s%ld/", BuildPath().c_str(), album.idAlbum);
    CFileItemPtr pItem(new CFileItem(strDir, album));
    items.Add(pItem);
  }

  musicdatabase.Close();
  return true;
}
