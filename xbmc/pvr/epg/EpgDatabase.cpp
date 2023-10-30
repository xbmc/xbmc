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
#include "pvr/epg/EpgSearchFilter.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace dbiplus;
using namespace PVR;

bool CPVREpgDatabase::Open()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return CDatabase::Open(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseEpg);
}

void CPVREpgDatabase::Close()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
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

  std::unique_lock<CCriticalSection> lock(m_critSection);

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
        "sSeriesLink     varchar(255), "
        "sParentalRatingCode varchar(64)"
      ")"
  );

  CLog::LogFC(LOGDEBUG, LOGEPG, "Creating table 'lastepgscan'");
  m_pDS->exec("CREATE TABLE lastepgscan ("
        "idEpg integer primary key, "
        "sLastScan varchar(20)"
      ")"
  );

  CLog::LogFC(LOGDEBUG, LOGEPG, "Creating table 'savedsearches'");
  m_pDS->exec("CREATE TABLE savedsearches ("
              "idSearch                  integer primary key,"
              "sTitle                    varchar(255), "
              "sLastExecutedDateTime     varchar(20), "
              "sSearchTerm               varchar(255), "
              "bSearchInDescription      bool, "
              "iGenreType                integer, "
              "sStartDateTime            varchar(20), "
              "sEndDateTime              varchar(20), "
              "bIsCaseSensitive          bool, "
              "iMinimumDuration          integer, "
              "iMaximumDuration          integer, "
              "bIsRadio                  bool, "
              "iClientId                 integer, "
              "iChannelUid               integer, "
              "bIncludeUnknownGenres     bool, "
              "bRemoveDuplicates         bool, "
              "bIgnoreFinishedBroadcasts bool, "
              "bIgnoreFutureBroadcasts   bool, "
              "bFreeToAirOnly            bool, "
              "bIgnorePresentTimers      bool, "
              "bIgnorePresentRecordings  bool,"
              "iChannelGroup             integer"
              ")");
}

void CPVREpgDatabase::CreateAnalytics()
{
  CLog::LogFC(LOGDEBUG, LOGEPG, "Creating EPG database indices");

  std::unique_lock<CCriticalSection> lock(m_critSection);
  m_pDS->exec("CREATE UNIQUE INDEX idx_epg_idEpg_iStartTime on epgtags(idEpg, iStartTime desc);");
  m_pDS->exec("CREATE INDEX idx_epg_iEndTime on epgtags(iEndTime);");
}

void CPVREpgDatabase::UpdateTables(int iVersion)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
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

  if (iVersion < 14)
  {
    m_pDS->exec("ALTER TABLE epgtags ADD sParentalRatingCode varchar(64);");
  }

  if (iVersion < 15)
  {
    m_pDS->exec("CREATE TABLE savedsearches ("
                "idSearch                  integer primary key,"
                "sTitle                    varchar(255), "
                "sLastExecutedDateTime     varchar(20), "
                "sSearchTerm               varchar(255), "
                "bSearchInDescription      bool, "
                "iGenreType                integer, "
                "sStartDateTime            varchar(20), "
                "sEndDateTime              varchar(20), "
                "bIsCaseSensitive          bool, "
                "iMinimumDuration          integer, "
                "iMaximumDuration          integer, "
                "bIsRadio                  bool, "
                "iClientId                 integer, "
                "iChannelUid               integer, "
                "bIncludeUnknownGenres     bool, "
                "bRemoveDuplicates         bool, "
                "bIgnoreFinishedBroadcasts bool, "
                "bIgnoreFutureBroadcasts   bool, "
                "bFreeToAirOnly            bool, "
                "bIgnorePresentTimers      bool, "
                "bIgnorePresentRecordings  bool"
                ")");
  }

  if (iVersion < 16)
  {
    m_pDS->exec("ALTER TABLE savedsearches ADD iChannelGroup integer;");
    m_pDS->exec("UPDATE savedsearches SET iChannelGroup = -1");
  }
}

bool CPVREpgDatabase::DeleteEpg()
{
  bool bReturn(false);
  CLog::LogFC(LOGDEBUG, LOGEPG, "Deleting all EPG data from the database");

  std::unique_lock<CCriticalSection> lock(m_critSection);

  bReturn = DeleteValues("epg") || bReturn;
  bReturn = DeleteValues("epgtags") || bReturn;
  bReturn = DeleteValues("lastepgscan") || bReturn;

  return bReturn;
}

