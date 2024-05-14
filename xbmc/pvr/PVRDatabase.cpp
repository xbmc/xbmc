/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRDatabase.h"

#include "ServiceBroker.h"
#include "dbwrappers/dataset.h"
#include "pvr/addons/PVRClient.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/channels/PVRChannelGroupMember.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/providers/PVRProvider.h"
#include "pvr/providers/PVRProviders.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimerType.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <cstdlib>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

using namespace dbiplus;
using namespace PVR;

namespace
{
// clang-format off

  static const std::string sqlCreateTimersTable =
    "CREATE TABLE timers ("
      "iClientIndex       integer primary key, "
      "iParentClientIndex integer, "
      "iClientId          integer, "
      "iTimerType         integer, "
      "iState             integer, "
      "sTitle             varchar(255), "
      "iClientChannelUid  integer, "
      "sSeriesLink        varchar(255), "
      "sStartTime         varchar(20), "
      "bStartAnyTime      bool, "
      "sEndTime           varchar(20), "
      "bEndAnyTime        bool, "
      "sFirstDay          varchar(20), "
      "iWeekdays          integer, "
      "iEpgUid            integer, "
      "iMarginStart       integer, "
      "iMarginEnd         integer, "
      "sEpgSearchString   varchar(255), "
      "bFullTextEpgSearch bool, "
      "iPreventDuplicates integer,"
      "iPrority           integer,"
      "iLifetime          integer,"
      "iMaxRecordings     integer,"
      "iRecordingGroup    integer"
  ")";

  static const std::string sqlCreateProvidersTable =
    "CREATE TABLE providers ("
      "idProvider           integer primary key, "
      "iUniqueId            integer, "
      "iClientId            integer, "
      "sName                varchar(64), "
      "iType                integer, "
      "sIconPath            varchar(255), "
      "sCountries           varchar(64), "
      "sLanguages           varchar(64) "
    ")";

  // clang-format on

  std::string GetClientIdsSQL(const std::vector<std::shared_ptr<CPVRClient>>& clients,
                              bool migrate = false)
  {
    if (clients.empty() && !migrate)
      return {};

    std::string clientIds = "(";
    for (auto it = clients.cbegin(); it != clients.cend(); ++it)
    {
      if (it != clients.cbegin())
        clientIds += " OR ";

      clientIds += "iClientId = ";
      clientIds += std::to_string((*it)->GetID());
    }
    if (migrate)
    {
      if (!clients.empty())
        clientIds += " OR ";

      clientIds += "iClientId = -2"; // PVR_GROUP_CLIENT_ID_UNKNOWN
    }
    clientIds += ")";
    return clientIds;
  }

} // unnamed namespace

bool CPVRDatabase::Open()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return CDatabase::Open(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseTV);
}

void CPVRDatabase::Close()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  CDatabase::Close();
}

void CPVRDatabase::Lock()
{
  m_critSection.lock();
}

void CPVRDatabase::Unlock()
{
  m_critSection.unlock();
}

void CPVRDatabase::CreateTables()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  CLog::LogF(LOGINFO, "Creating PVR database tables");

  CLog::LogFC(LOGDEBUG, LOGPVR, "Creating table 'channels'");
  m_pDS->exec("CREATE TABLE channels ("
              "idChannel            integer primary key, "
              "iUniqueId            integer, "
              "bIsRadio             bool, "
              "bIsHidden            bool, "
              "bIsUserSetIcon       bool, "
              "bIsUserSetName       bool, "
              "bIsLocked            bool, "
              "sIconPath            varchar(255), "
              "sChannelName         varchar(64), "
              "bIsVirtual           bool, "
              "bEPGEnabled          bool, "
              "sEPGScraper          varchar(32), "
              "iLastWatched         integer, "
              "iClientId            integer, " //! @todo use mapping table
              "idEpg                integer, "
              "bHasArchive          bool, "
              "iClientProviderUid   integer, "
              "bIsUserSetHidden     bool, "
              "iLastWatchedGroupId  integer, "
              "sDateTimeAdded       varchar(20)"
              ")");

  CLog::LogFC(LOGDEBUG, LOGPVR, "Creating table 'channelgroups'");
  m_pDS->exec("CREATE TABLE channelgroups ("
              "idGroup         integer primary key, "
              "bIsRadio        bool, "
              "iGroupType      integer, "
              "sName           varchar(64), "
              "iLastWatched    integer, "
              "bIsHidden       bool, "
              "iPosition       integer, "
              "iLastOpened     bigint unsigned, "
              "iClientId       integer, "
              "bIsUserSetName  bool, "
              "sClientName     varchar(64), "
              "iClientPosition integer"
              ")");

  CLog::LogFC(LOGDEBUG, LOGPVR, "Creating table 'map_channelgroups_channels'");
  m_pDS->exec(
      "CREATE TABLE map_channelgroups_channels ("
        "idChannel         integer, "
        "idGroup           integer, "
        "iChannelNumber    integer, "
        "iSubChannelNumber integer, "
        "iOrder            integer, "
        "iClientChannelNumber    integer, "
        "iClientSubChannelNumber integer"
      ")"
  );

  CLog::LogFC(LOGDEBUG, LOGPVR, "Creating table 'clients'");
  m_pDS->exec(
      "CREATE TABLE clients ("
        "idClient  integer primary key, "
        "iPriority integer"
      ")"
  );

  CLog::LogFC(LOGDEBUG, LOGPVR, "Creating table 'timers'");
  m_pDS->exec(sqlCreateTimersTable);

  CLog::LogFC(LOGDEBUG, LOGPVR, "Creating table 'providers'");
  m_pDS->exec(sqlCreateProvidersTable);
}

void CPVRDatabase::CreateAnalytics()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  CLog::LogF(LOGINFO, "Creating PVR database indices");
  m_pDS->exec("CREATE INDEX idx_clients_idClient on clients(idClient);");
  m_pDS->exec("CREATE UNIQUE INDEX idx_channels_iClientId_iUniqueId on channels(iClientId, iUniqueId);");
  m_pDS->exec("CREATE INDEX idx_channelgroups_bIsRadio on channelgroups(bIsRadio);");
  m_pDS->exec("CREATE UNIQUE INDEX idx_idGroup_idChannel on map_channelgroups_channels(idGroup, idChannel);");
  m_pDS->exec("CREATE INDEX idx_timers_iClientIndex on timers(iClientIndex);");
}

