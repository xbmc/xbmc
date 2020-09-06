/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EpgDatabase.h"

#include "ServiceBroker.h"
#include "dbwrappers/dataset.h"
#include "pvr/epg/Epg.h"
#include "pvr/epg/EpgInfoTag.h"
#include "pvr/epg/EpgSearchData.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

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

void CPVREpgDatabase::CreateTables()
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
        "sFirstAired     varchar(32), "
        "iParentalRating integer, "
        "iStarRating     integer, "
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

  if (iVersion < 13)
  {
    const bool isMySQL = StringUtils::EqualsNoCase(
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseEpg.type, "mysql");

    m_pDS->exec(
        "CREATE TABLE epgtags_new ("
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
        "sFirstAired     varchar(32), "
        "iParentalRating integer, "
        "iStarRating     integer, "
        "iSeriesId       integer, "
        "iEpisodeId      integer, "
        "iEpisodePart    integer, "
        "sEpisodeName    varchar(128), "
        "iFlags          integer, "
        "sSeriesLink     varchar(255)"
        ")"
    );

    m_pDS->exec(
        "INSERT INTO epgtags_new ("
        "idBroadcast, "
        "iBroadcastUid, "
        "idEpg, "
        "sTitle, "
        "sPlotOutline, "
        "sPlot, "
        "sOriginalTitle, "
        "sCast, "
        "sDirector, "
        "sWriter, "
        "iYear, "
        "sIMDBNumber, "
        "sIconPath, "
        "iStartTime, "
        "iEndTime, "
        "iGenreType, "
        "iGenreSubType, "
        "sGenre, "
        "sFirstAired, "
        "iParentalRating, "
        "iStarRating, "
        "iSeriesId, "
        "iEpisodeId, "
        "iEpisodePart, "
        "sEpisodeName, "
        "iFlags, "
        "sSeriesLink"
        ") "
        "SELECT "
        "idBroadcast, "
        "iBroadcastUid, "
        "idEpg, "
        "sTitle, "
        "sPlotOutline, "
        "sPlot, "
        "sOriginalTitle, "
        "sCast, "
        "sDirector, "
        "sWriter, "
        "iYear, "
        "sIMDBNumber, "
        "sIconPath, "
        "iStartTime, "
        "iEndTime, "
        "iGenreType, "
        "iGenreSubType, "
        "sGenre, "
        "'' AS sFirstAired, "
        "iParentalRating, "
        "iStarRating, "
        "iSeriesId, "
        "iEpisodeId, "
        "iEpisodePart, "
        "sEpisodeName, "
        "iFlags, "
        "sSeriesLink "
        "FROM epgtags"
    );

    if (isMySQL)
      m_pDS->exec(
        "UPDATE epgtags_new INNER JOIN epgtags ON epgtags_new.idBroadcast = epgtags.idBroadcast "
        "SET epgtags_new.sFirstAired = DATE(FROM_UNIXTIME(epgtags.iFirstAired)) "
        "WHERE epgtags.iFirstAired > 0"
      );
    else
      m_pDS->exec(
        "UPDATE epgtags_new SET sFirstAired = "
        "COALESCE((SELECT STRFTIME('%Y-%m-%d', iFirstAired, 'UNIXEPOCH') "
        "FROM epgtags WHERE epgtags.idBroadcast = epgtags_new.idBroadcast "
        "AND epgtags.iFirstAired > 0), '')"
      );

    m_pDS->exec("DROP TABLE epgtags");
    m_pDS->exec("ALTER TABLE epgtags_new RENAME TO epgtags");
  }
}

bool CPVREpgDatabase::DeleteEpg()
{
  bool bReturn(false);
  CLog::LogFC(LOGDEBUG, LOGEPG, "Deleting all EPG data from the database");

  CSingleLock lock(m_critSection);

  bReturn = DeleteValues("epg") || bReturn;
  bReturn = DeleteValues("epgtags") || bReturn;
  bReturn = DeleteValues("lastepgscan") || bReturn;

  return bReturn;
}

bool CPVREpgDatabase::Delete(const CPVREpg& table)
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

