/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDatabaseDirectory.h"

#include "File.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "VideoDatabaseDirectory/DirectoryNode.h"
#include "VideoDatabaseDirectory/QueryParams.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/TextureManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Crc32.h"
#include "utils/LegacyPathTranslation.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "video/VideoDatabase.h"

using namespace XFILE;
using namespace VIDEODATABASEDIRECTORY;

CVideoDatabaseDirectory::CVideoDatabaseDirectory(void) = default;

CVideoDatabaseDirectory::~CVideoDatabaseDirectory(void) = default;

namespace
{
std::string GetChildContentType(const std::unique_ptr<CDirectoryNode>& node)
{
  switch (node->GetChildType())
  {
    case NodeType::EPISODES:
    case NodeType::RECENTLY_ADDED_EPISODES:
      return "episodes";
    case NodeType::SEASONS:
      return "seasons";
    case NodeType::TITLE_MOVIES:
    case NodeType::RECENTLY_ADDED_MOVIES:
      return "movies";
    case NodeType::TITLE_TVSHOWS:
    case NodeType::INPROGRESS_TVSHOWS:
      return "tvshows";
    case NodeType::TITLE_MUSICVIDEOS:
    case NodeType::RECENTLY_ADDED_MUSICVIDEOS:
      return "musicvideos";
    case NodeType::GENRE:
      return "genres";
    case NodeType::COUNTRY:
      return "countries";
    case NodeType::ACTOR:
    {
      CQueryParams params;
      node->CollectQueryParams(params);
      if (static_cast<VideoDbContentType>(params.GetContentType()) ==
          VideoDbContentType::MUSICVIDEOS)
        return "artists";

      return "actors";
    }
    case NodeType::DIRECTOR:
      return "directors";
    case NodeType::STUDIO:
      return "studios";
    case NodeType::YEAR:
      return "years";
    case NodeType::MUSICVIDEOS_ALBUM:
      return "albums";
    case NodeType::SETS:
      return "sets";
    case NodeType::TAGS:
      return "tags";
    case NodeType::VIDEOVERSIONS:
    case NodeType::MOVIE_ASSETS_VERSIONS:
      return "videoversions";
    case NodeType::MOVIE_ASSETS_EXTRAS:
      return "videoextras";
    case NodeType::MOVIE_ASSETS:
      return "videoassets";
    default:
      break;
  }
  return {};
}

} // unnamed namespace

bool CVideoDatabaseDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string path = CLegacyPathTranslation::TranslateVideoDbPath(url);
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
    if (item->HasVideoInfoTag())
    {
      item->SetDynPath(item->GetVideoInfoTag()->GetPath());
    }
  }
  if (items.HasProperty("customtitle"))
    items.SetLabel(items.GetProperty("customtitle").asString());
  else
    items.SetLabel(pNode->GetLocalizedName());

  items.SetContent(GetChildContentType(pNode));

  return bResult;
}

NodeType CVideoDatabaseDirectory::GetDirectoryChildType(const std::string& strPath)
{
  std::string path = CLegacyPathTranslation::TranslateVideoDbPath(strPath);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode)
    return NodeType::NONE;

  return pNode->GetChildType();
}

NodeType CVideoDatabaseDirectory::GetDirectoryType(const std::string& strPath)
{
  std::string path = CLegacyPathTranslation::TranslateVideoDbPath(strPath);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode)
    return NodeType::NONE;

  return pNode->GetType();
}

NodeType CVideoDatabaseDirectory::GetDirectoryParentType(const std::string& strPath)
{
  std::string path = CLegacyPathTranslation::TranslateVideoDbPath(strPath);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode)
    return NodeType::NONE;

  CDirectoryNode* pParentNode = pNode->GetParent();

  if (!pParentNode)
    return NodeType::NONE;

  return pParentNode->GetChildType();
}

bool CVideoDatabaseDirectory::GetQueryParams(const std::string& strPath, CQueryParams& params)
{
  std::string path = CLegacyPathTranslation::TranslateVideoDbPath(strPath);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode)
    return false;

  CDirectoryNode::GetDatabaseInfo(strPath,params);
  return true;
}

void CVideoDatabaseDirectory::ClearDirectoryCache(const std::string& strDirectory)
{
  std::string path = CLegacyPathTranslation::TranslateVideoDbPath(strDirectory);
  URIUtils::RemoveSlashAtEnd(path);

  uint32_t crc = Crc32::ComputeFromLowerCase(path);

  std::string strFileName = StringUtils::Format("special://temp/archive_cache/{:08x}.fi", crc);
  CFile::Delete(strFileName);
}

