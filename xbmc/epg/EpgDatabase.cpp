/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "AdvancedSettings.h"
#include "settings/VideoSettings.h"
#include "utils/log.h"

#include "EpgDatabase.h"
#include "EpgContainer.h"

using namespace std;
using namespace dbiplus;

CEpgDatabase::CEpgDatabase(void)
{
  lastScanTime.SetValid(false);
}

CEpgDatabase::~CEpgDatabase(void)
{
}

bool CEpgDatabase::Open()
{
  return CDatabase::Open(g_advancedSettings.m_databaseEpg);
}

bool CEpgDatabase::CreateTables()
{
  bool bReturn = false;

  try
  {
    CDatabase::CreateTables();

    CLog::Log(LOGINFO, "EpgDB - %s - creating tables", __FUNCTION__);

    CLog::Log(LOGDEBUG, "EpgDB - %s - creating table 'epg'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE epg ("
          "idEpg           integer primary key, "
          "sName           text,"
          "sScraperName    text"
        ")"
    );

    CLog::Log(LOGDEBUG, "EpgDB - %s - creating table 'epgtags'", __FUNCTION__);
    m_pDS->exec(
        "CREATE TABLE epgtags ("
          "idBroadcast     integer primary key, "
          "iBroadcastUid   integer, "
          "idEpg           integer, "
          "sTitle          text, "
          "sPlotOutline    text, "
          "sPlot           text, "
          "iStartTime      integer, "
          "iEndTime        integer, "
          "iGenreType      integer, "
          "iGenreSubType   integer, "
          "iFirstAired     integer, "
          "iParentalRating integer, "
          "iStarRating     integer, "
          "bNotify         bool, "
          "sSeriesId       text, "
          "sEpisodeId      text, "
          "sEpisodePart    text, "
          "sEpisodeName    text"
        ");"
    );
    m_pDS->exec("CREATE UNIQUE INDEX idx_epg_idEpg_iStartTime on epgtags(idEpg, iStartTime desc);");
    m_pDS->exec("CREATE INDEX idx_epg_iBroadcastUid on epgtags(iBroadcastUid);");
    m_pDS->exec("CREATE INDEX idx_epg_idEpg on epgtags(idEpg);");
    m_pDS->exec("CREATE INDEX idx_epg_iStartTime on epgtags(iStartTime);");
    m_pDS->exec("CREATE INDEX idx_epg_iEndTime on epgtags(iEndTime);");

    // TODO keep separate value per epg table collection
    CLog::Log(LOGDEBUG, "EpgDB - %s - creating table 'lastepgscan'", __FUNCTION__);
    m_pDS->exec("CREATE TABLE lastepgscan ("
          "idEpg integer primary key, "
          "iLastScan integer"
        ")"
    );

    bReturn = true;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "EpgDB - %s - unable to create EPG tables:%i",
        __FUNCTION__, (int)GetLastError());
    bReturn = false;
  }

  return bReturn;
}

bool CEpgDatabase::UpdateOldVersion(int iVersion)
{
  return true;
}

bool CEpgDatabase::EraseEpg()
{
  bool bReturn = false;
  CLog::Log(LOGDEBUG, "EpgDB - %s - deleting all EPG data from the database", __FUNCTION__);

  bReturn = DeleteValues("epg") || bReturn;
  bReturn = DeleteValues("epgtags") || bReturn;
  bReturn = DeleteValues("lastepgscan") || bReturn;
  lastScanTime.SetValid(false);

  return bReturn;
}

bool CEpgDatabase::EraseEpgForTable(const CEpg &table, const CDateTime &start /* = NULL */, const CDateTime &end /* = NULL */)
{
  /* invalid channel */
  if (table.EpgID() <= 0)
  {
    CLog::Log(LOGERROR, "EpgDB - %s - invalid channel id: %d",
        __FUNCTION__, table.EpgID());
    return false;
  }

  CLog::Log(LOGDEBUG, "EpgDB - %s - clearing the EPG '%d'",
      __FUNCTION__, table.EpgID());

  CStdString strWhereClause;
  strWhereClause = FormatSQL("idEpg = %u", table.EpgID());

  if (start != NULL)
  {
    time_t iStartTime;
    start.GetAsTime(iStartTime);
    strWhereClause.append(FormatSQL(" AND iStartTime < %u", iStartTime).c_str());
  }

  if (end != NULL)
  {
    time_t iEndTime;
    end.GetAsTime(iEndTime);
    strWhereClause.append(FormatSQL(" AND iEndTime > %u", iEndTime).c_str());
  }

  return DeleteValues("epg", strWhereClause);
}

