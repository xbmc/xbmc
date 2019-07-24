/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoDatabase.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "video/VideoInfoTag.h"
#include "ServiceBroker.h"
#include "dbwrappers/dataset.h"
#include "FileItem.h"
#include "GUIPassword.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SystemClock.h"
#include "utils/log.h"

using namespace dbiplus;

extern DWORD castTime;
extern DWORD movieTime;

bool CVideoDatabase::LinkMovieToTvshow(int idMovie, int idShow, bool bRemove)
{
   try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    if (bRemove) // delete link
    {
      std::string strSQL=PrepareSQL("delete from movielinktvshow where idMovie=%i and idShow=%i", idMovie, idShow);
      m_pDS->exec(strSQL);
      return true;
    }

    std::string strSQL=PrepareSQL("insert into movielinktvshow (idShow,idMovie) values (%i,%i)", idShow,idMovie);
    m_pDS->exec(strSQL);

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i, %i) failed", __FUNCTION__, idMovie, idShow);
  }

  return false;
}

bool CVideoDatabase::IsLinkedToTvshow(int idMovie)
{
   try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL=PrepareSQL("select * from movielinktvshow where idMovie=%i", idMovie);
    m_pDS->query(strSQL);
    if (m_pDS->eof())
    {
      m_pDS->close();
      return false;
    }

    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idMovie);
  }

  return false;
}

bool CVideoDatabase::GetLinksToTvShow(int idMovie, std::vector<int>& ids)
{
   try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    std::string strSQL=PrepareSQL("select * from movielinktvshow where idMovie=%i", idMovie);
    m_pDS2->query(strSQL);
    while (!m_pDS2->eof())
    {
      ids.push_back(m_pDS2->fv(1).get_asInt());
      m_pDS2->next();
    }

    m_pDS2->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idMovie);
  }

  return false;
}

