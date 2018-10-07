/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgDatabase.h"

#include <cstdlib>

#include "ServiceBroker.h"
#include "addons/kodi-addon-dev-kit/include/kodi/xbmc_pvr_types.h"
#include "dbwrappers/dataset.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include "pvr/epg/EpgContainer.h"

using namespace dbiplus;
using namespace PVR;

bool CPVREpgDatabase::Open()
{
  CSingleLock lock(m_critSection);
  return CDatabase::Open(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseEpg);
}

void CPVREpgDatabase::Close()
{
  CSingleLock lock(m_critSection);
  CDatabase::Close();
}

void CPVREpgDatabase::Lock()
{
  m_critSection.lock();
}

void CPVREpgDatabase::Unlock()
{
  m_critSection.unlock();
}

void CPVREpgDatabase::CreateTables(void)
{
  CLog::Log(LOGINFO, "Creating EPG database tables");

  CLog::LogFC(LOGDEBUG, LOGEPG, "Creating table 'epg'");

  CSingleLock lock(m_critSection);

  m_pDS->exec(
      "CREATE TABLE epg ("
        "idEpg           integer primary key, "
        "sName           varchar(64),"
        "sScraperName    varchar(32)"
      ")"
  );

  CLog::LogFC(LOGDEBUG, LOGEPG, "Creating table 'epgtags'");
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
        "iFlags          integer, "
        "sSeriesLink     varchar(255)"
      ")"
  );

  CLog::LogFC(LOGDEBUG, LOGEPG, "Creating table 'lastepgscan'");
  m_pDS->exec("CREATE TABLE lastepgscan ("
        "idEpg integer primary key, "
        "sLastScan varchar(20)"
      ")"
  );
}

void CPVREpgDatabase::CreateAnalytics()
{
  CLog::LogFC(LOGDEBUG, LOGEPG, "Creating EPG database indices");

  CSingleLock lock(m_critSection);
  m_pDS->exec("CREATE UNIQUE INDEX idx_epg_idEpg_iStartTime on epgtags(idEpg, iStartTime desc);");
  m_pDS->exec("CREATE INDEX idx_epg_iEndTime on epgtags(iEndTime);");
}

void CPVREpgDatabase::UpdateTables(int iVersion)
{
  CSingleLock lock(m_critSection);
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

  if (iVersion < 12)
  {
    m_pDS->exec("ALTER TABLE epgtags ADD sSeriesLink varchar(255);");
  }
}

bool CPVREpgDatabase::DeleteEpg(void)
{
  bool bReturn(false);
  CLog::LogFC(LOGDEBUG, LOGEPG, "Deleting all EPG data from the database");

  CSingleLock lock(m_critSection);

  bReturn = DeleteValues("epg") || bReturn;
  bReturn = DeleteValues("epgtags") || bReturn;
  bReturn = DeleteValues("lastepgscan") || bReturn;

  return bReturn;
}

bool CPVREpgDatabase::Delete(const CPVREpg &table)
{
  /* invalid channel */
  if (table.EpgID() <= 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel id: %d", table.EpgID());
    return false;
  }

  Filter filter;

  CSingleLock lock(m_critSection);
  filter.AppendWhere(PrepareSQL("idEpg = %u", table.EpgID()));
  return DeleteValues("epg", filter);
}

bool CPVREpgDatabase::DeleteEpgEntries(const CDateTime &maxEndTime)
{
  time_t iMaxEndTime;
  maxEndTime.GetAsTime(iMaxEndTime);

  Filter filter;

  CSingleLock lock(m_critSection);
  filter.AppendWhere(PrepareSQL("iEndTime < %u", iMaxEndTime));
  return DeleteValues("epgtags", filter);
}

bool CPVREpgDatabase::Delete(const CPVREpgInfoTag &tag)
{
  /* tag without a database ID was not persisted */
  if (tag.DatabaseID() <= 0)
    return false;

  Filter filter;

  CSingleLock lock(m_critSection);
  filter.AppendWhere(PrepareSQL("idBroadcast = %u", tag.DatabaseID()));
  return DeleteValues("epgtags", filter);
}

std::vector<CPVREpgPtr> CPVREpgDatabase::Get(const CPVREpgContainer &container)
{
  std::vector<CPVREpgPtr> result;

  CSingleLock lock(m_critSection);
  std::string strQuery = PrepareSQL("SELECT idEpg, sName, sScraperName FROM epg;");
  if (ResultQuery(strQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        int iEpgID                 = m_pDS->fv("idEpg").get_asInt();
        std::string strName        = m_pDS->fv("sName").get_asString().c_str();
        std::string strScraperName = m_pDS->fv("sScraperName").get_asString().c_str();

        result.emplace_back(new CPVREpg(iEpgID, strName, strScraperName, true));
        m_pDS->next();
      }
      m_pDS->close();
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load EPG data from the database");
    }
  }

  return result;
}

