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

#include "DirectoryNodeMusicVideoAlbum.h"
#include "QueryParams.h"
#include "video/VideoDatabase.h"

using namespace XFILE::VIDEODATABASEDIRECTORY;

CDirectoryNodeMusicVideoAlbum::CDirectoryNodeMusicVideoAlbum(const CStdString& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NODE_TYPE_MUSICVIDEOS_ALBUM, strName, pParent)
{

}

NODE_TYPE CDirectoryNodeMusicVideoAlbum::GetChildType() const
{
  return NODE_TYPE_TITLE_MUSICVIDEOS;
}

CStdString CDirectoryNodeMusicVideoAlbum::GetLocalizedName() const
{
  CVideoDatabase db;
  if (db.Open())
    return db.GetMusicVideoAlbumById(GetID());
  return "";
}

bool CDirectoryNodeMusicVideoAlbum::GetContent(CFileItemList& items) const
{
  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess=videodatabase.GetMusicVideoAlbumsNav(BuildPath(), items, params.GetActorId());

  videodatabase.Close();

  return bSuccess;
}