//********************************************************************************************************************************
int CVideoDatabase::GetMovieId(const std::string& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    int idMovie = -1;

    // needed for query parameters
    int idFile = GetFileId(strFilenameAndPath);
    int idPath=-1;
    std::string strPath;
    if (idFile < 0)
    {
      std::string strFile;
      SplitPath(strFilenameAndPath,strPath,strFile);

      // have to join movieinfo table for correct results
      idPath = GetPathId(strPath);
      if (idPath < 0 && strPath != strFilenameAndPath)
        return -1;
    }

    if (idFile == -1 && strPath != strFilenameAndPath)
      return -1;

    std::string strSQL;
    if (idFile == -1)
      strSQL=PrepareSQL("select idMovie from movie join files on files.idFile=movie.idFile where files.idPath=%i",idPath);
    else
      strSQL=PrepareSQL("select idMovie from movie where idFile=%i", idFile);

    CLog::Log(LOGDEBUG, LOGDATABASE, "%s (%s), query = %s", __FUNCTION__, CURL::GetRedacted(strFilenameAndPath).c_str(), strSQL.c_str());
    m_pDS->query(strSQL);
    if (m_pDS->num_rows() > 0)
      idMovie = m_pDS->fv("idMovie").get_asInt();
    m_pDS->close();

    return idMovie;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

//********************************************************************************************************************************
int CVideoDatabase::AddMovie(const std::string& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    int idMovie = GetMovieId(strFilenameAndPath);
    if (idMovie < 0)
    {
      int idFile = AddFile(strFilenameAndPath);
      if (idFile < 0)
        return -1;
      UpdateFileDateAdded(idFile, strFilenameAndPath);
      std::string strSQL=PrepareSQL("insert into movie (idMovie, idFile) values (NULL, %i)", idFile);
      m_pDS->exec(strSQL);
      idMovie = (int)m_pDS->lastinsertid();
    }

    return idMovie;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

bool CVideoDatabase::HasMovieInfo(const std::string& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    int idMovie = GetMovieId(strFilenameAndPath);
    return (idMovie > 0); // index of zero is also invalid
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

//********************************************************************************************************************************
void CVideoDatabase::GetMoviesByActor(const std::string& name, CFileItemList& items)
{
  Filter filter;
  filter.join  = "LEFT JOIN actor_link ON actor_link.media_id=movie_view.idMovie AND actor_link.media_type='movie' "
                 "LEFT JOIN actor a ON a.actor_id=actor_link.actor_id "
                 "LEFT JOIN director_link ON director_link.media_id=movie_view.idMovie AND director_link.media_type='movie' "
                 "LEFT JOIN actor d ON d.actor_id=director_link.actor_id";
  filter.where = PrepareSQL("a.name='%s' OR d.name='%s'", name.c_str(), name.c_str());
  filter.group = "movie_view.idMovie";
  GetMoviesByWhere("videodb://movies/titles/", filter, items);
}

//********************************************************************************************************************************
bool CVideoDatabase::GetMovieInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idMovie /* = -1 */, int getDetails /* = VideoDbDetailsAll */)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    if (idMovie < 0)
      idMovie = GetMovieId(strFilenameAndPath);
    if (idMovie < 0) return false;

    std::string sql = PrepareSQL("select * from movie_view where idMovie=%i", idMovie);
    if (!m_pDS->query(sql))
      return false;
    details = GetDetailsForMovie(m_pDS, getDetails);
    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

int CVideoDatabase::SetDetailsForMovie(const std::string& strFilenameAndPath, CVideoInfoTag& details,
    const std::map<std::string, std::string> &artwork, int idMovie /* = -1 */)
{
  try
  {
    BeginTransaction();

    if (idMovie < 0)
      idMovie = GetMovieId(strFilenameAndPath);

    if (idMovie > -1)
      DeleteMovie(idMovie, true); // true to keep the table entry
    else
    {
      // only add a new movie if we don't already have a valid idMovie
      // (DeleteMovie is called with bKeepId == true so the movie won't
      // be removed from the movie table)
      idMovie = AddMovie(strFilenameAndPath);
      if (idMovie < 0)
      {
        RollbackTransaction();
        return idMovie;
      }
    }

    // update dateadded if it's set
    if (details.m_dateAdded.IsValid())
    {
      if (details.m_iFileId <= 0)
        details.m_iFileId = GetFileId(strFilenameAndPath);

      UpdateFileDateAdded(details.m_iFileId, strFilenameAndPath, details.m_dateAdded);
    }

    AddCast(idMovie, "movie", details.m_cast);
    AddLinksToItem(idMovie, MediaTypeMovie, "genre", details.m_genre);
    AddLinksToItem(idMovie, MediaTypeMovie, "studio", details.m_studio);
    AddLinksToItem(idMovie, MediaTypeMovie, "country", details.m_country);
    AddLinksToItem(idMovie, MediaTypeMovie, "tag", details.m_tags);
    AddActorLinksToItem(idMovie, MediaTypeMovie, "director", details.m_director);
    AddActorLinksToItem(idMovie, MediaTypeMovie, "writer", details.m_writingCredits);

    // add ratingsu
    details.m_iIdRating = AddRatings(idMovie, MediaTypeMovie, details.m_ratings, details.GetDefaultRating());

    // add unique ids
    details.m_iIdUniqueID = AddUniqueIDs(idMovie, MediaTypeMovie, details);

    // add set...
    int idSet = -1;
    if (!details.m_set.title.empty())
    {
      idSet = AddSet(details.m_set.title, details.m_set.overview);
      // add art if not available
      if (!HasArtForItem(idSet, MediaTypeVideoCollection))
      {
        std::map<std::string, std::string> setArt;
        for (const auto &it : artwork)
        {
          if (StringUtils::StartsWith(it.first, "set."))
            setArt[it.first.substr(4)] = it.second;
        }
        SetArtForItem(idSet, MediaTypeVideoCollection, setArt.empty() ? artwork : setArt);
      }
    }

    if (details.HasStreamDetails())
      SetStreamDetailsForFileId(details.m_streamDetails, GetFileId(strFilenameAndPath));

    SetArtForItem(idMovie, MediaTypeMovie, artwork);

    if (!details.HasUniqueID() && details.HasYear())
    { // query DB for any movies matching online id and year
      std::string strSQL = PrepareSQL("SELECT files.playCount, files.lastPlayed "
                                      "FROM movie "
                                      "  INNER JOIN files "
                                      "    ON files.idFile=movie.idFile "
                                      "  JOIN uniqueid "
                                      "    ON movie.idMovie=uniqueid.media_id AND uniqueid.media_type='movie' AND uniqueid.value='%s'"
                                      "WHERE movie.premiered LIKE '%i%%' AND movie.idMovie!=%i AND files.playCount > 0",
                                      details.GetUniqueID().c_str(), details.GetYear(), idMovie);
      m_pDS->query(strSQL);

      if (!m_pDS->eof())
      {
        int playCount = m_pDS->fv("files.playCount").get_asInt();

        CDateTime lastPlayed;
        lastPlayed.SetFromDBDateTime(m_pDS->fv("files.lastPlayed").get_asString());

        int idFile = GetFileId(strFilenameAndPath);

        // update with playCount and lastPlayed
        strSQL = PrepareSQL("update files set playCount=%i,lastPlayed='%s' where idFile=%i", playCount, lastPlayed.GetAsDBDateTime().c_str(), idFile);
        m_pDS->exec(strSQL);
      }

      m_pDS->close();
    }
    // update our movie table (we know it was added already above)
    // and insert the new row
    std::string sql = "UPDATE movie SET " + GetValueString(details, VIDEODB_ID_MIN, VIDEODB_ID_MAX, DbMovieOffsets);
    if (idSet > 0)
      sql += PrepareSQL(", idSet = %i", idSet);
    else
      sql += ", idSet = NULL";
    if (details.m_iUserRating > 0 && details.m_iUserRating < 11)
      sql += PrepareSQL(", userrating = %i", details.m_iUserRating);
    else
      sql += ", userrating = NULL";
    if (details.HasPremiered())
      sql += PrepareSQL(", premiered = '%s'", details.GetPremiered().GetAsDBDate().c_str());
    else
      sql += PrepareSQL(", premiered = '%i'", details.GetYear());
    sql += PrepareSQL(" where idMovie=%i", idMovie);
    m_pDS->exec(sql);
    CommitTransaction();

    return idMovie;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  RollbackTransaction();
  return -1;
}

int CVideoDatabase::UpdateDetailsForMovie(int idMovie, CVideoInfoTag& details, const std::map<std::string, std::string> &artwork, const std::set<std::string> &updatedDetails)
{
  if (idMovie < 0)
    return idMovie;

  try
  {
    CLog::Log(LOGINFO, "%s: Starting updates for movie %i", __FUNCTION__, idMovie);

    BeginTransaction();

    // process the link table updates
    if (updatedDetails.find("genre") != updatedDetails.end())
      UpdateLinksToItem(idMovie, MediaTypeMovie, "genre", details.m_genre);
    if (updatedDetails.find("studio") != updatedDetails.end())
      UpdateLinksToItem(idMovie, MediaTypeMovie, "studio", details.m_studio);
    if (updatedDetails.find("country") != updatedDetails.end())
      UpdateLinksToItem(idMovie, MediaTypeMovie, "country", details.m_country);
    if (updatedDetails.find("tag") != updatedDetails.end())
      UpdateLinksToItem(idMovie, MediaTypeMovie, "tag", details.m_tags);
    if (updatedDetails.find("director") != updatedDetails.end())
      UpdateActorLinksToItem(idMovie, MediaTypeMovie, "director", details.m_director);
    if (updatedDetails.find("writer") != updatedDetails.end())
      UpdateActorLinksToItem(idMovie, MediaTypeMovie, "writer", details.m_writingCredits);
    if (updatedDetails.find("art.altered") != updatedDetails.end())
      SetArtForItem(idMovie, MediaTypeMovie, artwork);
    if (updatedDetails.find("ratings") != updatedDetails.end())
      details.m_iIdRating = UpdateRatings(idMovie, MediaTypeMovie, details.m_ratings, details.GetDefaultRating());
    if (updatedDetails.find("uniqueid") != updatedDetails.end())
      details.m_iIdUniqueID = UpdateUniqueIDs(idMovie, MediaTypeMovie, details);
    if (updatedDetails.find("dateadded") != updatedDetails.end() && details.m_dateAdded.IsValid())
    {
      if (details.m_iFileId <= 0)
        details.m_iFileId = GetFileId(details.GetPath());

      UpdateFileDateAdded(details.m_iFileId, details.GetPath(), details.m_dateAdded);
    }

    // track if the set was updated
    int idSet = 0;
    if (updatedDetails.find("set") != updatedDetails.end())
    { // set
      idSet = -1;
      if (!details.m_set.title.empty())
      {
        idSet = AddSet(details.m_set.title, details.m_set.overview);
        // add art if not available
        if (!HasArtForItem(idSet, MediaTypeVideoCollection))
          SetArtForItem(idSet, MediaTypeVideoCollection, artwork);
      }
    }

    if (updatedDetails.find("showlink") != updatedDetails.end())
    {
      // remove existing links
      std::vector<int> tvShowIds;
      GetLinksToTvShow(idMovie, tvShowIds);
      for (const auto& idTVShow : tvShowIds)
        LinkMovieToTvshow(idMovie, idTVShow, true);

      // setup links to shows if the linked shows are in the db
      for (const auto& showLink : details.m_showLink)
      {
        CFileItemList items;
        GetTvShowsByName(showLink, items);
        if (!items.IsEmpty())
          LinkMovieToTvshow(idMovie, items[0]->GetVideoInfoTag()->m_iDbId, false);
        else
          CLog::Log(LOGWARNING, "%s: Failed to link movie %s to show %s", __FUNCTION__, details.m_strTitle.c_str(), showLink.c_str());
      }
    }

    // and update the movie table
    std::string sql = "UPDATE movie SET " + GetValueString(details, VIDEODB_ID_MIN, VIDEODB_ID_MAX, DbMovieOffsets);
    if (idSet > 0)
      sql += PrepareSQL(", idSet = %i", idSet);
    else if (idSet < 0)
      sql += ", idSet = NULL";
    if (details.m_iUserRating > 0 && details.m_iUserRating < 11)
      sql += PrepareSQL(", userrating = %i", details.m_iUserRating);
    else
      sql += ", userrating = NULL";
    if (details.HasPremiered())
      sql += PrepareSQL(", premiered = '%s'", details.GetPremiered().GetAsDBDate().c_str());
    else
      sql += PrepareSQL(", premiered = '%i'", details.GetYear());
    sql += PrepareSQL(" where idMovie=%i", idMovie);
    m_pDS->exec(sql);

    CommitTransaction();

    CLog::Log(LOGINFO, "%s: Finished updates for movie %i", __FUNCTION__, idMovie);

    return idMovie;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idMovie);
  }
  RollbackTransaction();
  return -1;
}

//********************************************************************************************************************************
void CVideoDatabase::DeleteMovie(const std::string& strFilenameAndPath, bool bKeepId /* = false */)
{
  int idMovie = GetMovieId(strFilenameAndPath);
  if (idMovie > -1)
    DeleteMovie(idMovie, bKeepId);
}

void CVideoDatabase::DeleteMovie(int idMovie, bool bKeepId /* = false */)
{
  if (idMovie < 0)
    return;

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    BeginTransaction();

    // keep the movie table entry, linking to tv shows, and bookmarks
    // so we can update the data in place
    // the ancillary tables are still purged
    if (!bKeepId)
    {
      int idFile = GetDbId(PrepareSQL("SELECT idFile FROM movie WHERE idMovie=%i", idMovie));
      std::string path = GetSingleValue(PrepareSQL("SELECT strPath FROM path JOIN files ON files.idPath=path.idPath WHERE files.idFile=%i", idFile));
      if (!path.empty())
        InvalidatePathHash(path);

      std::string strSQL = PrepareSQL("delete from movie where idMovie=%i", idMovie);
      m_pDS->exec(strSQL);
    }

    //! @todo move this below CommitTransaction() once UPnP doesn't rely on this anymore
    if (!bKeepId)
      AnnounceRemove(MediaTypeMovie, idMovie);

    CommitTransaction();

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
    RollbackTransaction();
  }
}

CVideoInfoTag CVideoDatabase::GetDetailsForMovie(std::unique_ptr<Dataset> &pDS, int getDetails /* = VideoDbDetailsNone */)
{
  return GetDetailsForMovie(pDS->get_sql_record(), getDetails);
}

CVideoInfoTag CVideoDatabase::GetDetailsForMovie(const dbiplus::sql_record* const record, int getDetails /* = VideoDbDetailsNone */)
{
  CVideoInfoTag details;

  if (record == NULL)
    return details;

  DWORD time = XbmcThreads::SystemClockMillis();
  int idMovie = record->at(0).get_asInt();

  GetDetailsFromDB(record, VIDEODB_ID_MIN, VIDEODB_ID_MAX, DbMovieOffsets, details);

  details.m_iDbId = idMovie;
  details.m_type = MediaTypeMovie;

  details.m_set.id = record->at(VIDEODB_DETAILS_MOVIE_SET_ID).get_asInt();
  details.m_set.title = record->at(VIDEODB_DETAILS_MOVIE_SET_NAME).get_asString();
  details.m_set.overview = record->at(VIDEODB_DETAILS_MOVIE_SET_OVERVIEW).get_asString();
  details.m_iFileId = record->at(VIDEODB_DETAILS_FILEID).get_asInt();
  details.m_strPath = record->at(VIDEODB_DETAILS_MOVIE_PATH).get_asString();
  std::string strFileName = record->at(VIDEODB_DETAILS_MOVIE_FILE).get_asString();
  ConstructPath(details.m_strFileNameAndPath,details.m_strPath,strFileName);
  details.SetPlayCount(record->at(VIDEODB_DETAILS_MOVIE_PLAYCOUNT).get_asInt());
  details.m_lastPlayed.SetFromDBDateTime(record->at(VIDEODB_DETAILS_MOVIE_LASTPLAYED).get_asString());
  details.m_dateAdded.SetFromDBDateTime(record->at(VIDEODB_DETAILS_MOVIE_DATEADDED).get_asString());
  details.SetResumePoint(record->at(VIDEODB_DETAILS_MOVIE_RESUME_TIME).get_asInt(),
                         record->at(VIDEODB_DETAILS_MOVIE_TOTAL_TIME).get_asInt(),
                         record->at(VIDEODB_DETAILS_MOVIE_PLAYER_STATE).get_asString());
  details.m_iUserRating = record->at(VIDEODB_DETAILS_MOVIE_USER_RATING).get_asInt();
  details.SetRating(record->at(VIDEODB_DETAILS_MOVIE_RATING).get_asFloat(),
                    record->at(VIDEODB_DETAILS_MOVIE_VOTES).get_asInt(),
                    record->at(VIDEODB_DETAILS_MOVIE_RATING_TYPE).get_asString(), true);
  details.SetUniqueID(record->at(VIDEODB_DETAILS_MOVIE_UNIQUEID_VALUE).get_asString(), record->at(VIDEODB_DETAILS_MOVIE_UNIQUEID_TYPE).get_asString() ,true);
  std::string premieredString = record->at(VIDEODB_DETAILS_MOVIE_PREMIERED).get_asString();
  if (premieredString.size() == 4)
    details.SetYear(record->at(VIDEODB_DETAILS_MOVIE_PREMIERED).get_asInt());
  else
    details.SetPremieredFromDBDate(premieredString);
  movieTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();

  if (getDetails)
  {
    if (getDetails & VideoDbDetailsCast)
    {
      GetCast(details.m_iDbId, MediaTypeMovie, details.m_cast);
      castTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();
    }

    if (getDetails & VideoDbDetailsTag)
      GetTags(details.m_iDbId, MediaTypeMovie, details.m_tags);

    if (getDetails & VideoDbDetailsRating)
      GetRatings(details.m_iDbId, MediaTypeMovie, details.m_ratings);

    if (getDetails & VideoDbDetailsUniqueID)
     GetUniqueIDs(details.m_iDbId, MediaTypeMovie, details);

    details.m_strPictureURL.Parse();

    if (getDetails & VideoDbDetailsShowLink)
    {
      // create tvshowlink string
      std::vector<int> links;
      GetLinksToTvShow(idMovie, links);
      for (unsigned int i = 0; i < links.size(); ++i)
      {
        std::string strSQL = PrepareSQL("select c%02d from tvshow where idShow=%i",
          VIDEODB_ID_TV_TITLE, links[i]);
        m_pDS2->query(strSQL);
        if (!m_pDS2->eof())
          details.m_showLink.emplace_back(m_pDS2->fv(0).get_asString());
      }
      m_pDS2->close();
    }

    if (getDetails & VideoDbDetailsStream)
      GetStreamDetails(details);

    details.m_parsedDetails = getDetails;
  }
  return details;
}

bool CVideoDatabase::GetMoviesNav(const std::string& strBaseDir, CFileItemList& items,
                                  int idGenre /* = -1 */, int idYear /* = -1 */, int idActor /* = -1 */, int idDirector /* = -1 */,
                                  int idStudio /* = -1 */, int idCountry /* = -1 */, int idSet /* = -1 */, int idTag /* = -1 */,
                                  const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(strBaseDir))
    return false;

  if (idGenre > 0)
    videoUrl.AddOption("genreid", idGenre);
  else if (idCountry > 0)
    videoUrl.AddOption("countryid", idCountry);
  else if (idStudio > 0)
    videoUrl.AddOption("studioid", idStudio);
  else if (idDirector > 0)
    videoUrl.AddOption("directorid", idDirector);
  else if (idYear > 0)
    videoUrl.AddOption("year", idYear);
  else if (idActor > 0)
    videoUrl.AddOption("actorid", idActor);
  else if (idSet > 0)
    videoUrl.AddOption("setid", idSet);
  else if (idTag > 0)
    videoUrl.AddOption("tagid", idTag);

  Filter filter;
  return GetMoviesByWhere(videoUrl.ToString(), filter, items, sortDescription, getDetails);
}

