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
#include "MusicDatabaseDirectory/DirectoryNode.h"
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
  NodeType type;
  NodeType childtype;
  GetDirectoryNodeInfo(path, type, childtype, params);
  if (childtype == NodeType::DISC)
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

NodeType CMusicDatabaseDirectory::GetDirectoryChildType(const std::string& strPath)
{
  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(strPath);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode)
    return NodeType::NONE;

  return pNode->GetChildType();
}

NodeType CMusicDatabaseDirectory::GetDirectoryType(const std::string& strPath)
{
  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(strPath);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode)
    return NodeType::NONE;

  return pNode->GetType();
}

NodeType CMusicDatabaseDirectory::GetDirectoryParentType(const std::string& strPath)
{
  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(strPath);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode)
    return NodeType::NONE;

  CDirectoryNode* pParentNode = pNode->GetParent();

  if (!pParentNode)
    return NodeType::NONE;

  return pParentNode->GetChildType();
}

bool CMusicDatabaseDirectory::GetDirectoryNodeInfo(const std::string& strPath,
                                                   MUSICDATABASEDIRECTORY::NodeType& type,
                                                   MUSICDATABASEDIRECTORY::NodeType& childtype,
                                                   MUSICDATABASEDIRECTORY::CQueryParams& params)
{
  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(strPath);
  if (!CDirectoryNode::GetNodeInfo(path, type, childtype, params))
    return false;

  return true;
}

bool CMusicDatabaseDirectory::IsArtistDir(const std::string& strDirectory)
{
  return GetDirectoryType(strDirectory) == NodeType::ARTIST;
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
      case NodeType::TOP100:
        strLabel = g_localizeStrings.Get(271); // Top 100
        break;
      case NodeType::GENRE:
        strLabel = g_localizeStrings.Get(135); // Genres
        break;
      case NodeType::SOURCE:
        strLabel = g_localizeStrings.Get(39030); // Sources
        break;
      case NodeType::ROLE:
        strLabel = g_localizeStrings.Get(38033); // Roles
        break;
      case NodeType::ARTIST:
        strLabel = g_localizeStrings.Get(133); // Artists
        break;
      case NodeType::ALBUM:
        strLabel = g_localizeStrings.Get(132); // Albums
        break;
      case NodeType::ALBUM_RECENTLY_ADDED:
      case NodeType::ALBUM_RECENTLY_ADDED_SONGS:
        strLabel = g_localizeStrings.Get(359); // Recently Added Albums
        break;
      case NodeType::ALBUM_RECENTLY_PLAYED:
      case NodeType::ALBUM_RECENTLY_PLAYED_SONGS:
        strLabel = g_localizeStrings.Get(517); // Recently Played Albums
        break;
      case NodeType::ALBUM_TOP100:
      case NodeType::ALBUM_TOP100_SONGS:
        strLabel = g_localizeStrings.Get(10505); // Top 100 Albums
        break;
      case NodeType::SINGLES:
        strLabel = g_localizeStrings.Get(1050); // Singles
        break;
      case NodeType::SONG:
        strLabel = g_localizeStrings.Get(134); // Songs
        break;
      case NodeType::SONG_TOP100:
        strLabel = g_localizeStrings.Get(10504); // Top 100 Songs
        break;
      case NodeType::YEAR:
        strLabel = g_localizeStrings.Get(652); // Years
        break;
      case NodeType::OVERVIEW:
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
  const auto type = GetDirectoryChildType(path);
  if (type == NodeType::SONG)
    return true;
  if (type == NodeType::SINGLES)
    return true;
  if (type == NodeType::ALBUM_RECENTLY_ADDED_SONGS)
    return true;
  if (type == NodeType::ALBUM_RECENTLY_PLAYED_SONGS)
    return true;
  if (type == NodeType::ALBUM_TOP100_SONGS)
    return true;
  if (type == NodeType::SONG_TOP100)
    return true;
  if (type == NodeType::DISC)
    return true;
  return false;
}

bool CMusicDatabaseDirectory::Exists(const CURL& url)
{
  std::string path = CLegacyPathTranslation::TranslateMusicDbPath(url);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode)
    return false;

  if (pNode->GetChildType() == NodeType::NONE)
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
    case NodeType::ARTIST:
      return "DefaultMusicArtists.png";
    case NodeType::GENRE:
      return "DefaultMusicGenres.png";
    case NodeType::SOURCE:
      return "DefaultMusicSources.png";
    case NodeType::ROLE:
      return "DefaultMusicRoles.png";
    case NodeType::TOP100:
      return "DefaultMusicTop100.png";
    case NodeType::ALBUM:
      return "DefaultMusicAlbums.png";
    case NodeType::ALBUM_RECENTLY_ADDED:
    case NodeType::ALBUM_RECENTLY_ADDED_SONGS:
      return "DefaultMusicRecentlyAdded.png";
    case NodeType::ALBUM_RECENTLY_PLAYED:
    case NodeType::ALBUM_RECENTLY_PLAYED_SONGS:
      return "DefaultMusicRecentlyPlayed.png";
    case NodeType::SINGLES:
    case NodeType::SONG:
      return "DefaultMusicSongs.png";
    case NodeType::ALBUM_TOP100:
    case NodeType::ALBUM_TOP100_SONGS:
      return "DefaultMusicTop100Albums.png";
    case NodeType::SONG_TOP100:
      return "DefaultMusicTop100Songs.png";
    case NodeType::YEAR:
      return "DefaultMusicYears.png";
    default:
      break;
  }

  return "";
}
