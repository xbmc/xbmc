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
#include "settings/SettingsComponent.h"
#include "threads/SystemClock.h"
#include "utils/log.h"

using namespace dbiplus;

extern DWORD castTime;
extern DWORD movieTime;

int CVideoDatabase::GetEpisodeId(const std::string& strFilenameAndPath, int idEpisode, int idSeason) // input value is episode/season number hint - for multiparters
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    // need this due to the nested GetEpisodeInfo query
    std::unique_ptr<Dataset> pDS;
    pDS.reset(m_pDB->CreateDataset());
    if (NULL == pDS.get()) return -1;

    int idFile = GetFileId(strFilenameAndPath);
    if (idFile < 0)
      return -1;

    std::string strSQL=PrepareSQL("select idEpisode from episode where idFile=%i", idFile);

    CLog::Log(LOGDEBUG, LOGDATABASE, "%s (%s), query = %s", __FUNCTION__, CURL::GetRedacted(strFilenameAndPath).c_str(), strSQL.c_str());
    pDS->query(strSQL);
    if (pDS->num_rows() > 0)
    {
      if (idEpisode == -1)
        idEpisode = pDS->fv("episode.idEpisode").get_asInt();
      else // use the hint!
      {
        while (!pDS->eof())
        {
          CVideoInfoTag tag;
          int idTmpEpisode = pDS->fv("episode.idEpisode").get_asInt();
          GetEpisodeBasicInfo(strFilenameAndPath, tag, idTmpEpisode);
          if (tag.m_iEpisode == idEpisode && (idSeason == -1 || tag.m_iSeason == idSeason)) {
            // match on the episode hint, and there's no season hint or a season hint match
            idEpisode = idTmpEpisode;
            break;
          }
          pDS->next();
        }
        if (pDS->eof())
          idEpisode = -1;
      }
    }
    else
      idEpisode = -1;

    pDS->close();

    return idEpisode;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

int CVideoDatabase::AddEpisode(int idShow, const std::string& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;

    int idFile = AddFile(strFilenameAndPath);
    if (idFile < 0)
      return -1;
    UpdateFileDateAdded(idFile, strFilenameAndPath);

    std::string strSQL=PrepareSQL("insert into episode (idEpisode, idFile, idShow) values (NULL, %i, %i)", idFile, idShow);
    m_pDS->exec(strSQL);
    return (int)m_pDS->lastinsertid();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return -1;
}