bool CPVREpgDatabase::QueueDeleteEpgQuery(const CPVREpg& table)
{
  /* invalid channel */
  if (table.EpgID() <= 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel id: {}", table.EpgID());
    return false;
  }

  Filter filter;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  filter.AppendWhere(PrepareSQL("idEpg = %u", table.EpgID()));

  std::string strQuery;
  if (BuildSQL(PrepareSQL("DELETE FROM %s ", "epg"), filter, strQuery))
    return QueueDeleteQuery(strQuery);

  return false;
}

bool CPVREpgDatabase::QueueDeleteTagQuery(const CPVREpgInfoTag& tag)
{
  /* tag without a database ID was not persisted */
  if (tag.DatabaseID() <= 0)
    return false;

  Filter filter;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  filter.AppendWhere(PrepareSQL("idBroadcast = %u", tag.DatabaseID()));

  std::string strQuery;
  BuildSQL(PrepareSQL("DELETE FROM %s ", "epgtags"), filter, strQuery);
  return QueueDeleteQuery(strQuery);
}

std::vector<std::shared_ptr<CPVREpg>> CPVREpgDatabase::GetAll()
{
  std::vector<std::shared_ptr<CPVREpg>> result;

  std::unique_lock<CCriticalSection> lock(m_critSection);
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
    const std::unique_ptr<dbiplus::Dataset>& pDS) const
{
  if (!pDS->eof())
  {
    std::shared_ptr<CPVREpgInfoTag> newTag(
        new CPVREpgInfoTag(m_pDS->fv("idEpg").get_asInt(), m_pDS->fv("sIconPath").get_asString()));

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

    newTag->m_iDatabaseID = m_pDS->fv("idBroadcast").get_asInt();
    newTag->m_strTitle = m_pDS->fv("sTitle").get_asString();
    newTag->m_strPlotOutline = m_pDS->fv("sPlotOutline").get_asString();
    newTag->m_strPlot = m_pDS->fv("sPlot").get_asString();
    newTag->m_strOriginalTitle = m_pDS->fv("sOriginalTitle").get_asString();
    newTag->m_cast = newTag->Tokenize(m_pDS->fv("sCast").get_asString());
    newTag->m_directors = newTag->Tokenize(m_pDS->fv("sDirector").get_asString());
    newTag->m_writers = newTag->Tokenize(m_pDS->fv("sWriter").get_asString());
    newTag->m_iYear = m_pDS->fv("iYear").get_asInt();
    newTag->m_strIMDBNumber = m_pDS->fv("sIMDBNumber").get_asString();
    newTag->m_iParentalRating = m_pDS->fv("iParentalRating").get_asInt();
    newTag->m_iStarRating = m_pDS->fv("iStarRating").get_asInt();
    newTag->m_iEpisodeNumber = m_pDS->fv("iEpisodeId").get_asInt();
    newTag->m_iEpisodePart = m_pDS->fv("iEpisodePart").get_asInt();
    newTag->m_strEpisodeName = m_pDS->fv("sEpisodeName").get_asString();
    newTag->m_iSeriesNumber = m_pDS->fv("iSeriesId").get_asInt();
    newTag->m_iFlags = m_pDS->fv("iFlags").get_asInt();
    newTag->m_strSeriesLink = m_pDS->fv("sSeriesLink").get_asString();
    newTag->m_strParentalRatingCode = m_pDS->fv("sParentalRatingCode").get_asString();
    newTag->m_iGenreType = m_pDS->fv("iGenreType").get_asInt();
    newTag->m_iGenreSubType = m_pDS->fv("iGenreSubType").get_asInt();
    newTag->m_strGenreDescription = m_pDS->fv("sGenre").get_asString();

    return newTag;
  }
  return {};
}

bool CPVREpgDatabase::HasTags(int iEpgID) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("SELECT iStartTime FROM epgtags WHERE idEpg = %u LIMIT 1;", iEpgID);
  std::string strValue = GetSingleValue(strQuery);
  return !strValue.empty();
}

CDateTime CPVREpgDatabase::GetLastEndTime(int iEpgID) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("SELECT MAX(iEndTime) FROM epgtags WHERE idEpg = %u;", iEpgID);
  std::string strValue = GetSingleValue(strQuery);
  if (!strValue.empty())
    return CDateTime(static_cast<time_t>(std::atoi(strValue.c_str())));

  return {};
}

std::pair<CDateTime, CDateTime> CPVREpgDatabase::GetFirstAndLastEPGDate() const
{
  CDateTime first;
  CDateTime last;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  // 1st query: get min start time
  std::string strQuery = PrepareSQL("SELECT MIN(iStartTime) FROM epgtags;");

  std::string strValue = GetSingleValue(strQuery);
  if (!strValue.empty())
    first = CDateTime(static_cast<time_t>(std::atoi(strValue.c_str())));

  // 2nd query: get max end time
  strQuery = PrepareSQL("SELECT MAX(iEndTime) FROM epgtags;");

  strValue = GetSingleValue(strQuery);
  if (!strValue.empty())
    last = CDateTime(static_cast<time_t>(std::atoi(strValue.c_str())));

  return {first, last};
}

