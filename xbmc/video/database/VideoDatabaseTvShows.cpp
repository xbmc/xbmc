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
#include "filesystem/File.h"
#include "GUIPassword.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

using namespace dbiplus;
using namespace XFILE;
using namespace VIDEO;

extern DWORD movieTime;
extern DWORD castTime;

bool CVideoDatabase::GetPathsLinkedToTvShow(int idShow, std::vector<std::string> &paths)
{
  std::string sql;
  try
  {
    sql = PrepareSQL("SELECT strPath FROM path JOIN tvshowlinkpath ON tvshowlinkpath.idPath=path.idPath WHERE idShow=%i", idShow);
    m_pDS->query(sql);
    while (!m_pDS->eof())
    {
      paths.emplace_back(m_pDS->fv(0).get_asString());
      m_pDS->next();
    }
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s error during query: %s",__FUNCTION__, sql.c_str());
  }
  return false;
}

bool CVideoDatabase::GetPathsForTvShow(int idShow, std::set<int>& paths)
{
  std::string strSQL;
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    // add base path
    strSQL = PrepareSQL("SELECT strPath FROM tvshow_view WHERE idShow=%i", idShow);
    if (m_pDS->query(strSQL))
      paths.insert(GetPathId(m_pDS->fv(0).get_asString()));

    // add all other known paths
    strSQL = PrepareSQL("SELECT DISTINCT idPath FROM files JOIN episode ON episode.idFile=files.idFile WHERE episode.idShow=%i",idShow);
    m_pDS->query(strSQL);
    while (!m_pDS->eof())
    {
      paths.insert(m_pDS->fv(0).get_asInt());
      m_pDS->next();
    }
    m_pDS->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s error during query: %s",__FUNCTION__, strSQL.c_str());
  }
  return false;
}

bool CVideoDatabase::AddPathToTvShow(int idShow, const std::string &path, const std::string &parentPath, const CDateTime& dateAdded /* = CDateTime() */)
{
  // Check if this path is already added
  int idPath = GetPathId(path);
  if (idPath < 0)
  {
    CDateTime finalDateAdded = dateAdded;
    // Skip looking at the files ctime/mtime if defined by the user through as.xml
    if (!finalDateAdded.IsValid() && CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_iVideoLibraryDateAdded > 0)
    {
      struct __stat64 buffer;
      if (CFile::Stat(path, &buffer) == 0)
      {
        time_t now = time(NULL);
        // Make sure we have a valid date (i.e. not in the future)
        if ((time_t)buffer.st_ctime <= now)
        {
          struct tm *time;
#ifdef HAVE_LOCALTIME_R
          struct tm result = {};
          time = localtime_r((const time_t*)&buffer.st_ctime, &result);
#else
          time = localtime((const time_t*)&buffer.st_ctime);
#endif
          if (time)
            finalDateAdded = *time;
        }
      }
    }

    if (!finalDateAdded.IsValid())
      finalDateAdded = CDateTime::GetCurrentDateTime();

    idPath = AddPath(path, parentPath, finalDateAdded);
  }

  return ExecuteQuery(PrepareSQL("REPLACE INTO tvshowlinkpath(idShow, idPath) VALUES (%i,%i)", idShow, idPath));
}

int CVideoDatabase::AddTvShow()
{
  if (ExecuteQuery("INSERT INTO tvshow(idShow) VALUES(NULL)"))
    return (int)m_pDS->lastinsertid();
  return -1;
}

bool CVideoDatabase::HasTvShowInfo(const std::string& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;
    int idTvShow = GetTvShowId(strPath);
    return (idTvShow > 0); // index of zero is also invalid
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
  return false;
}

void CVideoDatabase::DeleteDetailsForTvShow(const std::string& strPath)
{
  int idTvShow = GetTvShowId(strPath);
  if (idTvShow >= 0)
    DeleteDetailsForTvShow(idTvShow);
}

