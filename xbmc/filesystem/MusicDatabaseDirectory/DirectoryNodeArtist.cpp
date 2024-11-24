/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DirectoryNodeArtist.h"

#include "QueryParams.h"
#include "ServiceBroker.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicDatabase.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

using namespace XFILE::MUSICDATABASEDIRECTORY;

CDirectoryNodeArtist::CDirectoryNodeArtist(const std::string& strName, CDirectoryNode* pParent)
  : CDirectoryNode(NodeType::ARTIST, strName, pParent)
{

}

NodeType CDirectoryNodeArtist::GetChildType() const
{
  if (!CServiceBroker::GetSettingsComponent()
           ->GetAdvancedSettings()
           ->m_bMusicLibraryArtistNavigatesToSongs)
    return NodeType::ALBUM;
  else
    return NodeType::SONG;
}

std::string CDirectoryNodeArtist::GetLocalizedName() const
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

  bool bSuccess = musicdatabase.GetArtistsNav(BuildPath(), items, !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_MUSICLIBRARY_SHOWCOMPILATIONARTISTS), params.GetGenreId());

  musicdatabase.Close();

  return bSuccess;
}