bool CEpgDatabase::EraseOldEpgEntries()
{
  time_t iYesterday;
  CDateTime yesterday = CDateTime::GetCurrentDateTime() - CDateTimeSpan(1, 0, 0, 0);
  yesterday.GetAsTime(iYesterday);
  CStdString strWhereClause = FormatSQL("iEndTime < %u", iYesterday);

  return DeleteValues("epgtags", strWhereClause);
}

bool CEpgDatabase::RemoveEpgEntry(const CEpgInfoTag &tag)
{
  /* invalid tag */
  if (tag.BroadcastId() <= 0)
  {
    CLog::Log(LOGERROR, "EpgDB - %s - invalid EPG tag", __FUNCTION__);
    return false;
  }

  CStdString strWhereClause = FormatSQL("idBroadcast = %u", tag.BroadcastId());
  return DeleteValues("epgtags", strWhereClause);
}

CDateTime CEpgDatabase::GetEpgDataStart(long iEpgId /* = -1 */)
{
  time_t iFirstProgramme;
  CStdString strWhereClause;

  if (iEpgId > 0)
    strWhereClause = FormatSQL("idEpg = '%u'", iEpgId);

  CStdString strReturn = GetSingleValue("epg", "iStartTime", strWhereClause, "iStartTime ASC");
  if (!strReturn.IsEmpty())
  {
    iFirstProgramme = atoi(strReturn);
    CDateTime firstProgramme(iFirstProgramme);

    if (firstProgramme.IsValid())
      return firstProgramme;
  }

  return CDateTime::GetCurrentDateTime();
}

int CEpgDatabase::GetEpg(CEpg *epg, const CDateTime &start /* = NULL */, const CDateTime &end /* = NULL */)
{
  int iReturn = -1;

  CStdString strWhereClause;
  strWhereClause = FormatSQL("idEpg = %u", epg->EpgID());

  if (start != NULL)
  {
    time_t iStartTime;
    start.GetAsTime(iStartTime);
    strWhereClause.append(FormatSQL(" AND iStartTime < %u", iStartTime).c_str());
  }

  if (end != NULL)
  {
    time_t iEndTime;
    end.GetAsTime(iEndTime);
    strWhereClause.append(FormatSQL(" AND iEndTime > %u", iEndTime).c_str());
  }

  CStdString strQuery;
  strQuery.Format("SELECT * FROM epgtags WHERE %s ORDER BY iStartTime ASC;", strWhereClause.c_str());

  int iNumRows = ResultQuery(strQuery);

  if (iNumRows > 0)
  {
    try
    {
      while (!m_pDS->eof())
      {
        CEpgInfoTag newTag;

        time_t iStartTime, iEndTime, iFirstAired;
        iStartTime = (time_t) m_pDS->fv("iStartTime").get_asInt();
        CDateTime startTime(iStartTime);
        newTag.SetStart(startTime);

        iEndTime = (time_t) m_pDS->fv("iEndTime").get_asInt();
        CDateTime endTime(iEndTime);
        newTag.SetEnd(endTime);

        iFirstAired = (time_t) m_pDS->fv("iFirstAired").get_asInt();
        CDateTime firstAired(iFirstAired);
        newTag.SetFirstAired(firstAired);
        newTag.SetUniqueBroadcastID(m_pDS->fv("iBroadcastUid").get_asInt());
        newTag.SetBroadcastId      (m_pDS->fv("idBroadcast").get_asInt());
        newTag.SetTitle            (m_pDS->fv("sTitle").get_asString().c_str());
        newTag.SetPlotOutline      (m_pDS->fv("sPlotOutline").get_asString().c_str());
        newTag.SetPlot             (m_pDS->fv("sPlot").get_asString().c_str());
        newTag.SetGenre            (m_pDS->fv("iGenreType").get_asInt(),
                                    m_pDS->fv("iGenreSubType").get_asInt());
        newTag.SetParentalRating   (m_pDS->fv("iParentalRating").get_asInt());
        newTag.SetStarRating       (m_pDS->fv("iStarRating").get_asInt());
        newTag.SetNotify           (m_pDS->fv("bNotify").get_asBool());
        newTag.SetEpisodeNum       (m_pDS->fv("sEpisodeId").get_asString().c_str());
        newTag.SetEpisodePart      (m_pDS->fv("sEpisodePart").get_asString().c_str());
        newTag.SetEpisodeName      (m_pDS->fv("sEpisodeName").get_asString().c_str());
        newTag.SetSeriesNum        (m_pDS->fv("sSeriesId").get_asString().c_str());

        epg->UpdateEntry(newTag, false);
        m_pDS->next();
        ++iReturn;
      }
    }
    catch (...)
    {
      CLog::Log(LOGERROR, "%s - couldn't load EPG data from the database", __FUNCTION__);
    }
  }
  return iReturn;
}