void CPVRDatabase::UpdateTables(int iVersion)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (iVersion < 13)
    m_pDS->exec("ALTER TABLE channels ADD idEpg integer;");

  if (iVersion < 20)
    m_pDS->exec("ALTER TABLE channels ADD bIsUserSetIcon bool");

  if (iVersion < 21)
    m_pDS->exec("ALTER TABLE channelgroups ADD iGroupType integer");

  if (iVersion < 22)
    m_pDS->exec("ALTER TABLE channels ADD bIsLocked bool");

  if (iVersion < 23)
    m_pDS->exec("ALTER TABLE channelgroups ADD iLastWatched integer");

  if (iVersion < 24)
    m_pDS->exec("ALTER TABLE channels ADD bIsUserSetName bool");

  if (iVersion < 25)
    m_pDS->exec("DROP TABLE IF EXISTS channelsettings");

  if (iVersion < 26)
  {
    m_pDS->exec("ALTER TABLE channels ADD iClientSubChannelNumber integer");
    m_pDS->exec("UPDATE channels SET iClientSubChannelNumber = 0");
    m_pDS->exec("ALTER TABLE map_channelgroups_channels ADD iSubChannelNumber integer");
    m_pDS->exec("UPDATE map_channelgroups_channels SET iSubChannelNumber = 0");
  }

  if (iVersion < 27)
    m_pDS->exec("ALTER TABLE channelgroups ADD bIsHidden bool");

  if (iVersion < 28)
    m_pDS->exec("DROP TABLE clients");

  if (iVersion < 29)
    m_pDS->exec("ALTER TABLE channelgroups ADD iPosition integer");

  if (iVersion < 32)
    m_pDS->exec("CREATE TABLE clients (idClient integer primary key, iPriority integer)");

  if (iVersion < 33)
    m_pDS->exec(sqlCreateTimersTable);

  if (iVersion < 34)
    m_pDS->exec("ALTER TABLE channels ADD bHasArchive bool");

  if (iVersion < 35)
  {
    m_pDS->exec("ALTER TABLE map_channelgroups_channels ADD iOrder integer");
    m_pDS->exec("UPDATE map_channelgroups_channels SET iOrder = 0");
  }

  if (iVersion < 36)
  {
    m_pDS->exec("ALTER TABLE map_channelgroups_channels ADD iClientChannelNumber integer");
    m_pDS->exec("UPDATE map_channelgroups_channels SET iClientChannelNumber = 0");
    m_pDS->exec("ALTER TABLE map_channelgroups_channels ADD iClientSubChannelNumber integer");
    m_pDS->exec("UPDATE map_channelgroups_channels SET iClientSubChannelNumber = 0");
  }

  if (iVersion < 37)
    m_pDS->exec("ALTER TABLE channelgroups ADD iLastOpened integer");

  if (iVersion < 38)
  {
    m_pDS->exec("ALTER TABLE channelgroups "
                "RENAME TO channelgroups_old");

    m_pDS->exec("CREATE TABLE channelgroups ("
                "idGroup      integer primary key, "
                "bIsRadio     bool, "
                "iGroupType   integer, "
                "sName        varchar(64), "
                "iLastWatched integer, "
                "bIsHidden    bool, "
                "iPosition    integer, "
                "iLastOpened  bigint unsigned"
                ")");

    m_pDS->exec(
        "INSERT INTO channelgroups (bIsRadio, iGroupType, sName, iLastWatched, bIsHidden, "
        "iPosition, iLastOpened) "
        "SELECT bIsRadio, iGroupType, sName, iLastWatched, bIsHidden, iPosition, iLastOpened "
        "FROM channelgroups_old");

    m_pDS->exec("DROP TABLE channelgroups_old");
  }

  if (iVersion < 39)
  {
    m_pDS->exec(sqlCreateProvidersTable);
    m_pDS->exec("CREATE UNIQUE INDEX idx_iUniqueId_iClientId on providers(iUniqueId, iClientId);");
    m_pDS->exec("ALTER TABLE channels ADD iClientProviderUid integer");
    m_pDS->exec("UPDATE channels SET iClientProviderUid = -1");
  }

  if (iVersion < 40)
  {
    m_pDS->exec("ALTER TABLE channels ADD bIsUserSetHidden bool");
    m_pDS->exec("UPDATE channels SET bIsUserSetHidden = bIsHidden");
  }

  if (iVersion < 41)
  {
    try
    {
      // cleanup orphaned entries that might have piled up over time...
      m_pDS->exec("DELETE FROM map_channelgroups_channels WHERE idChannel NOT IN (SELECT "
                  "idChannel FROM channels)");
      m_pDS->exec("DELETE FROM channelgroups WHERE idGroup NOT IN (SELECT idGroup FROM "
                  "map_channelgroups_channels)");
    }
    catch (...)
    {
      CLog::LogFC(LOGERROR, LOGPVR, "Exception in database cleanup");
    }

    m_pDS->exec("ALTER TABLE channelgroups ADD iClientId integer");
    // set "PVR_GROUP_CLIENT_ID_UNKNOWN for migration of backend provided groups
    m_pDS->exec("UPDATE channelgroups SET iClientId = -2 WHERE iGroupType = 0");
    // set PVR_GROUP_CLIENT_ID_LOCAL for local groups
    m_pDS->exec("UPDATE channelgroups SET iClientId = -1 WHERE iGroupType != 0");
  }

  if (iVersion < 42)
  {
    m_pDS->exec("ALTER TABLE channelgroups ADD bIsUserSetName bool");
    m_pDS->exec("UPDATE channelgroups SET bIsUserSetName = 0");

    m_pDS->exec("ALTER TABLE channelgroups ADD sClientName varchar(64)");
    m_pDS->exec("UPDATE channelgroups SET sClientName = sName WHERE iGroupType = 0");
    m_pDS->exec("UPDATE channelgroups SET sClientName = '' WHERE iGroupType != 0");
  }

  if (iVersion < 43)
  {
    m_pDS->exec("ALTER TABLE channelgroups ADD iClientPosition integer");
    m_pDS->exec("UPDATE channelgroups SET iClientPosition = iPosition");
    // Setup initial local order, not perfect but at least unique across all groups.
    // Should mostly be the order the groups appeared from backend or were created by user locally.
    m_pDS->exec("UPDATE channelgroups SET iPosition = idGroup");
  }

  if (iVersion < 44)
  {
    m_pDS->exec("ALTER TABLE channels ADD iLastWatchedGroupId integer");
    m_pDS->exec("UPDATE channels SET iLastWatchedGroupId = -1");
  }

  if (iVersion < 45)
  {
    m_pDS->exec("ALTER TABLE channels ADD sDateTimeAdded varchar(20)");
    m_pDS->exec("UPDATE channels SET sDateTimeAdded = ''");
  }
}