void CVideoDatabase::DeleteDetailsForTvShow(int idTvShow)
{
  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    std::string strSQL;
    strSQL=PrepareSQL("DELETE from genre_link WHERE media_id=%i AND media_type='tvshow'", idTvShow);
    m_pDS->exec(strSQL);

    strSQL=PrepareSQL("DELETE FROM actor_link WHERE media_id=%i AND media_type='tvshow'", idTvShow);
    m_pDS->exec(strSQL);

    strSQL=PrepareSQL("DELETE FROM director_link WHERE media_id=%i AND media_type='tvshow'", idTvShow);
    m_pDS->exec(strSQL);

    strSQL=PrepareSQL("DELETE FROM studio_link WHERE media_id=%i AND media_type='tvshow'", idTvShow);
    m_pDS->exec(strSQL);

    strSQL = PrepareSQL("DELETE FROM rating WHERE media_id=%i AND media_type='tvshow'", idTvShow);
    m_pDS->exec(strSQL);

    strSQL = PrepareSQL("DELETE FROM uniqueid WHERE media_id=%i AND media_type='tvshow'", idTvShow);
    m_pDS->exec(strSQL);

    // remove all info other than the id
    // we do this due to the way we have the link between the file + movie tables.

    std::vector<std::string> ids;
    for (int iType = VIDEODB_ID_TV_MIN + 1; iType < VIDEODB_ID_TV_MAX; iType++)
      ids.emplace_back(StringUtils::Format("c%02d=NULL", iType));

    strSQL = "update tvshow set ";
    strSQL += StringUtils::Join(ids, ", ");
    strSQL += PrepareSQL(" where idShow=%i", idTvShow);
    m_pDS->exec(strSQL);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idTvShow);
  }
}

void CVideoDatabase::GetTvShowsByActor(const std::string& name, CFileItemList& items)
{
  Filter filter;
  filter.join  = "LEFT JOIN actor_link ON actor_link.media_id=tvshow_view.idShow AND actor_link.media_type='tvshow' "
                 "LEFT JOIN actor a ON a.actor_id=actor_link.actor_id "
                 "LEFT JOIN director_link ON director_link.media_id=tvshow_view.idShow AND director_link.media_type='tvshow' "
                 "LEFT JOIN actor d ON d.actor_id=director_link.actor_id";
  filter.where = PrepareSQL("a.name='%s' OR d.name='%s'", name.c_str(), name.c_str());
  GetTvShowsByWhere("videodb://tvshows/titles/", filter, items);
}