CDateTime CEpgDatabase::GetEpgDataEnd(long iEpgId /* = -1 */)
{
  time_t iLastProgramme;
  CStdString strWhereClause;

  if (iEpgId > 0)
    strWhereClause = FormatSQL("idEpg = '%u'", iEpgId);

  CStdString strReturn = GetSingleValue("epgtags", "iEndTime", strWhereClause, "iEndTime DESC");
  if (!strReturn.IsEmpty())
  {
    iLastProgramme = atoi(strReturn);
    CDateTime lastProgramme(iLastProgramme);

    if (lastProgramme.IsValid())
      return lastProgramme;
  }

  return CDateTime::GetCurrentDateTime();
}

CDateTime CEpgDatabase::GetLastEpgScanTime()
{
  if (lastScanTime.IsValid())
    return lastScanTime;

  CStdString strValue = GetSingleValue("lastepgscan", "iLastScan", "idEpg = 0");

  if (strValue.IsEmpty())
    return -1;

  time_t iLastScan = atoi(strValue.c_str());
  CDateTime lastScan(iLastScan);
  lastScanTime = lastScan;

  return lastScan;
}

bool CEpgDatabase::UpdateLastEpgScanTime(void)
{
  CDateTime now = CDateTime::GetCurrentDateTime();
  CLog::Log(LOGDEBUG, "EpgDB - %s - updating last scan time to '%s'",
      __FUNCTION__, now.GetAsDBDateTime().c_str());
  lastScanTime = now;

  bool bReturn = true;
  time_t iLastScan;
  now.GetAsTime(iLastScan);
  CStdString strQuery = FormatSQL("REPLACE INTO lastepgscan(idEpg, iLastScan) VALUES (0, %u);",
      iLastScan);

  bReturn = ExecuteQuery(strQuery);

  return bReturn;
}

int CEpgDatabase::Persist(const CEpg &epg, bool bSingleUpdate /* = true */, bool bLastUpdate /* = false */)
{
  bool iReturn = -1;

  CStdString strQuery;
  if (epg.EpgID() > 0)
  {
    strQuery = FormatSQL("REPLACE INTO epg (idEpg, sName, sScraperName) "
        "VALUES (%u, '%s', '%s');", epg.EpgID(), epg.Name().c_str(), epg.ScraperName().c_str());
  }
  else
  {
    strQuery = FormatSQL("REPLACE INTO epg (sName, sScraperName) "
        "VALUES ('%s', '%s');"), epg.Name().c_str(), epg.ScraperName().c_str();
  }

  if (bSingleUpdate)
  {
    if (ExecuteQuery(strQuery))
      iReturn = m_pDS->lastinsertid();
  }
  else
  {
    if (QueueInsertQuery(strQuery))
      iReturn = 0;

    if (bLastUpdate)
      CommitInsertQueries();
  }

  return iReturn;
}