/********** Client methods **********/

bool CPVRDatabase::DeleteClients()
{
  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting all clients from the database");

  std::unique_lock<CCriticalSection> lock(m_critSection);
  return DeleteValues("clients");
}

bool CPVRDatabase::Persist(const CPVRClient& client)
{
  if (client.GetID() == PVR_INVALID_CLIENT_ID)
    return false;

  CLog::LogFC(LOGDEBUG, LOGPVR, "Persisting client {} to database", client.GetID());

  std::unique_lock<CCriticalSection> lock(m_critSection);

  const std::string strQuery = PrepareSQL("REPLACE INTO clients (idClient, iPriority) VALUES (%i, %i);",
                                          client.GetID(), client.GetPriority());

  return ExecuteQuery(strQuery);
}

bool CPVRDatabase::Delete(const CPVRClient& client)
{
  if (client.GetID() == PVR_INVALID_CLIENT_ID)
    return false;

  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting client {} from the database", client.GetID());

  std::unique_lock<CCriticalSection> lock(m_critSection);

  Filter filter;
  filter.AppendWhere(PrepareSQL("idClient = '%i'", client.GetID()));

  return DeleteValues("clients", filter);
}

int CPVRDatabase::GetPriority(const CPVRClient& client) const
{
  if (client.GetID() == PVR_INVALID_CLIENT_ID)
    return 0;

  CLog::LogFC(LOGDEBUG, LOGPVR, "Getting priority for client {} from the database", client.GetID());

  std::unique_lock<CCriticalSection> lock(m_critSection);

  const std::string strWhereClause = PrepareSQL("idClient = '%i'", client.GetID());
  const std::string strValue = GetSingleValue("clients", "iPriority", strWhereClause);

  if (strValue.empty())
    return 0;

  return atoi(strValue.c_str());
}

/********** Channel provider methods **********/

bool CPVRDatabase::DeleteProviders()
{
  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting all providers from the database");

  std::unique_lock<CCriticalSection> lock(m_critSection);
  return DeleteValues("providers");
}

bool CPVRDatabase::Persist(CPVRProvider& provider, bool updateRecord /* = false */)
{
  bool bReturn = false;
  if (provider.GetName().empty())
  {
    CLog::LogF(LOGERROR, "Empty provider name");
    return bReturn;
  }

  std::string strQuery;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  {
    /* insert a new entry when this is a new group, or replace the existing one otherwise */
    if (!updateRecord)
      strQuery =
          PrepareSQL("INSERT INTO providers (idProvider, iUniqueId, iClientId, sName, "
                     "iType, sIconPath, sCountries, sLanguages) "
                     "VALUES (%i, %i, %i, '%s', %i, '%s', '%s', '%s');",
                     provider.GetDatabaseId(), provider.GetUniqueId(), provider.GetClientId(),
                     provider.GetName().c_str(), static_cast<int>(provider.GetType()),
                     provider.GetClientIconPath().c_str(), provider.GetCountriesDBString().c_str(),
                     provider.GetLanguagesDBString().c_str());
    else
      strQuery =
          PrepareSQL("REPLACE INTO providers (idProvider, iUniqueId, iClientId, sName, "
                     "iType, sIconPath, sCountries, sLanguages) "
                     "VALUES (%i, %i, %i, '%s', %i, '%s', '%s', '%s');",
                     provider.GetDatabaseId(), provider.GetUniqueId(), provider.GetClientId(),
                     provider.GetName().c_str(), static_cast<int>(provider.GetType()),
                     provider.GetClientIconPath().c_str(), provider.GetCountriesDBString().c_str(),
                     provider.GetLanguagesDBString().c_str());

    bReturn = ExecuteQuery(strQuery);

    /* set the provider id if it was <= 0 */
    if (bReturn && provider.GetDatabaseId() <= 0)
    {
      provider.SetDatabaseId(static_cast<int>(m_pDS->lastinsertid()));
    }
  }

  return bReturn;
}

bool CPVRDatabase::Delete(const CPVRProvider& provider)
{
  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting provider '{}' from the database",
              provider.GetName());

  std::unique_lock<CCriticalSection> lock(m_critSection);

  Filter filter;
  filter.AppendWhere(PrepareSQL("idProvider = '%i'", provider.GetDatabaseId()));

  return DeleteValues("providers", filter);
}

bool CPVRDatabase::Get(CPVRProviders& results,
                       const std::vector<std::shared_ptr<CPVRClient>>& clients) const
{
  bool bReturn = false;

  std::string strQuery = "SELECT * from providers ";
  const std::string clientIds = GetClientIdsSQL(clients);
  if (!clientIds.empty())
    strQuery += "WHERE " + clientIds + " OR iType = 1"; // always load addon providers

  std::unique_lock<CCriticalSection> lock(m_critSection);
  strQuery = PrepareSQL(strQuery);
  if (ResultQuery(strQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        std::shared_ptr<CPVRProvider> provider = std::make_shared<CPVRProvider>(
            m_pDS->fv("iUniqueId").get_asInt(), m_pDS->fv("iClientId").get_asInt());

        provider->SetDatabaseId(m_pDS->fv("idProvider").get_asInt());
        provider->SetName(m_pDS->fv("sName").get_asString());
        provider->SetType(
            static_cast<PVR_PROVIDER_TYPE>(m_pDS->fv("iType").get_asInt()));
        provider->SetIconPath(m_pDS->fv("sIconPath").get_asString());
        provider->SetCountriesFromDBString(m_pDS->fv("sCountries").get_asString());
        provider->SetLanguagesFromDBString(m_pDS->fv("sLanguages").get_asString());

        results.CheckAndAddEntry(provider, ProviderUpdateMode::BY_DATABASE);

        CLog::LogFC(LOGDEBUG, LOGPVR, "Channel Provider '{}' loaded from PVR database",
                    provider->GetName());
        m_pDS->next();
      }

      m_pDS->close();
      bReturn = true;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Couldn't load providers from PVR database");
    }
  }

  return bReturn;
}

