/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDatabase.h"

#include <map>
#include <string>
#include <vector>

#include "dbwrappers/dataset.h"
#include "FileItem.h"
#include "utils/log.h"

using namespace dbiplus;

void CVideoDatabase::SetArtForItem(int mediaId, const MediaType &mediaType, const std::map<std::string, std::string> &art)
{
  for (const auto &i : art)
    SetArtForItem(mediaId, mediaType, i.first, i.second);
}

void CVideoDatabase::SetArtForItem(int mediaId, const MediaType &mediaType, const std::string &artType, const std::string &url)
{
  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    // don't set <foo>.<bar> art types - these are derivative types from parent items
    if (artType.find('.') != std::string::npos)
      return;

    std::string sql = PrepareSQL("SELECT art_id,url FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str());
    m_pDS->query(sql);
    if (!m_pDS->eof())
    { // update
      int artId = m_pDS->fv(0).get_asInt();
      std::string oldUrl = m_pDS->fv(1).get_asString();
      m_pDS->close();
      if (oldUrl != url)
      {
        sql = PrepareSQL("UPDATE art SET url='%s' where art_id=%d", url.c_str(), artId);
        m_pDS->exec(sql);
      }
    }
    else
    { // insert
      m_pDS->close();
      sql = PrepareSQL("INSERT INTO art(media_id, media_type, type, url) VALUES (%d, '%s', '%s', '%s')", mediaId, mediaType.c_str(), artType.c_str(), url.c_str());
      m_pDS->exec(sql);
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d, '%s', '%s', '%s') failed", __FUNCTION__, mediaId, mediaType.c_str(), artType.c_str(), url.c_str());
  }
}

bool CVideoDatabase::GetArtForItem(int mediaId, const MediaType &mediaType, std::map<std::string, std::string> &art)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false; // using dataset 2 as we're likely called in loops on dataset 1

    std::string sql = PrepareSQL("SELECT type,url FROM art WHERE media_id=%i AND media_type='%s'", mediaId, mediaType.c_str());
    m_pDS2->query(sql);
    while (!m_pDS2->eof())
    {
      art.insert(make_pair(m_pDS2->fv(0).get_asString(), m_pDS2->fv(1).get_asString()));
      m_pDS2->next();
    }
    m_pDS2->close();
    return !art.empty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, mediaId);
  }
  return false;
}

std::string CVideoDatabase::GetArtForItem(int mediaId, const MediaType &mediaType, const std::string &artType)
{
  std::string query = PrepareSQL("SELECT url FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str());
  return GetSingleValue(query, m_pDS2);
}

bool CVideoDatabase::RemoveArtForItem(int mediaId, const MediaType &mediaType, const std::string &artType)
{
  return ExecuteQuery(PrepareSQL("DELETE FROM art WHERE media_id=%i AND media_type='%s' AND type='%s'", mediaId, mediaType.c_str(), artType.c_str()));
}

bool CVideoDatabase::RemoveArtForItem(int mediaId, const MediaType &mediaType, const std::set<std::string> &artTypes)
{
  bool result = true;
  for (const auto &i : artTypes)
    result &= RemoveArtForItem(mediaId, mediaType, i);

  return result;
}

bool CVideoDatabase::HasArtForItem(int mediaId, const MediaType &mediaType)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false; // using dataset 2 as we're likely called in loops on dataset 1

    std::string sql = PrepareSQL("SELECT 1 FROM art WHERE media_id=%i AND media_type='%s' LIMIT 1", mediaId, mediaType.c_str());
    m_pDS2->query(sql);
    bool result = !m_pDS2->eof();
    m_pDS2->close();
    return result;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, mediaId);
  }
  return false;
}

bool CVideoDatabase::GetTvShowSeasonArt(int showId, std::map<int, std::map<std::string, std::string> > &seasonArt)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false; // using dataset 2 as we're likely called in loops on dataset 1

    std::map<int, int> seasons;
    GetTvShowSeasons(showId, seasons);

    for (const auto &i : seasons)
    {
      std::map<std::string, std::string> art;
      GetArtForItem(i.second, MediaTypeSeason, art);
      seasonArt.insert(std::make_pair(i.first,art));
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, showId);
  }
  return false;
}

bool CVideoDatabase::GetArtTypes(const MediaType &mediaType, std::vector<std::string> &artTypes)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string sql = PrepareSQL("SELECT DISTINCT type FROM art WHERE media_type='%s'", mediaType.c_str());
    int numRows = RunQuery(sql);
    if (numRows <= 0)
      return numRows == 0;

    while (!m_pDS->eof())
    {
      artTypes.emplace_back(m_pDS->fv(0).get_asString());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%s) failed", __FUNCTION__, mediaType.c_str());
  }
  return false;
}

