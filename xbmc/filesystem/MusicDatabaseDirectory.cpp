/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicDatabaseDirectory.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "MusicDatabaseDirectory/QueryParams.h"
#include "ServiceBroker.h"
#include "filesystem/File.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/TextureManager.h"
#include "music/MusicDatabase.h"
#include "music/MusicDbUrl.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Crc32.h"
#include "utils/LegacyPathTranslation.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace XFILE;
using namespace MUSICDATABASEDIRECTORY;

CMusicDatabaseDirectory::CMusicDatabaseDirectory(void) = default;

CMusicDatabaseDirectory::~CMusicDatabaseDirectory(void) = default;

bool CMusicDatabaseDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(url);

  // Adjust path to control navigation from albums to discs or directly to songs
  CQueryParams params;
  NODE_TYPE type;
  NODE_TYPE childtype;
  GetDirectoryNodeInfo(path, type, childtype, params);
  if (childtype == NODE_TYPE_DISC)
  {
    bool bFlatten = false;
    if (params.GetAlbumId() < 0)
      bFlatten = true; // Showing *all albums next always songs
    else
    {
      // Option to show discs for ordinary albums (not just boxed sets)
      bFlatten = !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
          CSettings::SETTING_MUSICLIBRARY_SHOWDISCS);
      CMusicDatabase musicdatabase;
      if (musicdatabase.Open())
      {
        if (bFlatten) // Check for boxed set
          bFlatten = !musicdatabase.IsAlbumBoxset(params.GetAlbumId());
        if (!bFlatten)
        { // Check we will get more than 1 disc when path filter options applied
          int iDiscTotal = musicdatabase.GetDiscsCount(path);
          bFlatten = iDiscTotal <= 1;
        }
      }
      musicdatabase.Close();
    }
    if (bFlatten)
    { // Skip discs level and go directly to songs
      CMusicDbUrl musicUrl;
      if (!musicUrl.FromString(path))
        return false;
      musicUrl.AppendPath("-2/"); // Flattened so adjust list label etc.
      path = musicUrl.ToString();
    }
  }

  items.SetPath(path);
  items.m_dwSize = -1;  // No size

  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode)
    return false;

  bool bResult = pNode->GetChilds(items);
  for (int i=0;i<items.Size();++i)
  {
    CFileItemPtr item = items[i];
    if (item->m_bIsFolder && !item->HasArt("icon") && !item->HasArt("thumb"))
    {
      std::string strImage = GetIcon(item->GetPath());
      if (!strImage.empty() && CServiceBroker::GetGUI()->GetTextureManager().HasTexture(strImage))
        item->SetArt("icon", strImage);
    }
  }
  if (items.GetLabel().empty())
    items.SetLabel(pNode->GetLocalizedName());

  return bResult;
}

NODE_TYPE CMusicDatabaseDirectory::GetDirectoryChildType(const std::string& strPath)
{
  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(strPath);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode)
    return NODE_TYPE_NONE;

  return pNode->GetChildType();
}

NODE_TYPE CMusicDatabaseDirectory::GetDirectoryType(const std::string& strPath)
{
  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(strPath);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode)
    return NODE_TYPE_NONE;

  return pNode->GetType();
}

NODE_TYPE CMusicDatabaseDirectory::GetDirectoryParentType(const std::string& strPath)
{
  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(strPath);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode)
    return NODE_TYPE_NONE;

  CDirectoryNode* pParentNode=pNode->GetParent();

  if (!pParentNode)
    return NODE_TYPE_NONE;

  return pParentNode->GetChildType();
}

bool CMusicDatabaseDirectory::GetDirectoryNodeInfo(const std::string& strPath,
                                                   MUSICDATABASEDIRECTORY::NODE_TYPE& type,
                                                   MUSICDATABASEDIRECTORY::NODE_TYPE& childtype,
                                                   MUSICDATABASEDIRECTORY::CQueryParams& params)
{
  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(strPath);
  if (!CDirectoryNode::GetNodeInfo(path, type, childtype, params))
    return false;

  return true;
}

bool CMusicDatabaseDirectory::IsArtistDir(const std::string& strDirectory)
{
  NODE_TYPE node=GetDirectoryType(strDirectory);
  return (node==NODE_TYPE_ARTIST);
}

void CMusicDatabaseDirectory::ClearDirectoryCache(const std::string& strDirectory)
{
  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(strDirectory);
  URIUtils::RemoveSlashAtEnd(path);

  uint32_t crc = Crc32::ComputeFromLowerCase(path);

  std::string strFileName = StringUtils::Format("special://temp/archive_cache/{:08x}.fi", crc);
  CFile::Delete(strFileName);
}

bool CMusicDatabaseDirectory::IsAllItem(const std::string& strDirectory)
{
  //Last query parameter, ignoring any appended options, is -1 or -2
  CURL url(strDirectory);
  if (StringUtils::EndsWith(url.GetWithoutOptions(), "/-1/") || // any albumid
      StringUtils::EndsWith(url.GetWithoutOptions(), "/-1/-2/")) // any albumid + flattened
    return true;
  return false;
}

