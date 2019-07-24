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

#include "video/VideoInfoTag.h"
#include "ServiceBroker.h"
#include "dbwrappers/dataset.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "GUIPassword.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

using namespace dbiplus;

int CVideoDatabase::AddSeason(int showID, int season, const std::string& name /* = "" */)
{
  int seasonId = GetSeasonId(showID, season);
  if (seasonId < 0)
  {
    if (ExecuteQuery(PrepareSQL("INSERT INTO seasons (idShow, season, name) VALUES(%i, %i, '%s')", showID, season, name.c_str())))
      seasonId = (int)m_pDS->lastinsertid();
  }
  return seasonId;
}

int CVideoDatabase::GetSeasonId(int showID, int season)
{
  std::string sql = PrepareSQL("idShow=%i AND season=%i", showID, season);
  std::string id = GetSingleValue("seasons", "idSeason", sql);
  if (id.empty())
    return -1;
  return strtol(id.c_str(), NULL, 10);
}

bool CVideoDatabase::GetSeasonInfo(int idSeason, CVideoInfoTag& details, bool allDetails /* = true */)
{
  if (idSeason < 0)
    return false;

  try
  {
    if (!m_pDB.get() || !m_pDS.get())
      return false;

    std::string sql = PrepareSQL("SELECT idSeason, idShow, season, name, userrating FROM seasons WHERE idSeason=%i", idSeason);
    if (!m_pDS->query(sql))
      return false;

    if (m_pDS->num_rows() != 1)
      return false;

    if (allDetails)
    {
      int idShow = m_pDS->fv(1).get_asInt();

      // close the current result because we are going to query the season view for all details
      m_pDS->close();

      if (idShow < 0)
        return false;

      CFileItemList seasons;
      if (!GetSeasonsNav(StringUtils::Format("videodb://tvshows/titles/%i/", idShow), seasons, -1, -1, -1, -1, idShow, false) || seasons.Size() <= 0)
        return false;

      for (int index = 0; index < seasons.Size(); index++)
      {
        const CFileItemPtr season = seasons.Get(index);
        if (season->HasVideoInfoTag() && season->GetVideoInfoTag()->m_iDbId == idSeason && season->GetVideoInfoTag()->m_iIdShow == idShow)
        {
          details = *season->GetVideoInfoTag();
          return true;
        }
      }

      return false;
    }

    const int season = m_pDS->fv(2).get_asInt();
    std::string name = m_pDS->fv(3).get_asString();

    if (name.empty())
    {
      if (season == 0)
        name = g_localizeStrings.Get(20381);
      else
        name = StringUtils::Format(g_localizeStrings.Get(20358).c_str(), season);
    }

    details.m_strTitle = name;
    if (!name.empty())
      details.m_strSortTitle = name;
    details.m_iSeason = season;
    details.m_iDbId = m_pDS->fv(0).get_asInt();
    details.m_iIdSeason = details.m_iDbId;
    details.m_type = MediaTypeSeason;
    details.m_iUserRating = m_pDS->fv(4).get_asInt();
    details.m_iIdShow = m_pDS->fv(1).get_asInt();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idSeason);
  }
  return false;
}

int CVideoDatabase::SetDetailsForSeason(const CVideoInfoTag& details, const std::map<std::string,
    std::string> &artwork, int idShow, int idSeason /* = -1 */)
{
  if (idShow < 0 || details.m_iSeason < -1)
    return -1;

   try
  {
    BeginTransaction();
    if (idSeason < 0)
    {
      idSeason = AddSeason(idShow, details.m_iSeason);
      if (idSeason < 0)
      {
        RollbackTransaction();
        return -1;
      }
    }

    SetArtForItem(idSeason, MediaTypeSeason, artwork);

    // and insert the new row
    std::string sql = PrepareSQL("UPDATE seasons SET season=%i", details.m_iSeason);
    if (!details.m_strSortTitle.empty())
      sql += PrepareSQL(", name='%s'", details.m_strSortTitle.c_str());
    if (details.m_iUserRating > 0 && details.m_iUserRating < 11)
      sql += PrepareSQL(", userrating = %i", details.m_iUserRating);
    else
      sql += ", userrating = NULL";
    sql += PrepareSQL(" WHERE idSeason=%i", idSeason);
    m_pDS->exec(sql.c_str());
    CommitTransaction();

    return idSeason;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%i) failed", __FUNCTION__, idSeason);
  }
  RollbackTransaction();
  return -1;
}