int CPVRDatabase::GetMaxProviderId() const
{
  std::string strQuery = PrepareSQL("SELECT max(idProvider) as maxProviderId from providers");
  std::unique_lock<CCriticalSection> lock(m_critSection);

  return GetSingleValueInt(strQuery);
}

/********** Channel methods **********/

int CPVRDatabase::Get(bool bRadio,
                      const std::vector<std::shared_ptr<CPVRClient>>& clients,
                      std::map<std::pair<int, int>, std::shared_ptr<CPVRChannel>>& results) const
{
  int iReturn = 0;

  std::string strQuery = "SELECT * from channels WHERE bIsRadio = %u ";
  const std::string clientIds = GetClientIdsSQL(clients);
  if (!clientIds.empty())
    strQuery += "AND " + clientIds;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  strQuery = PrepareSQL(strQuery, bRadio);
  if (ResultQuery(strQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        const std::shared_ptr<CPVRChannel> channel(new CPVRChannel(
            m_pDS->fv("bIsRadio").get_asBool(), m_pDS->fv("sIconPath").get_asString()));

        channel->m_iChannelId = m_pDS->fv("idChannel").get_asInt();
        channel->m_iUniqueId = m_pDS->fv("iUniqueId").get_asInt();
        channel->m_bIsHidden = m_pDS->fv("bIsHidden").get_asBool();
        channel->m_bIsUserSetIcon = m_pDS->fv("bIsUserSetIcon").get_asBool();
        channel->m_bIsUserSetName = m_pDS->fv("bIsUserSetName").get_asBool();
        channel->m_bIsLocked = m_pDS->fv("bIsLocked").get_asBool();
        channel->m_strChannelName = m_pDS->fv("sChannelName").get_asString();
        channel->m_bEPGEnabled = m_pDS->fv("bEPGEnabled").get_asBool();
        channel->m_strEPGScraper = m_pDS->fv("sEPGScraper").get_asString();
        channel->m_iLastWatched = static_cast<time_t>(m_pDS->fv("iLastWatched").get_asInt());
        channel->m_iClientId = m_pDS->fv("iClientId").get_asInt();
        channel->m_iEpgId = m_pDS->fv("idEpg").get_asInt();
        channel->m_bHasArchive = m_pDS->fv("bHasArchive").get_asBool();
        channel->m_iClientProviderUid = m_pDS->fv("iClientProviderUid").get_asInt();
        channel->m_bIsUserSetHidden = m_pDS->fv("bIsUserSetHidden").get_asBool();
        channel->m_lastWatchedGroupId = m_pDS->fv("iLastWatchedGroupId").get_asInt();
        const std::string dateTimeAdded{m_pDS->fv("sDateTimeAdded").get_asString()};
        if (!dateTimeAdded.empty())
          channel->m_dateTimeAdded = CDateTime::FromDBDateTime(dateTimeAdded);

        channel->UpdateEncryptionName();

        results.insert({channel->StorageId(), channel});

        m_pDS->next();
        ++iReturn;
      }
      m_pDS->close();
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Couldn't load channels from PVR database");
    }
  }
  else
  {
    CLog::LogF(LOGERROR, "PVR database query failed");
  }

  m_pDS->close();
  return iReturn;
}

bool CPVRDatabase::DeleteChannels()
{
  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting all channels from the database");

  std::unique_lock<CCriticalSection> lock(m_critSection);
  return DeleteValues("channels");
}

bool CPVRDatabase::QueueDeleteQuery(const CPVRChannel& channel)
{
  /* invalid channel */
  if (channel.ChannelID() <= 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel id: {}", channel.ChannelID());
    return false;
  }

  CLog::LogFC(LOGDEBUG, LOGPVR, "Queueing delete for channel '{}' from the database",
              channel.ChannelName());

  Filter filter;
  filter.AppendWhere(PrepareSQL("idChannel = %i", channel.ChannelID()));

  std::string strQuery;
  if (BuildSQL(PrepareSQL("DELETE FROM %s ", "channels"), filter, strQuery))
    return CDatabase::QueueDeleteQuery(strQuery);

  return false;
}

/********** Channel group member methods **********/

bool CPVRDatabase::QueueDeleteQuery(const CPVRChannelGroupMember& groupMember)
{
  CLog::LogFC(LOGDEBUG, LOGPVR, "Queueing delete for channel group member '{}' from the database",
              groupMember.Channel() ? groupMember.Channel()->ChannelName()
                                    : std::to_string(groupMember.ChannelDatabaseID()));

  Filter filter;
  filter.AppendWhere(PrepareSQL("idGroup = %i", groupMember.GroupID()));
  filter.AppendWhere(PrepareSQL("idChannel = %i", groupMember.ChannelDatabaseID()));

  std::string strQuery;
  if (BuildSQL(PrepareSQL("DELETE FROM %s ", "map_channelgroups_channels"), filter, strQuery))
    return CDatabase::QueueDeleteQuery(strQuery);

  return false;
}

/********** Channel group methods **********/

bool CPVRDatabase::RemoveChannelsFromGroup(const CPVRChannelGroup& group)
{
  Filter filter;
  filter.AppendWhere(PrepareSQL("idGroup = %i", group.GroupID()));

  std::unique_lock<CCriticalSection> lock(m_critSection);
  return DeleteValues("map_channelgroups_channels", filter);
}

bool CPVRDatabase::DeleteChannelGroups()
{
  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting all channel groups from the database");

  std::unique_lock<CCriticalSection> lock(m_critSection);
  return DeleteValues("channelgroups") && DeleteValues("map_channelgroups_channels");
}

bool CPVRDatabase::Delete(const CPVRChannelGroup& group)
{
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel group id: {}", group.GroupID());
    return false;
  }

  std::unique_lock<CCriticalSection> lock(m_critSection);

  Filter filter;
  filter.AppendWhere(PrepareSQL("idGroup = %i", group.GroupID()));
  filter.AppendWhere(PrepareSQL("bIsRadio = %u", group.IsRadio()));

  return RemoveChannelsFromGroup(group) && DeleteValues("channelgroups", filter);
}

