/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeAlbumTop100.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "music/MusicDatabase.h"
#include "utils/StringUtils.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeAlbumTop100::CDirectoryNodeAlbumTop100(const std::string& strName,
                                                     CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::ALBUM_TOP100, strName, pParent)
{

}

NodeType CDirectoryNodeAlbumTop100::GetChildType() const
{
  if (GetName() == "-1")
    return NodeType::ALBUM_TOP100_SONGS;

  return NodeType::SONG;
}

std::string CDirectoryNodeAlbumTop100::GetLocalizedName() const
{
  CMusicDatabase db;
  if (db.Open())
    return db.GetAlbumById(GetID());
  return "";
}

bool CDirectoryNodeAlbumTop100::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  VECALBUMS albums;
  if (!musicdatabase.GetTop100Albums(albums))
  {
    musicdatabase.Close();
    return false;
  }

  for (int i=0; i<(int)albums.size(); ++i)
  {
    CAlbum& album=albums[i];
    std::string strDir = StringUtils::Format("{}{}/", BuildPath(), album.idAlbum);
    CFileItemPtr pItem(new CFileItem(strDir, album));
    items.Add(pItem);
  }

  musicdatabase.Close();

  return true;
}