void CVideoDatabase::DeleteSeason(int idSeason, bool bKeepId /* = false */)
{
  if (idSeason < 0)
    return;

  try
  {
    if (m_pDB.get() == NULL ||
        m_pDS.get() == NULL ||
        m_pDS2.get() == NULL)
      return;

    BeginTransaction();

    std::string strSQL = PrepareSQL("SELECT episode.idEpisode FROM episode "
                                    "JOIN seasons ON seasons.idSeason = %d AND episode.idShow = seasons.idShow AND episode.c%02d = seasons.season ",
                                   idSeason, VIDEODB_ID_EPISODE_SEASON);
    m_pDS2->query(strSQL);
    while (!m_pDS2->eof())
    {
      DeleteEpisode(m_pDS2->fv(0).get_asInt(), bKeepId);
      m_pDS2->next();
    }

    ExecuteQuery(PrepareSQL("DELETE FROM seasons WHERE idSeason = %i", idSeason));

    CommitTransaction();
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s (%d) failed", __FUNCTION__, idSeason);
    RollbackTransaction();
  }
}

bool CVideoDatabase::GetTvShowSeasons(int showId, std::map<int, int> &seasons)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false; // using dataset 2 as we're likely called in loops on dataset 1

    // get all seasons for this show
    std::string sql = PrepareSQL("select idSeason,season from seasons where idShow=%i", showId);
    m_pDS2->query(sql);

    seasons.clear();
    while (!m_pDS2->eof())
    {
      seasons.insert(std::make_pair(m_pDS2->fv(1).get_asInt(), m_pDS2->fv(0).get_asInt()));
      m_pDS2->next();
    }
    m_pDS2->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, showId);
  }
  return false;
}

bool CVideoDatabase::GetTvShowNamedSeasons(int showId, std::map<int, std::string> &seasons)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS2.get()) return false; // using dataset 2 as we're likely called in loops on dataset 1

    // get all named seasons for this show
    std::string sql = PrepareSQL("select season, name from seasons where season > 0 and name is not null and name <> '' and idShow = %i", showId);
    m_pDS2->query(sql);

    seasons.clear();
    while (!m_pDS2->eof())
    {
      seasons.insert(std::make_pair(m_pDS2->fv(0).get_asInt(), m_pDS2->fv(1).get_asString()));
      m_pDS2->next();
    }
    m_pDS2->close();
    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s(%d) failed", __FUNCTION__, showId);
  }
  return false;
}

bool CVideoDatabase::GetSeasonsNav(const std::string& strBaseDir, CFileItemList& items, int idActor, int idDirector, int idGenre, int idYear, int idShow, bool getLinkedMovies /* = true */)
{
  // parse the base path to get additional filters
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(strBaseDir))
    return false;

  if (idShow != -1)
    videoUrl.AddOption("tvshowid", idShow);
  if (idActor != -1)
    videoUrl.AddOption("actorid", idActor);
  else if (idDirector != -1)
    videoUrl.AddOption("directorid", idDirector);
  else if (idGenre != -1)
    videoUrl.AddOption("genreid", idGenre);
  else if (idYear != -1)
    videoUrl.AddOption("year", idYear);

  if (!GetSeasonsByWhere(videoUrl.ToString(), Filter(), items, false))
    return false;

  // now add any linked movies
  if (getLinkedMovies && idShow != -1)
  {
    Filter movieFilter;
    movieFilter.join = PrepareSQL("join movielinktvshow on movielinktvshow.idMovie=movie_view.idMovie");
    movieFilter.where = PrepareSQL("movielinktvshow.idShow = %i", idShow);
    CFileItemList movieItems;
    GetMoviesByWhere("videodb://movies/titles/", movieFilter, movieItems);

    if (movieItems.Size() > 0)
      items.Append(movieItems);
  }

  return true;
}

