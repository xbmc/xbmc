/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <cstdlib>

#include "system.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "dbwrappers/dataset.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include "EpgContainer.h"
#include "EpgDatabase.h"

using namespace dbiplus;
using namespace EPG;

bool CEpgDatabase::Open(void)
{
  return CDatabase::Open(g_advancedSettings.m_databaseEpg);
}

void CEpgDatabase::CreateTables(void)
{
  CLog::Log(LOGINFO, "EpgDB - %s - creating tables", __FUNCTION__);

  CLog::Log(LOGDEBUG, "EpgDB - %s - creating table 'epg'", __FUNCTION__);
  m_pDS->exec(
      "CREATE TABLE epg ("
        "idEpg           integer primary key, "
        "sName           varchar(64),"
        "sScraperName    varchar(32)"
      ")"
  );

  CLog::Log(LOGDEBUG, "EpgDB - %s - creating table 'epgtags'", __FUNCTION__);
  m_pDS->exec(
      "CREATE TABLE epgtags ("
        "idBroadcast     integer primary key, "
        "iBroadcastUid   integer, "
        "idEpg           integer, "
        "sTitle          varchar(128), "
        "sPlotOutline    text, "
        "sPlot           text, "
        "sOriginalTitle  varchar(128), "
        "sCast           varchar(255), "
        "sDirector       varchar(255), "
        "sWriter         varchar(255), "
        "iYear           integer, "
        "sIMDBNumber     varchar(50), "
        "sIconPath       varchar(255), "
        "iStartTime      integer, "
        "iEndTime        integer, "
        "iGenreType      integer, "
        "iGenreSubType   integer, "
        "sGenre          varchar(128), "
        "iFirstAired     integer, "
        "iParentalRating integer, "
        "iStarRating     integer, "
        "bNotify         bool, "
        "iSeriesId       integer, "
        "iEpisodeId      integer, "
        "iEpisodePart    integer, "
        "sEpisodeName    varchar(128), "
        "iFlags          integer"
      ")"
  );
  CLog::Log(LOGDEBUG, "EpgDB - %s - creating table 'lastepgscan'", __FUNCTION__);
  m_pDS->exec("CREATE TABLE lastepgscan ("
        "idEpg integer primary key, "
        "sLastScan varchar(20)"
      ")"
  );
}

void CEpgDatabase::CreateAnalytics()
{
  CLog::Log(LOGDEBUG, "%s - creating indices", __FUNCTION__);
  m_pDS->exec("CREATE UNIQUE INDEX idx_epg_idEpg_iStartTime on epgtags(idEpg, iStartTime desc);");
  m_pDS->exec("CREATE INDEX idx_epg_iEndTime on epgtags(iEndTime);");
}

void CEpgDatabase::UpdateTables(int iVersion)
{
  if (iVersion < 5)
    m_pDS->exec("ALTER TABLE epgtags ADD sGenre varchar(128);");

  if (iVersion < 9)
    m_pDS->exec("ALTER TABLE epgtags ADD sIconPath varchar(255);");

  if (iVersion < 10)
  {
    m_pDS->exec("ALTER TABLE epgtags ADD sOriginalTitle varchar(128);");
    m_pDS->exec("ALTER TABLE epgtags ADD sCast varchar(255);");
    m_pDS->exec("ALTER TABLE epgtags ADD sDirector varchar(255);");
    m_pDS->exec("ALTER TABLE epgtags ADD sWriter varchar(255);");
    m_pDS->exec("ALTER TABLE epgtags ADD iYear integer;");
    m_pDS->exec("ALTER TABLE epgtags ADD sIMDBNumber varchar(50);");
  }

  if (iVersion < 11)
  {
    m_pDS->exec("ALTER TABLE epgtags ADD iFlags integer;");
  }
}

bool CEpgDatabase::DeleteEpg(void)
{
  bool bReturn(false);
  CLog::Log(LOGDEBUG, "EpgDB - %s - deleting all EPG data from the database", __FUNCTION__);

  bReturn = DeleteValues("epg") || bReturn;
  bReturn = DeleteValues("epgtags") || bReturn;
  bReturn = DeleteValues("lastepgscan") || bReturn;

  return bReturn;
}