CDateTime CPVREpgDatabase::GetMinStartTime(int iEpgID, const CDateTime& minStart) const
{
  time_t t;
  minStart.GetAsTime(t);

  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string strQuery = PrepareSQL("SELECT MIN(iStartTime) "
                                          "FROM epgtags "
                                          "WHERE idEpg = %u AND iStartTime > %u;",
                                          iEpgID, static_cast<unsigned int>(t));
  std::string strValue = GetSingleValue(strQuery);
  if (!strValue.empty())
    return CDateTime(static_cast<time_t>(std::atoi(strValue.c_str())));

  return {};
}

CDateTime CPVREpgDatabase::GetMaxEndTime(int iEpgID, const CDateTime& maxEnd) const
{
  time_t t;
  maxEnd.GetAsTime(t);

  std::unique_lock<CCriticalSection> lock(m_critSection);
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

class CSearchTermConverter
{
public:
  explicit CSearchTermConverter(const std::string& strSearchTerm) { Parse(strSearchTerm); }

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
    const PVREpgSearchData& searchData) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  std::string strQuery = PrepareSQL("SELECT * FROM epgtags");

  Filter filter;

  /////////////////////////////////////////////////////////////////////////////////////////////
  // min start datetime
  /////////////////////////////////////////////////////////////////////////////////////////////

  if (searchData.m_startDateTime.IsValid())
  {
    time_t minStart;
    searchData.m_startDateTime.GetAsTime(minStart);
    filter.AppendWhere(PrepareSQL("iStartTime >= %u", static_cast<unsigned int>(minStart)));
  }

  /////////////////////////////////////////////////////////////////////////////////////////////
  // max end datetime
  /////////////////////////////////////////////////////////////////////////////////////////////

  if (searchData.m_endDateTime.IsValid())
  {
    time_t maxEnd;
    searchData.m_endDateTime.GetAsTime(maxEnd);
    filter.AppendWhere(PrepareSQL("iEndTime <= %u", static_cast<unsigned int>(maxEnd)));
  }

  /////////////////////////////////////////////////////////////////////////////////////////////
  // ignore finished broadcasts
  /////////////////////////////////////////////////////////////////////////////////////////////

  if (searchData.m_bIgnoreFinishedBroadcasts)
  {
    const time_t minEnd = std::time(nullptr); // now
    filter.AppendWhere(PrepareSQL("iEndTime > %u", static_cast<unsigned int>(minEnd)));
  }

  /////////////////////////////////////////////////////////////////////////////////////////////
  // ignore future broadcasts
  /////////////////////////////////////////////////////////////////////////////////////////////

  if (searchData.m_bIgnoreFutureBroadcasts)
  {
    const time_t maxStart = std::time(nullptr); // now
    filter.AppendWhere(PrepareSQL("iStartTime < %u", static_cast<unsigned int>(maxStart)));
  }

  /////////////////////////////////////////////////////////////////////////////////////////////
  // genre type
  /////////////////////////////////////////////////////////////////////////////////////////////

  if (searchData.m_iGenreType != EPG_SEARCH_UNSET)
  {
    if (searchData.m_bIncludeUnknownGenres)
    {
      // match the exact genre and everything with unknown genre
      filter.AppendWhere(PrepareSQL("(iGenreType == %u) OR (iGenreType < %u) OR (iGenreType > %u)",
                                    searchData.m_iGenreType, EPG_EVENT_CONTENTMASK_MOVIEDRAMA,
                                    EPG_EVENT_CONTENTMASK_USERDEFINED));
    }
    else
    {
      // match only the exact genre
      filter.AppendWhere(PrepareSQL("iGenreType == %u", searchData.m_iGenreType));
    }
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
    int iEpgID, unsigned int iUniqueBroadcastId) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string strQuery = PrepareSQL("SELECT * "
                                          "FROM epgtags "
                                          "WHERE idEpg = %u AND iBroadcastUid = %u;",
                                          iEpgID, iUniqueBroadcastId);

  if (ResultQuery(strQuery))
  {
    try
    {
      std::shared_ptr<CPVREpgInfoTag> tag = CreateEpgTag(m_pDS);
      m_pDS->close();
      return tag;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load EPG tag with unique broadcast ID ({}) from the database",
                 iUniqueBroadcastId);
    }
  }

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgDatabase::GetEpgTagByDatabaseID(int iEpgID,
                                                                       int iDatabaseId) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string strQuery = PrepareSQL("SELECT * "
                                          "FROM epgtags "
                                          "WHERE idEpg = %u AND idBroadcast = %u;",
                                          iEpgID, iDatabaseId);

  if (ResultQuery(strQuery))
  {
    try
    {
      std::shared_ptr<CPVREpgInfoTag> tag = CreateEpgTag(m_pDS);
      m_pDS->close();
      return tag;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load EPG tag with database ID ({}) from the database",
                 iDatabaseId);
    }
  }

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgDatabase::GetEpgTagByStartTime(
    int iEpgID, const CDateTime& startTime) const
{
  time_t start;
  startTime.GetAsTime(start);

  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string strQuery = PrepareSQL("SELECT * "
                                          "FROM epgtags "
                                          "WHERE idEpg = %u AND iStartTime = %u;",
                                          iEpgID, static_cast<unsigned int>(start));

  if (ResultQuery(strQuery))
  {
    try
    {
      std::shared_ptr<CPVREpgInfoTag> tag = CreateEpgTag(m_pDS);
      m_pDS->close();
      return tag;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load EPG tag with start time ({}) from the database",
                 startTime.GetAsDBDateTime());
    }
  }

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgDatabase::GetEpgTagByMinStartTime(
    int iEpgID, const CDateTime& minStartTime) const
{
  time_t minStart;
  minStartTime.GetAsTime(minStart);

  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("SELECT * "
                 "FROM epgtags "
                 "WHERE idEpg = %u AND iStartTime >= %u ORDER BY iStartTime ASC LIMIT 1;",
                 iEpgID, static_cast<unsigned int>(minStart));

  if (ResultQuery(strQuery))
  {
    try
    {
      std::shared_ptr<CPVREpgInfoTag> tag = CreateEpgTag(m_pDS);
      m_pDS->close();
      return tag;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load tags with min start time ({}) for EPG ({})",
                 minStartTime.GetAsDBDateTime(), iEpgID);
    }
  }

  return {};
}