//********************************************************************************************************************************
bool CVideoDatabase::GetTvShowInfo(const std::string& strPath, CVideoInfoTag& details, int idTvShow /* = -1 */, CFileItem *item /* = NULL */, int getDetails /* = VideoDbDetailsAll */)
{
  try
  {
    if (m_pDB == nullptr || m_pDS == nullptr)
      return false;

    if (idTvShow < 0)
      idTvShow = GetTvShowId(strPath);
    if (idTvShow < 0) return false;

    std::string sql = PrepareSQL("SELECT * FROM tvshow_view WHERE idShow=%i GROUP BY idShow", idTvShow);
    if (!m_pDS->query(sql))
      return false;
    details = GetDetailsForTvShow(m_pDS, getDetails, item);
    return !details.IsEmpty();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
  return false;
}

int CVideoDatabase::GetMatchingTvShow(const CVideoInfoTag &details)
{
  // first try matching on uniqueid, then on title + year
  int id = -1;
  if (!details.HasUniqueID())
    id = GetDbId(PrepareSQL("SELECT idShow FROM tvshow JOIN uniqueid ON uniqueid.media_id=tvshow.idShow AND uniqueid.media_type='tvshow' WHERE uniqueid.value='%s'", details.GetUniqueID().c_str()));
  if (id < 0)
    id = GetDbId(PrepareSQL("SELECT idShow FROM tvshow WHERE c%02d='%s' AND c%02d='%s'", VIDEODB_ID_TV_TITLE, details.m_strTitle.c_str(), VIDEODB_ID_TV_PREMIERED, details.GetPremiered().GetAsDBDate().c_str()));
  return id;
}

int CVideoDatabase::SetDetailsForTvShow(const std::vector<std::pair<std::string, std::string> > &paths,
    CVideoInfoTag& details, const std::map<std::string, std::string> &artwork,
    const std::map<int, std::map<std::string, std::string> > &seasonArt, int idTvShow /*= -1 */)
{

  /*
   The steps are as follows.
   1. Check if the tvshow is found on any of the given paths.  If found, we have the show id.
   2. Search for a matching show.  If found, we have the show id.
   3. If we don't have the id, add a new show.
   4. Add the paths to the show.
   5. Add details for the show.
   */

  if (idTvShow < 0)
  {
    for (const auto &i : paths)
    {
      idTvShow = GetTvShowId(i.first);
      if (idTvShow > -1)
        break;
    }
  }
  if (idTvShow < 0)
    idTvShow = GetMatchingTvShow(details);
  if (idTvShow < 0)
  {
    idTvShow = AddTvShow();
    if (idTvShow < 0)
      return -1;
  }

  // add any paths to the tvshow
  for (const auto &i : paths)
    AddPathToTvShow(idTvShow, i.first, i.second, details.m_dateAdded);

  UpdateDetailsForTvShow(idTvShow, details, artwork, seasonArt);

  return idTvShow;
}

bool CVideoDatabase::UpdateDetailsForTvShow(int idTvShow, CVideoInfoTag &details,
    const std::map<std::string, std::string> &artwork, const std::map<int, std::map<std::string, std::string>> &seasonArt)
{
  BeginTransaction();

  DeleteDetailsForTvShow(idTvShow);

  AddCast(idTvShow, "tvshow", details.m_cast);
  AddLinksToItem(idTvShow, MediaTypeTvShow, "genre", details.m_genre);
  AddLinksToItem(idTvShow, MediaTypeTvShow, "studio", details.m_studio);
  AddLinksToItem(idTvShow, MediaTypeTvShow, "tag", details.m_tags);
  AddActorLinksToItem(idTvShow, MediaTypeTvShow, "director", details.m_director);

  // add ratings
  details.m_iIdRating = AddRatings(idTvShow, MediaTypeTvShow, details.m_ratings, details.GetDefaultRating());

  // add unique ids
  details.m_iIdUniqueID = UpdateUniqueIDs(idTvShow, MediaTypeTvShow, details);

  // add "all seasons" - the rest are added in SetDetailsForEpisode
  AddSeason(idTvShow, -1);

  // add any named seasons
  for (const auto& namedSeason : details.m_namedSeasons)
  {
    // make sure the named season exists
    int seasonId = AddSeason(idTvShow, namedSeason.first, namedSeason.second);

    // get any existing details for the named season
    CVideoInfoTag season;
    if (!GetSeasonInfo(seasonId, season, false) || season.m_strSortTitle == namedSeason.second)
      continue;

    season.SetSortTitle(namedSeason.second);
    SetDetailsForSeason(season, std::map<std::string, std::string>(), idTvShow, seasonId);
  }

  SetArtForItem(idTvShow, MediaTypeTvShow, artwork);
  for (const auto &i : seasonArt)
  {
    int idSeason = AddSeason(idTvShow, i.first);
    if (idSeason > -1)
      SetArtForItem(idSeason, MediaTypeSeason, i.second);
  }

  // and insert the new row
  std::string sql = "UPDATE tvshow SET " + GetValueString(details, VIDEODB_ID_TV_MIN, VIDEODB_ID_TV_MAX, DbTvShowOffsets);
  if (details.m_iUserRating > 0 && details.m_iUserRating < 11)
    sql += PrepareSQL(", userrating = %i", details.m_iUserRating);
  else
    sql += ", userrating = NULL";
  if (details.GetDuration() > 0)
    sql += PrepareSQL(", duration = %i", details.GetDuration());
  else
    sql += ", duration = NULL";
  sql += PrepareSQL(" WHERE idShow=%i", idTvShow);
  if (ExecuteQuery(sql))
  {
    CommitTransaction();
    return true;
  }
  RollbackTransaction();
  return false;
}

void CVideoDatabase::DeleteTvShow(const std::string& strPath)
{
  int idTvShow = GetTvShowId(strPath);
  if (idTvShow >= 0)
    DeleteTvShow(idTvShow);
}

void CVideoDatabase::DeleteTvShow(int idTvShow, bool bKeepId /* = false */)
{
  if (idTvShow < 0)
    return;

  try
  {
    if (NULL == m_pDB.get()) return ;
    if (NULL == m_pDS.get()) return ;

    BeginTransaction();

    std::set<int> paths;
    GetPathsForTvShow(idTvShow, paths);

    std::string strSQL=PrepareSQL("SELECT episode.idEpisode FROM episode WHERE episode.idShow=%i",idTvShow);
    m_pDS2->query(strSQL);
    while (!m_pDS2->eof())
    {
      DeleteEpisode(m_pDS2->fv(0).get_asInt(), bKeepId);
      m_pDS2->next();
    }

    DeleteDetailsForTvShow(idTvShow);

    strSQL=PrepareSQL("delete from seasons where idShow=%i", idTvShow);
    m_pDS->exec(strSQL);

    // keep tvshow table and movielink table so we can update data in place
    if (!bKeepId)
    {
      strSQL=PrepareSQL("delete from tvshow where idShow=%i", idTvShow);
      m_pDS->exec(strSQL);

      for (const auto &i : paths)
      {
        std::string path = GetSingleValue(PrepareSQL("SELECT strPath FROM path WHERE idPath=%i", i));
        if (!path.empty())
          InvalidatePathHash(path);
      }
    }

    //! @todo move this below CommitTransaction() once UPnP doesn't rely on this anymore
    if (!bKeepId)
      AnnounceRemove(MediaTypeTvShow, idTvShow);

    CommitTransaction();

  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%d) failed", __FUNCTION__, idTvShow);
    RollbackTransaction();
  }
}