bool CEpgDatabase::Delete(const CEpg &table)
{
  /* invalid channel */
  if (table.EpgID() <= 0)
  {
    CLog::Log(LOGERROR, "EpgDB - %s - invalid channel id: %d", __FUNCTION__, table.EpgID());
    return false;
  }

  Filter filter;
  filter.AppendWhere(PrepareSQL("idEpg = %u", table.EpgID()));

  return DeleteValues("epg", filter);
}

bool CEpgDatabase::DeleteEpgEntries(const CDateTime &maxEndTime)
{
  time_t iMaxEndTime;
  maxEndTime.GetAsTime(iMaxEndTime);

  Filter filter;
  filter.AppendWhere(PrepareSQL("iEndTime < %u", iMaxEndTime));

  return DeleteValues("epgtags", filter);
}

bool CEpgDatabase::Delete(const CEpgInfoTag &tag)
{
  /* tag without a database ID was not persisted */
  if (tag.BroadcastId() <= 0)
    return false;

  Filter filter;
  filter.AppendWhere(PrepareSQL("idBroadcast = %u", tag.BroadcastId()));

  return DeleteValues("epgtags", filter);
}

int CEpgDatabase::Get(CEpgContainer &container)
{
  int iReturn(-1);

  std::string strQuery = PrepareSQL("SELECT idEpg, sName, sScraperName FROM epg;");
  if (ResultQuery(strQuery))
  {
    iReturn = 0;

    try
    {
      while (!m_pDS->eof())
      {
        int iEpgID                 = m_pDS->fv("idEpg").get_asInt();
        std::string strName        = m_pDS->fv("sName").get_asString().c_str();
        std::string strScraperName = m_pDS->fv("sScraperName").get_asString().c_str();

        container.InsertFromDatabase(iEpgID, strName, strScraperName);
        ++iReturn;
        m_pDS->next();
      }
      m_pDS->close();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - couldn't load EPG data from the database", __FUNCTION__);
    }
  }

  return iReturn;
}

int CEpgDatabase::Get(CEpg &epg)
{
  int iReturn(-1);

  std::string strQuery = PrepareSQL("SELECT * FROM epgtags WHERE idEpg = %u;", epg.EpgID());
  if (ResultQuery(strQuery))
  {
    iReturn = 0;
    try
    {
      while (!m_pDS->eof())
      {
        CEpgInfoTagPtr newTag(new CEpgInfoTag());

        time_t iStartTime, iEndTime, iFirstAired;
        iStartTime = (time_t) m_pDS->fv("iStartTime").get_asInt();
        CDateTime startTime(iStartTime);
        newTag->m_startTime = startTime;

        iEndTime = (time_t) m_pDS->fv("iEndTime").get_asInt();
        CDateTime endTime(iEndTime);
        newTag->m_endTime = endTime;

        iFirstAired = (time_t) m_pDS->fv("iFirstAired").get_asInt();
        CDateTime firstAired(iFirstAired);
        newTag->m_firstAired = firstAired;

        int iBroadcastUID = m_pDS->fv("iBroadcastUid").get_asInt();
        // Compat: null value for broadcast uid changed from numerical -1 to 0 with PVR Addon API v4.0.0
        newTag->m_iUniqueBroadcastID = iBroadcastUID == -1 ? EPG_TAG_INVALID_UID : iBroadcastUID;

        newTag->m_iBroadcastId       = m_pDS->fv("idBroadcast").get_asInt();
        newTag->m_strTitle           = m_pDS->fv("sTitle").get_asString().c_str();
        newTag->m_strPlotOutline     = m_pDS->fv("sPlotOutline").get_asString().c_str();
        newTag->m_strPlot            = m_pDS->fv("sPlot").get_asString().c_str();
        newTag->m_strOriginalTitle   = m_pDS->fv("sOriginalTitle").get_asString().c_str();
        newTag->m_strCast            = m_pDS->fv("sCast").get_asString().c_str();
        newTag->m_strDirector        = m_pDS->fv("sDirector").get_asString().c_str();
        newTag->m_strWriter          = m_pDS->fv("sWriter").get_asString().c_str();
        newTag->m_iYear              = m_pDS->fv("iYear").get_asInt();
        newTag->m_strIMDBNumber      = m_pDS->fv("sIMDBNumber").get_asString().c_str();
        newTag->m_iGenreType         = m_pDS->fv("iGenreType").get_asInt();
        newTag->m_iGenreSubType      = m_pDS->fv("iGenreSubType").get_asInt();
        newTag->m_genre              = StringUtils::Split(m_pDS->fv("sGenre").get_asString().c_str(), g_advancedSettings.m_videoItemSeparator);
        newTag->m_iParentalRating    = m_pDS->fv("iParentalRating").get_asInt();
        newTag->m_iStarRating        = m_pDS->fv("iStarRating").get_asInt();
        newTag->m_bNotify            = m_pDS->fv("bNotify").get_asBool();
        newTag->m_iEpisodeNumber     = m_pDS->fv("iEpisodeId").get_asInt();
        newTag->m_iEpisodePart       = m_pDS->fv("iEpisodePart").get_asInt();
        newTag->m_strEpisodeName     = m_pDS->fv("sEpisodeName").get_asString().c_str();
        newTag->m_iSeriesNumber      = m_pDS->fv("iSeriesId").get_asInt();
        newTag->m_strIconPath        = m_pDS->fv("sIconPath").get_asString().c_str();
        newTag->m_iFlags             = m_pDS->fv("iFlags").get_asInt();

        epg.AddEntry(*newTag);
        ++iReturn;

        m_pDS->next();
      }
      m_pDS->close();
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - couldn't load EPG data from the database", __FUNCTION__);
    }
  }
  return iReturn;
}

