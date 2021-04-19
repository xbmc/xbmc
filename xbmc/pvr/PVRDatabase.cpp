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
#include "pvr/channels/PVRChannelGroup.h"
#include "pvr/channels/PVRChannelGroups.h"
#include "pvr/timers/PVRTimerInfoTag.h"
#include "pvr/timers/PVRTimerType.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "threads/SingleLock.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <cstdlib>
#include <map>
#include <memory>
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

  static const std::string sqlCreateChannelGroupsTable =
    "CREATE TABLE channelgroups ("
      "idGroup         integer primary key,"
      "bIsRadio        bool, "
      "iGroupType      integer, "
      "sName           varchar(64), "
      "iLastWatched    integer, "
      "bIsHidden       bool, "
      "iPosition       integer, "
      "iLastOpened     bigint unsigned"
  ")";

// clang-format on
} // unnamed namespace

bool CPVRDatabase::Open()
{
  CSingleLock lock(m_critSection);
  return CDatabase::Open(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseTV);
}

void CPVRDatabase::Close()
{
  CSingleLock lock(m_critSection);
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
  CSingleLock lock(m_critSection);

  CLog::LogF(LOGINFO, "Creating PVR database tables");

  CLog::LogFC(LOGDEBUG, LOGPVR, "Creating table 'channels'");
  m_pDS->exec(
      "CREATE TABLE channels ("
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
        "bHasArchive          bool"
      ")"
  );

  CLog::LogFC(LOGDEBUG, LOGPVR, "Creating table 'channelgroups'");
  m_pDS->exec(sqlCreateChannelGroupsTable);

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
}

void CPVRDatabase::CreateAnalytics()
{
  CSingleLock lock(m_critSection);

  CLog::LogF(LOGINFO, "Creating PVR database indices");
  m_pDS->exec("CREATE INDEX idx_clients_idClient on clients(idClient);");
  m_pDS->exec("CREATE UNIQUE INDEX idx_channels_iClientId_iUniqueId on channels(iClientId, iUniqueId);");
  m_pDS->exec("CREATE INDEX idx_channelgroups_bIsRadio on channelgroups(bIsRadio);");
  m_pDS->exec("CREATE UNIQUE INDEX idx_idGroup_idChannel on map_channelgroups_channels(idGroup, idChannel);");
  m_pDS->exec("CREATE INDEX idx_timers_iClientIndex on timers(iClientIndex);");
}

void CPVRDatabase::UpdateTables(int iVersion)
{
  CSingleLock lock(m_critSection);

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

    m_pDS->exec(sqlCreateChannelGroupsTable);

    m_pDS->exec(
        "INSERT INTO channelgroups (bIsRadio, iGroupType, sName, iLastWatched, bIsHidden, "
        "iPosition, iLastOpened) "
        "SELECT bIsRadio, iGroupType, sName, iLastWatched, bIsHidden, iPosition, iLastOpened "
        "FROM channelgroups_old");

    m_pDS->exec("DROP TABLE channelgroups_old");
  }
}

/********** Client methods **********/

bool CPVRDatabase::DeleteClients()
{
  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting all clients from the database");

  CSingleLock lock(m_critSection);
  return DeleteValues("clients");
}

bool CPVRDatabase::Persist(const CPVRClient& client)
{
  if (client.GetID() == PVR_INVALID_CLIENT_ID)
    return false;

  CLog::LogFC(LOGDEBUG, LOGPVR, "Persisting client '{}' to database", client.ID());

  CSingleLock lock(m_critSection);

  const std::string strQuery = PrepareSQL("REPLACE INTO clients (idClient, iPriority) VALUES (%i, %i);",
                                          client.GetID(), client.GetPriority());

  return ExecuteQuery(strQuery);
}

bool CPVRDatabase::Delete(const CPVRClient& client)
{
  if (client.GetID() == PVR_INVALID_CLIENT_ID)
    return false;

  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting client '{}' from the database", client.ID());

  CSingleLock lock(m_critSection);

  Filter filter;
  filter.AppendWhere(PrepareSQL("idClient = '%i'", client.GetID()));

  return DeleteValues("clients", filter);
}