bool CVideoDatabase::GetSeasonsByWhere(const std::string& strBaseDir, const Filter &filter, CFileItemList& items, bool appendFullShowPath /* = true */, const SortDescription &sortDescription /* = SortDescription() */)
{
  try
  {
    if (NULL == m_pDB.get()) return false;
    if (NULL == m_pDS.get()) return false;

    int total = -1;

    std::string strSQL = "SELECT %s FROM season_view ";
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

    std::set<std::pair<int, int>> mapSeasons;
    while (!m_pDS->eof())
    {
      int id = m_pDS->fv(VIDEODB_ID_SEASON_ID).get_asInt();
      int showId = m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_ID).get_asInt();
      int iSeason = m_pDS->fv(VIDEODB_ID_SEASON_NUMBER).get_asInt();
      std::string name = m_pDS->fv(VIDEODB_ID_SEASON_NAME).get_asString();
      std::string path = m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_PATH).get_asString();

      if (mapSeasons.find(std::make_pair(showId, iSeason)) == mapSeasons.end() &&
         (m_profileManager.GetMasterProfile().getLockMode() == LOCK_MODE_EVERYONE || g_passwordManager.bMasterUser ||
          g_passwordManager.IsDatabasePathUnlocked(path, *CMediaSourceSettings::GetInstance().GetSources("video"))))
      {
        mapSeasons.insert(std::make_pair(showId, iSeason));

        std::string strLabel = name;
        if (strLabel.empty())
        {
          if (iSeason == 0)
            strLabel = g_localizeStrings.Get(20381);
          else
            strLabel = StringUtils::Format(g_localizeStrings.Get(20358).c_str(), iSeason);
        }
        CFileItemPtr pItem(new CFileItem(strLabel));

        CVideoDbUrl itemUrl = videoUrl;
        std::string strDir;
        if (appendFullShowPath)
          strDir += StringUtils::Format("%d/", showId);
        strDir += StringUtils::Format("%d/", iSeason);
        itemUrl.AppendPath(strDir);
        pItem->SetPath(itemUrl.ToString());

        pItem->m_bIsFolder = true;
        pItem->GetVideoInfoTag()->m_strTitle = strLabel;
        if (!name.empty())
          pItem->GetVideoInfoTag()->m_strSortTitle = name;
        pItem->GetVideoInfoTag()->m_iSeason = iSeason;
        pItem->GetVideoInfoTag()->m_iDbId = id;
        pItem->GetVideoInfoTag()->m_iIdSeason = id;
        pItem->GetVideoInfoTag()->m_type = MediaTypeSeason;
        pItem->GetVideoInfoTag()->m_strPath = path;
        pItem->GetVideoInfoTag()->m_strShowTitle = m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_TITLE).get_asString();
        pItem->GetVideoInfoTag()->m_strPlot = m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_PLOT).get_asString();
        pItem->GetVideoInfoTag()->SetPremieredFromDBDate(m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_PREMIERED).get_asString());
        pItem->GetVideoInfoTag()->m_firstAired.SetFromDBDate(m_pDS->fv(VIDEODB_ID_SEASON_PREMIERED).get_asString());
        pItem->GetVideoInfoTag()->m_iUserRating = m_pDS->fv(VIDEODB_ID_SEASON_USER_RATING).get_asInt();
        // season premiered date based on first episode airdate associated to the season
        // tvshow premiered date is used as a fallback
        if (pItem->GetVideoInfoTag()->m_firstAired.IsValid())
          pItem->GetVideoInfoTag()->SetPremiered(pItem->GetVideoInfoTag()->m_firstAired);
        else if (pItem->GetVideoInfoTag()->HasPremiered())
          pItem->GetVideoInfoTag()->SetPremiered(pItem->GetVideoInfoTag()->GetPremiered());
        else if (pItem->GetVideoInfoTag()->HasYear())
          pItem->GetVideoInfoTag()->SetYear(pItem->GetVideoInfoTag()->GetYear());
        pItem->GetVideoInfoTag()->m_genre = StringUtils::Split(m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_GENRE).get_asString(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
        pItem->GetVideoInfoTag()->m_studio = StringUtils::Split(m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_STUDIO).get_asString(), CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
        pItem->GetVideoInfoTag()->m_strMPAARating = m_pDS->fv(VIDEODB_ID_SEASON_TVSHOW_MPAA).get_asString();
        pItem->GetVideoInfoTag()->m_iIdShow = showId;

        int totalEpisodes = m_pDS->fv(VIDEODB_ID_SEASON_EPISODES_TOTAL).get_asInt();
        int watchedEpisodes = m_pDS->fv(VIDEODB_ID_SEASON_EPISODES_WATCHED).get_asInt();
        pItem->GetVideoInfoTag()->m_iEpisode = totalEpisodes;
        pItem->SetProperty("totalepisodes", totalEpisodes);
        pItem->SetProperty("numepisodes", totalEpisodes); // will be changed later to reflect watchmode setting
        pItem->SetProperty("watchedepisodes", watchedEpisodes);
        pItem->SetProperty("unwatchedepisodes", totalEpisodes - watchedEpisodes);
        if (iSeason == 0)
          pItem->SetProperty("isspecial", true);
        pItem->GetVideoInfoTag()->SetPlayCount((totalEpisodes == watchedEpisodes) ? 1 : 0);
        pItem->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED, (pItem->GetVideoInfoTag()->GetPlayCount() > 0) && (pItem->GetVideoInfoTag()->m_iEpisode > 0));

        items.Add(pItem);
      }

      m_pDS->next();
    }
    m_pDS->close();

    return true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s failed", __FUNCTION__);
  }
  return false;
}

int CVideoDatabase::GetSeasonForEpisode(int idEpisode)
{
  char column[5];
  sprintf(column, "c%0d", VIDEODB_ID_EPISODE_SEASON);
  std::string id = GetSingleValue("episode", column, PrepareSQL("idEpisode=%i", idEpisode));
  if (id.empty())
    return -1;
  return atoi(id.c_str());
}