bool CVideoDatabase::GetMoviesByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
{
  try
  {
    movieTime = 0;
    castTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // parse the base path to get additional filters
    CVideoDbUrl videoUrl;
    Filter extFilter = filter;
    SortDescription sorting = sortDescription;
    if (!videoUrl.FromString(strBaseDir) || !GetFilter(videoUrl, extFilter, sorting))
      return false;

    int total = -1;

    std::string strSQL = "select %s from movie_view ";
    std::string strSQLExtra;
    if (!CDatabase::BuildSQL(strSQLExtra, extFilter, strSQLExtra))
      return false;

    // Apply the limiting directly here if there's no special sorting but limiting
    if (extFilter.limit.empty() &&
        sorting.sortBy == SortByNone &&
       (sorting.limitStart > 0 || sorting.limitEnd > 0))
    {
      total = (int)strtol(GetSingleValue(PrepareSQL(strSQL, "COUNT(1)") + strSQLExtra, m_pDS).c_str(), NULL, 10);
      strSQLExtra += DatabaseUtils::BuildLimitClause(sorting.limitEnd, sorting.limitStart);
    }

    strSQL = PrepareSQL(strSQL, !extFilter.fields.empty() ? extFilter.fields.c_str() : "*") + strSQLExtra;

    int iRowsFound = RunQuery(strSQL);
    if (iRowsFound <= 0)
      return iRowsFound == 0;

    // store the total value of items as a property
    if (total < iRowsFound)
      total = iRowsFound;
    items.SetProperty("total", total);

    DatabaseResults results;
    results.reserve(iRowsFound);

    if (!SortUtils::SortFromDataset(sortDescription, MediaTypeMovie, m_pDS, results))
      return false;

    // get data from returned rows
    items.Reserve(results.size());
    const query_data &data = m_pDS->get_result_set().records;
    for (const auto &i : results)
    {
      unsigned int targetRow = (unsigned int)i.at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);

      CVideoInfoTag movie = GetDetailsForMovie(record, getDetails);
      if (m_profileManager.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE ||
          g_passwordManager.bMasterUser                                   ||
          g_passwordManager.IsDatabasePathUnlocked(movie.m_strPath, *CMediaSourceSettings::GetInstance().GetSources("video")))
      {
        CFileItemPtr pItem(new CFileItem(movie));

        CVideoDbUrl itemUrl = videoUrl;
        std::string path = StringUtils::Format("%i", movie.m_iDbId);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());
        pItem->SetDynPath(movie.m_strFileNameAndPath);

        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED,movie.GetPlayCount() > 0);
        items.Add(pItem);
      }
    }

    // cleanup
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