std::shared_ptr<CPVREpgInfoTag> CPVREpgDatabase::GetEpgTagByMaxEndTime(
    int iEpgID, const CDateTime& maxEndTime) const
{
  time_t maxEnd;
  maxEndTime.GetAsTime(maxEnd);

  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("SELECT * "
                 "FROM epgtags "
                 "WHERE idEpg = %u AND iEndTime <= %u ORDER BY iStartTime DESC LIMIT 1;",
                 iEpgID, static_cast<unsigned int>(maxEnd));

  if (ResultQuery(strQuery))
  {
    try
    {
      std::shared_ptr<CPVREpgInfoTag> tag = CreateEpgTag(m_pDS);
      m_pDS->close();
      return tag;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load tags with max end time ({}) for EPG ({})",
                 maxEndTime.GetAsDBDateTime(), iEpgID);
    }
  }

  return {};
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgDatabase::GetEpgTagsByMinStartMaxEndTime(
    int iEpgID, const CDateTime& minStartTime, const CDateTime& maxEndTime) const
{
  time_t minStart;
  minStartTime.GetAsTime(minStart);

  time_t maxEnd;
  maxEndTime.GetAsTime(maxEnd);

  std::unique_lock<CCriticalSection> lock(m_critSection);
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
                 "Could not load tags with min start time ({}) and max end time ({}) for EPG ({})",
                 minStartTime.GetAsDBDateTime(), maxEndTime.GetAsDBDateTime(), iEpgID);
    }
  }

  return {};
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgDatabase::GetEpgTagsByMinEndMaxStartTime(
    int iEpgID, const CDateTime& minEndTime, const CDateTime& maxStartTime) const
{
  time_t minEnd;
  minEndTime.GetAsTime(minEnd);

  time_t maxStart;
  maxStartTime.GetAsTime(maxStart);

  std::unique_lock<CCriticalSection> lock(m_critSection);
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
                 "Could not load tags with min end time ({}) and max start time ({}) for EPG ({})",
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

  std::unique_lock<CCriticalSection> lock(m_critSection);
  filter.AppendWhere(PrepareSQL("idEpg = %u AND iEndTime >= %u AND iStartTime <= %u", iEpgID,
                                static_cast<unsigned int>(minEnd),
                                static_cast<unsigned int>(maxStart)));

  std::string strQuery;
  if (BuildSQL("DELETE FROM epgtags", filter, strQuery))
    return QueueDeleteQuery(strQuery);

  return false;
}

