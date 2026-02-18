/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeAlbum.h"

#include "QueryParams.h"
#include "ServiceBroker.h"
#include "music/MusicDatabase.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeAlbum::CDirectoryNodeAlbum(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::ALBUM, strName, pParent)
{

}

NodeType CDirectoryNodeAlbum::GetChildType() const
{
  return NodeType::DISC;
}

std::string CDirectoryNodeAlbum::GetLocalizedName() const
{
  if (GetID() == -1)
    return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(15102); // All Albums
  CMusicDatabase db;
  if (db.Open())
    return db.GetAlbumById(GetID());
  return "";
}

bool CDirectoryNodeAlbum::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess = musicdatabase.GetAlbumsNav(BuildPath(), items, SortDescription(),
                                             params.GetGenreId(), params.GetArtistId());

  musicdatabase.Close();

  return bSuccess;
}