bool CVideoDatabase::GetRecentlyAddedMoviesNav(const std::string& strBaseDir, CFileItemList& items, unsigned int limit /* = 0 */, int getDetails /* = VideoDbDetailsNone */)
{
  Filter filter;
  filter.order = "dateAdded desc, idMovie desc";
  filter.limit = PrepareSQL("%u", limit ? limit : CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iVideoLibraryRecentlyAddedItems);
  return GetMoviesByWhere(strBaseDir, filter, items, SortDescription(), getDetails);
}

void CVideoDatabase::GetMovieGenresByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("SELECT genre.genre_id, genre.name, path.strPath FROM genre INNER JOIN genre_link ON genre_link.genre_id = genre.genre_id INNER JOIN movie ON (genre_link.media_type='movie' = genre_link.media_id=movie.idMovie) INNER JOIN files ON files.idFile=movie.idFile INNER JOIN path ON path.idPath=files.idPath WHERE genre.name LIKE '%%%s%%'",strSearch.c_str());
    else
      strSQL=PrepareSQL("SELECT DISTINCT genre.genre_id, genre.name FROM genre INNER JOIN genre_link ON genre_link.genre_id=genre.genre_id WHERE genre_link.media_type='movie' AND name LIKE '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),
                                                      *CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
      pItem->SetPath("videodb://movies/genres/"+ strDir);
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}