bool CVideoDatabase::HasEpisodeInfo(const std::string& strFilenameAndPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    int idEpisode = GetEpisodeId(strFilenameAndPath);
    return (idEpisode > 0); // index of zero is also invalid
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

void CVideoDatabase::GetEpisodesByActor(const std::string& name, CFileItemList& items)
{
  Filter filter;
  filter.join  = "LEFT JOIN actor_link ON actor_link.media_id=episode_view.idEpisode AND actor_link.media_type='episode' "
                 "LEFT JOIN actor a ON a.actor_id=actor_link.actor_id "
                 "LEFT JOIN director_link ON director_link.media_id=episode_view.idEpisode AND director_link.media_type='episode' "
                 "LEFT JOIN actor d ON d.actor_id=director_link.actor_id";
  filter.where = PrepareSQL("a.name='%s' OR d.name='%s'", name.c_str(), name.c_str());
  filter.group = "episode_view.idEpisode";
  GetEpisodesByWhere("videodb://tvshows/titles/", filter, items);
}

bool CVideoDatabase::GetEpisodeBasicInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idEpisode /* = -1 */)
{
  try
  {
    if (idEpisode < 0)
      idEpisode = GetEpisodeId(strFilenameAndPath);

    if (idEpisode < 0)
      return false;

    std::string sql = PrepareSQL("select * from episode where idEpisode=%i",idEpisode);
    if (!m_pDS->query(sql))
      return false;
    details = GetBasicDetailsForEpisode(m_pDS);
    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

bool CVideoDatabase::GetEpisodeInfo(const std::string& strFilenameAndPath, CVideoInfoTag& details, int idEpisode /* = -1 */, int getDetails /* = VideoDbDetailsAll */)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    if (idEpisode < 0)
      idEpisode = GetEpisodeId(strFilenameAndPath, details.m_iEpisode, details.m_iSeason);
    if (idEpisode < 0) return false;

    std::string sql = PrepareSQL("select * from episode_view where idEpisode=%i",idEpisode);
    if (!m_pDS->query(sql))
      return false;
    details = GetDetailsForEpisode(m_pDS, getDetails);
    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  return false;
}

int CVideoDatabase::SetDetailsForEpisode(const std::string& strFilenameAndPath, CVideoInfoTag& details,
    const std::map<std::string, std::string> &artwork, int idShow, int idEpisode)
{
  try
  {
    BeginTransaction();
    if (idEpisode < 0)
      idEpisode = GetEpisodeId(strFilenameAndPath);

    if (idEpisode > 0)
      DeleteEpisode(idEpisode, true); // true to keep the table entry
    else
    {
      // only add a new episode if we don't already have a valid idEpisode
      // (DeleteEpisode is called with bKeepId == true so the episode won't
      // be removed from the episode table)
      idEpisode = AddEpisode(idShow,strFilenameAndPath);
      if (idEpisode < 0)
      {
        RollbackTransaction();
        return -1;
      }
    }

    // update dateadded if it's set
    if (details.m_dateAdded.IsValid())
    {
      if (details.m_iFileId <= 0)
        details.m_iFileId = GetFileId(strFilenameAndPath);

      UpdateFileDateAdded(details.m_iFileId, strFilenameAndPath, details.m_dateAdded);
    }

    AddCast(idEpisode, "episode", details.m_cast);
    AddActorLinksToItem(idEpisode, MediaTypeEpisode, "director", details.m_director);
    AddActorLinksToItem(idEpisode, MediaTypeEpisode, "writer", details.m_writingCredits);

    // add ratings
    details.m_iIdRating = AddRatings(idEpisode, MediaTypeEpisode, details.m_ratings, details.GetDefaultRating());

    // add unique ids
    details.m_iIdUniqueID = UpdateUniqueIDs(idEpisode, MediaTypeEpisode, details);

    if (details.HasStreamDetails())
    {
      if (details.m_iFileId != -1)
        SetStreamDetailsForFileId(details.m_streamDetails, details.m_iFileId);
      else
        SetStreamDetailsForFile(details.m_streamDetails, strFilenameAndPath);
    }

    // ensure we have this season already added
    int idSeason = AddSeason(idShow, details.m_iSeason);

    SetArtForItem(idEpisode, MediaTypeEpisode, artwork);

    if (details.m_iEpisode != -1 && details.m_iSeason != -1)
    { // query DB for any episodes matching idShow, Season and Episode
      std::string strSQL = PrepareSQL("SELECT files.playCount, files.lastPlayed "
                                      "FROM episode INNER JOIN files ON files.idFile=episode.idFile "
                                      "WHERE episode.c%02d=%i AND episode.c%02d=%i AND episode.idShow=%i "
                                      "AND episode.idEpisode!=%i AND files.playCount > 0",
                                      VIDEODB_ID_EPISODE_SEASON, details.m_iSeason, VIDEODB_ID_EPISODE_EPISODE,
                                      details.m_iEpisode, idShow, idEpisode);
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
    // and insert the new row
    std::string sql = "UPDATE episode SET " + GetValueString(details, VIDEODB_ID_EPISODE_MIN, VIDEODB_ID_EPISODE_MAX, DbEpisodeOffsets);
    if (details.m_iUserRating > 0 && details.m_iUserRating < 11)
      sql += PrepareSQL(", userrating = %i", details.m_iUserRating);
    else
      sql += ", userrating = NULL";
    sql += PrepareSQL(", idSeason = %i", idSeason);
    sql += PrepareSQL(" where idEpisode=%i", idEpisode);
    m_pDS->exec(sql);
    CommitTransaction();

    return idEpisode;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
  RollbackTransaction();
  return -1;
}

void CVideoDatabase::GetEpisodesByFile(const std::string& strFilenameAndPath, std::vector<CVideoInfoTag>& episodes)
{
  try
  {
    std::string strSQL = PrepareSQL("select * from episode_view where idFile=%i order by c%02d, c%02d asc", GetFileId(strFilenameAndPath), VIDEODB_ID_EPISODE_SORTSEASON, VIDEODB_ID_EPISODE_SORTEPISODE);
    m_pDS->query(strSQL);
    while (!m_pDS->eof())
    {
      episodes.emplace_back(GetDetailsForEpisode(m_pDS));
      m_pDS->next();
    }
    m_pDS->close();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
  }
}

void CVideoDatabase::DeleteEpisode(const std::string& strFilenameAndPath, bool bKeepId /* = false */)
{
  int idEpisode = GetEpisodeId(strFilenameAndPath);
  if (idEpisode > -1)
    DeleteEpisode(idEpisode, bKeepId);
}

void CVideoDatabase::DeleteEpisode(int idEpisode, bool bKeepId /* = false */)
{
  if (idEpisode < 0)
    return;

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    //! @todo move this below CommitTransaction() once UPnP doesn't rely on this anymore
    if (!bKeepId)
      AnnounceRemove(MediaTypeEpisode, idEpisode);

    // keep episode table entry and bookmarks so we can update the data in place
    // the ancillary tables are still purged
    if (!bKeepId)
    {
      int idFile = GetDbId(PrepareSQL("SELECT idFile FROM episode WHERE idEpisode=%i", idEpisode));
      std::string path = GetSingleValue(PrepareSQL("SELECT strPath FROM path JOIN files ON files.idPath=path.idPath WHERE files.idFile=%i", idFile));
      if (!path.empty())
        InvalidatePathHash(path);

      std::string strSQL = PrepareSQL("delete from episode where idEpisode=%i", idEpisode);
      m_pDS->exec(strSQL);
    }

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%d) failed", __FUNCTION__, idEpisode);
  }
}

CVideoInfoTag CVideoDatabase::GetBasicDetailsForEpisode(std::unique_ptr<Dataset> &pDS)
{
  return GetBasicDetailsForEpisode(pDS->get_sql_record());
}

CVideoInfoTag CVideoDatabase::GetBasicDetailsForEpisode(const dbiplus::sql_record* const record)
{
  CVideoInfoTag details;

  if (record == nullptr)
    return details;

  unsigned int time = XbmcThreads::SystemClockMillis();
  int idEpisode = record->at(0).get_asInt();

  GetDetailsFromDB(record, VIDEODB_ID_EPISODE_MIN, VIDEODB_ID_EPISODE_MAX, DbEpisodeOffsets, details);
  details.m_iDbId = idEpisode;
  details.m_type = MediaTypeEpisode;
  details.m_iFileId = record->at(VIDEODB_DETAILS_FILEID).get_asInt();
  details.m_iIdShow = record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_ID).get_asInt();
  details.m_iIdSeason = record->at(VIDEODB_DETAILS_EPISODE_SEASON_ID).get_asInt();
  details.m_iUserRating = record->at(VIDEODB_DETAILS_EPISODE_USER_RATING).get_asInt();

  movieTime += XbmcThreads::SystemClockMillis() - time;
  return details;
}