int CPVRDatabase::GetPriority(const CPVRClient& client)
{
  if (client.GetID() == PVR_INVALID_CLIENT_ID)
    return 0;

  CLog::LogFC(LOGDEBUG, LOGPVR, "Getting priority for client '{}' from the database", client.ID());

  CSingleLock lock(m_critSection);

  const std::string strWhereClause = PrepareSQL("idClient = '%i'", client.GetID());
  const std::string strValue = GetSingleValue("clients", "iPriority", strWhereClause);

  if (strValue.empty())
    return 0;

  return atoi(strValue.c_str());
}

/********** Channel methods **********/

bool CPVRDatabase::DeleteChannels()
{
  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting all channels from the database");

  CSingleLock lock(m_critSection);
  return DeleteValues("channels");
}

bool CPVRDatabase::QueueDeleteQuery(const CPVRChannel& channel)
{
  /* invalid channel */
  if (channel.ChannelID() <= 0)
    return false;

  CLog::LogFC(LOGDEBUG, LOGPVR, "Queueing delete for channel '{}' from the database",
              channel.ChannelName());

  Filter filter;
  filter.AppendWhere(PrepareSQL("idChannel = %u", channel.ChannelID()));

  std::string strQuery;
  if (BuildSQL(PrepareSQL("DELETE FROM %s ", "channels"), filter, strQuery))
    return CDatabase::QueueDeleteQuery(strQuery);

  return false;
}

