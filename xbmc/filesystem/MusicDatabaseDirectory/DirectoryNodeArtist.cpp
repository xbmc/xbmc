/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "DirectoryNodeArtist.h"
#include "QueryParams.h"
#include "music/MusicDatabase.h"
#include "settings/GUISettings.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeArtist::CDirectoryNodeArtist(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_ARTIST, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeArtist::GetChildType() const
{
  return NODE_TYPE_ALBUM;
}

CStdString CDirectoryNodeArtist::GetLocalizedName() const
{
  if (GetID() == -1)
    return g_localizeStrings.Get(15103); // All Artists
  CMusicDatabase db;
  if (db.Open())
    return db.GetArtistById(GetID());
  return "";
}

bool CDirectoryNodeArtist::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess = musicdatabase.GetArtistsNav(BuildPath(), items, !g_guiSettings.GetBool("musiclibrary.showcompilationartists"), params.GetGenreId());

  musicdatabase.Close();

  return bSuccess;
}
