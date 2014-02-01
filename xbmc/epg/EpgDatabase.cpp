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

#include "dbwrappers/dataset.h"
#include "settings/AdvancedSettings.h"
#include "settings/VideoSettings.h"
#include "utils/log.h"
#include "addons/include/xbmc_pvr_types.h"

#include "EpgDatabase.h"
#include "EpgContainer.h"
#include "system.h"

using namespace std;
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
        "sEpisodeName    varchar(128)"
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

bool CEpgDatabase::DeleteOldEpgEntries(void)
{
  time_t iCleanupTime;
  CDateTime cleanupTime = CDateTime::GetCurrentDateTime().GetAsUTCDateTime() -
      CDateTimeSpan(0, g_advancedSettings.m_iEpgLingerTime / 60, g_advancedSettings.m_iEpgLingerTime % 60, 0);
  cleanupTime.GetAsTime(iCleanupTime);

  Filter filter;
  filter.AppendWhere(PrepareSQL("iEndTime < %u", iCleanupTime));

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

  CStdString strQuery = PrepareSQL("SELECT idEpg, sName, sScraperName FROM epg;");
  if (ResultQuery(strQuery))
  {
    iReturn = 0;

    try
    {
      while (!m_pDS->eof())
      {
        int iEpgID                = m_pDS->fv("idEpg").get_asInt();
        CStdString strName        = m_pDS->fv("sName").get_asString().c_str();
        CStdString strScraperName = m_pDS->fv("sScraperName").get_asString().c_str();

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

  CStdString strQuery = PrepareSQL("SELECT * FROM epgtags WHERE idEpg = %u;", epg.EpgID());
  if (ResultQuery(strQuery))
  {
    iReturn = 0;
    try
    {
      while (!m_pDS->eof())
      {
        CEpgInfoTag newTag;

        time_t iStartTime, iEndTime, iFirstAired;
        iStartTime = (time_t) m_pDS->fv("iStartTime").get_asInt();
        CDateTime startTime(iStartTime);
        newTag.m_startTime = startTime;

        iEndTime = (time_t) m_pDS->fv("iEndTime").get_asInt();
        CDateTime endTime(iEndTime);
        newTag.m_endTime = endTime;

        iFirstAired = (time_t) m_pDS->fv("iFirstAired").get_asInt();
        CDateTime firstAired(iFirstAired);
        newTag.m_firstAired = firstAired;

        newTag.m_iUniqueBroadcastID = m_pDS->fv("iBroadcastUid").get_asInt();
        newTag.m_iBroadcastId       = m_pDS->fv("idBroadcast").get_asInt();
        newTag.m_strTitle           = m_pDS->fv("sTitle").get_asString().c_str();
        newTag.m_strPlotOutline     = m_pDS->fv("sPlotOutline").get_asString().c_str();
        newTag.m_strPlot            = m_pDS->fv("sPlot").get_asString().c_str();
        newTag.m_iGenreType         = m_pDS->fv("iGenreType").get_asInt();
        newTag.m_iGenreSubType      = m_pDS->fv("iGenreSubType").get_asInt();
        newTag.m_genre              = StringUtils::Split(m_pDS->fv("sGenre").get_asString().c_str(), g_advancedSettings.m_videoItemSeparator);
        newTag.m_iParentalRating    = m_pDS->fv("iParentalRating").get_asInt();
        newTag.m_iStarRating        = m_pDS->fv("iStarRating").get_asInt();
        newTag.m_bNotify            = m_pDS->fv("bNotify").get_asBool();
        newTag.m_iEpisodeNumber     = m_pDS->fv("iEpisodeId").get_asInt();
        newTag.m_iEpisodePart       = m_pDS->fv("iEpisodePart").get_asInt();
        newTag.m_strEpisodeName     = m_pDS->fv("sEpisodeName").get_asString().c_str();
        newTag.m_iSeriesNumber      = m_pDS->fv("iSeriesId").get_asInt();

        epg.AddEntry(newTag);
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
  CStdString strWhereClause = PrepareSQL("idEpg = %u", iEpgId);
  CStdString strValue = GetSingleValue("lastepgscan", "sLastScan", strWhereClause);

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
  CStdString strQuery = PrepareSQL("REPLACE INTO lastepgscan(idEpg, sLastScan) VALUES (%u, '%s');",
      iEpgId, CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsDBDateTime().c_str());

  return bQueueWrite ? QueueInsertQuery(strQuery) : ExecuteQuery(strQuery);
}

bool CEpgDatabase::Persist(const CEpgContainer &epg)
{
  for (map<unsigned int, CEpg *>::const_iterator it = epg.m_epgs.begin(); it != epg.m_epgs.end(); it++)
  {
    CEpg *epg = it->second;
    if (epg)
      Persist(*epg, true);
  }

  return CommitInsertQueries();
}

int CEpgDatabase::Persist(const CEpg &epg, bool bQueueWrite /* = false */)
{
  int iReturn(-1);

  CStdString strQuery;
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
  CStdString strQuery;
  
  /* Only store the genre string when needed */
  CStdString strGenre = (tag.GenreType() == EPG_GENRE_USE_STRING) ? StringUtils::Join(tag.Genre(), g_advancedSettings.m_videoItemSeparator) : "";

  if (iBroadcastId < 0)
  {
    strQuery = PrepareSQL("REPLACE INTO epgtags (idEpg, iStartTime, "
        "iEndTime, sTitle, sPlotOutline, sPlot, iGenreType, iGenreSubType, sGenre, "
        "iFirstAired, iParentalRating, iStarRating, bNotify, iSeriesId, "
        "iEpisodeId, iEpisodePart, sEpisodeName, iBroadcastUid) "
        "VALUES (%u, %u, %u, '%s', '%s', '%s', %i, %i, '%s', %u, %i, %i, %i, %i, %i, %i, '%s', %i);",
        tag.EpgID(), iStartTime, iEndTime,
        tag.Title(true).c_str(), tag.PlotOutline(true).c_str(), tag.Plot(true).c_str(), tag.GenreType(), tag.GenreSubType(), strGenre.c_str(),
        iFirstAired, tag.ParentalRating(), tag.StarRating(), tag.Notify(),
        tag.SeriesNum(), tag.EpisodeNum(), tag.EpisodePart(), tag.EpisodeName().c_str(),
        tag.UniqueBroadcastID());
  }
  else
  {
    strQuery = PrepareSQL("REPLACE INTO epgtags (idEpg, iStartTime, "
        "iEndTime, sTitle, sPlotOutline, sPlot, iGenreType, iGenreSubType, sGenre, "
        "iFirstAired, iParentalRating, iStarRating, bNotify, iSeriesId, "
        "iEpisodeId, iEpisodePart, sEpisodeName, iBroadcastUid, idBroadcast) "
        "VALUES (%u, %u, %u, '%s', '%s', '%s', %i, %i, '%s', %u, %i, %i, %i, %i, %i, %i, '%s', %i, %i);",
        tag.EpgID(), iStartTime, iEndTime,
        tag.Title(true).c_str(), tag.PlotOutline(true).c_str(), tag.Plot(true).c_str(), tag.GenreType(), tag.GenreSubType(), strGenre.c_str(),
        iFirstAired, tag.ParentalRating(), tag.StarRating(), tag.Notify(),
        tag.SeriesNum(), tag.EpisodeNum(), tag.EpisodePart(), tag.EpisodeName().c_str(),
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
  CStdString strQuery = PrepareSQL("SELECT MAX(idEpg) FROM epg");
  CStdString strValue = GetSingleValue(strQuery);
  if (!strValue.empty())
    return atoi(strValue.c_str());
  return 0;
}