CVideoInfoTag CVideoDatabase::GetDetailsForEpisode(std::unique_ptr<Dataset> &pDS, int getDetails /* = VideoDbDetailsNone */)
{
  return GetDetailsForEpisode(pDS->get_sql_record(), getDetails);
}

CVideoInfoTag CVideoDatabase::GetDetailsForEpisode(const dbiplus::sql_record* const record, int getDetails /* = VideoDbDetailsNone */)
{
  CVideoInfoTag details;

  if (record == nullptr)
    return details;

  details = GetBasicDetailsForEpisode(record);

  unsigned int time = XbmcThreads::SystemClockMillis();

  details.m_strPath = record->at(VIDEODB_DETAILS_EPISODE_PATH).get_asString();
  std::string strFileName = record->at(VIDEODB_DETAILS_EPISODE_FILE).get_asString();
  ConstructPath(details.m_strFileNameAndPath,details.m_strPath,strFileName);
  details.SetPlayCount(record->at(VIDEODB_DETAILS_EPISODE_PLAYCOUNT).get_asInt());
  details.m_lastPlayed.SetFromDBDateTime(record->at(VIDEODB_DETAILS_EPISODE_LASTPLAYED).get_asString());
  details.m_dateAdded.SetFromDBDateTime(record->at(VIDEODB_DETAILS_EPISODE_DATEADDED).get_asString());
  details.m_strMPAARating = record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_MPAA).get_asString();
  details.m_strShowTitle = record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_NAME).get_asString();
  details.m_genre = StringUtils::Split(record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_GENRE).get_asString(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
  details.m_studio = StringUtils::Split(record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_STUDIO).get_asString(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
  details.SetPremieredFromDBDate(record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_AIRED).get_asString());

  details.SetResumePoint(record->at(VIDEODB_DETAILS_EPISODE_RESUME_TIME).get_asInt(),
                         record->at(VIDEODB_DETAILS_EPISODE_TOTAL_TIME).get_asInt(),
                         record->at(VIDEODB_DETAILS_EPISODE_PLAYER_STATE).get_asString());

  details.SetRating(record->at(VIDEODB_DETAILS_EPISODE_RATING).get_asFloat(),
                    record->at(VIDEODB_DETAILS_EPISODE_VOTES).get_asInt(),
                    record->at(VIDEODB_DETAILS_EPISODE_RATING_TYPE).get_asString(), true);
  details.SetUniqueID(record->at(VIDEODB_DETAILS_EPISODE_UNIQUEID_VALUE).get_asString(), record->at(VIDEODB_DETAILS_EPISODE_UNIQUEID_TYPE).get_asString(), true);
  movieTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();

  if (getDetails)
  {
    if (getDetails & VideoDbDetailsCast)
    {
      GetCast(details.m_iDbId, MediaTypeEpisode, details.m_cast);
      GetCast(details.m_iIdShow, MediaTypeTvShow, details.m_cast);
      castTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();
    }

    if (getDetails & VideoDbDetailsRating)
      GetRatings(details.m_iDbId, MediaTypeEpisode, details.m_ratings);

    if (getDetails & VideoDbDetailsUniqueID)
      GetUniqueIDs(details.m_iDbId, MediaTypeEpisode, details);

    details.m_strPictureURL.Parse();

    if (getDetails &  VideoDbDetailsBookmark)
      GetBookMarkForEpisode(details, details.m_EpBookmark);

    if (getDetails & VideoDbDetailsStream)
      GetStreamDetails(details);

    details.m_parsedDetails = getDetails;
  }
  return details;
}