CVideoInfoTag CVideoDatabase::GetDetailsForTvShow(std::unique_ptr<Dataset> &pDS, int getDetails /* = VideoDbDetailsNone */, CFileItem* item /* = NULL */)
{
  return GetDetailsForTvShow(pDS->get_sql_record(), getDetails, item);
}

CVideoInfoTag CVideoDatabase::GetDetailsForTvShow(const dbiplus::sql_record* const record, int getDetails /* = VideoDbDetailsNone */, CFileItem* item /* = NULL */)
{
  CVideoInfoTag details;

  if (record == NULL)
    return details;

  DWORD time = XbmcThreads::SystemClockMillis();
  int idTvShow = record->at(0).get_asInt();

  GetDetailsFromDB(record, VIDEODB_ID_TV_MIN, VIDEODB_ID_TV_MAX, DbTvShowOffsets, details, 1);
  details.m_bHasPremiered = details.m_premiered.IsValid();
  details.m_iDbId = idTvShow;
  details.m_type = MediaTypeTvShow;
  details.m_strPath = record->at(VIDEODB_DETAILS_TVSHOW_PATH).get_asString();
  details.m_basePath = details.m_strPath;
  details.m_parentPathID = record->at(VIDEODB_DETAILS_TVSHOW_PARENTPATHID).get_asInt();
  details.m_dateAdded.SetFromDBDateTime(record->at(VIDEODB_DETAILS_TVSHOW_DATEADDED).get_asString());
  details.m_lastPlayed.SetFromDBDateTime(record->at(VIDEODB_DETAILS_TVSHOW_LASTPLAYED).get_asString());
  details.m_iSeason = record->at(VIDEODB_DETAILS_TVSHOW_NUM_SEASONS).get_asInt();
  details.m_iEpisode = record->at(VIDEODB_DETAILS_TVSHOW_NUM_EPISODES).get_asInt();
  details.SetPlayCount(record->at(VIDEODB_DETAILS_TVSHOW_NUM_WATCHED).get_asInt());
  details.m_strShowTitle = details.m_strTitle;
  details.m_iUserRating = record->at(VIDEODB_DETAILS_TVSHOW_USER_RATING).get_asInt();
  details.SetRating(record->at(VIDEODB_DETAILS_TVSHOW_RATING).get_asFloat(),
                    record->at(VIDEODB_DETAILS_TVSHOW_VOTES).get_asInt(),
                    record->at(VIDEODB_DETAILS_TVSHOW_RATING_TYPE).get_asString(), true);
  details.SetUniqueID(record->at(VIDEODB_DETAILS_TVSHOW_UNIQUEID_VALUE).get_asString(), record->at(VIDEODB_DETAILS_TVSHOW_UNIQUEID_TYPE).get_asString(), true);
  details.SetDuration(record->at(VIDEODB_DETAILS_TVSHOW_DURATION).get_asInt());

  movieTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();

  if (getDetails)
  {
    if (getDetails & VideoDbDetailsCast)
    {
      GetCast(details.m_iDbId, "tvshow", details.m_cast);
      castTime += XbmcThreads::SystemClockMillis() - time; time = XbmcThreads::SystemClockMillis();
    }

    if (getDetails & VideoDbDetailsTag)
      GetTags(details.m_iDbId, MediaTypeTvShow, details.m_tags);

    if (getDetails & VideoDbDetailsRating)
      GetRatings(details.m_iDbId, MediaTypeTvShow, details.m_ratings);

    if (getDetails & VideoDbDetailsUniqueID)
      GetUniqueIDs(details.m_iDbId, MediaTypeTvShow, details);

    details.m_strPictureURL.Parse();

    details.m_parsedDetails = getDetails;
  }

  if (item != NULL)
  {
    item->m_dateTime = details.GetPremiered();
    item->SetProperty("totalseasons", details.m_iSeason);
    item->SetProperty("totalepisodes", details.m_iEpisode);
    item->SetProperty("numepisodes", details.m_iEpisode); // will be changed later to reflect watchmode setting
    item->SetProperty("watchedepisodes", details.GetPlayCount());
    item->SetProperty("unwatchedepisodes", details.m_iEpisode - details.GetPlayCount());
  }
  details.SetPlayCount((details.m_iEpisode <= details.GetPlayCount()) ? 1 : 0);

  return details;
}