bool CPVREpgDatabase::QueueDeleteTagQuery(const CPVREpgInfoTag& tag)
{
  /* tag without a database ID was not persisted */
  if (tag.DatabaseID() <= 0)
    return false;

  Filter filter;

  CSingleLock lock(m_critSection);
  filter.AppendWhere(PrepareSQL("idBroadcast = %u", tag.DatabaseID()));

  std::string strQuery;
  BuildSQL(PrepareSQL("DELETE FROM %s ", "epgtags"), filter, strQuery);
  return QueueDeleteQuery(strQuery);
}

std::vector<std::shared_ptr<CPVREpg>> CPVREpgDatabase::GetAll()
{
  std::vector<std::shared_ptr<CPVREpg>> result;

  CSingleLock lock(m_critSection);
  std::string strQuery = PrepareSQL("SELECT idEpg, sName, sScraperName FROM epg;");
  if (ResultQuery(strQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        int iEpgID = m_pDS->fv("idEpg").get_asInt();
        std::string strName = m_pDS->fv("sName").get_asString().c_str();
        std::string strScraperName = m_pDS->fv("sScraperName").get_asString().c_str();

        result.emplace_back(new CPVREpg(iEpgID, strName, strScraperName, shared_from_this()));
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

std::shared_ptr<CPVREpgInfoTag> CPVREpgDatabase::CreateEpgTag(
    const std::unique_ptr<dbiplus::Dataset>& pDS)
{
  if (!pDS->eof())
  {
    const std::shared_ptr<CPVREpgInfoTag> newTag(new CPVREpgInfoTag());

    time_t iStartTime;
    iStartTime = static_cast<time_t>(m_pDS->fv("iStartTime").get_asInt());
    const CDateTime startTime(iStartTime);
    newTag->m_startTime = startTime;

    time_t iEndTime = static_cast<time_t>(m_pDS->fv("iEndTime").get_asInt());
    const CDateTime endTime(iEndTime);
    newTag->m_endTime = endTime;

    const std::string sFirstAired = m_pDS->fv("sFirstAired").get_asString();
    if (sFirstAired.length() > 0)
      newTag->m_firstAired.SetFromW3CDate(sFirstAired);

    int iBroadcastUID = m_pDS->fv("iBroadcastUid").get_asInt();
    // Compat: null value for broadcast uid changed from numerical -1 to 0 with PVR Addon API v4.0.0
    newTag->m_iUniqueBroadcastID = iBroadcastUID == -1 ? EPG_TAG_INVALID_UID : iBroadcastUID;

    newTag->m_iEpgID = m_pDS->fv("idEpg").get_asInt();
    newTag->m_iDatabaseID = m_pDS->fv("idBroadcast").get_asInt();
    newTag->m_strTitle = m_pDS->fv("sTitle").get_asString().c_str();
    newTag->m_strPlotOutline = m_pDS->fv("sPlotOutline").get_asString().c_str();
    newTag->m_strPlot = m_pDS->fv("sPlot").get_asString().c_str();
    newTag->m_strOriginalTitle = m_pDS->fv("sOriginalTitle").get_asString().c_str();
    newTag->m_cast = newTag->Tokenize(m_pDS->fv("sCast").get_asString());
    newTag->m_directors = newTag->Tokenize(m_pDS->fv("sDirector").get_asString());
    newTag->m_writers = newTag->Tokenize(m_pDS->fv("sWriter").get_asString());
    newTag->m_iYear = m_pDS->fv("iYear").get_asInt();
    newTag->m_strIMDBNumber = m_pDS->fv("sIMDBNumber").get_asString().c_str();
    newTag->m_iParentalRating = m_pDS->fv("iParentalRating").get_asInt();
    newTag->m_iStarRating = m_pDS->fv("iStarRating").get_asInt();
    newTag->m_iEpisodeNumber = m_pDS->fv("iEpisodeId").get_asInt();
    newTag->m_iEpisodePart = m_pDS->fv("iEpisodePart").get_asInt();
    newTag->m_strEpisodeName = m_pDS->fv("sEpisodeName").get_asString().c_str();
    newTag->m_iSeriesNumber = m_pDS->fv("iSeriesId").get_asInt();
    newTag->m_strIconPath = m_pDS->fv("sIconPath").get_asString().c_str();
    newTag->m_iFlags = m_pDS->fv("iFlags").get_asInt();
    newTag->m_strSeriesLink = m_pDS->fv("sSeriesLink").get_asString().c_str();

    newTag->SetGenre(m_pDS->fv("iGenreType").get_asInt(), m_pDS->fv("iGenreSubType").get_asInt(),
                     m_pDS->fv("sGenre").get_asString().c_str());
    newTag->UpdatePath();

    return newTag;
  }
  return {};
}

CDateTime CPVREpgDatabase::GetFirstStartTime(int iEpgID)
{
  CSingleLock lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("SELECT MIN(iStartTime) FROM epgtags WHERE idEpg = %u;", iEpgID);
  std::string strValue = GetSingleValue(strQuery);
  if (!strValue.empty())
    return CDateTime(static_cast<time_t>(std::atoi(strValue.c_str())));

  return {};
}

CDateTime CPVREpgDatabase::GetLastEndTime(int iEpgID)
{
  CSingleLock lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("SELECT MAX(iEndTime) FROM epgtags WHERE idEpg = %u;", iEpgID);
  std::string strValue = GetSingleValue(strQuery);
  if (!strValue.empty())
    return CDateTime(static_cast<time_t>(std::atoi(strValue.c_str())));

  return {};
}

CDateTime CPVREpgDatabase::GetMinStartTime(int iEpgID, const CDateTime& minStart)
{
  time_t t;
  minStart.GetAsTime(t);

  CSingleLock lock(m_critSection);
  const std::string strQuery = PrepareSQL("SELECT MIN(iStartTime) "
                                          "FROM epgtags "
                                          "WHERE idEpg = %u AND iStartTime > %u;",
                                          iEpgID, static_cast<unsigned int>(t));
  std::string strValue = GetSingleValue(strQuery);
  if (!strValue.empty())
    return CDateTime(static_cast<time_t>(std::atoi(strValue.c_str())));

  return {};
}

CDateTime CPVREpgDatabase::GetMaxEndTime(int iEpgID, const CDateTime& maxEnd)
{
  time_t t;
  maxEnd.GetAsTime(t);

  CSingleLock lock(m_critSection);
  const std::string strQuery = PrepareSQL("SELECT MAX(iEndTime) "
                                          "FROM epgtags "
                                          "WHERE idEpg = %u AND iEndTime <= %u;",
                                          iEpgID, static_cast<unsigned int>(t));
  std::string strValue = GetSingleValue(strQuery);
  if (!strValue.empty())
    return CDateTime(static_cast<time_t>(std::atoi(strValue.c_str())));

  return {};
}

namespace
{

CDateTime ConvertLocalTimeToUTC(const CDateTime& local)
{
  time_t time = 0;
  local.GetAsTime(time);

  struct tm* tms;

  // obtain dst flag for given datetime
#ifdef HAVE_LOCALTIME_R
  struct tm loc_buf;
  tms = localtime_r(&time, &loc_buf);
#else
  tms = localtime(&time);
#endif

  int isdst = tms->tm_isdst;

#ifdef HAVE_GMTIME_R
  struct tm gm_buf;
  tms = gmtime_r(&time, &gm_buf);
#else
  tms = gmtime(&time);
#endif

  tms->tm_isdst = isdst;
  return CDateTime(mktime(tms));
}

class CSearchTermConverter
{
public:
  CSearchTermConverter(const std::string& strSearchTerm) { Parse(strSearchTerm); }

  std::string ToSQL(const std::string& strFieldName) const
  {
    std::string result = "(";

    for (auto it = m_fragments.cbegin(); it != m_fragments.cend();)
    {
      result += (*it);

      ++it;
      if (it != m_fragments.cend())
        result += strFieldName;
    }

    StringUtils::TrimRight(result);
    result += ")";
    return result;
  }

private:
  void Parse(const std::string& strSearchTerm)
  {
    std::string strParsedSearchTerm(strSearchTerm);
    StringUtils::Trim(strParsedSearchTerm);

    std::string strFragment;

    bool bNextOR = false;
    while (!strParsedSearchTerm.empty())
    {
      StringUtils::TrimLeft(strParsedSearchTerm);

      if (StringUtils::StartsWith(strParsedSearchTerm, "!") ||
          StringUtils::StartsWithNoCase(strParsedSearchTerm, "not"))
      {
        std::string strDummy;
        GetAndCutNextTerm(strParsedSearchTerm, strDummy);
        strFragment += " NOT ";
        bNextOR = false;
      }
      else if (StringUtils::StartsWith(strParsedSearchTerm, "+") ||
               StringUtils::StartsWithNoCase(strParsedSearchTerm, "and"))
      {
        std::string strDummy;
        GetAndCutNextTerm(strParsedSearchTerm, strDummy);
        strFragment += " AND ";
        bNextOR = false;
      }
      else if (StringUtils::StartsWith(strParsedSearchTerm, "|") ||
               StringUtils::StartsWithNoCase(strParsedSearchTerm, "or"))
      {
        std::string strDummy;
        GetAndCutNextTerm(strParsedSearchTerm, strDummy);
        strFragment += " OR ";
        bNextOR = false;
      }
      else
      {
        std::string strTerm;
        GetAndCutNextTerm(strParsedSearchTerm, strTerm);
        if (!strTerm.empty())
        {
          if (bNextOR && !m_fragments.empty())
            strFragment += " OR "; // default operator

          strFragment += "(UPPER(";

          m_fragments.emplace_back(strFragment);
          strFragment.clear();

          strFragment += ") LIKE UPPER('%";
          StringUtils::Replace(strTerm, "'", "''"); // escape '
          strFragment += strTerm;
          strFragment += "%')) ";

          bNextOR = true;
        }
        else
        {
          break;
        }
      }

      StringUtils::TrimLeft(strParsedSearchTerm);
    }

    if (!strFragment.empty())
      m_fragments.emplace_back(strFragment);
  }

  static void GetAndCutNextTerm(std::string& strSearchTerm, std::string& strNextTerm)
  {
    std::string strFindNext(" ");

    if (StringUtils::EndsWith(strSearchTerm, "\""))
    {
      strSearchTerm.erase(0, 1);
      strFindNext = "\"";
    }

    const size_t iNextPos = strSearchTerm.find(strFindNext);
    if (iNextPos != std::string::npos)
    {
      strNextTerm = strSearchTerm.substr(0, iNextPos);
      strSearchTerm.erase(0, iNextPos + 1);
    }
    else
    {
      strNextTerm = strSearchTerm;
      strSearchTerm.clear();
    }
  }

  std::vector<std::string> m_fragments;
};

} // unnamed namespace

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgDatabase::GetEpgTags(
    const PVREpgSearchData& searchData)
{
  CSingleLock lock(m_critSection);

  std::string strQuery = PrepareSQL("SELECT * FROM epgtags");

  Filter filter;

  /////////////////////////////////////////////////////////////////////////////////////////////
  // broadcast UID
  /////////////////////////////////////////////////////////////////////////////////////////////

  if (searchData.m_iUniqueBroadcastId != EPG_TAG_INVALID_UID)
  {
    filter.AppendWhere(PrepareSQL("iBroadcastUid = %u", searchData.m_iUniqueBroadcastId));
  }

  /////////////////////////////////////////////////////////////////////////////////////////////
  // min start datetime
  /////////////////////////////////////////////////////////////////////////////////////////////

  const CDateTime minStartTime = ConvertLocalTimeToUTC(searchData.m_startDateTime);
  time_t minStart;
  minStartTime.GetAsTime(minStart);
  filter.AppendWhere(PrepareSQL("iStartTime >= %u", static_cast<unsigned int>(minStart)));

  /////////////////////////////////////////////////////////////////////////////////////////////
  // max end datetime
  /////////////////////////////////////////////////////////////////////////////////////////////

  const CDateTime maxEndTime = ConvertLocalTimeToUTC(searchData.m_endDateTime);
  time_t maxEnd;
  maxEndTime.GetAsTime(maxEnd);
  filter.AppendWhere(PrepareSQL("iEndTime <= %u", static_cast<unsigned int>(maxEnd)));

  /////////////////////////////////////////////////////////////////////////////////////////////
  // genre type
  /////////////////////////////////////////////////////////////////////////////////////////////

  if (searchData.m_iGenreType != EPG_SEARCH_UNSET)
  {
    filter.AppendWhere(PrepareSQL("(iGenreType < %u) OR (iGenreType > %u) OR (iGenreType = %u)",
                                  EPG_EVENT_CONTENTMASK_MOVIEDRAMA,
                                  EPG_EVENT_CONTENTMASK_USERDEFINED, searchData.m_iGenreType));
  }

  /////////////////////////////////////////////////////////////////////////////////////////////
  // search term
  /////////////////////////////////////////////////////////////////////////////////////////////

  if (!searchData.m_strSearchTerm.empty())
  {
    const CSearchTermConverter conv(searchData.m_strSearchTerm);

    // title
    std::string strWhere = conv.ToSQL("sTitle");

    // plot outline
    strWhere += " OR ";
    strWhere += conv.ToSQL("sPlotOutline");

    if (searchData.m_bSearchInDescription)
    {
      // plot
      strWhere += " OR ";
      strWhere += conv.ToSQL("sPlot");
    }

    filter.AppendWhere(strWhere);
  }

  if (BuildSQL(strQuery, filter, strQuery))
  {
    try
    {
      if (m_pDS->query(strQuery))
      {
        std::vector<std::shared_ptr<CPVREpgInfoTag>> tags;
        while (!m_pDS->eof())
        {
          tags.emplace_back(CreateEpgTag(m_pDS));
          m_pDS->next();
        }
        m_pDS->close();
        return tags;
      }
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load tags for given search criteria");
    }
  }

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgDatabase::GetEpgTagByUniqueBroadcastID(
    int iEpgID, unsigned int iUniqueBroadcastId)
{
  CSingleLock lock(m_critSection);
  const std::string strQuery = PrepareSQL("SELECT * "
                                          "FROM epgtags "
                                          "WHERE idEpg = %u AND iBroadcastUid = %u;",
                                          iEpgID, iUniqueBroadcastId);

  if (ResultQuery(strQuery))
  {
    try
    {
      const std::shared_ptr<CPVREpgInfoTag> tag = CreateEpgTag(m_pDS);
      m_pDS->close();
      return tag;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load EPG tag with unique broadcast ID (%u) from the database",
                 iUniqueBroadcastId);
    }
  }

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgDatabase::GetEpgTagByStartTime(int iEpgID,
                                                                      const CDateTime& startTime)
{
  time_t start;
  startTime.GetAsTime(start);

  CSingleLock lock(m_critSection);
  const std::string strQuery = PrepareSQL("SELECT * "
                                          "FROM epgtags "
                                          "WHERE idEpg = %u AND iStartTime = %u;",
                                          iEpgID, static_cast<unsigned int>(start));

  if (ResultQuery(strQuery))
  {
    try
    {
      const std::shared_ptr<CPVREpgInfoTag> tag = CreateEpgTag(m_pDS);
      m_pDS->close();
      return tag;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load EPG tag with start time (%s) from the database",
                 startTime.GetAsDBDateTime());
    }
  }

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgDatabase::GetEpgTagByMinStartTime(
    int iEpgID, const CDateTime& minStartTime)
{
  time_t minStart;
  minStartTime.GetAsTime(minStart);

  CSingleLock lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("SELECT * "
                 "FROM epgtags "
                 "WHERE idEpg = %u AND iStartTime >= %u ORDER BY iStartTime ASC LIMIT 1;",
                 iEpgID, static_cast<unsigned int>(minStart));

  if (ResultQuery(strQuery))
  {
    try
    {
      const std::shared_ptr<CPVREpgInfoTag> tag = CreateEpgTag(m_pDS);
      m_pDS->close();
      return tag;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load tags with min start time (%u) for EPG (%d)",
                 minStartTime.GetAsDBDateTime(), iEpgID);
    }
  }

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgDatabase::GetEpgTagByMaxEndTime(int iEpgID,
                                                                       const CDateTime& maxEndTime)
{
  time_t maxEnd;
  maxEndTime.GetAsTime(maxEnd);

  CSingleLock lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("SELECT * "
                 "FROM epgtags "
                 "WHERE idEpg = %u AND iEndTime <= %u ORDER BY iStartTime DESC LIMIT 1;",
                 iEpgID, static_cast<unsigned int>(maxEnd));

  if (ResultQuery(strQuery))
  {
    try
    {
      const std::shared_ptr<CPVREpgInfoTag> tag = CreateEpgTag(m_pDS);
      m_pDS->close();
      return tag;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load tags with max end time (%u) for EPG (%d)",
                 maxEndTime.GetAsDBDateTime(), iEpgID);
    }
  }

  return {};
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgDatabase::GetEpgTagsByMinStartMaxEndTime(
    int iEpgID, const CDateTime& minStartTime, const CDateTime& maxEndTime)
{
  time_t minStart;
  minStartTime.GetAsTime(minStart);

  time_t maxEnd;
  maxEndTime.GetAsTime(maxEnd);

  CSingleLock lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("SELECT * "
                 "FROM epgtags "
                 "WHERE idEpg = %u AND iStartTime >= %u AND iEndTime <= %u ORDER BY iStartTime;",
                 iEpgID, static_cast<unsigned int>(minStart), static_cast<unsigned int>(maxEnd));

  if (ResultQuery(strQuery))
  {
    try
    {
      std::vector<std::shared_ptr<CPVREpgInfoTag>> tags;
      while (!m_pDS->eof())
      {
        tags.emplace_back(CreateEpgTag(m_pDS));
        m_pDS->next();
      }
      m_pDS->close();
      return tags;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR,
                 "Could not load tags with min start time (%u) and max end time (%u) for EPG (%d)",
                 minStartTime.GetAsDBDateTime(), maxEndTime.GetAsDBDateTime(), iEpgID);
    }
  }

  return {};
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgDatabase::GetEpgTagsByMinEndMaxStartTime(
    int iEpgID, const CDateTime& minEndTime, const CDateTime& maxStartTime)
{
  time_t minEnd;
  minEndTime.GetAsTime(minEnd);

  time_t maxStart;
  maxStartTime.GetAsTime(maxStart);

  CSingleLock lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("SELECT * "
                 "FROM epgtags "
                 "WHERE idEpg = %u AND iEndTime >= %u AND iStartTime <= %u ORDER BY iStartTime;",
                 iEpgID, static_cast<unsigned int>(minEnd), static_cast<unsigned int>(maxStart));

  if (ResultQuery(strQuery))
  {
    try
    {
      std::vector<std::shared_ptr<CPVREpgInfoTag>> tags;
      while (!m_pDS->eof())
      {
        tags.emplace_back(CreateEpgTag(m_pDS));
        m_pDS->next();
      }
      m_pDS->close();
      return tags;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR,
                 "Could not load tags with min end time (%u) and max start time (%u) for EPG (%d)",
                 minEndTime.GetAsDBDateTime(), maxStartTime.GetAsDBDateTime(), iEpgID);
    }
  }

  return {};
}

bool CPVREpgDatabase::QueueDeleteEpgTagsByMinEndMaxStartTimeQuery(int iEpgID,
                                                                  const CDateTime& minEndTime,
                                                                  const CDateTime& maxStartTime)
{
  time_t minEnd;
  minEndTime.GetAsTime(minEnd);

  time_t maxStart;
  maxStartTime.GetAsTime(maxStart);

  Filter filter;

  CSingleLock lock(m_critSection);
  filter.AppendWhere(PrepareSQL("idEpg = %u AND iEndTime >= %u AND iStartTime <= %u", iEpgID,
                                static_cast<unsigned int>(minEnd),
                                static_cast<unsigned int>(maxStart)));

  std::string strQuery;
  BuildSQL("DELETE FROM epgtags", filter, strQuery);
  return QueueDeleteQuery(strQuery);
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgDatabase::GetAllEpgTags(int iEpgID)
{
  CSingleLock lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("SELECT * FROM epgtags WHERE idEpg = %u ORDER BY iStartTime;", iEpgID);
  if (ResultQuery(strQuery))
  {
    try
    {
      std::vector<std::shared_ptr<CPVREpgInfoTag>> tags;
      while (!m_pDS->eof())
      {
        tags.emplace_back(CreateEpgTag(m_pDS));
        m_pDS->next();
      }
      m_pDS->close();
      return tags;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load tags for EPG (%d)", iEpgID);
    }
  }
  return {};
}

bool CPVREpgDatabase::GetLastEpgScanTime(int iEpgId, CDateTime* lastScan)
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

bool CPVREpgDatabase::QueuePersistLastEpgScanTimeQuery(int iEpgId, const CDateTime& lastScanTime)
{
  CSingleLock lock(m_critSection);
  std::string strQuery = PrepareSQL("REPLACE INTO lastepgscan(idEpg, sLastScan) VALUES (%u, '%s');",
      iEpgId, lastScanTime.GetAsDBDateTime().c_str());

  return QueueInsertQuery(strQuery);
}

int CPVREpgDatabase::Persist(const CPVREpg& epg, bool bQueueWrite)
{
  int iReturn = -1;
  std::string strQuery;

  CSingleLock lock(m_critSection);
  if (epg.EpgID() > 0)
    strQuery = PrepareSQL("REPLACE INTO epg (idEpg, sName, sScraperName) "
                          "VALUES (%u, '%s', '%s');",
                          epg.EpgID(), epg.Name().c_str(), epg.ScraperName().c_str());
  else
    strQuery = PrepareSQL("INSERT INTO epg (sName, sScraperName) "
                          "VALUES ('%s', '%s');",
                          epg.Name().c_str(), epg.ScraperName().c_str());

  if (bQueueWrite)
  {
    if (QueueInsertQuery(strQuery))
      iReturn = epg.EpgID() <= 0 ? 0 : epg.EpgID();
  }
  else
  {
    if (ExecuteQuery(strQuery))
      iReturn = epg.EpgID() <= 0 ? static_cast<int>(m_pDS->lastinsertid()) : epg.EpgID();
  }

  return iReturn;
}

bool CPVREpgDatabase::DeleteEpgTags(int iEpgId, const CDateTime& maxEndTime)
{
  time_t iMaxEndTime;
  maxEndTime.GetAsTime(iMaxEndTime);

  Filter filter;

  CSingleLock lock(m_critSection);
  filter.AppendWhere(
      PrepareSQL("idEpg = %u AND iEndTime < %u", iEpgId, static_cast<unsigned int>(iMaxEndTime)));
  return DeleteValues("epgtags", filter);
}

bool CPVREpgDatabase::DeleteEpgTags(int iEpgId)
{
  Filter filter;

  CSingleLock lock(m_critSection);
  filter.AppendWhere(PrepareSQL("idEpg = %u", iEpgId));
  return DeleteValues("epgtags", filter);
}

bool CPVREpgDatabase::QueuePersistQuery(const CPVREpgInfoTag& tag)
{
  if (tag.EpgID() <= 0)
  {
    CLog::LogF(LOGERROR, "Tag '%s' does not have a valid table", tag.Title().c_str());
    return false;
  }

  time_t iStartTime, iEndTime;
  tag.StartAsUTC().GetAsTime(iStartTime);
  tag.EndAsUTC().GetAsTime(iEndTime);

  std::string sFirstAired;
  if (tag.FirstAired().IsValid())
    sFirstAired = tag.FirstAired().GetAsW3CDate();

  int iBroadcastId = tag.DatabaseID();
  std::string strQuery;

  /* Only store the genre string when needed */
  std::string strGenre = (tag.GenreType() == EPG_GENRE_USE_STRING || tag.GenreSubType() == EPG_GENRE_USE_STRING) ? tag.DeTokenize(tag.Genre()) : "";

  CSingleLock lock(m_critSection);

  if (iBroadcastId < 0)
  {
    strQuery = PrepareSQL("REPLACE INTO epgtags (idEpg, iStartTime, "
        "iEndTime, sTitle, sPlotOutline, sPlot, sOriginalTitle, sCast, sDirector, sWriter, iYear, sIMDBNumber, "
        "sIconPath, iGenreType, iGenreSubType, sGenre, sFirstAired, iParentalRating, iStarRating, iSeriesId, "
        "iEpisodeId, iEpisodePart, sEpisodeName, iFlags, sSeriesLink, iBroadcastUid) "
        "VALUES (%u, %u, %u, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %i, '%s', '%s', %i, %i, '%s', '%s', %i, %i, %i, %i, %i, '%s', %i, '%s', %i);",
        tag.EpgID(), static_cast<unsigned int>(iStartTime), static_cast<unsigned int>(iEndTime),
        tag.Title().c_str(), tag.PlotOutline().c_str(), tag.Plot().c_str(),
        tag.OriginalTitle().c_str(), tag.DeTokenize(tag.Cast()).c_str(), tag.DeTokenize(tag.Directors()).c_str(),
        tag.DeTokenize(tag.Writers()).c_str(), tag.Year(), tag.IMDBNumber().c_str(),
        tag.Icon().c_str(), tag.GenreType(), tag.GenreSubType(), strGenre.c_str(),
        sFirstAired.c_str(), tag.ParentalRating(), tag.StarRating(),
        tag.SeriesNumber(), tag.EpisodeNumber(), tag.EpisodePart(), tag.EpisodeName().c_str(), tag.Flags(), tag.SeriesLink().c_str(),
        tag.UniqueBroadcastID());
  }
  else
  {
    strQuery = PrepareSQL("REPLACE INTO epgtags (idEpg, iStartTime, "
        "iEndTime, sTitle, sPlotOutline, sPlot, sOriginalTitle, sCast, sDirector, sWriter, iYear, sIMDBNumber, "
        "sIconPath, iGenreType, iGenreSubType, sGenre, sFirstAired, iParentalRating, iStarRating, iSeriesId, "
        "iEpisodeId, iEpisodePart, sEpisodeName, iFlags, sSeriesLink, iBroadcastUid, idBroadcast) "
        "VALUES (%u, %u, %u, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %i, '%s', '%s', %i, %i, '%s', '%s', %i, %i, %i, %i, %i, '%s', %i, '%s', %i, %i);",
        tag.EpgID(), static_cast<unsigned int>(iStartTime), static_cast<unsigned int>(iEndTime),
        tag.Title().c_str(), tag.PlotOutline().c_str(), tag.Plot().c_str(),
        tag.OriginalTitle().c_str(), tag.DeTokenize(tag.Cast()).c_str(), tag.DeTokenize(tag.Directors()).c_str(),
        tag.DeTokenize(tag.Writers()).c_str(), tag.Year(), tag.IMDBNumber().c_str(),
        tag.Icon().c_str(), tag.GenreType(), tag.GenreSubType(), strGenre.c_str(),
        sFirstAired.c_str(), tag.ParentalRating(), tag.StarRating(),
        tag.SeriesNumber(), tag.EpisodeNumber(), tag.EpisodePart(), tag.EpisodeName().c_str(), tag.Flags(), tag.SeriesLink().c_str(),
        tag.UniqueBroadcastID(), iBroadcastId);
  }

  QueueInsertQuery(strQuery);
  return true;
}

int CPVREpgDatabase::GetLastEPGId()
{
  CSingleLock lock(m_critSection);
  std::string strQuery = PrepareSQL("SELECT MAX(idEpg) FROM epg");
  std::string strValue = GetSingleValue(strQuery);
  if (!strValue.empty())
    return std::atoi(strValue.c_str());
  return 0;
}