int CPVRDatabase::GetGroups(CPVRChannelGroups& results, const std::string& query) const
{
  int iLoaded = 0;
  if (ResultQuery(query))
  {
    try
    {
      while (!m_pDS->eof())
      {
        const std::shared_ptr<CPVRChannelGroup> group = results.CreateChannelGroup(
            m_pDS->fv("iGroupType").get_asInt(),
            CPVRChannelsPath(m_pDS->fv("bIsRadio").get_asBool(), m_pDS->fv("sName").get_asString(),
                             m_pDS->fv("iClientId").get_asInt()));

        group->m_iGroupId = m_pDS->fv("idGroup").get_asInt();
        group->m_iLastWatched = static_cast<time_t>(m_pDS->fv("iLastWatched").get_asInt());
        group->m_bHidden = m_pDS->fv("bIsHidden").get_asBool();
        group->m_iPosition = m_pDS->fv("iPosition").get_asInt();
        group->m_iLastOpened = static_cast<uint64_t>(m_pDS->fv("iLastOpened").get_asInt64());
        group->m_isUserSetName = m_pDS->fv("bIsUserSetName").get_asBool();
        group->m_clientGroupName = m_pDS->fv("sClientName").get_asString();
        group->m_iClientPosition = m_pDS->fv("iClientPosition").get_asInt();
        results.Update(group);

        CLog::LogFC(LOGDEBUG, LOGPVR, "Group '{}' loaded from PVR database", group->GroupName());
        m_pDS->next();
        iLoaded++;
      }
      m_pDS->close();
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Couldn't load channel groups from PVR database. Exception.");
    }
  }
  else
  {
    CLog::LogF(LOGERROR, "Couldn't load channel groups from PVR database. Query failed.");
  }

  return iLoaded;
}

int CPVRDatabase::GetLocalGroups(CPVRChannelGroups& results) const
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string query = PrepareSQL(
      "SELECT * from channelgroups WHERE bIsRadio = %u AND iClientId = -1", results.IsRadio());
  return GetGroups(results, query);
}

int CPVRDatabase::Get(CPVRChannelGroups& results,
                      const std::vector<std::shared_ptr<CPVRClient>>& clients) const
{
  std::string query = "SELECT * from channelgroups WHERE bIsRadio = %u ";

  const std::string clientIds =
      GetClientIdsSQL(clients, true /* add PVR_GROUP_CLIENT_ID_UNKNOWN for db data migration */);
  if (!clientIds.empty())
    query += "AND " + clientIds;

  std::unique_lock<CCriticalSection> lock(m_critSection);
  query = PrepareSQL(query, results.IsRadio());
  return GetGroups(results, query);
}

std::vector<std::shared_ptr<CPVRChannelGroupMember>> CPVRDatabase::Get(
    const CPVRChannelGroup& group, const std::vector<std::shared_ptr<CPVRClient>>& clients) const
{
  std::vector<std::shared_ptr<CPVRChannelGroupMember>> results;

  /* invalid group id */
  if (group.GroupID() < 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel group id: {}", group.GroupID());
    return results;
  }

  std::string strQuery =
      "SELECT map_channelgroups_channels.idChannel, "
      "map_channelgroups_channels.iChannelNumber, "
      "map_channelgroups_channels.iSubChannelNumber, "
      "map_channelgroups_channels.iOrder, "
      "map_channelgroups_channels.iClientChannelNumber, "
      "map_channelgroups_channels.iClientSubChannelNumber, "
      "channels.iClientId, channels.iUniqueId, channels.bIsRadio "
      "FROM map_channelgroups_channels "
      "LEFT JOIN channels ON channels.idChannel = map_channelgroups_channels.idChannel "
      "WHERE map_channelgroups_channels.idGroup = %i ";
  const std::string clientIds = GetClientIdsSQL(clients);
  if (!clientIds.empty())
    strQuery += "AND " + clientIds;
  strQuery += " ORDER BY map_channelgroups_channels.iChannelNumber";

  std::unique_lock<CCriticalSection> lock(m_critSection);
  strQuery = PrepareSQL(strQuery, group.GroupID());
  if (ResultQuery(strQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        const auto newMember = std::make_shared<CPVRChannelGroupMember>();
        newMember->m_iChannelDatabaseID = m_pDS->fv("idChannel").get_asInt();
        newMember->m_iChannelClientID = m_pDS->fv("iClientId").get_asInt();
        newMember->m_iChannelUID = m_pDS->fv("iUniqueId").get_asInt();
        newMember->m_iGroupID = group.GroupID();
        newMember->m_iGroupClientID = group.GetClientID();
        newMember->m_bIsRadio = m_pDS->fv("bIsRadio").get_asBool();
        newMember->m_channelNumber = {
            static_cast<unsigned int>(m_pDS->fv("iChannelNumber").get_asInt()),
            static_cast<unsigned int>(m_pDS->fv("iSubChannelNumber").get_asInt())};
        newMember->m_clientChannelNumber = {
            static_cast<unsigned int>(m_pDS->fv("iClientChannelNumber").get_asInt()),
            static_cast<unsigned int>(m_pDS->fv("iClientSubChannelNumber").get_asInt())};
        newMember->m_iOrder = static_cast<int>(m_pDS->fv("iOrder").get_asInt());
        newMember->SetGroupName(group.GroupName());

        results.emplace_back(newMember);
        m_pDS->next();
      }
      m_pDS->close();
    }
    catch(...)
    {
      CLog::LogF(LOGERROR, "Failed to get channel group members");
    }
  }

  return results;
}

bool CPVRDatabase::PersistChannels(const CPVRChannelGroup& group)
{
  /* invalid group id */
  if (group.GroupID() < 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel group id: {}", group.GroupID());
    return false;
  }

  bool bReturn(true);

  std::shared_ptr<CPVRChannel> channel;
  for (const auto& groupMember : group.m_members)
  {
    channel = groupMember.second->Channel();
    if (channel->IsChanged() || channel->IsNew())
    {
      if (Persist(*channel, false))
      {
        channel->Persisted();
        bReturn = true;
      }
    }
  }

  bReturn &= CommitInsertQueries();

  if (bReturn)
  {
    std::string strQuery;
    std::string strValue;
    for (const auto& groupMember : group.m_members)
    {
      channel = groupMember.second->Channel();
      strQuery =
          PrepareSQL("iUniqueId = %i AND iClientId = %i", channel->UniqueID(), channel->ClientID());
      strValue = GetSingleValue("channels", "idChannel", strQuery);
      if (!strValue.empty() && StringUtils::IsInteger(strValue))
      {
        const int iChannelID = std::atoi(strValue.c_str());
        channel->SetChannelID(iChannelID);
        groupMember.second->m_iChannelDatabaseID = iChannelID;
      }
    }
  }

  return bReturn;
}