std::vector<CPVREpgInfoTagPtr> CPVREpgDatabase::Get(const CPVREpg &epg)
{
  std::vector<CPVREpgInfoTagPtr> result;

  CSingleLock lock(m_critSection);
  std::string strQuery = PrepareSQL("SELECT * FROM epgtags WHERE idEpg = %u;", epg.EpgID());
  if (ResultQuery(strQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        CPVREpgInfoTagPtr newTag(new CPVREpgInfoTag());

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

        newTag->m_iDatabaseID        = m_pDS->fv("idBroadcast").get_asInt();
        newTag->m_strTitle           = m_pDS->fv("sTitle").get_asString().c_str();
        newTag->m_strPlotOutline     = m_pDS->fv("sPlotOutline").get_asString().c_str();
        newTag->m_strPlot            = m_pDS->fv("sPlot").get_asString().c_str();
        newTag->m_strOriginalTitle   = m_pDS->fv("sOriginalTitle").get_asString().c_str();
        newTag->m_cast               = newTag->Tokenize(m_pDS->fv("sCast").get_asString());
        newTag->m_directors          = newTag->Tokenize(m_pDS->fv("sDirector").get_asString());
        newTag->m_writers            = newTag->Tokenize(m_pDS->fv("sWriter").get_asString());
        newTag->m_iYear              = m_pDS->fv("iYear").get_asInt();
        newTag->m_strIMDBNumber      = m_pDS->fv("sIMDBNumber").get_asString().c_str();
        newTag->m_iGenreType         = m_pDS->fv("iGenreType").get_asInt();
        newTag->m_iGenreSubType      = m_pDS->fv("iGenreSubType").get_asInt();
        newTag->m_genre              = newTag->Tokenize(m_pDS->fv("sGenre").get_asString());
        newTag->m_iParentalRating    = m_pDS->fv("iParentalRating").get_asInt();
        newTag->m_iStarRating        = m_pDS->fv("iStarRating").get_asInt();
        newTag->m_bNotify            = m_pDS->fv("bNotify").get_asBool();
        newTag->m_iEpisodeNumber     = m_pDS->fv("iEpisodeId").get_asInt();
        newTag->m_iEpisodePart       = m_pDS->fv("iEpisodePart").get_asInt();
        newTag->m_strEpisodeName     = m_pDS->fv("sEpisodeName").get_asString().c_str();
        newTag->m_iSeriesNumber      = m_pDS->fv("iSeriesId").get_asInt();
        newTag->m_strIconPath        = m_pDS->fv("sIconPath").get_asString().c_str();
        newTag->m_iFlags             = m_pDS->fv("iFlags").get_asInt();
        newTag->m_strSeriesLink      = m_pDS->fv("sSeriesLink").get_asString().c_str();

        result.emplace_back(newTag);

        m_pDS->next();
      }
      m_pDS->close();
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load EPG data from the database");
    }
  }
  return result;
}