std::vector<std::shared_ptr<CPVREpgInfoTag>> CPVREpgDatabase::GetAllEpgTags(int iEpgID) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
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
      CLog::LogF(LOGERROR, "Could not load tags for EPG ({})", iEpgID);
    }
  }
  return {};
}

std::vector<std::string> CPVREpgDatabase::GetAllIconPaths(int iEpgID) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("SELECT sIconPath FROM epgtags WHERE idEpg = %u;", iEpgID);
  if (ResultQuery(strQuery))
  {
    try
    {
      std::vector<std::string> paths;
      while (!m_pDS->eof())
      {
        paths.emplace_back(m_pDS->fv("sIconPath").get_asString());
        m_pDS->next();
      }
      m_pDS->close();
      return paths;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load tags for EPG ({})", iEpgID);
    }
  }
  return {};
}

bool CPVREpgDatabase::GetLastEpgScanTime(int iEpgId, CDateTime* lastScan) const
{
  bool bReturn = false;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  std::string strWhereClause = PrepareSQL("idEpg = %u", iEpgId);
  std::string strValue = GetSingleValue("lastepgscan", "sLastScan", strWhereClause);

  if (!strValue.empty())
  {
    lastScan->SetFromDBDateTime(strValue);
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
  std::unique_lock<CCriticalSection> lock(m_critSection);
  std::string strQuery = PrepareSQL("REPLACE INTO lastepgscan(idEpg, sLastScan) VALUES (%u, '%s');",
      iEpgId, lastScanTime.GetAsDBDateTime().c_str());

  return QueueInsertQuery(strQuery);
}

bool CPVREpgDatabase::QueueDeleteLastEpgScanTimeQuery(const CPVREpg& table)
{
  if (table.EpgID() <= 0)
  {
    CLog::LogF(LOGERROR, "Invalid EPG id: {}", table.EpgID());
    return false;
  }

  Filter filter;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  filter.AppendWhere(PrepareSQL("idEpg = %u", table.EpgID()));

  std::string strQuery;
  if (BuildSQL(PrepareSQL("DELETE FROM %s ", "lastepgscan"), filter, strQuery))
    return QueueDeleteQuery(strQuery);

  return false;
}

int CPVREpgDatabase::Persist(const CPVREpg& epg, bool bQueueWrite)
{
  int iReturn = -1;
  std::string strQuery;

  std::unique_lock<CCriticalSection> lock(m_critSection);
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

  std::unique_lock<CCriticalSection> lock(m_critSection);
  filter.AppendWhere(
      PrepareSQL("idEpg = %u AND iEndTime < %u", iEpgId, static_cast<unsigned int>(iMaxEndTime)));
  return DeleteValues("epgtags", filter);
}

bool CPVREpgDatabase::DeleteEpgTags(int iEpgId)
{
  Filter filter;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  filter.AppendWhere(PrepareSQL("idEpg = %u", iEpgId));
  return DeleteValues("epgtags", filter);
}

bool CPVREpgDatabase::QueueDeleteEpgTags(int iEpgId)
{
  Filter filter;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  filter.AppendWhere(PrepareSQL("idEpg = %u", iEpgId));

  std::string strQuery;
  BuildSQL(PrepareSQL("DELETE FROM %s ", "epgtags"), filter, strQuery);
  return QueueDeleteQuery(strQuery);
}

bool CPVREpgDatabase::QueuePersistQuery(const CPVREpgInfoTag& tag)
{
  if (tag.EpgID() <= 0)
  {
    CLog::LogF(LOGERROR, "Tag '{}' does not have a valid table", tag.Title());
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

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (iBroadcastId < 0)
  {
    strQuery = PrepareSQL(
        "REPLACE INTO epgtags (idEpg, iStartTime, "
        "iEndTime, sTitle, sPlotOutline, sPlot, sOriginalTitle, sCast, sDirector, sWriter, iYear, "
        "sIMDBNumber, "
        "sIconPath, iGenreType, iGenreSubType, sGenre, sFirstAired, iParentalRating, iStarRating, "
        "iSeriesId, "
        "iEpisodeId, iEpisodePart, sEpisodeName, iFlags, sSeriesLink, sParentalRatingCode, "
        "iBroadcastUid) "
        "VALUES (%u, %u, %u, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %i, '%s', '%s', %i, %i, "
        "'%s', '%s', %i, %i, %i, %i, %i, '%s', %i, '%s', '%s', %i);",
        tag.EpgID(), static_cast<unsigned int>(iStartTime), static_cast<unsigned int>(iEndTime),
        tag.Title().c_str(), tag.PlotOutline().c_str(), tag.Plot().c_str(),
        tag.OriginalTitle().c_str(), tag.DeTokenize(tag.Cast()).c_str(),
        tag.DeTokenize(tag.Directors()).c_str(), tag.DeTokenize(tag.Writers()).c_str(), tag.Year(),
        tag.IMDBNumber().c_str(), tag.ClientIconPath().c_str(), tag.GenreType(), tag.GenreSubType(),
        tag.GenreDescription().c_str(), sFirstAired.c_str(), tag.ParentalRating(), tag.StarRating(),
        tag.SeriesNumber(), tag.EpisodeNumber(), tag.EpisodePart(), tag.EpisodeName().c_str(),
        tag.Flags(), tag.SeriesLink().c_str(), tag.ParentalRatingCode().c_str(),
        tag.UniqueBroadcastID());
  }
  else
  {
    strQuery = PrepareSQL(
        "REPLACE INTO epgtags (idEpg, iStartTime, "
        "iEndTime, sTitle, sPlotOutline, sPlot, sOriginalTitle, sCast, sDirector, sWriter, iYear, "
        "sIMDBNumber, "
        "sIconPath, iGenreType, iGenreSubType, sGenre, sFirstAired, iParentalRating, iStarRating, "
        "iSeriesId, "
        "iEpisodeId, iEpisodePart, sEpisodeName, iFlags, sSeriesLink, sParentalRatingCode, "
        "iBroadcastUid, idBroadcast) "
        "VALUES (%u, %u, %u, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %i, '%s', '%s', %i, %i, "
        "'%s', '%s', %i, %i, %i, %i, %i, '%s', %i, '%s', '%s', %i, %i);",
        tag.EpgID(), static_cast<unsigned int>(iStartTime), static_cast<unsigned int>(iEndTime),
        tag.Title().c_str(), tag.PlotOutline().c_str(), tag.Plot().c_str(),
        tag.OriginalTitle().c_str(), tag.DeTokenize(tag.Cast()).c_str(),
        tag.DeTokenize(tag.Directors()).c_str(), tag.DeTokenize(tag.Writers()).c_str(), tag.Year(),
        tag.IMDBNumber().c_str(), tag.ClientIconPath().c_str(), tag.GenreType(), tag.GenreSubType(),
        tag.GenreDescription().c_str(), sFirstAired.c_str(), tag.ParentalRating(), tag.StarRating(),
        tag.SeriesNumber(), tag.EpisodeNumber(), tag.EpisodePart(), tag.EpisodeName().c_str(),
        tag.Flags(), tag.SeriesLink().c_str(), tag.ParentalRatingCode().c_str(),
        tag.UniqueBroadcastID(), iBroadcastId);
  }

  QueueInsertQuery(strQuery);
  return true;
}

int CPVREpgDatabase::GetLastEPGId() const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  std::string strQuery = PrepareSQL("SELECT MAX(idEpg) FROM epg");
  std::string strValue = GetSingleValue(strQuery);
  if (!strValue.empty())
    return std::atoi(strValue.c_str());
  return 0;
}

/********** Saved searches methods **********/

std::shared_ptr<CPVREpgSearchFilter> CPVREpgDatabase::CreateEpgSearchFilter(
    bool bRadio, const std::unique_ptr<dbiplus::Dataset>& pDS) const
{
  if (!pDS->eof())
  {
    auto newSearch = std::make_shared<CPVREpgSearchFilter>(bRadio);

    newSearch->SetDatabaseId(m_pDS->fv("idSearch").get_asInt());
    newSearch->SetTitle(m_pDS->fv("sTitle").get_asString());

    const std::string lastExec = m_pDS->fv("sLastExecutedDateTime").get_asString();
    if (!lastExec.empty())
      newSearch->SetLastExecutedDateTime(CDateTime::FromDBDateTime(lastExec));

    newSearch->SetSearchTerm(m_pDS->fv("sSearchTerm").get_asString());
    newSearch->SetSearchInDescription(m_pDS->fv("bSearchInDescription").get_asBool());
    newSearch->SetGenreType(m_pDS->fv("iGenreType").get_asInt());

    const std::string start = m_pDS->fv("sStartDateTime").get_asString();
    if (!start.empty())
      newSearch->SetStartDateTime(CDateTime::FromDBDateTime(start));

    const std::string end = m_pDS->fv("sEndDateTime").get_asString();
    if (!end.empty())
      newSearch->SetEndDateTime(CDateTime::FromDBDateTime(end));

    newSearch->SetCaseSensitive(m_pDS->fv("bIsCaseSensitive").get_asBool());
    newSearch->SetMinimumDuration(m_pDS->fv("iMinimumDuration").get_asInt());
    newSearch->SetMaximumDuration(m_pDS->fv("iMaximumDuration").get_asInt());
    newSearch->SetClientID(m_pDS->fv("iClientId").get_asInt());
    newSearch->SetChannelUID(m_pDS->fv("iChannelUid").get_asInt());
    newSearch->SetIncludeUnknownGenres(m_pDS->fv("bIncludeUnknownGenres").get_asBool());
    newSearch->SetRemoveDuplicates(m_pDS->fv("bRemoveDuplicates").get_asBool());
    newSearch->SetIgnoreFinishedBroadcasts(m_pDS->fv("bIgnoreFinishedBroadcasts").get_asBool());
    newSearch->SetIgnoreFutureBroadcasts(m_pDS->fv("bIgnoreFutureBroadcasts").get_asBool());
    newSearch->SetFreeToAirOnly(m_pDS->fv("bFreeToAirOnly").get_asBool());
    newSearch->SetIgnorePresentTimers(m_pDS->fv("bIgnorePresentTimers").get_asBool());
    newSearch->SetIgnorePresentRecordings(m_pDS->fv("bIgnorePresentRecordings").get_asBool());
    newSearch->SetChannelGroupID(m_pDS->fv("iChannelGroup").get_asInt());

    newSearch->SetChanged(false);

    return newSearch;
  }
  return {};
}

std::vector<std::shared_ptr<CPVREpgSearchFilter>> CPVREpgDatabase::GetSavedSearches(
    bool bRadio) const
{
  std::vector<std::shared_ptr<CPVREpgSearchFilter>> result;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("SELECT * FROM savedsearches WHERE bIsRadio = %u", bRadio);
  if (ResultQuery(strQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        result.emplace_back(CreateEpgSearchFilter(bRadio, m_pDS));
        m_pDS->next();
      }
      m_pDS->close();
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load EPG search data from the database");
    }
  }
  return result;
}

