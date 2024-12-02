/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicDbUrl.h"

#include "filesystem/MusicDatabaseDirectory.h"
#include "filesystem/MusicDatabaseDirectory/DirectoryNode.h"
#include "filesystem/MusicDatabaseDirectory/QueryParams.h"
#include "playlists/SmartPlayList.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

using namespace KODI;
using namespace XFILE;
using namespace XFILE::MUSICDATABASEDIRECTORY;

CMusicDbUrl::CMusicDbUrl()
  : CDbUrl()
{ }

CMusicDbUrl::~CMusicDbUrl() = default;

bool CMusicDbUrl::parse()
{
  // the URL must start with musicdb://
  if (!m_url.IsProtocol("musicdb") || m_url.GetFileName().empty())
    return false;

  std::string path = m_url.Get();

  // Parse path for directory node types and query params
  NodeType dirType;
  NodeType childType;
  CQueryParams queryParams;
  if (!CMusicDatabaseDirectory::GetDirectoryNodeInfo(path, dirType, childType, queryParams))
    return false;

  switch (dirType)
  {
    case NodeType::ARTIST:
      m_type = "artists";
      break;

    case NodeType::ALBUM:
    case NodeType::ALBUM_RECENTLY_ADDED:
    case NodeType::ALBUM_RECENTLY_PLAYED:
    case NodeType::ALBUM_TOP100:
      m_type = "albums";
      break;

    case NodeType::DISC:
      m_type = "discs";
      break;

    case NodeType::ALBUM_RECENTLY_ADDED_SONGS:
    case NodeType::ALBUM_RECENTLY_PLAYED_SONGS:
    case NodeType::ALBUM_TOP100_SONGS:
    case NodeType::SONG:
    case NodeType::SONG_TOP100:
    case NodeType::SINGLES:
      m_type = "songs";
      break;

    default:
      break;
  }

  switch (childType)
  {
    case NodeType::ARTIST:
      m_type = "artists";
      break;

    case NodeType::ALBUM:
    case NodeType::ALBUM_RECENTLY_ADDED:
    case NodeType::ALBUM_RECENTLY_PLAYED:
    case NodeType::ALBUM_TOP100:
      m_type = "albums";
      break;

    case NodeType::DISC:
      m_type = "discs";
      break;

    case NodeType::SONG:
    case NodeType::ALBUM_RECENTLY_ADDED_SONGS:
    case NodeType::ALBUM_RECENTLY_PLAYED_SONGS:
    case NodeType::ALBUM_TOP100_SONGS:
    case NodeType::SONG_TOP100:
    case NodeType::SINGLES:
      m_type = "songs";
      break;

    case NodeType::GENRE:
      m_type = "genres";
      break;

    case NodeType::SOURCE:
      m_type = "sources";
      break;

    case NodeType::ROLE:
      m_type = "roles";
      break;

    case NodeType::YEAR:
      m_type = "years";
      break;

    case NodeType::TOP100:
      m_type = "top100";
      break;

    case NodeType::ROOT:
    case NodeType::OVERVIEW:
    default:
      return false;
  }

  if (m_type.empty())
    return false;

  // retrieve and parse all options
  AddOptions(m_url.GetOptions());

  // add options based on the node type
  if (dirType == NodeType::SINGLES || childType == NodeType::SINGLES)
    AddOption("singles", true);

  // add options based on the QueryParams
  if (queryParams.GetArtistId() != -1)
    AddOption("artistid", (int)queryParams.GetArtistId());
  if (queryParams.GetAlbumId() != -1)
    AddOption("albumid", (int)queryParams.GetAlbumId());
  if (queryParams.GetGenreId() != -1)
    AddOption("genreid", (int)queryParams.GetGenreId());
  if (queryParams.GetSongId() != -1)
    AddOption("songid", (int)queryParams.GetSongId());
  if (queryParams.GetYear() != -1)
    AddOption("year", (int)queryParams.GetYear());

  // Decode legacy use of "musicdb://compilations/" path for filtered albums
  if (m_url.GetFileName() == "compilations/")
    AddOption("compilation", true);

  return true;
}

bool CMusicDbUrl::validateOption(const std::string &key, const CVariant &value)
{
  if (!CDbUrl::validateOption(key, value))
    return false;

  // if the value is empty it will remove the option which is ok
  // otherwise we only care about the "filter" option here
  if (value.empty() || !StringUtils::EqualsNoCase(key, "filter"))
    return true;

  if (!value.isString())
    return false;

  PLAYLIST::CSmartPlaylist xspFilter;
  if (!xspFilter.LoadFromJson(value.asString()))
    return false;

  // check if the filter playlist matches the item type
  return xspFilter.GetType() == m_type;
}