int CPVRDatabase::Get(CPVRChannelGroup& results, bool bCompressDB)
{
  int iReturn = 0;
  std::string strQuery = PrepareSQL("SELECT channels.idChannel, channels.iUniqueId, channels.bIsRadio, channels.bIsHidden, channels.bIsUserSetIcon, channels.bIsUserSetName, "
      "channels.sIconPath, channels.sChannelName, channels.bIsVirtual, channels.bEPGEnabled, channels.sEPGScraper, channels.iLastWatched, channels.iClientId, channels.bIsLocked, "
      "map_channelgroups_channels.iChannelNumber, map_channelgroups_channels.iSubChannelNumber, map_channelgroups_channels.iOrder, map_channelgroups_channels.iClientChannelNumber, "
      "map_channelgroups_channels.iClientSubChannelNumber, channels.idEpg, channels.bHasArchive "
      "FROM map_channelgroups_channels "
      "LEFT JOIN channels ON channels.idChannel = map_channelgroups_channels.idChannel "
      "WHERE map_channelgroups_channels.idGroup = %u", results.GroupID());

  CSingleLock lock(m_critSection);
  if (ResultQuery(strQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        std::shared_ptr<CPVRChannel> channel = std::shared_ptr<CPVRChannel>(new CPVRChannel());

        channel->m_iChannelId = m_pDS->fv("idChannel").get_asInt();
        channel->m_iUniqueId = m_pDS->fv("iUniqueId").get_asInt();
        channel->m_bIsRadio = m_pDS->fv("bIsRadio").get_asBool();
        channel->m_bIsHidden = m_pDS->fv("bIsHidden").get_asBool();
        channel->m_bIsUserSetIcon = m_pDS->fv("bIsUserSetIcon").get_asBool();
        channel->m_bIsUserSetName = m_pDS->fv("bIsUserSetName").get_asBool();
        channel->m_bIsLocked = m_pDS->fv("bIsLocked").get_asBool();
        channel->m_strIconPath = m_pDS->fv("sIconPath").get_asString();
        channel->m_strChannelName = m_pDS->fv("sChannelName").get_asString();
        channel->m_bEPGEnabled = m_pDS->fv("bEPGEnabled").get_asBool();
        channel->m_strEPGScraper = m_pDS->fv("sEPGScraper").get_asString();
        channel->m_iLastWatched = static_cast<time_t>(m_pDS->fv("iLastWatched").get_asInt());
        channel->m_iClientId = m_pDS->fv("iClientId").get_asInt();
        channel->m_iEpgId = m_pDS->fv("idEpg").get_asInt();
        channel->m_bHasArchive = m_pDS->fv("bHasArchive").get_asBool();
        channel->UpdateEncryptionName();

        auto newMember = std::make_shared<PVRChannelGroupMember>(channel,
                                                                 CPVRChannelNumber(static_cast<unsigned int>(m_pDS->fv("iChannelNumber").get_asInt()),
                                                                                   static_cast<unsigned int>(m_pDS->fv("iSubChannelNumber").get_asInt())),
                                                                 0, static_cast<int>(m_pDS->fv("iOrder").get_asInt()),
                                                                 CPVRChannelNumber(static_cast<unsigned int>(m_pDS->fv("iClientChannelNumber").get_asInt()),
                                                                                   static_cast<unsigned int>(m_pDS->fv("iClientSubChannelNumber").get_asInt()))
        );
        results.m_sortedMembers.emplace_back(newMember);
        results.m_members.insert(std::make_pair(channel->StorageId(), newMember));

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

  if (iReturn > 0 && bCompressDB)
    Compress(true);

  return iReturn;
}

/********** Channel group methods **********/

bool CPVRDatabase::QueueDeleteChannelGroupMembersQuery(int iGroupID)
{
  Filter filter;
  filter.AppendWhere(PrepareSQL("idGroup = %u", iGroupID));

  CSingleLock lock(m_critSection);

  std::string strQuery;
  if (BuildSQL(PrepareSQL("DELETE FROM %s ", "map_channelgroups_channels"), filter, strQuery))
    return CDatabase::QueueDeleteQuery(strQuery);

  return false;
}

bool CPVRDatabase::RemoveChannelsFromGroup(const CPVRChannelGroup& group)
{
  Filter filter;
  filter.AppendWhere(PrepareSQL("idGroup = %u", group.GroupID()));

  CSingleLock lock(m_critSection);
  return DeleteValues("map_channelgroups_channels", filter);
}

bool CPVRDatabase::GetCurrentGroupMembers(const CPVRChannelGroup& group, std::vector<int>& members)
{
  bool bReturn(false);
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel group id: {}", group.GroupID());
    return false;
  }

  CSingleLock lock(m_critSection);

  const std::string strCurrentMembersQuery = PrepareSQL("SELECT idChannel FROM map_channelgroups_channels WHERE idGroup = %u", group.GroupID());
  if (ResultQuery(strCurrentMembersQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        members.emplace_back(m_pDS->fv("idChannel").get_asInt());
        m_pDS->next();
      }
      m_pDS->close();
      bReturn = true;
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

  return bReturn;
}

bool CPVRDatabase::DeleteChannelsFromGroup(const CPVRChannelGroup& group, const std::vector<int>& channelsToDelete)
{
  bool bDelete(true);
  unsigned int iDeletedChannels(0);
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel group id: {}", group.GroupID());
    return false;
  }

  CSingleLock lock(m_critSection);

  while (iDeletedChannels < channelsToDelete.size())
  {
    std::string strChannelsToDelete;

    for (unsigned int iChannelPtr = 0; iChannelPtr + iDeletedChannels < channelsToDelete.size() && iChannelPtr < 50; iChannelPtr++)
      strChannelsToDelete += StringUtils::Format(", %d", channelsToDelete.at(iDeletedChannels + iChannelPtr));

    if (!strChannelsToDelete.empty())
    {
      strChannelsToDelete.erase(0, 2);

      Filter filter;
      filter.AppendWhere(PrepareSQL("idGroup = %u", group.GroupID()));
      filter.AppendWhere(PrepareSQL("idChannel IN (%s)", strChannelsToDelete.c_str()));

      bDelete = DeleteValues("map_channelgroups_channels", filter) && bDelete;
    }

    iDeletedChannels += 50;
  }

  return bDelete;
}

int CPVRDatabase::GetClientIdByChannelId(int iChannelId)
{
  const std::string strQuery = PrepareSQL("idChannel = %u", iChannelId);
  const std::string strValue = GetSingleValue("channels", "iClientId", strQuery);
  if (!strValue.empty())
    return std::atoi(strValue.c_str());

  return PVR_INVALID_CLIENT_ID;
}

bool CPVRDatabase::RemoveStaleChannelsFromGroup(const CPVRChannelGroup& group)
{
  bool bDelete(true);
  /* invalid group id */
  if (group.GroupID() <= 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel group id: {}", group.GroupID());
    return false;
  }

  CSingleLock lock(m_critSection);

  if (!group.IsInternalGroup())
  {
    /* First remove channels that don't exist in the main channels table */

    // XXX work around for frodo: fix this up so it uses one query for all db types
    // mysql doesn't support subqueries when deleting and sqlite doesn't support joins when deleting
    if (StringUtils::EqualsNoCase(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_databaseTV.type, "mysql"))
    {
      const std::string strQuery = PrepareSQL("DELETE m FROM map_channelgroups_channels m LEFT JOIN channels c ON (c.idChannel = m.idChannel) WHERE c.idChannel IS NULL");
      bDelete = ExecuteQuery(strQuery);
    }
    else
    {
      Filter filter;
      filter.AppendWhere("idChannel IN (SELECT m.idChannel FROM map_channelgroups_channels m LEFT JOIN channels on m.idChannel = channels.idChannel WHERE channels.idChannel IS NULL)");

      bDelete = DeleteValues("map_channelgroups_channels", filter);
    }
  }

  if (group.HasChannels())
  {
    std::vector<int> currentMembers;
    if (GetCurrentGroupMembers(group, currentMembers))
    {
      std::vector<int> channelsToDelete;
      for (int iChannelId : currentMembers)
      {
        if (!group.IsGroupMember(iChannelId))
        {
          int iClientId = GetClientIdByChannelId(iChannelId);
          if (iClientId == PVR_INVALID_CLIENT_ID || group.HasValidDataFromClient(iClientId))
          {
            channelsToDelete.emplace_back(iChannelId);
          }
        }
      }

      if (!channelsToDelete.empty())
        bDelete = DeleteChannelsFromGroup(group, channelsToDelete) && bDelete;
    }
  }
  else
  {
    Filter filter;
    filter.AppendWhere(PrepareSQL("idGroup = %u", group.GroupID()));

    bDelete = DeleteValues("map_channelgroups_channels", filter) && bDelete;
  }

  return bDelete;
}

bool CPVRDatabase::DeleteChannelGroups()
{
  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting all channel groups from the database");

  CSingleLock lock(m_critSection);
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

  CSingleLock lock(m_critSection);

  bool bIgnoreChannel = false;
  std::vector<int> currentMembers;
  if (GetCurrentGroupMembers(group, currentMembers))
  {
    for (int iChannelId : currentMembers)
    {
      int iClientId = GetClientIdByChannelId(iChannelId);
      if (iClientId != PVR_INVALID_CLIENT_ID && !group.HasValidDataFromClient(iClientId))
      {
        bIgnoreChannel = true;
        break;
      }
    }
  }

  if (!bIgnoreChannel)
  {
    Filter filter;
    filter.AppendWhere(PrepareSQL("idGroup = %u", group.GroupID()));
    filter.AppendWhere(PrepareSQL("bIsRadio = %u", group.IsRadio()));

    return RemoveChannelsFromGroup(group) && DeleteValues("channelgroups", filter);
  }

  return true;
}

bool CPVRDatabase::Get(CPVRChannelGroups& results)
{
  bool bReturn = false;
  CSingleLock lock(m_critSection);

  const std::string strQuery = PrepareSQL("SELECT * from channelgroups WHERE bIsRadio = %u", results.IsRadio());
  if (ResultQuery(strQuery))
  {
    try
    {
      while (!m_pDS->eof())
      {
        CPVRChannelGroup data(CPVRChannelsPath(m_pDS->fv("bIsRadio").get_asBool(), m_pDS->fv("sName").get_asString()),
                              m_pDS->fv("idGroup").get_asInt(),
                              results.GetGroupAll());
        data.SetGroupType(m_pDS->fv("iGroupType").get_asInt());
        data.SetLastWatched(static_cast<time_t>(m_pDS->fv("iLastWatched").get_asInt()));
        data.SetHidden(m_pDS->fv("bIsHidden").get_asBool());
        data.SetPosition(m_pDS->fv("iPosition").get_asInt());
        data.SetLastOpened(static_cast<uint64_t>(m_pDS->fv("iLastOpened").get_asInt64()));
        results.Update(data);

        CLog::LogFC(LOGDEBUG, LOGPVR, "Group '{}' loaded from PVR database", data.GroupName());
        m_pDS->next();
      }
      m_pDS->close();
      bReturn = true;
    }
    catch (...)
    {
      CLog::LogF(LOGERROR, "Couldn't load channels from PVR database");
    }
  }

  return bReturn;
}

int CPVRDatabase::Get(CPVRChannelGroup& group, const CPVRChannelGroup& allGroup)
{
  int iReturn = -1;

  /* invalid group id */
  if (group.GroupID() < 0)
  {
    CLog::LogF(LOGERROR, "Invalid channel group id: {}", group.GroupID());
    return -1;
  }

  CSingleLock lock(m_critSection);

  const std::string strQuery = PrepareSQL("SELECT idChannel, iChannelNumber, iSubChannelNumber, iOrder, iClientChannelNumber, iClientSubChannelNumber FROM map_channelgroups_channels "
                                          "WHERE idGroup = %u ORDER BY iChannelNumber", group.GroupID());
  if (ResultQuery(strQuery))
  {
    iReturn = 0;

    // create a map to speedup data lookup
    std::map<int, std::shared_ptr<CPVRChannel>> allChannels;
    for (const auto& groupMember : allGroup.m_sortedMembers)
    {
      allChannels.insert(std::make_pair(groupMember->channel->ChannelID(), groupMember->channel));
    }

    try
    {
      while (!m_pDS->eof())
      {
        int iChannelId = m_pDS->fv("idChannel").get_asInt();
        const auto& channel = allChannels.find(iChannelId);

        if (channel != allChannels.end())
        {
          auto newMember = std::make_shared<PVRChannelGroupMember>(channel->second,
                                                                   CPVRChannelNumber(static_cast<unsigned int>(m_pDS->fv("iChannelNumber").get_asInt()),
                                                                                     static_cast<unsigned int>(m_pDS->fv("iSubChannelNumber").get_asInt())),
                                                                   0, static_cast<int>(m_pDS->fv("iOrder").get_asInt()),
                                                                   CPVRChannelNumber(static_cast<unsigned int>(m_pDS->fv("iClientChannelNumber").get_asInt()),
                                                                                     static_cast<unsigned int>(m_pDS->fv("iClientSubChannelNumber").get_asInt())));

          group.m_sortedMembers.emplace_back(newMember);
          group.m_members.insert(std::make_pair(channel->second->StorageId(), newMember));
          ++iReturn;
        }
        else
        {
          // remove the channel from the table if it doesn't exist on client (anymore)
          int iClientId = GetClientIdByChannelId(iChannelId);
          if (iClientId == PVR_INVALID_CLIENT_ID || allGroup.HasValidDataFromClient(iClientId))
          {
            Filter filter;
            filter.AppendWhere(PrepareSQL("idGroup = %u", group.GroupID()));
            filter.AppendWhere(PrepareSQL("idChannel = %u", iChannelId));
            DeleteValues("map_channelgroups_channels", filter);
          }
        }

        m_pDS->next();
      }
      m_pDS->close();
    }
    catch(...)
    {
      CLog::LogF(LOGERROR, "Failed to get channels");
    }
  }

  if (iReturn > 0)
    group.SortByChannelNumber();

  return iReturn;
}

bool CPVRDatabase::PersistChannels(CPVRChannelGroup& group)
{
  bool bReturn(true);

  std::shared_ptr<CPVRChannel> channel;
  for (const auto& groupMember : group.m_members)
  {
    channel = groupMember.second->channel;
    if (channel->IsChanged() || channel->IsNew())
    {
      if (Persist(*channel, false))
      {
        groupMember.second->channel->Persisted();
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
      channel = groupMember.second->channel;
      strQuery = PrepareSQL("iUniqueId = %u AND iClientId = %u", channel->UniqueID(), channel->ClientID());
      strValue = GetSingleValue("channels", "idChannel", strQuery);
      if (!strValue.empty() && StringUtils::IsInteger(strValue))
        channel->SetChannelID(atoi(strValue.c_str()));
    }
  }

  return bReturn;
}

bool CPVRDatabase::PersistGroupMembers(const CPVRChannelGroup& group)
{
  bool bReturn = true;
  bool bRemoveChannels = true;
  std::string strQuery;
  CSingleLock lock(group.m_critSection);

  if (group.HasChannels())
  {
    for (const auto& groupMember : group.m_sortedMembers)
    {
      const std::string strWhereClause = PrepareSQL("idChannel = %u AND idGroup = %u AND iChannelNumber = %u AND iSubChannelNumber = %u AND iOrder = %u AND iClientChannelNumber = %u AND iClientSubChannelNumber = %u",
          groupMember->channel->ChannelID(), group.GroupID(), groupMember->channelNumber.GetChannelNumber(), groupMember->channelNumber.GetSubChannelNumber(), groupMember->iOrder,
          groupMember->clientChannelNumber.GetChannelNumber(), groupMember->clientChannelNumber.GetSubChannelNumber());

      const std::string strValue = GetSingleValue("map_channelgroups_channels", "idChannel", strWhereClause);
      if (strValue.empty())
      {
        strQuery = PrepareSQL("REPLACE INTO map_channelgroups_channels ("
            "idGroup, idChannel, iChannelNumber, iSubChannelNumber, iOrder, iClientChannelNumber, iClientSubChannelNumber) "
            "VALUES (%i, %i, %i, %i, %i, %i, %i);",
            group.GroupID(), groupMember->channel->ChannelID(), groupMember->channelNumber.GetChannelNumber(), groupMember->channelNumber.GetSubChannelNumber(), groupMember->iOrder,
            groupMember->clientChannelNumber.GetChannelNumber(), groupMember->clientChannelNumber.GetSubChannelNumber());
        QueueInsertQuery(strQuery);
      }
    }

    bReturn = CommitInsertQueries();
    bRemoveChannels = RemoveStaleChannelsFromGroup(group);
  }

  return bReturn && bRemoveChannels;
}

/********** Client methods **********/

bool CPVRDatabase::ResetEPG()
{
  CSingleLock lock(m_critSection);
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

  CSingleLock lock(m_critSection);
  {
    /* insert a new entry when this is a new group, or replace the existing one otherwise */
    if (group.GroupID() <= 0)
      strQuery =
          PrepareSQL("INSERT INTO channelgroups (bIsRadio, iGroupType, sName, iLastWatched, "
                     "bIsHidden, iPosition, iLastOpened) VALUES (%i, %i, '%s', %u, %i, %i, %llu)",
                     (group.IsRadio() ? 1 : 0), group.GroupType(), group.GroupName().c_str(),
                     static_cast<unsigned int>(group.LastWatched()), group.IsHidden(),
                     group.GetPosition(), group.LastOpened());
    else
      strQuery = PrepareSQL(
          "REPLACE INTO channelgroups (idGroup, bIsRadio, iGroupType, sName, iLastWatched, "
          "bIsHidden, iPosition, iLastOpened) VALUES (%i, %i, %i, '%s', %u, %i, %i, %llu)",
          group.GroupID(), (group.IsRadio() ? 1 : 0), group.GroupType(), group.GroupName().c_str(),
          static_cast<unsigned int>(group.LastWatched()), group.IsHidden(), group.GetPosition(),
          group.LastOpened());

    bReturn = ExecuteQuery(strQuery);

    /* set the group id if it was <= 0 */
    if (bReturn && group.GroupID() <= 0)
    {
      CSingleLock lock(group.m_critSection);
      group.m_iGroupId = (int) m_pDS->lastinsertid();
    }
  }

  /* only persist the channel data for the internal groups */
  if (group.IsInternalGroup())
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

  CSingleLock lock(m_critSection);

  // Note: Do not use channel.ChannelID value to check presence of channel in channels table. It might not yet be set correctly.
  std::string strQuery = PrepareSQL("iUniqueId = %u AND iClientId = %u", channel.UniqueID(), channel.ClientID());
  const std::string strValue = GetSingleValue("channels", "idChannel", strQuery);
  if (strValue.empty())
  {
    /* new channel */
    strQuery = PrepareSQL("INSERT INTO channels ("
        "iUniqueId, bIsRadio, bIsHidden, bIsUserSetIcon, bIsUserSetName, bIsLocked, "
        "sIconPath, sChannelName, bIsVirtual, bEPGEnabled, sEPGScraper, iLastWatched, iClientId, "
        "idEpg, bHasArchive) "
        "VALUES (%i, %i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %u, %i, %i, %i)",
        channel.UniqueID(), (channel.IsRadio() ? 1 :0), (channel.IsHidden() ? 1 : 0), (channel.IsUserSetIcon() ? 1 : 0), (channel.IsUserSetName() ? 1 : 0), (channel.IsLocked() ? 1 : 0),
        channel.IconPath().c_str(), channel.ChannelName().c_str(), 0, (channel.EPGEnabled() ? 1 : 0), channel.EPGScraper().c_str(), static_cast<unsigned int>(channel.LastWatched()), channel.ClientID(),
        channel.EpgID(), channel.HasArchive());
  }
  else
  {
    /* update channel */
    strQuery = PrepareSQL("REPLACE INTO channels ("
        "iUniqueId, bIsRadio, bIsHidden, bIsUserSetIcon, bIsUserSetName, bIsLocked, "
        "sIconPath, sChannelName, bIsVirtual, bEPGEnabled, sEPGScraper, iLastWatched, iClientId, "
        "idChannel, idEpg, bHasArchive) "
        "VALUES (%i, %i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %u, %i, %s, %i, %i)",
        channel.UniqueID(), (channel.IsRadio() ? 1 :0), (channel.IsHidden() ? 1 : 0), (channel.IsUserSetIcon() ? 1 : 0), (channel.IsUserSetName() ? 1 : 0), (channel.IsLocked() ? 1 : 0),
        channel.IconPath().c_str(), channel.ChannelName().c_str(), 0, (channel.EPGEnabled() ? 1 : 0), channel.EPGScraper().c_str(), static_cast<unsigned int>(channel.LastWatched()), channel.ClientID(),
        strValue.c_str(),
        channel.EpgID(), channel.HasArchive());
  }

  if (QueueInsertQuery(strQuery))
  {
    bReturn = true;

    if (bCommit)
      bReturn = CommitInsertQueries();
  }

  return bReturn;
}

bool CPVRDatabase::UpdateLastWatched(const CPVRChannel& channel)
{
  CSingleLock lock(m_critSection);
  const std::string strQuery = PrepareSQL("UPDATE channels SET iLastWatched = %u WHERE idChannel = %d",
    static_cast<unsigned int>(channel.LastWatched()), channel.ChannelID());
  return ExecuteQuery(strQuery);
}

bool CPVRDatabase::UpdateLastWatched(const CPVRChannelGroup& group)
{
  CSingleLock lock(m_critSection);
  const std::string strQuery = PrepareSQL("UPDATE channelgroups SET iLastWatched = %u WHERE idGroup = %d",
    static_cast<unsigned int>(group.LastWatched()), group.GroupID());
  return ExecuteQuery(strQuery);
}

bool CPVRDatabase::UpdateLastOpened(const CPVRChannelGroup& group)
{
  CSingleLock lock(m_critSection);
  const std::string strQuery =
      PrepareSQL("UPDATE channelgroups SET iLastOpened = %llu WHERE idGroup = %d",
                 group.LastOpened(), group.GroupID());
  return ExecuteQuery(strQuery);
}

/********** Timer methods **********/

std::vector<std::shared_ptr<CPVRTimerInfoTag>> CPVRDatabase::GetTimers(CPVRTimers& timers)
{
  std::vector<std::shared_ptr<CPVRTimerInfoTag>> result;

  CSingleLock lock(m_critSection);
  const std::string strQuery = PrepareSQL("SELECT * FROM timers");
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
  CSingleLock lock(m_critSection);

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

  CSingleLock lock(m_critSection);

  Filter filter;
  filter.AppendWhere(PrepareSQL("iClientIndex = '%i'", -timer.m_iClientIndex));

  return DeleteValues("timers", filter);
}

bool CPVRDatabase::DeleteTimers()
{
  CLog::LogFC(LOGDEBUG, LOGPVR, "Deleting all timers from the database");

  CSingleLock lock(m_critSection);
  return DeleteValues("timers");
}