bool CVideoDatabase::GetTvShowsNav(const std::string& strBaseDir, CFileItemList& items,
                                  int idGenre /* = -1 */, int idYear /* = -1 */, int idActor /* = -1 */, int idDirector /* = -1 */, int idStudio /* = -1 */, int idTag /* = -1 */,
                                  const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
{
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(strBaseDir))
    return false;

  if (idGenre != -1)
    videoUrl.AddOption("genreid", idGenre);
  else if (idStudio != -1)
    videoUrl.AddOption("studioid", idStudio);
  else if (idDirector != -1)
    videoUrl.AddOption("directorid", idDirector);
  else if (idYear != -1)
    videoUrl.AddOption("year", idYear);
  else if (idActor != -1)
    videoUrl.AddOption("actorid", idActor);
  else if (idTag != -1)
    videoUrl.AddOption("tagid", idTag);

  Filter filter;
  if (!CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_VIDEOLIBRARY_SHOWEMPTYTVSHOWS))
    filter.AppendWhere("totalCount IS NOT NULL AND totalCount > 0");
  return GetTvShowsByWhere(videoUrl.ToString(), filter, items, sortDescription, getDetails);
}

bool CVideoDatabase::GetTvShowsByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, const SortDescription &sortDescription /* = SortDescription() */, int getDetails /* = VideoDbDetailsNone */)
{
  try
  {
    movieTime = 0;

    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int total = -1;

    std::string strSQL = "SELECT %s FROM tvshow_view ";
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
    if (!SortUtils::SortFromDataset(sorting, MediaTypeTvShow, m_pDS, results))
      return false;

    // get data from returned rows
    items.Reserve(results.size());
    const query_data &data = m_pDS->get_result_set().records;
    for (const auto &i : results)
    {
      unsigned int targetRow = (unsigned int)i.at(FieldRow).asInteger();
      const dbiplus::sql_record* const record = data.at(targetRow);

      CFileItemPtr pItem(new CFileItem());
      CVideoInfoTag movie = GetDetailsForTvShow(record, getDetails, pItem.get());
      if (m_profileManager.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE ||
           g_passwordManager.bMasterUser                                     ||
           g_passwordManager.IsDatabasePathUnlocked(movie.m_strPath, *CMediaSourceSettings::GetInstance().GetSources("video")))
      {
        pItem->SetFromVideoInfoTag(movie);

        CVideoDbUrl itemUrl = videoUrl;
        std::string path = StringUtils::Format("%i/", record->at(0).get_asInt());
        itemUrl.AppendPath(path);
        pItem->SetPath(itemUrl.ToString());

        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, (pItem->GetVideoInfoTag()->GetPlayCount() > 0) && (pItem->GetVideoInfoTag()->m_iEpisode > 0));
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

bool CVideoDatabase::GetInProgressTvShowsNav(const std::string& strBaseDir, CFileItemList& items, unsigned int limit /* = 0 */, int getDetails /* = VideoDbDetailsNone */)
{
  Filter filter;
  filter.order = PrepareSQL("c%02d", VIDEODB_ID_TV_TITLE);
  filter.where = "watchedCount != 0 AND totalCount != watchedCount";
  return GetTvShowsByWhere(strBaseDir, filter, items, SortDescription(), getDetails);
}

std::string CVideoDatabase::GetTvShowTitleById(int id)
{
  return GetSingleValue("tvshow", PrepareSQL("c%02d", VIDEODB_ID_TV_TITLE), PrepareSQL("idShow=%i", id));
}

int CVideoDatabase::GetTvShowForEpisode(int idEpisode)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false;

    // make sure we use m_pDS2, as this is called in loops using m_pDS
    std::string strSQL=PrepareSQL("select idShow from episode where idEpisode=%i", idEpisode);
    m_pDS2->query( strSQL );

    int result=-1;
    if (!m_pDS2->eof())
      result=m_pDS2->fv(0).get_asInt();
    m_pDS2->close();

    return result;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idEpisode);
  }
  return false;
}