void CVideoDatabase::GetMovieCountriesByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("SELECT country.country_id, country.name, path.strPath FROM country INNER JOIN country_link ON country_link.country_id=country.country_id INNER JOIN movie ON country_link.media_id=movie.idMovie INNER JOIN files ON files.idFile=movie.idFile INNER JOIN path ON path.idPath=files.idPath WHERE country_link.media_type='movie' AND country.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL=PrepareSQL("SELECT DISTINCT country.country_id, country.name FROM country INNER JOIN country_link ON country_link.country_id=country.country_id WHERE country_link.media_type='movie' AND name like '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),
                                                      *CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
      pItem->SetPath("videodb://movies/genres/"+ strDir);
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}

void CVideoDatabase::GetMovieActorsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("SELECT actor.actor_id, actor.name, path.strPath FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id INNER JOIN movie ON actor_link.media_id=movie.idMovie INNER JOIN files ON files.idFile=movie.idFile INNER JOIN path ON path.idPath=files.idPath WHERE actor_link.media_type='movie' AND actor.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL=PrepareSQL("SELECT DISTINCT actor.actor_id, actor.name FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id INNER JOIN movie ON actor_link.media_id=movie.idMovie WHERE actor_link.media_type='movie' AND actor.name LIKE '%%%s%%'", strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
      pItem->SetPath("videodb://movies/actors/"+ strDir);
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}