std::shared_ptr<CPVREpgSearchFilter> CPVREpgDatabase::GetSavedSearchById(bool bRadio, int iId) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("SELECT * FROM savedsearches WHERE bIsRadio = %u AND idSearch = %u;", bRadio, iId);

  if (ResultQuery(strQuery))
  {
    try
    {
      std::shared_ptr<CPVREpgSearchFilter> filter = CreateEpgSearchFilter(bRadio, m_pDS);
      m_pDS->close();
      return filter;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load EPG search filter with id ({}) from the database", iId);
    }
  }

  return {};
}

bool CPVREpgDatabase::Persist(CPVREpgSearchFilter& epgSearch)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  // Insert a new entry if this is a new search, replace the existing otherwise
  std::string strQuery;
  if (epgSearch.GetDatabaseId() == -1)
    strQuery = PrepareSQL(
        "INSERT INTO savedsearches "
        "(sTitle, sLastExecutedDateTime, sSearchTerm, bSearchInDescription, bIsCaseSensitive, "
        "iGenreType, bIncludeUnknownGenres, sStartDateTime, sEndDateTime, iMinimumDuration, "
        "iMaximumDuration, bIsRadio, iClientId, iChannelUid, bRemoveDuplicates, "
        "bIgnoreFinishedBroadcasts, bIgnoreFutureBroadcasts, bFreeToAirOnly, bIgnorePresentTimers, "
        "bIgnorePresentRecordings, iChannelGroup) "
        "VALUES ('%s', '%s', '%s', %i, %i, %i, %i, '%s', '%s', %i, %i, %i, %i, %i, %i, %i, %i, "
        "%i, %i, %i, %i);",
        epgSearch.GetTitle().c_str(),
        epgSearch.GetLastExecutedDateTime().IsValid()
            ? epgSearch.GetLastExecutedDateTime().GetAsDBDateTime().c_str()
            : "",
        epgSearch.GetSearchTerm().c_str(), epgSearch.ShouldSearchInDescription() ? 1 : 0,
        epgSearch.IsCaseSensitive() ? 1 : 0, epgSearch.GetGenreType(),
        epgSearch.ShouldIncludeUnknownGenres() ? 1 : 0,
        epgSearch.GetStartDateTime().IsValid()
            ? epgSearch.GetStartDateTime().GetAsDBDateTime().c_str()
            : "",
        epgSearch.GetEndDateTime().IsValid() ? epgSearch.GetEndDateTime().GetAsDBDateTime().c_str()
                                             : "",
        epgSearch.GetMinimumDuration(), epgSearch.GetMaximumDuration(), epgSearch.IsRadio() ? 1 : 0,
        epgSearch.GetClientID(), epgSearch.GetChannelUID(),
        epgSearch.ShouldRemoveDuplicates() ? 1 : 0,
        epgSearch.ShouldIgnoreFinishedBroadcasts() ? 1 : 0,
        epgSearch.ShouldIgnoreFutureBroadcasts() ? 1 : 0, epgSearch.IsFreeToAirOnly() ? 1 : 0,
        epgSearch.ShouldIgnorePresentTimers() ? 1 : 0,
        epgSearch.ShouldIgnorePresentRecordings() ? 1 : 0, epgSearch.GetChannelGroupID());
  else
    strQuery = PrepareSQL(
        "REPLACE INTO savedsearches "
        "(idSearch, sTitle, sLastExecutedDateTime, sSearchTerm, bSearchInDescription, "
        "bIsCaseSensitive, iGenreType, bIncludeUnknownGenres, sStartDateTime, sEndDateTime, "
        "iMinimumDuration, iMaximumDuration, bIsRadio, iClientId, iChannelUid, bRemoveDuplicates, "
        "bIgnoreFinishedBroadcasts, bIgnoreFutureBroadcasts, bFreeToAirOnly, bIgnorePresentTimers, "
        "bIgnorePresentRecordings, iChannelGroup) "
        "VALUES (%i, '%s', '%s', '%s', %i, %i, %i, %i, '%s', '%s', %i, %i, %i, %i, %i, %i, %i, %i, "
        "%i, %i, %i, %i);",
        epgSearch.GetDatabaseId(), epgSearch.GetTitle().c_str(),
        epgSearch.GetLastExecutedDateTime().IsValid()
            ? epgSearch.GetLastExecutedDateTime().GetAsDBDateTime().c_str()
            : "",
        epgSearch.GetSearchTerm().c_str(), epgSearch.ShouldSearchInDescription() ? 1 : 0,
        epgSearch.IsCaseSensitive() ? 1 : 0, epgSearch.GetGenreType(),
        epgSearch.ShouldIncludeUnknownGenres() ? 1 : 0,
        epgSearch.GetStartDateTime().IsValid()
            ? epgSearch.GetStartDateTime().GetAsDBDateTime().c_str()
            : "",
        epgSearch.GetEndDateTime().IsValid() ? epgSearch.GetEndDateTime().GetAsDBDateTime().c_str()
                                             : "",
        epgSearch.GetMinimumDuration(), epgSearch.GetMaximumDuration(), epgSearch.IsRadio() ? 1 : 0,
        epgSearch.GetClientID(), epgSearch.GetChannelUID(),
        epgSearch.ShouldRemoveDuplicates() ? 1 : 0,
        epgSearch.ShouldIgnoreFinishedBroadcasts() ? 1 : 0,
        epgSearch.ShouldIgnoreFutureBroadcasts() ? 1 : 0, epgSearch.IsFreeToAirOnly() ? 1 : 0,
        epgSearch.ShouldIgnorePresentTimers() ? 1 : 0,
        epgSearch.ShouldIgnorePresentRecordings() ? 1 : 0, epgSearch.GetChannelGroupID());

  bool bReturn = ExecuteQuery(strQuery);

  if (bReturn)
  {
    // Set the database id for searches persisted for the first time
    if (epgSearch.GetDatabaseId() == -1)
      epgSearch.SetDatabaseId(static_cast<int>(m_pDS->lastinsertid()));

    epgSearch.SetChanged(false);
  }

  return bReturn;
}