bool CVideoDatabaseDirectory::IsAllItem(const std::string& strDirectory)
{
  if (StringUtils::EndsWith(strDirectory, "/-1/"))
    return true;
  return false;
}

bool CVideoDatabaseDirectory::GetLabel(const std::string& strDirectory, std::string& strLabel)
{
  strLabel = "";

  std::string path = CLegacyPathTranslation::TranslateVideoDbPath(strDirectory);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
  if (!pNode || path.empty())
    return false;

  // first see if there's any filter criteria
  CQueryParams params;
  CDirectoryNode::GetDatabaseInfo(path, params);

  CVideoDatabase videodatabase;
  if (!videodatabase.Open())
    return false;

  // get genre
  if (params.GetGenreId() != -1)
    strLabel += videodatabase.GetGenreById(params.GetGenreId());

  // get country
  if (params.GetCountryId() != -1)
    strLabel += videodatabase.GetCountryById(params.GetCountryId());

  // get set
  if (params.GetSetId() != -1)
    strLabel += videodatabase.GetSetById(params.GetSetId());

  // get tag
  if (params.GetTagId() != -1)
    strLabel += videodatabase.GetTagById(params.GetTagId());

  // get year
  if (params.GetYear() != -1)
  {
    std::string strTemp = std::to_string(params.GetYear());
    if (!strLabel.empty())
      strLabel += " / ";
    strLabel += strTemp;
  }

  // get videoversions
  if (params.GetVideoVersionId() != -1)
    strLabel += videodatabase.GetVideoVersionById(params.GetVideoVersionId());

  if (strLabel.empty())
  {
    switch (pNode->GetChildType())
    {
      case NodeType::TITLE_MOVIES:
      case NodeType::TITLE_TVSHOWS:
      case NodeType::TITLE_MUSICVIDEOS:
        strLabel = g_localizeStrings.Get(369);
        break;
      case NodeType::ACTOR: // Actor
        strLabel = g_localizeStrings.Get(344);
        break;
      case NodeType::GENRE: // Genres
        strLabel = g_localizeStrings.Get(135);
        break;
      case NodeType::COUNTRY: // Countries
        strLabel = g_localizeStrings.Get(20451);
        break;
      case NodeType::YEAR: // Year
        strLabel = g_localizeStrings.Get(562);
        break;
      case NodeType::DIRECTOR: // Director
        strLabel = g_localizeStrings.Get(20348);
        break;
      case NodeType::SETS: // Sets
        strLabel = g_localizeStrings.Get(20434);
        break;
      case NodeType::TAGS: // Tags
        strLabel = g_localizeStrings.Get(20459);
        break;
      case NodeType::VIDEOVERSIONS: // Video versions
        strLabel = g_localizeStrings.Get(40000);
        break;
      case NodeType::MOVIES_OVERVIEW: // Movies
        strLabel = g_localizeStrings.Get(342);
        break;
      case NodeType::TVSHOWS_OVERVIEW: // TV Shows
        strLabel = g_localizeStrings.Get(20343);
        break;
      case NodeType::RECENTLY_ADDED_MOVIES: // Recently Added Movies
        strLabel = g_localizeStrings.Get(20386);
        break;
      case NodeType::RECENTLY_ADDED_EPISODES: // Recently Added Episodes
        strLabel = g_localizeStrings.Get(20387);
        break;
      case NodeType::STUDIO: // Studios
        strLabel = g_localizeStrings.Get(20388);
        break;
      case NodeType::MUSICVIDEOS_OVERVIEW: // Music Videos
        strLabel = g_localizeStrings.Get(20389);
        break;
      case NodeType::RECENTLY_ADDED_MUSICVIDEOS: // Recently Added Music Videos
        strLabel = g_localizeStrings.Get(20390);
        break;
      case NodeType::SEASONS: // Seasons
        strLabel = g_localizeStrings.Get(33054);
        break;
      case NodeType::EPISODES: // Episodes
        strLabel = g_localizeStrings.Get(20360);
        break;
      case NodeType::INPROGRESS_TVSHOWS: // InProgress TvShows
        strLabel = g_localizeStrings.Get(626);
        break;
      case NodeType::MOVIE_ASSETS: // Video assets
        strLabel = g_localizeStrings.Get(40209);
        break;
      case NodeType::MOVIE_ASSETS_VERSIONS: // Video versions
        strLabel = g_localizeStrings.Get(40210);
        break;
      case NodeType::MOVIE_ASSETS_EXTRAS: // Video extras
        strLabel = g_localizeStrings.Get(40211);
        break;

      default:
        return false;
    }
  }

  return true;
}