bool CPVRDatabase::PersistGroupMembers(const CPVRChannelGroup& group)
{
  /* invalid group id */
  if (group.GroupID() < 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel group id: {}", group.GroupID());
    return false;
  }

  bool bReturn = true;

  if (group.HasChannels())
  {
    for (const auto& groupMember : group.m_sortedMembers)
    {
      if (groupMember->NeedsSave())
      {
        if (groupMember->ChannelDatabaseID() <= 0)
        {
          CLog::LogF(LOGERROR, "Invalid channel id: {}", groupMember->ChannelDatabaseID());
          continue;
        }

        const std::string strWhereClause =
            PrepareSQL("idChannel = %i AND idGroup = %i AND iChannelNumber = %u AND "
                       "iSubChannelNumber = %u AND "
                       "iOrder = %i AND iClientChannelNumber = %u AND iClientSubChannelNumber = %u",
                       groupMember->ChannelDatabaseID(), group.GroupID(),
                       groupMember->ChannelNumber().GetChannelNumber(),
                       groupMember->ChannelNumber().GetSubChannelNumber(), groupMember->Order(),
                       groupMember->ClientChannelNumber().GetChannelNumber(),
                       groupMember->ClientChannelNumber().GetSubChannelNumber());

        const std::string strValue =
            GetSingleValue("map_channelgroups_channels", "idChannel", strWhereClause);
        if (strValue.empty())
        {
          const std::string strQuery =
              PrepareSQL("REPLACE INTO map_channelgroups_channels ("
                         "idGroup, idChannel, iChannelNumber, iSubChannelNumber, iOrder, "
                         "iClientChannelNumber, iClientSubChannelNumber) "
                         "VALUES (%i, %i, %i, %i, %i, %i, %i);",
                         group.GroupID(), groupMember->ChannelDatabaseID(),
                         groupMember->ChannelNumber().GetChannelNumber(),
                         groupMember->ChannelNumber().GetSubChannelNumber(), groupMember->Order(),
                         groupMember->ClientChannelNumber().GetChannelNumber(),
                         groupMember->ClientChannelNumber().GetSubChannelNumber());
          QueueInsertQuery(strQuery);
        }
      }
    }

    bReturn = CommitInsertQueries();

    if (bReturn)
    {
      for (const auto& groupMember : group.m_sortedMembers)
      {
        groupMember->SetSaved();
      }
    }
  }

  return bReturn;
}

bool CPVRDatabase::ResetEPG()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string strQuery = PrepareSQL("UPDATE channels SET idEpg = 0");
  return ExecuteQuery(strQuery);
}

bool CPVRDatabase::Persist(CPVRChannelGroup& group)
{
  bool bReturn(false);
  if (group.GroupName().empty())
  {
    CLog::LogF(LOGERROR, "Empty group name");
    return bReturn;
  }

  std::string strQuery;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  if (group.HasChanges() || group.IsNew())
  {
    /* insert a new entry when this is a new group, or replace the existing one otherwise */
    if (group.IsNew())
      strQuery = PrepareSQL("INSERT INTO channelgroups (bIsRadio, iGroupType, sName, iLastWatched, "
                            "bIsHidden, iPosition, iLastOpened, iClientId, bIsUserSetName, "
                            "sClientName, iClientPosition) VALUES (%i, %i, '%s', "
                            "%u, %i, %i, %llu, %i, %i, '%s', %i)",
                            (group.IsRadio() ? 1 : 0), group.GroupType(), group.GroupName().c_str(),
                            static_cast<unsigned int>(group.LastWatched()), group.IsHidden(),
                            group.GetPosition(), group.LastOpened(), group.GetClientID(),
                            (group.IsUserSetName() ? 1 : 0), group.ClientGroupName().c_str(),
                            group.GetClientPosition());
    else
      strQuery = PrepareSQL(
          "REPLACE INTO channelgroups (idGroup, bIsRadio, iGroupType, sName, iLastWatched, "
          "bIsHidden, iPosition, iLastOpened, iClientId, bIsUserSetName, sClientName, "
          "iClientPosition) VALUES (%i, %i, %i, '%s', %u, %i, %i, %llu, %i, %i, '%s', %i)",
          group.GroupID(), (group.IsRadio() ? 1 : 0), group.GroupType(), group.GroupName().c_str(),
          static_cast<unsigned int>(group.LastWatched()), group.IsHidden(), group.GetPosition(),
          group.LastOpened(), group.GetClientID(), (group.IsUserSetName() ? 1 : 0),
          group.ClientGroupName().c_str(), group.GetClientPosition());

    bReturn = ExecuteQuery(strQuery);

    // set the group ID for new groups
    if (bReturn && group.IsNew())
      group.SetGroupID(static_cast<int>(m_pDS->lastinsertid()));
  }
  else
    bReturn = true;

  // only persist the channel data for the all channels groups as all groups
  // share the same channel instances and all groups has them all.
  if (group.IsChannelsOwner())
    bReturn &= PersistChannels(group);

  /* persist the group member entries */
  if (bReturn)
    bReturn = PersistGroupMembers(group);

  return bReturn;
}