bool CEpgDatabase::GetLastEpgScanTime(int iEpgId, CDateTime *lastScan)
{
  bool bReturn = false;
  std::string strWhereClause = PrepareSQL("idEpg = %u", iEpgId);
  std::string strValue = GetSingleValue("lastepgscan", "sLastScan", strWhereClause);

  if (!strValue.empty())
  {
    lastScan->SetFromDBDateTime(strValue.c_str());
    bReturn = true;
  }
  else
  {
    lastScan->SetValid(false);
  }

  return bReturn;
}

bool CEpgDatabase::PersistLastEpgScanTime(int iEpgId /* = 0 */, bool bQueueWrite /* = false */)
{
  std::string strQuery = PrepareSQL("REPLACE INTO lastepgscan(idEpg, sLastScan) VALUES (%u, '%s');",
      iEpgId, CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsDBDateTime().c_str());

  return bQueueWrite ? QueueInsertQuery(strQuery) : ExecuteQuery(strQuery);
}

bool CEpgDatabase::Persist(const EPGMAP &epgs)
{
  for (const auto &epgEntry : epgs)
  {
    if (epgEntry.second)
      Persist(*epgEntry.second, true);
  }

  return CommitInsertQueries();
}

int CEpgDatabase::Persist(const CEpg &epg, bool bQueueWrite /* = false */)
{
  int iReturn(-1);

  std::string strQuery;
  if (epg.EpgID() > 0)
    strQuery = PrepareSQL("REPLACE INTO epg (idEpg, sName, sScraperName) "
        "VALUES (%u, '%s', '%s');", epg.EpgID(), epg.Name().c_str(), epg.ScraperName().c_str());
  else
    strQuery = PrepareSQL("INSERT INTO epg (sName, sScraperName) "
        "VALUES ('%s', '%s');", epg.Name().c_str(), epg.ScraperName().c_str());

  if (bQueueWrite)
  {
    if (QueueInsertQuery(strQuery))
      iReturn = epg.EpgID() <= 0 ? 0 : epg.EpgID();
  }
  else
  {
    if (ExecuteQuery(strQuery))
      iReturn = epg.EpgID() <= 0 ? (int) m_pDS->lastinsertid() : epg.EpgID();
  }

  return iReturn;
}