bool CPVREpgDatabase::GetLastEpgScanTime(int iEpgId, CDateTime *lastScan)
{
  bool bReturn = false;

  CSingleLock lock(m_critSection);
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

bool CPVREpgDatabase::PersistLastEpgScanTime(int iEpgId /* = 0 */, bool bQueueWrite /* = false */)
{
  CSingleLock lock(m_critSection);
  std::string strQuery = PrepareSQL("REPLACE INTO lastepgscan(idEpg, sLastScan) VALUES (%u, '%s');",
      iEpgId, CDateTime::GetCurrentDateTime().GetAsUTCDateTime().GetAsDBDateTime().c_str());

  return bQueueWrite ? QueueInsertQuery(strQuery) : ExecuteQuery(strQuery);
}

int CPVREpgDatabase::Persist(const CPVREpg &epg, bool bQueueWrite /* = false */)
{
  int iReturn(-1);
  std::string strQuery;

  CSingleLock lock(m_critSection);
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

int CPVREpgDatabase::Persist(const CPVREpgInfoTag &tag, bool bSingleUpdate /* = true */)
{
  int iReturn(-1);

  if (tag.EpgID() <= 0)
  {
    CLog::LogF(LOGERROR, "Tag '%s' does not have a valid table", tag.Title(true).c_str());
    return iReturn;
  }

  time_t iStartTime, iEndTime, iFirstAired;
  tag.StartAsUTC().GetAsTime(iStartTime);
  tag.EndAsUTC().GetAsTime(iEndTime);
  tag.FirstAiredAsUTC().GetAsTime(iFirstAired);

  int iBroadcastId = tag.DatabaseID();
  std::string strQuery;

  /* Only store the genre string when needed */
  std::string strGenre = (tag.GenreType() == EPG_GENRE_USE_STRING) ? tag.DeTokenize(tag.Genre()) : "";

  CSingleLock lock(m_critSection);

  if (iBroadcastId < 0)
  {
    strQuery = PrepareSQL("REPLACE INTO epgtags (idEpg, iStartTime, "
        "iEndTime, sTitle, sPlotOutline, sPlot, sOriginalTitle, sCast, sDirector, sWriter, iYear, sIMDBNumber, "
        "sIconPath, iGenreType, iGenreSubType, sGenre, iFirstAired, iParentalRating, iStarRating, bNotify, iSeriesId, "
        "iEpisodeId, iEpisodePart, sEpisodeName, iFlags, sSeriesLink, iBroadcastUid) "
        "VALUES (%u, %u, %u, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %i, '%s', '%s', %i, %i, '%s', %u, %i, %i, %i, %i, %i, %i, '%s', %i, '%s', %i);",
        tag.EpgID(), static_cast<unsigned int>(iStartTime), static_cast<unsigned int>(iEndTime),
        tag.Title(true).c_str(), tag.PlotOutline(true).c_str(), tag.Plot(true).c_str(),
        tag.OriginalTitle(true).c_str(), tag.DeTokenize(tag.Cast()).c_str(), tag.DeTokenize(tag.Directors()).c_str(),
        tag.DeTokenize(tag.Writers()).c_str(), tag.Year(), tag.IMDBNumber().c_str(),
        tag.Icon().c_str(), tag.GenreType(), tag.GenreSubType(), strGenre.c_str(),
        static_cast<unsigned int>(iFirstAired), tag.ParentalRating(), tag.StarRating(), tag.Notify(),
        tag.SeriesNumber(), tag.EpisodeNumber(), tag.EpisodePart(), tag.EpisodeName(true).c_str(), tag.Flags(), tag.SeriesLink().c_str(),
        tag.UniqueBroadcastID());
  }
  else
  {
    strQuery = PrepareSQL("REPLACE INTO epgtags (idEpg, iStartTime, "
        "iEndTime, sTitle, sPlotOutline, sPlot, sOriginalTitle, sCast, sDirector, sWriter, iYear, sIMDBNumber, "
        "sIconPath, iGenreType, iGenreSubType, sGenre, iFirstAired, iParentalRating, iStarRating, bNotify, iSeriesId, "
        "iEpisodeId, iEpisodePart, sEpisodeName, iFlags, sSeriesLink, iBroadcastUid, idBroadcast) "
        "VALUES (%u, %u, %u, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %i, '%s', '%s', %i, %i, '%s', %u, %i, %i, %i, %i, %i, %i, '%s', %i, '%s', %i, %i);",
        tag.EpgID(), static_cast<unsigned int>(iStartTime), static_cast<unsigned int>(iEndTime),
        tag.Title(true).c_str(), tag.PlotOutline(true).c_str(), tag.Plot(true).c_str(),
        tag.OriginalTitle(true).c_str(), tag.DeTokenize(tag.Cast()).c_str(), tag.DeTokenize(tag.Directors()).c_str(),
        tag.DeTokenize(tag.Writers()).c_str(), tag.Year(), tag.IMDBNumber().c_str(),
        tag.Icon().c_str(), tag.GenreType(), tag.GenreSubType(), strGenre.c_str(),
        static_cast<unsigned int>(iFirstAired), tag.ParentalRating(), tag.StarRating(), tag.Notify(),
        tag.SeriesNumber(), tag.EpisodeNumber(), tag.EpisodePart(), tag.EpisodeName(true).c_str(), tag.Flags(), tag.SeriesLink().c_str(),
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

int CPVREpgDatabase::GetLastEPGId(void)
{
  CSingleLock lock(m_critSection);
  std::string strQuery = PrepareSQL("SELECT MAX(idEpg) FROM epg");
  std::string strValue = GetSingleValue(strQuery);
  if (!strValue.empty())
    return atoi(strValue.c_str());
  return 0;
}