bool CPVRDatabase::Persist(CPVRChannel& channel, bool bCommit)
{
  bool bReturn(false);

  /* invalid channel */
  if (channel.UniqueID() <= 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel uid: {}", channel.UniqueID());
    return bReturn;
  }

  std::string dateTimeAdded;
  if (channel.DateTimeAdded().IsValid())
    dateTimeAdded = channel.DateTimeAdded().GetAsDBDateTime();

  std::unique_lock<CCriticalSection> lock(m_critSection);

  // Note: Do not use channel.ChannelID value to check presence of channel in channels table. It might not yet be set correctly.
  std::string strQuery =
      PrepareSQL("iUniqueId = %i AND iClientId = %i", channel.UniqueID(), channel.ClientID());
  const std::string strValue = GetSingleValue("channels", "idChannel", strQuery);
  if (strValue.empty())
  {
    /* new channel */
    strQuery = PrepareSQL(
        "INSERT INTO channels ("
        "iUniqueId, bIsRadio, bIsHidden, bIsUserSetIcon, bIsUserSetName, bIsLocked, "
        "sIconPath, sChannelName, bIsVirtual, bEPGEnabled, sEPGScraper, iLastWatched, iClientId, "
        "idEpg, bHasArchive, iClientProviderUid, bIsUserSetHidden, iLastWatchedGroupId, "
        "sDateTimeAdded) "
        "VALUES (%i, %i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %u, %i, %i, %i, %i, %i, %i, "
        "'%s')",
        channel.UniqueID(), (channel.IsRadio() ? 1 : 0), (channel.IsHidden() ? 1 : 0),
        (channel.IsUserSetIcon() ? 1 : 0), (channel.IsUserSetName() ? 1 : 0),
        (channel.IsLocked() ? 1 : 0), channel.IconPath().c_str(), channel.ChannelName().c_str(), 0,
        (channel.EPGEnabled() ? 1 : 0), channel.EPGScraper().c_str(),
        static_cast<unsigned int>(channel.LastWatched()), channel.ClientID(), channel.EpgID(),
        channel.HasArchive(), channel.ClientProviderUid(), channel.IsUserSetHidden() ? 1 : 0,
        channel.LastWatchedGroupId(), dateTimeAdded.c_str());
  }
  else
  {
    /* update channel */
    strQuery = PrepareSQL(
        "REPLACE INTO channels ("
        "iUniqueId, bIsRadio, bIsHidden, bIsUserSetIcon, bIsUserSetName, bIsLocked, "
        "sIconPath, sChannelName, bIsVirtual, bEPGEnabled, sEPGScraper, iLastWatched, iClientId, "
        "idChannel, idEpg, bHasArchive, iClientProviderUid, bIsUserSetHidden, iLastWatchedGroupId, "
        "sDateTimeAdded) "
        "VALUES (%i, %i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %u, %i, %s, %i, %i, %i, %i, %i, "
        "'%s')",
        channel.UniqueID(), (channel.IsRadio() ? 1 : 0), (channel.IsHidden() ? 1 : 0),
        (channel.IsUserSetIcon() ? 1 : 0), (channel.IsUserSetName() ? 1 : 0),
        (channel.IsLocked() ? 1 : 0), channel.ClientIconPath().c_str(),
        channel.ChannelName().c_str(), 0, (channel.EPGEnabled() ? 1 : 0),
        channel.EPGScraper().c_str(), static_cast<unsigned int>(channel.LastWatched()),
        channel.ClientID(), strValue.c_str(), channel.EpgID(), channel.HasArchive(),
        channel.ClientProviderUid(), channel.IsUserSetHidden() ? 1 : 0,
        channel.LastWatchedGroupId(), dateTimeAdded.c_str());
  }

  if (QueueInsertQuery(strQuery))
  {
    bReturn = true;

    if (bCommit)
      bReturn = CommitInsertQueries();
  }

  return bReturn;
}

bool CPVRDatabase::UpdateLastWatched(const CPVRChannel& channel, int groupId)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string strQuery = PrepareSQL(
      "UPDATE channels SET iLastWatched = %u, iLastWatchedGroupId = %i WHERE idChannel = %i",
      static_cast<unsigned int>(channel.LastWatched()), groupId, channel.ChannelID());
  return ExecuteQuery(strQuery);
}

bool CPVRDatabase::UpdateLastWatched(const CPVRChannelGroup& group)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("UPDATE channelgroups SET iLastWatched = %u WHERE idGroup = %i",
                 static_cast<unsigned int>(group.LastWatched()), group.GroupID());
  return ExecuteQuery(strQuery);
}

bool CPVRDatabase::UpdateLastOpened(const CPVRChannelGroup& group)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("UPDATE channelgroups SET iLastOpened = %llu WHERE idGroup = %i",
                 group.LastOpened(), group.GroupID());
  return ExecuteQuery(strQuery);
}

/********** Timer methods **********/

std::vector<std::shared_ptr<CPVRTimerInfoTag>> CPVRDatabase::GetTimers(
    CPVRTimers& timers, const std::vector<std::shared_ptr<CPVRClient>>& clients) const
{
  std::vector<std::shared_ptr<CPVRTimerInfoTag>> result;

  std::string strQuery = "SELECT * FROM timers ";
  const std::string clientIds = GetClientIdsSQL(clients);
  if (!clientIds.empty())
    strQuery += "WHERE " + clientIds + " OR (iClientId = -1)"; // always load client agnostic timers

  std::unique_lock<CCriticalSection> lock(m_critSection);
  strQuery = PrepareSQL(strQuery);
  if (ResultQuery(strQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        std::shared_ptr<CPVRTimerInfoTag> newTag(new CPVRTimerInfoTag());

        newTag->m_iClientIndex = -m_pDS->fv("iClientIndex").get_asInt();
        newTag->m_iParentClientIndex = m_pDS->fv("iParentClientIndex").get_asInt();
        newTag->m_iClientId = m_pDS->fv("iClientId").get_asInt();
        newTag->SetTimerType(CPVRTimerType::CreateFromIds(m_pDS->fv("iTimerType").get_asInt(), -1));
        newTag->m_state = static_cast<PVR_TIMER_STATE>(m_pDS->fv("iState").get_asInt());
        newTag->m_strTitle = m_pDS->fv("sTitle").get_asString().c_str();
        newTag->m_iClientChannelUid = m_pDS->fv("iClientChannelUid").get_asInt();
        newTag->m_strSeriesLink = m_pDS->fv("sSeriesLink").get_asString().c_str();
        newTag->SetStartFromUTC(CDateTime::FromDBDateTime(m_pDS->fv("sStartTime").get_asString().c_str()));
        newTag->m_bStartAnyTime = m_pDS->fv("bStartAnyTime").get_asBool();
        newTag->SetEndFromUTC(CDateTime::FromDBDateTime(m_pDS->fv("sEndTime").get_asString().c_str()));
        newTag->m_bEndAnyTime = m_pDS->fv("bEndAnyTime").get_asBool();
        newTag->SetFirstDayFromUTC(CDateTime::FromDBDateTime(m_pDS->fv("sFirstDay").get_asString().c_str()));
        newTag->m_iWeekdays = m_pDS->fv("iWeekdays").get_asInt();
        newTag->m_iEpgUid = m_pDS->fv("iEpgUid").get_asInt();
        newTag->m_iMarginStart = m_pDS->fv("iMarginStart").get_asInt();
        newTag->m_iMarginEnd = m_pDS->fv("iMarginEnd").get_asInt();
        newTag->m_strEpgSearchString = m_pDS->fv("sEpgSearchString").get_asString().c_str();
        newTag->m_bFullTextEpgSearch = m_pDS->fv("bFullTextEpgSearch").get_asBool();
        newTag->m_iPreventDupEpisodes = m_pDS->fv("iPreventDuplicates").get_asInt();
        newTag->m_iPriority = m_pDS->fv("iPrority").get_asInt();
        newTag->m_iLifetime = m_pDS->fv("iLifetime").get_asInt();
        newTag->m_iMaxRecordings = m_pDS->fv("iMaxRecordings").get_asInt();
        newTag->m_iRecordingGroup = m_pDS->fv("iRecordingGroup").get_asInt();
        newTag->UpdateSummary();

        result.emplace_back(newTag);

        m_pDS->next();
      }
      m_pDS->close();
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Could not load timer data from the database");
    }
  }
  return result;
}