bool CPVREpgDatabase::UpdateSavedSearchLastExecuted(const CPVREpgSearchFilter& epgSearch)
{
  if (epgSearch.GetDatabaseId() == -1)
    return false;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  const std::string strQuery = PrepareSQL(
      "UPDATE savedsearches SET sLastExecutedDateTime = '%s' WHERE idSearch = %i",
      epgSearch.GetLastExecutedDateTime().GetAsDBDateTime().c_str(), epgSearch.GetDatabaseId());
  return ExecuteQuery(strQuery);
}

bool CPVREpgDatabase::Delete(const CPVREpgSearchFilter& epgSearch)
{
  if (epgSearch.GetDatabaseId() == -1)
    return false;

  CLog::LogFC(LOGDEBUG, LOGEPG, "Deleting saved search '{}' from the database",
              epgSearch.GetTitle());

  std::unique_lock<CCriticalSection> lock(m_critSection);

  Filter filter;
  filter.AppendWhere(PrepareSQL("idSearch = '%i'", epgSearch.GetDatabaseId()));

  return DeleteValues("savedsearches", filter);
}

bool CPVREpgDatabase::DeleteSavedSearches()
{
  CLog::LogFC(LOGDEBUG, LOGEPG, "Deleting all saved searches from the database");

  std::unique_lock<CCriticalSection> lock(m_critSection);
  return DeleteValues("savedsearches");
}