int CEpgDatabase::Persist(const CEpgInfoTag &tag, bool bSingleUpdate /* = true */)
{
  int iReturn(-1);

  if (tag.EpgID() <= 0)
  {
    CLog::Log(LOGERROR, "%s - tag '%s' does not have a valid table", __FUNCTION__, tag.Title(true).c_str());
    return iReturn;
  }

  time_t iStartTime, iEndTime, iFirstAired;
  tag.StartAsUTC().GetAsTime(iStartTime);
  tag.EndAsUTC().GetAsTime(iEndTime);
  tag.FirstAiredAsUTC().GetAsTime(iFirstAired);

  int iBroadcastId = tag.BroadcastId();
  std::string strQuery;

  /* Only store the genre string when needed */
  std::string strGenre = (tag.GenreType() == EPG_GENRE_USE_STRING) ? StringUtils::Join(tag.Genre(), g_advancedSettings.m_videoItemSeparator) : "";

  if (iBroadcastId < 0)
  {
    strQuery = PrepareSQL("REPLACE INTO epgtags (idEpg, iStartTime, "
        "iEndTime, sTitle, sPlotOutline, sPlot, sOriginalTitle, sCast, sDirector, sWriter, iYear, sIMDBNumber, "
        "sIconPath, iGenreType, iGenreSubType, sGenre, iFirstAired, iParentalRating, iStarRating, bNotify, iSeriesId, "
        "iEpisodeId, iEpisodePart, sEpisodeName, iFlags, iBroadcastUid) "
        "VALUES (%u, %u, %u, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %i, '%s', '%s', %i, %i, '%s', %u, %i, %i, %i, %i, %i, %i, '%s', %i, %i);",
        tag.EpgID(), iStartTime, iEndTime,
        tag.Title(true).c_str(), tag.PlotOutline(true).c_str(), tag.Plot(true).c_str(),
        tag.OriginalTitle(true).c_str(), tag.Cast().c_str(), tag.Director().c_str(), tag.Writer().c_str(), tag.Year(), tag.IMDBNumber().c_str(),
        tag.Icon().c_str(), tag.GenreType(), tag.GenreSubType(), strGenre.c_str(),
        iFirstAired, tag.ParentalRating(), tag.StarRating(), tag.Notify(),
        tag.SeriesNumber(), tag.EpisodeNumber(), tag.EpisodePart(), tag.EpisodeName().c_str(), tag.Flags(),
        tag.UniqueBroadcastID());
  }
  else
  {
    strQuery = PrepareSQL("REPLACE INTO epgtags (idEpg, iStartTime, "
        "iEndTime, sTitle, sPlotOutline, sPlot, sOriginalTitle, sCast, sDirector, sWriter, iYear, sIMDBNumber, "
        "sIconPath, iGenreType, iGenreSubType, sGenre, iFirstAired, iParentalRating, iStarRating, bNotify, iSeriesId, "
        "iEpisodeId, iEpisodePart, sEpisodeName, iFlags, iBroadcastUid, idBroadcast) "
        "VALUES (%u, %u, %u, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %i, '%s', '%s', %i, %i, '%s', %u, %i, %i, %i, %i, %i, %i, '%s', %i, %i, %i);",
        tag.EpgID(), iStartTime, iEndTime,
        tag.Title(true).c_str(), tag.PlotOutline(true).c_str(), tag.Plot(true).c_str(),
        tag.OriginalTitle(true).c_str(), tag.Cast().c_str(), tag.Director().c_str(), tag.Writer().c_str(), tag.Year(), tag.IMDBNumber().c_str(),
        tag.Icon().c_str(), tag.GenreType(), tag.GenreSubType(), strGenre.c_str(),
        iFirstAired, tag.ParentalRating(), tag.StarRating(), tag.Notify(),
        tag.SeriesNumber(), tag.EpisodeNumber(), tag.EpisodePart(), tag.EpisodeName().c_str(), tag.Flags(),
        tag.UniqueBroadcastID(), iBroadcastId);
  }

  if (bSingleUpdate)
  {
    if (ExecuteQuery(strQuery))
      iReturn = (int) m_pDS->lastinsertid();
  }
  else
  {
    QueueInsertQuery(strQuery);
    iReturn = 0;
  }

  return iReturn;
}

int CEpgDatabase::GetLastEPGId(void)
{
  std::string strQuery = PrepareSQL("SELECT MAX(idEpg) FROM epg");
  std::string strValue = GetSingleValue(strQuery);
  if (!strValue.empty())
    return atoi(strValue.c_str());
  return 0;
}