bool CMusicDatabaseDirectory::GetLabel(const std::string& strDirectory, std::string& strLabel)
{
  strLabel = "";

  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(strDirectory);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
  if (!pNode)
    return false;

  // first see if there's any filter criteria
  CQueryParams params;
  CDirectoryNode::GetDatabaseInfo(path, params);

  CMusicDatabase musicdatabase;
  if (!musicdatabase.Open())
    return false;

  // get genre
  if (params.GetGenreId() >= 0)
    strLabel += musicdatabase.GetGenreById(params.GetGenreId());

  // get artist
  if (params.GetArtistId() >= 0)
  {
    if (!strLabel.empty())
      strLabel += " / ";
    strLabel += musicdatabase.GetArtistById(params.GetArtistId());
  }

  // get album
  if (params.GetAlbumId() >= 0)
  {
    if (!strLabel.empty())
      strLabel += " / ";
    strLabel += musicdatabase.GetAlbumById(params.GetAlbumId());
  }

  if (strLabel.empty())
  {
    switch (pNode->GetChildType())
    {
    case NODE_TYPE_TOP100:
      strLabel = g_localizeStrings.Get(271); // Top 100
      break;
    case NODE_TYPE_GENRE:
      strLabel = g_localizeStrings.Get(135); // Genres
      break;
    case NODE_TYPE_SOURCE:
      strLabel = g_localizeStrings.Get(39030); // Sources
      break;
    case NODE_TYPE_ROLE:
      strLabel = g_localizeStrings.Get(38033); // Roles
      break;
    case NODE_TYPE_ARTIST:
      strLabel = g_localizeStrings.Get(133); // Artists
      break;
    case NODE_TYPE_ALBUM:
      strLabel = g_localizeStrings.Get(132); // Albums
      break;
    case NODE_TYPE_ALBUM_RECENTLY_ADDED:
    case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
      strLabel = g_localizeStrings.Get(359); // Recently Added Albums
      break;
    case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
    case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
      strLabel = g_localizeStrings.Get(517); // Recently Played Albums
      break;
    case NODE_TYPE_ALBUM_TOP100:
    case NODE_TYPE_ALBUM_TOP100_SONGS:
      strLabel = g_localizeStrings.Get(10505); // Top 100 Albums
      break;
    case NODE_TYPE_SINGLES:
      strLabel = g_localizeStrings.Get(1050); // Singles
      break;
    case NODE_TYPE_SONG:
      strLabel = g_localizeStrings.Get(134); // Songs
      break;
    case NODE_TYPE_SONG_TOP100:
      strLabel = g_localizeStrings.Get(10504); // Top 100 Songs
      break;
    case NODE_TYPE_YEAR:
      strLabel = g_localizeStrings.Get(652);  // Years
      break;
    case NODE_TYPE_OVERVIEW:
      strLabel = "";
      break;
    default:
      return false;
    }
  }

  return true;
}

bool CMusicDatabaseDirectory::ContainsSongs(const std::string &path)
{
  MUSICDATABASEDIRECTORY::NODE_TYPE type = GetDirectoryChildType(path);
  if (type == MUSICDATABASEDIRECTORY::NODE_TYPE_SONG) return true;
  if (type == MUSICDATABASEDIRECTORY::NODE_TYPE_SINGLES) return true;
  if (type == MUSICDATABASEDIRECTORY::NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS) return true;
  if (type == MUSICDATABASEDIRECTORY::NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS) return true;
  if (type == MUSICDATABASEDIRECTORY::NODE_TYPE_ALBUM_TOP100_SONGS) return true;
  if (type == MUSICDATABASEDIRECTORY::NODE_TYPE_SONG_TOP100) return true;
  if (type == MUSICDATABASEDIRECTORY::NODE_TYPE_DISC) return true;
  return false;
}

bool CMusicDatabaseDirectory::Exists(const CURL& url)
{
  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(url);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode)
    return false;

  if (pNode->GetChildType() == MUSICDATABASEDIRECTORY::NODE_TYPE_NONE)
    return false;

  return true;
}

bool CMusicDatabaseDirectory::CanCache(const std::string& strPath)
{
  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(strPath);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
  if (!pNode)
    return false;
  return pNode->CanCache();
}

std::string CMusicDatabaseDirectory::GetIcon(const std::string &strDirectory)
{
  switch (GetDirectoryChildType(strDirectory))
  {
  case NODE_TYPE_ARTIST:
      return "DefaultMusicArtists.png";
  case NODE_TYPE_GENRE:
      return "DefaultMusicGenres.png";
  case NODE_TYPE_SOURCE:
    return "DefaultMusicSources.png";
  case NODE_TYPE_ROLE:
    return "DefaultMusicRoles.png";
  case NODE_TYPE_TOP100:
      return "DefaultMusicTop100.png";
  case NODE_TYPE_ALBUM:
    return "DefaultMusicAlbums.png";
  case NODE_TYPE_ALBUM_RECENTLY_ADDED:
  case NODE_TYPE_ALBUM_RECENTLY_ADDED_SONGS:
    return "DefaultMusicRecentlyAdded.png";
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED:
  case NODE_TYPE_ALBUM_RECENTLY_PLAYED_SONGS:
    return "DefaultMusicRecentlyPlayed.png";
  case NODE_TYPE_SINGLES:
  case NODE_TYPE_SONG:
    return "DefaultMusicSongs.png";
  case NODE_TYPE_ALBUM_TOP100:
  case NODE_TYPE_ALBUM_TOP100_SONGS:
    return "DefaultMusicTop100Albums.png";
  case NODE_TYPE_SONG_TOP100:
    return "DefaultMusicTop100Songs.png";
  case NODE_TYPE_YEAR:
    return "DefaultMusicYears.png";
  default:
    break;
  }

  return "";
}
