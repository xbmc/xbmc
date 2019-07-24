/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDatabase.h"

#include <string>

#include "dbwrappers/dataset.h"
#include "utils/log.h"

using namespace dbiplus;

int CVideoDatabase::UpdateRatings(int mediaId, const char *mediaType, const RatingMap& values, const std::string& defaultRating)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    std::string sql = PrepareSQL("DELETE FROM rating WHERE media_id=%i AND media_type='%s'", mediaId, mediaType);
    m_pDS->exec(sql);

    return AddRatings(mediaId, mediaType, values, defaultRating);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s unable to update ratings of  (%s)", __FUNCTION__, mediaType);
  }
  return -1;
}

int CVideoDatabase::AddRatings(int mediaId, const char *mediaType, const RatingMap& values, const std::string& defaultRating)
{
  int ratingid = -1;
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    for (const auto& i : values)
    {
      int id;
      std::string strSQL = PrepareSQL("SELECT rating_id FROM rating WHERE media_id=%i AND media_type='%s' AND rating_type = '%s'", mediaId, mediaType, i.first.c_str());
      m_pDS->query(strSQL);
      if (m_pDS->num_rows() == 0)
      {
        m_pDS->close();
        // doesnt exists, add it
        strSQL = PrepareSQL("INSERT INTO rating (media_id, media_type, rating_type, rating, votes) VALUES (%i, '%s', '%s', %f, %i)", mediaId, mediaType, i.first.c_str(), i.second.rating, i.second.votes);
        m_pDS->exec(strSQL);
        id = (int)m_pDS->lastinsertid();
      }
      else
      {
        id = m_pDS->fv(0).get_asInt();
        m_pDS->close();
        strSQL = PrepareSQL("UPDATE rating SET rating = %f, votes = %i WHERE rating_id = %i", i.second.rating, i.second.votes, id);
        m_pDS->exec(strSQL);
      }
      if (i.first == defaultRating)
        ratingid = id;
    }
    return ratingid;

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i - %s) failed", __FUNCTION__, mediaId, mediaType);
  }

  return ratingid;
}

void CVideoDatabase::GetRatings(int media_id, const std::string &media_type, RatingMap &ratings)
{
  try
  {
    if (!m_pDB.get()) return;
    if (!m_pDS2.get()) return;

    std::string sql = PrepareSQL("SELECT rating.rating_type, rating.rating, rating.votes FROM rating WHERE rating.media_id = %i AND rating.media_type = '%s'", media_id, media_type.c_str());
    m_pDS2->query(sql);
    while (!m_pDS2->eof())
    {
      ratings[m_pDS2->fv(0).get_asString()] = CRating(m_pDS2->fv(1).get_asFloat(), m_pDS2->fv(2).get_asInt());
      m_pDS2->next();
    }
    m_pDS2->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%i,%s) failed", __FUNCTION__, media_id, media_type.c_str());
  }
}

bool CVideoDatabase::SetVideoUserRating(int dbId, int rating, const MediaType& mediaType)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (mediaType == MediaTypeNone)
      return false;

    std::string sql;
    if (mediaType == MediaTypeMovie)
      sql = PrepareSQL("UPDATE movie SET userrating=%i WHERE idMovie = %i", rating, dbId);
    else if (mediaType == MediaTypeEpisode)
      sql = PrepareSQL("UPDATE episode SET userrating=%i WHERE idEpisode = %i", rating, dbId);
    else if (mediaType == MediaTypeMusicVideo)
      sql = PrepareSQL("UPDATE musicvideo SET userrating=%i WHERE idMVideo = %i", rating, dbId);
    else if (mediaType == MediaTypeTvShow)
      sql = PrepareSQL("UPDATE tvshow SET userrating=%i WHERE idShow = %i", rating, dbId);
    else if (mediaType == MediaTypeSeason)
      sql = PrepareSQL("UPDATE seasons SET userrating=%i WHERE idSeason = %i", rating, dbId);

    m_pDS->exec(sql);
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i, %s, %i) failed", __FUNCTION__, dbId, mediaType.c_str(), rating);
  }
  return false;
}