bool CVideoDatabase::GetEpisodesNav(const std::string& strBaseDir, CFileItemList& items, int idGenre, int idYear, int idActor, int idDirector, int idShow, int idSeason, const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(strBaseDir))
    return false;

  std::string strIn;
  if (idShow != -1)
  {
    videoUrl.AddOption("tvshowid", idShow);
    if (idSeason >= 0)
      videoUrl.AddOption("season", idSeason);

    if (idGenre != -1)
      videoUrl.AddOption("genreid", idGenre);
    else if (idYear !=-1)
      videoUrl.AddOption("year", idYear);
    else if (idActor != -1)
      videoUrl.AddOption("actorid", idActor);
  }
  else if (idYear != -1)
    videoUrl.AddOption("year", idYear);

  if (idDirector != -1)
    videoUrl.AddOption("directorid", idDirector);

  Filter filter;
  bool ret = GetEpisodesByWhere(videoUrl.ToString(), filter, items, false, sortDescription, getDetails);

  if (idSeason == -1 && idShow != -1)
  { // add any linked movies
    Filter movieFilter;
    movieFilter.join  = PrepareSQL("join movielinktvshow on movielinktvshow.idMovie=movie_view.idMovie");
    movieFilter.where = PrepareSQL("movielinktvshow.idShow = %i", idShow);
    CFileItemList movieItems;
    GetMoviesByWhere("videodb://movies/titles/", movieFilter, movieItems);

    if (movieItems.Size() > 0)
      items.Append(movieItems);
  }

  return ret;
}