namespace
{
std::vector<std::string> GetBasicItemAvailableArtTypes(const CVideoInfoTag& tag)
{
  std::vector<std::string> result;

  //! @todo artwork: fanart stored separately, doesn't need to be
  if (tag.m_fanart.GetNumFanarts() && std::find(result.cbegin(), result.cend(), "fanart") == result.cend())
    result.push_back("fanart");

  // all other images
  for (const auto& urlEntry : tag.m_strPictureURL.m_url)
  {
    std::string artType = urlEntry.m_aspect;
    if (artType.empty())
      artType = tag.m_type == MediaTypeEpisode ? "thumb" : "poster";
    if (urlEntry.m_type == CScraperUrl::URL_TYPE_GENERAL && // exclude season artwork for TV shows
      !StringUtils::StartsWith(artType, "set.") && // exclude movie set artwork for movies
      std::find(result.cbegin(), result.cend(), artType) == result.cend())
    {
      result.push_back(artType);
    }
  }
  return result;
}

std::vector<std::string> GetSeasonAvailableArtTypes(int mediaId, CVideoDatabase& db)
{
  CVideoInfoTag tag;
  db.GetSeasonInfo(mediaId, tag);

  std::vector<std::string> result;

  CVideoInfoTag sourceShow;
  db.GetTvShowInfo("", sourceShow, tag.m_iIdShow);
  for (const auto& urlEntry : sourceShow.m_strPictureURL.m_url)
  {
    std::string artType = urlEntry.m_aspect;
    if (artType.empty())
      artType = "poster";
    if (urlEntry.m_type == CScraperUrl::URL_TYPE_SEASON && urlEntry.m_season == tag.m_iSeason &&
      std::find(result.cbegin(), result.cend(), artType) == result.cend())
    {
      result.push_back(artType);
    }
  }
  return result;
}

std::vector<std::string> GetMovieSetAvailableArtTypes(int mediaId, CVideoDatabase& db)
{
  std::vector<std::string> result;
  CFileItemList items;
  std::string baseDir = StringUtils::Format("videodb://movies/sets/%d", mediaId);
  if (db.GetMoviesNav(baseDir, items))
  {
    for (const auto& item : items)
    {
      CVideoInfoTag* pTag = item->GetVideoInfoTag();
      pTag->m_strPictureURL.Parse();
      //! @todo artwork: fanart stored separately, doesn't need to be
      pTag->m_fanart.Unpack();
      if (pTag->m_fanart.GetNumFanarts() &&
        std::find(result.cbegin(), result.cend(), "fanart") == result.cend())
      {
        result.push_back("fanart");
      }

      // all other images
      for (const auto& urlEntry : pTag->m_strPictureURL.m_url)
      {
        std::string artType = urlEntry.m_aspect;
        if (artType.empty())
          artType = "poster";
        else if (StringUtils::StartsWith(artType, "set."))
          artType = artType.substr(4);

        if (std::find(result.cbegin(), result.cend(), artType) == result.cend())
          result.push_back(artType);
      }
    }
  }
  return result;
}
}

std::vector<std::string> CVideoDatabase::GetAvailableArtTypesForItem(int mediaId,
  const MediaType& mediaType)
{
  VIDEODB_CONTENT_TYPE dbType{VIDEODB_CONTENT_UNKNOWN};
  if (mediaType == MediaTypeTvShow)
    dbType = VIDEODB_CONTENT_TVSHOWS;
  else if (mediaType == MediaTypeMovie)
    dbType = VIDEODB_CONTENT_MOVIES;
  else if (mediaType == MediaTypeEpisode)
    dbType = VIDEODB_CONTENT_EPISODES;
  else if (mediaType == MediaTypeMusicVideo)
    dbType = VIDEODB_CONTENT_MUSICVIDEOS;

  if (dbType != VIDEODB_CONTENT_UNKNOWN)
  {
    CVideoInfoTag tag = GetDetailsByTypeAndId(dbType, mediaId);
    return GetBasicItemAvailableArtTypes(tag);
  }
  if (mediaType == MediaTypeSeason)
    return GetSeasonAvailableArtTypes(mediaId, *this);
  if (mediaType == MediaTypeVideoCollection)
    return GetMovieSetAvailableArtTypes(mediaId, *this);
  return {};
}

void CVideoDatabase::UpdateFanart(const CFileItem &item, VIDEODB_CONTENT_TYPE type)
{
  if (NULL == m_pDB.get()) return;
  if (NULL == m_pDS.get()) return;
  if (!item.HasVideoInfoTag() || item.GetVideoInfoTag()->m_iDbId < 0) return;

  std::string exec;
  if (type == VIDEODB_CONTENT_TVSHOWS)
    exec = PrepareSQL("UPDATE tvshow set c%02d='%s' WHERE idShow=%i", VIDEODB_ID_TV_FANART, item.GetVideoInfoTag()->m_fanart.m_xml.c_str(), item.GetVideoInfoTag()->m_iDbId);
  else if (type == VIDEODB_CONTENT_MOVIES)
    exec = PrepareSQL("UPDATE movie set c%02d='%s' WHERE idMovie=%i", VIDEODB_ID_FANART, item.GetVideoInfoTag()->m_fanart.m_xml.c_str(), item.GetVideoInfoTag()->m_iDbId);

  try
  {
    m_pDS->exec(exec);

    if (type == VIDEODB_CONTENT_TVSHOWS)
      AnnounceUpdate(MediaTypeTvShow, item.GetVideoInfoTag()->m_iDbId);
    else if (type == VIDEODB_CONTENT_MOVIES)
      AnnounceUpdate(MediaTypeMovie, item.GetVideoInfoTag()->m_iDbId);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - error updating fanart for %s", __FUNCTION__, item.GetPath().c_str());
  }
}