bool CEpgDatabase::UpdateEpgEntry(const CEpgInfoTag &tag, bool bSingleUpdate /* = true */, bool bLastUpdate /* = false */)
{
  int bReturn = false;

  CEpg *epg = tag.GetTable();
  if (!epg || epg->EpgID() <= 0)
  {
    CLog::Log(LOGERROR, "%s - tag '%s' does not have a valid table", __FUNCTION__, tag.Title().c_str());
    return bReturn;
  }

  time_t iStartTime, iEndTime, iFirstAired;
  tag.Start().GetAsTime(iStartTime);
  tag.End().GetAsTime(iEndTime);
  tag.FirstAired().GetAsTime(iFirstAired);
  int iEpgId = epg->EpgID();

  int iBroadcastId = tag.BroadcastId();
  if (iBroadcastId <= 0)
  {
    CStdString strWhereClause;
    if (tag.UniqueBroadcastID() > 0)
    {
      strWhereClause = FormatSQL("(iBroadcastUid = '%u' OR iStartTime = %u) AND idEpg = %u",
          tag.UniqueBroadcastID(), iStartTime, iEpgId);
    }
    else
    {
      strWhereClause = FormatSQL("iStartTime = %u AND idEpg = '%u'",
          iStartTime, iEpgId);
    }
    CStdString strValue = GetSingleValue("epgtags", "idBroadcast", strWhereClause);

    if (!strValue.IsEmpty())
      iBroadcastId = atoi(strValue);
  }

  CStdString strQuery;

  if (iBroadcastId < 0)
  {
    strQuery = FormatSQL("INSERT INTO epgtags (idEpg, iStartTime, "
        "iEndTime, sTitle, sPlotOutline, sPlot, iGenreType, iGenreSubType, "
        "iFirstAired, iParentalRating, iStarRating, bNotify, sSeriesId, "
        "sEpisodeId, sEpisodePart, sEpisodeName, iBroadcastUid) "
        "VALUES (%u, %u, %u, '%s', '%s', '%s', %i, %i, %u, %i, %i, %i, '%s', '%s', '%s', '%s', %i);",
        iEpgId, iStartTime, iEndTime,
        tag.Title().c_str(), tag.PlotOutline().c_str(), tag.Plot().c_str(), tag.GenreType(), tag.GenreSubType(),
        iFirstAired, tag.ParentalRating(), tag.StarRating(), tag.Notify(),
        tag.SeriesNum().c_str(), tag.EpisodeNum().c_str(), tag.EpisodePart().c_str(), tag.EpisodeName().c_str(),
        tag.UniqueBroadcastID());
  }
  else
  {
    strQuery = FormatSQL("REPLACE INTO epgtags (idEpg, iStartTime, "
        "iEndTime, sTitle, sPlotOutline, sPlot, iGenreType, iGenreSubType, "
        "iFirstAired, iParentalRating, iStarRating, bNotify, sSeriesId, "
        "sEpisodeId, sEpisodePart, sEpisodeName, iBroadcastUid, idBroadcast) "
        "VALUES (%u, %u, %u, '%s', '%s', '%s', %i, %i, %u, %i, %i, %i, '%s', '%s', '%s', '%s', %i, %i);",
        iEpgId, iStartTime, iEndTime,
        tag.Title().c_str(), tag.PlotOutline().c_str(), tag.Plot().c_str(), tag.GenreType(), tag.GenreSubType(),
        tag.FirstAired().GetAsDBDateTime().c_str(), tag.ParentalRating(), tag.StarRating(), tag.Notify(),
        tag.SeriesNum().c_str(), tag.EpisodeNum().c_str(), tag.EpisodePart().c_str(), tag.EpisodeName().c_str(),
        tag.UniqueBroadcastID(), iBroadcastId);
  }

  if (bSingleUpdate)
  {
    bReturn = ExecuteQuery(strQuery);
  }
  else
  {
    bReturn = QueueInsertQuery(strQuery);

    if (bLastUpdate)
      CommitInsertQueries();
  }

  if ((bSingleUpdate || bLastUpdate) && GetEpgDataEnd(iEpgId) > tag.End())
    EraseEpgForTable(*epg, NULL, tag.End());

  return bReturn;
}