bool CVideoDatabase::GetEpisodesByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, bool appendFullShowPath /* = true */, const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
{
  try
  {
    movieTime = 0;
    castTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int total = -1;

    std::string strSQL = "select %s from episode_view ";
    CVideoDbUrl videoUrl;
    std::string strSQLExtra;
    Filter extFilter = filter;
    SortDescription sorting = sortDescription;
    if (!BuildSQL(strBaseDir, strSQLExtra, extFilter, strSQLExtra, videoUrl, sorting))
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
    if (!SortUtils::SortFromDataset(sorting, MediaTypeEpisode, m_pDS, results))
      return false;

    // get data from returned rows
    items.Reserve(results.size());
    CLabelFormatter formatter("%H. %T", "");

    const query_data &data = m_pDS->get_result_set().records;
    for (const auto &i : results)
    {
      unsigned int targetRow = (unsigned int)i.at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);

      CVideoInfoTag episode = GetDetailsForEpisode(record, getDetails);
      if (m_profileManager.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE ||
          g_passwordManager.bMasterUser                                     ||
          g_passwordManager.IsDatabasePathUnlocked(episode.m_strPath, *CMediaSourceSettings::GetInstance().GetSources("video")))
      {
        CFileItemPtr pItem(new CFileItem(episode));
        formatter.FormatLabel(pItem.get());

        int idEpisode = record->at(0).get_asInt();

        CVideoDbUrl itemUrl = videoUrl;
        std::string path;
        if (appendFullShowPath && videoUrl.GetItemType() != "episodes")
          path = StringUtils::Format("%i/%i/%i", record->at(VIDEODB_DETAILS_EPISODE_TVSHOW_ID).get_asInt(), episode.m_iSeason, idEpisode);
        else
          path = StringUtils::Format("%i", idEpisode);
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());
        pItem->SetDynPath(episode.m_strFileNameAndPath);

        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, episode.GetPlayCount() > 0);
        pItem->m_dateTime = episode.m_firstAired;
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

bool CVideoDatabase::GetRecentlyAddedEpisodesNav(const std::string& strBaseDir, CFileItemList& items, unsigned int limit /* = 0 */, int getDetails /* = VideoDbDetailsNone */)
{
  Filter filter;
  filter.order = "dateAdded desc, idEpisode desc";
  filter.limit = PrepareSQL("%u", limit ? limit : CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iVideoLibraryRecentlyAddedItems);
  return GetEpisodesByWhere(strBaseDir, filter, items, false, SortDescription(), getDetails);
}

void CVideoDatabase::GetEpisodesByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT episode.idEpisode, episode.c%02d, episode.c%02d, episode.idShow, tvshow.c%02d, path.strPath FROM episode INNER JOIN tvshow ON tvshow.idShow=episode.idShow INNER JOIN files ON files.idFile=episode.idFile INNER JOIN path ON path.idPath=files.idPath WHERE episode.c%02d LIKE '%%%s%%'", VIDEODB_ID_EPISODE_TITLE, VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_TV_TITLE, VIDEODB_ID_EPISODE_TITLE, strSearch.c_str());
    else
      strSQL = PrepareSQL("SELECT episode.idEpisode, episode.c%02d, episode.c%02d, episode.idShow, tvshow.c%02d FROM episode INNER JOIN tvshow ON tvshow.idShow=episode.idShow WHERE episode.c%02d like '%%%s%%'", VIDEODB_ID_EPISODE_TITLE, VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_TV_TITLE, VIDEODB_ID_EPISODE_TITLE, strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()+" ("+m_pDS->fv(4).get_asString()+")"));
      std::string path = StringUtils::Format("videodb://tvshows/titles/%i/%i/%i",m_pDS->fv("episode.idShow").get_asInt(),m_pDS->fv(2).get_asInt(),m_pDS->fv(0).get_asInt());
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

void CVideoDatabase::GetEpisodesByPlot(const std::string& strSearch, CFileItemList& items)
{
// Alternative searching - not quite as fast though due to
// retrieving all information
//  Filter filter;
//  filter.where = PrepareSQL("c%02d like '%s%%' or c%02d like '%% %s%%'", VIDEODB_ID_EPISODE_PLOT, strSearch.c_str(), VIDEODB_ID_EPISODE_PLOT, strSearch.c_str());
//  filter.where += PrepareSQL("or c%02d like '%s%%' or c%02d like '%% %s%%'", VIDEODB_ID_EPISODE_TITLE, strSearch.c_str(), VIDEODB_ID_EPISODE_TITLE, strSearch.c_str());
//  GetEpisodesByWhere("videodb://tvshows/titles/", filter, items);
//  return;
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT episode.idEpisode, episode.c%02d, episode.c%02d, episode.idShow, tvshow.c%02d, path.strPath FROM episode INNER JOIN tvshow ON tvshow.idShow=episode.idShow INNER JOIN files ON files.idFile=episode.idFile INNER JOIN path ON path.idPath=files.idPath WHERE episode.c%02d LIKE '%%%s%%'", VIDEODB_ID_EPISODE_TITLE, VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_TV_TITLE, VIDEODB_ID_EPISODE_PLOT, strSearch.c_str());
    else
      strSQL = PrepareSQL("SELECT episode.idEpisode, episode.c%02d, episode.c%02d, episode.idShow, tvshow.c%02d FROM episode INNER JOIN tvshow ON tvshow.idShow=episode.idShow WHERE episode.c%02d LIKE '%%%s%%'", VIDEODB_ID_EPISODE_TITLE, VIDEODB_ID_EPISODE_SEASON, VIDEODB_ID_TV_TITLE, VIDEODB_ID_EPISODE_PLOT, strSearch.c_str());
    m_pDS->query( strSQL );

    while (!m_pDS->eof())
    {
      if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
        if (!g_passwordManager.IsDatabasePathUnlocked(m_pDS->fv("path.strPath").get_asString(),*CMediaSourceSettings::GetInstance().GetSources("video")))
        {
          m_pDS->next();
          continue;
        }

      CFileItemPtr pItem(new CFileItem(m_pDS->fv(1).get_asString()+" ("+m_pDS->fv(4).get_asString()+")"));
      std::string path = StringUtils::Format("videodb://tvshows/titles/%i/%i/%i",m_pDS->fv("episode.idShow").get_asInt(),m_pDS->fv(2).get_asInt(),m_pDS->fv(0).get_asInt());
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
