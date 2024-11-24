/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeDiscs.h"

#include "QueryParams.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicDatabase.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeDiscs::CDirectoryNodeDiscs(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::DISC, strName, pParent)
{
}

NodeType CDirectoryNodeDiscs::GetChildType() const
{
  return NodeType::SONG;
}

std::string CDirectoryNodeDiscs::GetLocalizedName() const
{
  CQueryParams params;
  CollectQueryParams(params);
  std::string title;
  CMusicDatabase db;
  if (db.Open())
    title = db.GetAlbumDiscTitle(params.GetAlbumId(), params.GetDisc());
  db.Close();
  if (title.empty())
    title = g_localizeStrings.Get(15102); // All Albums

  return title;
}

bool CDirectoryNodeDiscs::GetContent(CFileItemList& items) const
{
  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  CQueryParams params;
  CollectQueryParams(params);

  bool bSuccess = musicdatabase.GetDiscsNav(BuildPath(), items, params.GetAlbumId());

  musicdatabase.Close();

  return bSuccess;
}