void CVideoDatabase::GetMoviesByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT movie.idMovie, movie.c%02d, path.strPath, movie.idSet FROM movie INNER JOIN files ON files.idFile=movie.idFile INNER JOIN path ON path.idPath=files.idPath WHERE movie.c%02d LIKE '%%%s%%'", VIDEODB_ID_TITLE, VIDEODB_ID_TITLE, strSearch.c_str());
    else
      strSQL = PrepareSQL("select movie.idMovie,movie.c%02d, movie.idSet from movie where movie.c%02d like '%%%s%%'",VIDEODB_ID_TITLE,VIDEODB_ID_TITLE,strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      int movieId = m_pDS->fv("movie.idMovie").get_asInt();
      int setId = m_pDS->fv("movie.idSet").get_asInt();
      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string path;
      if (setId <= 0 || !CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOLIBRARY_GROUPMOVIESETS))
        path = StringUtils::Format("videodb://movies/titles/%i", movieId);
      else
        path = StringUtils::Format("videodb://movies/sets/%i/%i", setId, movieId);
      pItem->SetPath(path);
      pItem->m_bIsFolder=false;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}

void CVideoDatabase::GetMoviesByPlot(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("select movie.idMovie, movie.c%02d, path.strPath FROM movie INNER JOIN files ON files.idFile=movie.idFile INNER JOIN path ON path.idPath=files.idPath WHERE movie.c%02d LIKE '%%%s%%' OR movie.c%02d LIKE '%%%s%%' OR movie.c%02d LIKE '%%%s%%'", VIDEODB_ID_TITLE,VIDEODB_ID_PLOT, strSearch.c_str(), VIDEODB_ID_PLOTOUTLINE, strSearch.c_str(), VIDEODB_ID_TAGLINE,strSearch.c_str());
    else
      strSQL = PrepareSQL("SELECT movie.idMovie, movie.c%02d FROM movie WHERE (movie.c%02d LIKE '%%%s%%' OR movie.c%02d LIKE '%%%s%%' OR movie.c%02d LIKE '%%%s%%')", VIDEODB_ID_TITLE, VIDEODB_ID_PLOT, strSearch.c_str(), VIDEODB_ID_PLOTOUTLINE, strSearch.c_str(), VIDEODB_ID_TAGLINE, strSearch.c_str());

    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv(2).get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));
      std::string path = StringUtils::Format("videodb://movies/titles/%i", m_pDS->fv(0).get_asInt());
      pItem->SetPath(path);
      pItem->m_bIsFolder=false;

      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}

void CVideoDatabase::GetMovieDirectorsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT DISTINCT director_link.actor_id, actor.name, path.strPath FROM movie INNER JOIN director_link ON (director_link.media_id=movie.idMovie AND director_link.media_type='movie') INNER JOIN actor ON actor.actor_id=director_link.actor_id INNER JOIN files ON files.idFile=movie.idFile INNER JOIN path ON path.idPath=files.idPath WHERE actor.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL = PrepareSQL("SELECT DISTINCT director_link.actor_id, actor.name FROM actor INNER JOIN director_link ON director_link.actor_id=actor.actor_id INNER JOIN movie ON director_link.media_id=movie.idMovie WHERE director_link.media_type='movie' AND actor.name like '%%%s%%'", strSearch.c_str());

    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      std::string strDir = StringUtils::Format("%i/", m_pDS->fv(0).get_asInt());
      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()));

      pItem->SetPath("videodb://movies/directors/"+ strDir);
      pItem->m_bIsFolder=true;
      items.Add(pItem);
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strSQL.c_str());
  }
}