std::string CVideoDatabaseDirectory::GetIcon(const std::string &strDirectory)
{
  std::string path = CLegacyPathTranslation::TranslateVideoDbPath(strDirectory);
  switch (GetDirectoryChildType(path))
  {
    case NodeType::TITLE_MOVIES:
      if (URIUtils::PathEquals(path, "videodb://movies/titles/"))
      {
        if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                CSettings::SETTING_MYVIDEOS_FLATTEN))
          return "DefaultMovies.png";
        return "DefaultMovieTitle.png";
      }
      return "";
    case NodeType::TITLE_TVSHOWS:
      if (URIUtils::PathEquals(path, "videodb://tvshows/titles/"))
      {
        if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                CSettings::SETTING_MYVIDEOS_FLATTEN))
          return "DefaultTVShows.png";
        return "DefaultTVShowTitle.png";
      }
      return "";
    case NodeType::TITLE_MUSICVIDEOS:
      if (URIUtils::PathEquals(path, "videodb://musicvideos/titles/"))
      {
        if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                CSettings::SETTING_MYVIDEOS_FLATTEN))
          return "DefaultMusicVideos.png";
        return "DefaultMusicVideoTitle.png";
      }
      return "";
    case NodeType::ACTOR: // Actor
      return "DefaultActor.png";
    case NodeType::GENRE: // Genres
      return "DefaultGenre.png";
    case NodeType::COUNTRY: // Countries
      return "DefaultCountry.png";
    case NodeType::SETS: // Sets
      return "DefaultSets.png";
    case NodeType::TAGS: // Tags
      return "DefaultTags.png";
    case NodeType::VIDEOVERSIONS: // Video versions
      return "DefaultVideoVersions.png";
    case NodeType::YEAR: // Year
      return "DefaultYear.png";
    case NodeType::DIRECTOR: // Director
      return "DefaultDirector.png";
    case NodeType::MOVIES_OVERVIEW: // Movies
      return "DefaultMovies.png";
    case NodeType::TVSHOWS_OVERVIEW: // TV Shows
      return "DefaultTVShows.png";
    case NodeType::RECENTLY_ADDED_MOVIES: // Recently Added Movies
      return "DefaultRecentlyAddedMovies.png";
    case NodeType::RECENTLY_ADDED_EPISODES: // Recently Added Episodes
      return "DefaultRecentlyAddedEpisodes.png";
    case NodeType::RECENTLY_ADDED_MUSICVIDEOS: // Recently Added Episodes
      return "DefaultRecentlyAddedMusicVideos.png";
    case NodeType::INPROGRESS_TVSHOWS: // InProgress TvShows
      return "DefaultInProgressShows.png";
    case NodeType::STUDIO: // Studios
      return "DefaultStudios.png";
    case NodeType::MUSICVIDEOS_OVERVIEW: // Music Videos
      return "DefaultMusicVideos.png";
    case NodeType::MUSICVIDEOS_ALBUM: // Music Videos - Albums
      return "DefaultMusicAlbums.png";
    default:
      break;
  }

  return "";
}

bool CVideoDatabaseDirectory::ContainsMovies(const std::string &path)
{
  const auto type = GetDirectoryChildType(path);
  if (type == NodeType::TITLE_MOVIES || type == NodeType::EPISODES ||
      type == NodeType::TITLE_MUSICVIDEOS || type == NodeType::VIDEOVERSIONS)
    return true;
  return false;
}

bool CVideoDatabaseDirectory::Exists(const CURL& url)
{
  std::string path = CLegacyPathTranslation::TranslateVideoDbPath(url);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));

  if (!pNode)
    return false;

  if (pNode->GetChildType() == NodeType::NONE)
    return false;

  return true;
}

bool CVideoDatabaseDirectory::CanCache(const std::string& strPath)
{
  std::string path = CLegacyPathTranslation::TranslateVideoDbPath(strPath);
  std::unique_ptr<CDirectoryNode> pNode(CDirectoryNode::ParseURL(path));
  if (!pNode)
    return false;
  return pNode->CanCache();
}