void CVideoDatabase::GetTvShowGenresByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("SELECT genre.genre_id, genre.name, path.strPath FROM genre INNER JOIN genre_link ON genre_link.genre_id=genre.genre_id INNER JOIN tvshow ON genre_link.media_id=tvshow.idShow INNER JOIN tvshowlinkpath ON tvshowlinkpath.idShow=tvshow.idShow INNER JOIN path ON path.idPath=tvshowlinkpath.idPath WHERE genre_link.media_type='tvshow' AND genre.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL=PrepareSQL("SELECT DISTINCT genre.genre_id, genre.name FROM genre INNER JOIN genre_link ON genre_link.genre_id=genre.genre_id WHERE genre_link.media_type='tvshow' AND name LIKE '%%%s%%'", strSearch.c_str());
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
      pItem->SetPath("videodb://tvshows/genres/"+ strDir);
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

void CVideoDatabase::GetTvShowsActorsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL=PrepareSQL("SELECT actor.actor_id, actor.name, path.strPath FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id INNER JOIN tvshow ON actor_link.media_id=tvshow.idShow INNER JOIN tvshowlinkpath ON tvshowlinkpath.idPath=tvshow.idShow INNER JOIN path ON path.idPath=tvshowlinkpath.idPath WHERE actor_link.media_type='tvshow' AND actor.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL=PrepareSQL("SELECT DISTINCT actor.actor_id, actor.name FROM actor INNER JOIN actor_link ON actor_link.actor_id=actor.actor_id INNER JOIN tvshow ON actor_link.media_id=tvshow.idShow WHERE actor_link.media_type='tvshow' AND actor.name LIKE '%%%s%%'",strSearch.c_str());
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
      pItem->SetPath("videodb://tvshows/actors/"+ strDir);
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

void CVideoDatabase::GetTvShowsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT tvshow.idShow, tvshow.c%02d, path.strPath FROM tvshow INNER JOIN tvshowlinkpath ON tvshowlinkpath.idShow=tvshow.idShow INNER JOIN path ON path.idPath=tvshowlinkpath.idPath WHERE tvshow.c%02d LIKE '%%%s%%'", VIDEODB_ID_TV_TITLE, VIDEODB_ID_TV_TITLE, strSearch.c_str());
    else
      strSQL = PrepareSQL("select tvshow.idShow,tvshow.c%02d from tvshow where tvshow.c%02d like '%%%s%%'",VIDEODB_ID_TV_TITLE,VIDEODB_ID_TV_TITLE,strSearch.c_str());
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
      std::string strDir = StringUtils::Format("tvshows/titles/%i/", m_pDS->fv("tvshow.idShow").get_asInt());

      pItem->SetPath("videodb://"+ strDir);
      pItem->m_bIsFolder=true;
      pItem->GetVideoInfoTag()->m_iDbId = m_pDS->fv("tvshow.idShow").get_asInt();
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

void CVideoDatabase::GetTvShowsDirectorsByName(const std::string& strSearch, CFileItemList& items)
{
  std::string strSQL;

  try
  {
    if (NULL == m_pDB.get()) return;
    if (NULL == m_pDS.get()) return;

    if (m_profileManager.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && !g_passwordManager.bMasterUser)
      strSQL = PrepareSQL("SELECT DISTINCT director_link.actor_id, actor.name, path.strPath FROM actor INNER JOIN director_link ON director_link.actor_id=actor.actor_id INNER JOIN tvshow ON director_link.media_id=tvshow.idShow INNER JOIN tvshowlinkpath ON tvshowlinkpath.idShow=tvshow.idShow INNER JOIN path ON path.idPath=tvshowlinkpath.idPath WHERE director_link.media_type='tvshow' AND actor.name LIKE '%%%s%%'", strSearch.c_str());
    else
      strSQL = PrepareSQL("SELECT DISTINCT director_link.actor_id, actor.name FROM actor INNER JOIN director_link ON director_link.actor_id=actor.actor_id INNER JOIN tvshow ON director_link.media_id=tvshow.idShow WHERE director_link.media_type='tvshow' AND actor.name LIKE '%%%s%%'", strSearch.c_str());

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

      pItem->SetPath("videodb://tvshows/directors/"+ strDir);
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

int CVideoDatabase::GetTvShowId(const std::string& strPath)
{
  try
  {
    if (NULL == m_pDB.get()) return -1;
    if (NULL == m_pDS.get()) return -1;
    int idTvShow = -1;

    // have to join movieinfo table for correct results
    int idPath = GetPathId(strPath);
    if (idPath < 0)
      return -1;

    std::string strSQL;
    std::string strPath1=strPath;
    std::string strParent;
    int iFound=0;

    strSQL=PrepareSQL("select idShow from tvshowlinkpath where tvshowlinkpath.idPath=%i",idPath);
    m_pDS->query(strSQL);
    if (!m_pDS->eof())
      iFound = 1;

    while (iFound == 0 && URIUtils::GetParentPath(strPath1, strParent))
    {
      strSQL=PrepareSQL("SELECT idShow FROM path INNER JOIN tvshowlinkpath ON tvshowlinkpath.idPath=path.idPath WHERE strPath='%s'",strParent.c_str());
      m_pDS->query(strSQL);
      if (!m_pDS->eof())
      {
        int idShow = m_pDS->fv("idShow").get_asInt();
        if (idShow != -1)
          iFound = 2;
      }
      strPath1 = strParent;
    }

    if (m_pDS->num_rows() > 0)
      idTvShow = m_pDS->fv("idShow").get_asInt();
    m_pDS->close();

    return idTvShow;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strPath.c_str());
  }
  return -1;
}