bool CPVRDatabase::Persist(CPVRTimerInfoTag& timer)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  // insert a new entry if this is a new timer, or replace the existing one otherwise
  std::string strQuery;
  if (timer.m_iClientIndex == PVR_TIMER_NO_CLIENT_INDEX)
    strQuery = PrepareSQL("INSERT INTO timers "
                          "(iParentClientIndex, iClientId, iTimerType, iState, sTitle, iClientChannelUid, sSeriesLink, sStartTime,"
                          " bStartAnyTime, sEndTime, bEndAnyTime, sFirstDay, iWeekdays, iEpgUid, iMarginStart, iMarginEnd,"
                          " sEpgSearchString, bFullTextEpgSearch, iPreventDuplicates, iPrority, iLifetime, iMaxRecordings, iRecordingGroup) "
                          "VALUES (%i, %i, %u, %i, '%s', %i, '%s', '%s', %i, '%s', %i, '%s', %i, %u, %i, %i, '%s', %i, %i, %i, %i, %i, %i);",
                          timer.m_iParentClientIndex, timer.m_iClientId, timer.GetTimerType()->GetTypeId(), timer.m_state,
                          timer.Title().c_str(), timer.m_iClientChannelUid, timer.SeriesLink().c_str(),
                          timer.StartAsUTC().GetAsDBDateTime().c_str(), timer.m_bStartAnyTime ? 1 : 0,
                          timer.EndAsUTC().GetAsDBDateTime().c_str(), timer.m_bEndAnyTime ? 1 : 0,
                          timer.FirstDayAsUTC().GetAsDBDateTime().c_str(), timer.m_iWeekdays, timer.UniqueBroadcastID(),
                          timer.m_iMarginStart, timer.m_iMarginEnd, timer.m_strEpgSearchString.c_str(), timer.m_bFullTextEpgSearch ? 1 : 0,
                          timer.m_iPreventDupEpisodes, timer.m_iPriority, timer.m_iLifetime, timer.m_iMaxRecordings, timer.m_iRecordingGroup);
  else
    strQuery = PrepareSQL("REPLACE INTO timers "
                          "(iClientIndex,"
                          " iParentClientIndex, iClientId, iTimerType, iState, sTitle, iClientChannelUid, sSeriesLink, sStartTime,"
                          " bStartAnyTime, sEndTime, bEndAnyTime, sFirstDay, iWeekdays, iEpgUid, iMarginStart, iMarginEnd,"
                          " sEpgSearchString, bFullTextEpgSearch, iPreventDuplicates, iPrority, iLifetime, iMaxRecordings, iRecordingGroup) "
                          "VALUES (%i, %i, %i, %u, %i, '%s', %i, '%s', '%s', %i, '%s', %i, '%s', %i, %u, %i, %i, '%s', %i, %i, %i, %i, %i, %i);",
                          -timer.m_iClientIndex,
                          timer.m_iParentClientIndex, timer.m_iClientId, timer.GetTimerType()->GetTypeId(), timer.m_state,
                          timer.Title().c_str(), timer.m_iClientChannelUid, timer.SeriesLink().c_str(),
                          timer.StartAsUTC().GetAsDBDateTime().c_str(), timer.m_bStartAnyTime ? 1 : 0,
                          timer.EndAsUTC().GetAsDBDateTime().c_str(), timer.m_bEndAnyTime ? 1 : 0,
                          timer.FirstDayAsUTC().GetAsDBDateTime().c_str(), timer.m_iWeekdays, timer.UniqueBroadcastID(),
                          timer.m_iMarginStart, timer.m_iMarginEnd, timer.m_strEpgSearchString.c_str(), timer.m_bFullTextEpgSearch ? 1 : 0,
                          timer.m_iPreventDupEpisodes, timer.m_iPriority, timer.m_iLifetime, timer.m_iMaxRecordings, timer.m_iRecordingGroup);

  bool bReturn = ExecuteQuery(strQuery);

  // set the client index for just inserted timers
  if (bReturn && timer.m_iClientIndex == PVR_TIMER_NO_CLIENT_INDEX)
  {
    // index must be negative for local timers!
    timer.m_iClientIndex = -static_cast<int>(m_pDS->lastinsertid());
  }

  return bReturn;
}

bool CPVRDatabase::Delete(const CPVRTimerInfoTag& timer)
{
  if (timer.m_iClientIndex == PVR_TIMER_NO_CLIENT_INDEX)
    return false;

  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting timer '{}' from the database", timer.m_iClientIndex);

  std::unique_lock<CCriticalSection> lock(m_critSection);

  Filter filter;
  filter.AppendWhere(PrepareSQL("iClientIndex = '%i'", -timer.m_iClientIndex));

  return DeleteValues("timers", filter);
}

bool CPVRDatabase::DeleteTimers()
{
  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting all timers from the database");

  std::unique_lock<CCriticalSection> lock(m_critSection);
  return DeleteValues("timers");
}
